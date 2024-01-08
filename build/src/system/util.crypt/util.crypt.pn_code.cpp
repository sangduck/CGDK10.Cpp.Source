#include "pch.h"

namespace CGDK
{


shared_buffer crypt::pn_code::initialize_seed()
{
	// 1) srand
	srand(static_cast<uint32_t>(chrono::tick::now().time_since_epoch().count()));

	// 1) generate value
	auto temp_type	 = eCRYPTION_TYPE::PNCODE_BYTE;
	auto key_count	 = rand()%64 + 128;
	auto temp_offset = rand()%256;
	auto temp_round	 = rand()%256;

	// 2) set writable buffer
	auto buf_seed_info = alloc_shared_buffer(64 + sizeof(uint8_t)* 256);

	// 3) append parameters
	buf_seed_info.append<eCRYPTION_TYPE>(temp_type);
	buf_seed_info.append<uint32_t>(key_count);
	buf_seed_info.append<uint32_t>(temp_offset);
	buf_seed_info.append<uint32_t>(temp_round);

	// 4) random한 key값을 써넣는다.
	for(int i = 0; i < key_count; ++i)
	{
		buf_seed_info.append<uint8_t>(static_cast<uint8_t>(rand()));
	}

	// 4) Key로 Generation을 함.
	initialize_seed(buf_seed_info);

	// return) 
	return buf_seed_info;
}

void crypt::pn_code::initialize_seed(const_buffer_view _seed_info)
{
	// check) _seed_info가 0이어서는 안됀다.
	CGASSERT_ERROR(_seed_info.size() != 0);

	// 1) Type정보를 얻는다.
	auto temp_type = _seed_info.extract<eCRYPTION_TYPE>();

	// check) Cryption Type이 PNCode여야만 한다.
	THROW_IF(temp_type != eCRYPTION_TYPE::PNCODE_BYTE, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 2) Key의 갯수를 얻는다.
	size_t key_count = _seed_info.extract<uint32_t>();

	// check) Key의 갯수가 1개보다 크고 256개보다 작어야 한다.
	THROW_IF(key_count < 1 || key_count > 256, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// check) _length가 16+key_count*sizeof(uint32_t) 여야한다.
	THROW_IF(_seed_info.size() != (8 + key_count * sizeof(uint8_t)), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 3) 기타 정보를 얻는다.
	auto temp_offset = _seed_info.extract<uint32_t>();
	auto temp_round	 = _seed_info.extract<uint32_t>();

	// declare)
	std::array<uint8_t, 256> array_key{};

	// 4) Key를 Generate한다.(일반적으로 4개의 Seed로 key를 Generate한다.)
	{
		// 1) 일단 받은 키값은 복사를 한다.
		size_t i = 0;
		size_t i_end = key_count;
		for (; i < i_end; ++i)
		{
			array_key[i] = _seed_info[i];
		}

		// 2) 받은 키값 이후에서 255까지의 값은 만들어 낸다.
		size_t pos_a = i;
		size_t pos_b = i >> 1;
		for (; i < 256; ++i)
		{
			array_key[i] = array_key[i - pos_a] * array_key[i - pos_b];

			if (array_key[i] == 0)
			{
				array_key[i] = array_key[i - 1];
			}
		}
	}

	// 5) 최초 회전을 수행한다.
	uint8_t	pos_output_a = 0;
	uint8_t	pos_output_b = (uint8_t)temp_offset;

	for (uint32_t i = 0; i < temp_round; ++i)
	{
		array_key[pos_output_a] = array_key[pos_output_a] ^ array_key[pos_output_b];

		++pos_output_a;
		++pos_output_b;
	}

	// 6) 최종 값을 설정한다.
	m_array_key		  = std::move(array_key);
	m_pos_out = pos_output_a;
	m_pos_get		  = (uint8_t)temp_offset;
}

buffer_view crypt::pn_code::encode(buffer_view _dest, const_buffer_view _source)
{
	// check) 
	CGASSERT(_source.size() != 0, 0);

	// 1) 값을 준비한다.
	auto	 src	 = _source.data<const uint8_t>();
	auto	 src_end = _source.data<const uint8_t>() + _source.size();
	auto	 dest	 = _dest.data<uint8_t>();
	uint8_t  pos_out = m_pos_out;
	uint8_t  pos_get = pos_out + m_pos_get;

	// 2) 암/복호화를 수행한다.
	do
	{
		// - Key를 계산하고 Target을 XOR하여서 암/복호화 한다.
		m_array_key[pos_out] = m_array_key[pos_out] ^ m_array_key[pos_get];
		*dest = (*src) ^ m_array_key[pos_out];

		++dest;
		++src;
		++pos_out;
		++pos_get;
	} while(src < src_end);

	// 3) ...
#if defined(_MSC_VER)
	#pragma warning(disable:4244)
#endif
	m_pos_out = static_cast<uint8_t>(m_pos_out + _source.size());
#if defined(_MSC_VER)
	#pragma warning(default:4244)
#endif

	// return) ...
	return buffer_view{ _dest.data(), _source.size() };
}

shared_buffer crypt::pn_code::encode(const_buffer_view _source)
{
	// check) 
	RETURN_IF(_source.size() == 0, shared_buffer());

	// 1) get size
	const auto size_alloc = _source.size() + 1;

	// 1) alloc buffer
	auto buf_result = alloc_shared_buffer(size_alloc) ^ size_alloc;

	// 2) encode
	buf_result ^= encode(buf_result,  _source);

	// return) 
	return buf_result;
}


}