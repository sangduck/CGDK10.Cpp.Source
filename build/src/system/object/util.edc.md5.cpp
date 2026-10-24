#include "pch.h"
#include <bit>
#include <cstring>

//#if defined(_WIN32)
namespace CGDK
{

#define	SIZE_OF_MD5_FILEBUFFER	65536

namespace
{
	void _md5_blocks(uint32_t* _state, const uint8_t* _p, size_t _count) noexcept;
}


edc::md5::md5() noexcept :
	m_buffer{},
	m_is_process(false),
	m_state(),
	m_count{0,0}
{
}

edc::md5::~md5() noexcept
{
}

void edc::md5::push_md5(const void* _buffer, uint32_t _size)
{ 
	// check) 
	RETURN_IF(m_is_process != true);

	// check) 
	RETURN_IF(_size ==0);


	//-----------------------------------------------------------------
	// 1. 전체 길이를 구해준다.(Bit단위로...)
	//-----------------------------------------------------------------
	// declare) 
	uint32_t	s_index; 
	uint32_t	s_part_size; 

	// 1) 남은 길이를 구한다.(512Bit인지확인)
	s_index		 = (m_count[0] >> 3) & 0x3f; 

	// 2) Bit로 크기를 구하기 위해 8을 곱한다.
	// - Bit로 변환하기 위해 8을 곱해서 길이를 더한다.
	m_count[0]	+= (_size << 3);

	// - 자리올림처리...(Overflow났을 경우 1을 더해줌, 값이 더 작아졌다는 것은 Overflow를 의미함.)
	if(m_count[0] < (_size << 3))
	{
		m_count[1]++; 
	}

	// - 상위 3Bit를 더함.
	m_count[1]	+= _size >> 29;

	// 3) 무자란 길이를 구한다.
	s_part_size	 = 64 - s_index; 

	// check) 512Bit단위로만 동작하게 함...(512가 안 될 경우 값만 복사하고 끝낸다.)
	if(_size < s_part_size)
	{
		// 값을 복사한다.
		memcpy(m_buffer+s_index, _buffer, _size);
		return;
	}


	//-----------------------------------------------------------------
	// 2. 이미 존재하던 데이터어 붙혀진 데이터 MD5처리
	//-----------------------------------------------------------------
	// - m_buffer에 이전에 512Bit가 되지 않아서 처리되지 않고 그냥 복사
	//   만 해놓았던 데이터에 이어 붙여 512Bit를 만든후 MD5로 계산한다.

	// 1) m_buffer의 뒷부분에 데이터를 붙인다.
	memcpy(m_buffer + s_index, _buffer, s_part_size);

	// 2) 붙인 Data의 MD5계산을 한다.
	transform_md5(reinterpret_cast<void*>(&m_state), reinterpret_cast<void*>(m_buffer));


	//-----------------------------------------------------------------
	// 3. 가운데부분 MD5처리.
	//-----------------------------------------------------------------
	// - 이어붙힌 이후로 MD5는 512Bit단위로 MD5를 구하면 된다.

	// declare)
	uint32_t i;

	// 1) 그 이후부터 16 uint32_t식 Block으로 MD5값을 구한다.
	const uint32_t count_blocks = (_size - s_part_size) / 64;
	_md5_blocks(m_state.data.data(), reinterpret_cast<const uint8_t*>(_buffer) + s_part_size, count_blocks);
	i = s_part_size + count_blocks * 64;

	//-----------------------------------------------------------------
	// 4. 남은 Byte처리
	//-----------------------------------------------------------------
	// - 512Bit가 되지 않은 나머지는 다음을 위해 복사해둔다.

	// 1) 남은 Byte수를 구한다.
	uint32_t	remainSize	 = _size - i;

	// 1) 512Bit의 배수가 되지 않고 남은 값들은 복사해 놓는다.
	memcpy(m_buffer, reinterpret_cast<const void*>(reinterpret_cast<const uint8_t*>(_buffer) + i), remainSize);
} 

bool edc::md5::push_md5(FILE_HANDLE _handle_file, uint32_t _Start, uint32_t _End)
{
	// check) 
	RETURN_IF(m_is_process!=true, false);

	// check) File이 제대로 열였는가?
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE, false);

	// check) File이 제대로 열였는가?
	RETURN_IF(_Start == _End, false);

	// 2) 임시 File shared_buffer를 만든다.
	auto buf_file = alloc_shared_buffer(SIZE_OF_MD5_FILEBUFFER);
	auto pbuf_file = buf_file.data<uint8_t>();

	// check) 메모리가 제대로 할당되었는지 확인한다.
	RETURN_IF(pbuf_file == nullptr, false);

	// 3) MD5 값 만들기
	for(uint32_t position= _Start; position<_End; position+=SIZE_OF_MD5_FILEBUFFER)
	{ 
		// - file seek
		api::file::seek(_handle_file, position, eFILE_ORIGIN::BEGIN);

		// - 읽을 크기
		auto dwToRead = _End-position;

		SATURATE_MAX(dwToRead, SIZE_OF_MD5_FILEBUFFER);

		// - File에서 읽어들인다.
		auto dwReaded = api::file::read(_handle_file, dwToRead, pbuf_file);

		// Assert) 읽어야 하는 Byte와 읽은 Byte수가 같은가?
		CGASSERT_ERROR(dwToRead==dwReaded);

		// check) 읽어들인 값이 0이면 끝낸다.
		if(dwReaded == 0)
			break;

		// - MD5값을 계산한다.
		push_md5(pbuf_file, static_cast<uint32_t>(dwReaded));
	}

	// return) 성공
	return true;
}

//-----------------------------------------------------------------------------
// md5 block transform (RFC 1321)
//
//  - MD5 is a chain of dependent 32-bit operations inside one stream, so SIMD
//    can't speed up a single stream. The blocks are processed directly from the
//    source with the state kept in registers.
//-----------------------------------------------------------------------------
namespace
{
	#define CGDK_MD5_F(b, c, d)	((d) ^ ((b) & ((c) ^ (d))))
	#define CGDK_MD5_G(b, c, d)	((c) ^ ((d) & ((b) ^ (c))))
	#define CGDK_MD5_H(b, c, d)	((b) ^ (c) ^ (d))
	#define CGDK_MD5_I(b, c, d)	((c) ^ ((b) | ~(d)))
	#define CGDK_MD5_STEP(f, a, b, c, d, x, t, s) \
		a += f(b, c, d) + (x) + static_cast<uint32_t>(t); \
		a = std::rotl(a, s) + (b);

	[[nodiscard]] inline uint32_t _md5_load32(const uint8_t* _p) noexcept
	{
		uint32_t value;
		memcpy(&value, _p, sizeof(value));
		if constexpr (std::endian::native == std::endian::big)
			value = (value >> 24) | ((value >> 8) & 0x0000ff00u) | ((value << 8) & 0x00ff0000u) | (value << 24);
		return value;
	}

	void _md5_blocks(uint32_t* _state, const uint8_t* _p, size_t _count) noexcept
	{
		uint32_t a = _state[0];
		uint32_t b = _state[1];
		uint32_t c = _state[2];
		uint32_t d = _state[3];

		for (; _count != 0; --_count, _p += 64)
		{
			uint32_t x[16];
			for (int i = 0; i < 16; ++i)
				x[i] = _md5_load32(_p + i * 4);

			const uint32_t aa = a, bb = b, cc = c, dd = d;

			// 1) round 1
			CGDK_MD5_STEP(CGDK_MD5_F, a, b, c, d, x[ 0], 0xd76aa478,  7)
			CGDK_MD5_STEP(CGDK_MD5_F, d, a, b, c, x[ 1], 0xe8c7b756, 12)
			CGDK_MD5_STEP(CGDK_MD5_F, c, d, a, b, x[ 2], 0x242070db, 17)
			CGDK_MD5_STEP(CGDK_MD5_F, b, c, d, a, x[ 3], 0xc1bdceee, 22)
			CGDK_MD5_STEP(CGDK_MD5_F, a, b, c, d, x[ 4], 0xf57c0faf,  7)
			CGDK_MD5_STEP(CGDK_MD5_F, d, a, b, c, x[ 5], 0x4787c62a, 12)
			CGDK_MD5_STEP(CGDK_MD5_F, c, d, a, b, x[ 6], 0xa8304613, 17)
			CGDK_MD5_STEP(CGDK_MD5_F, b, c, d, a, x[ 7], 0xfd469501, 22)
			CGDK_MD5_STEP(CGDK_MD5_F, a, b, c, d, x[ 8], 0x698098d8,  7)
			CGDK_MD5_STEP(CGDK_MD5_F, d, a, b, c, x[ 9], 0x8b44f7af, 12)
			CGDK_MD5_STEP(CGDK_MD5_F, c, d, a, b, x[10], 0xffff5bb1, 17)
			CGDK_MD5_STEP(CGDK_MD5_F, b, c, d, a, x[11], 0x895cd7be, 22)
			CGDK_MD5_STEP(CGDK_MD5_F, a, b, c, d, x[12], 0x6b901122,  7)
			CGDK_MD5_STEP(CGDK_MD5_F, d, a, b, c, x[13], 0xfd987193, 12)
			CGDK_MD5_STEP(CGDK_MD5_F, c, d, a, b, x[14], 0xa679438e, 17)
			CGDK_MD5_STEP(CGDK_MD5_F, b, c, d, a, x[15], 0x49b40821, 22)

			// 2) round 2
			CGDK_MD5_STEP(CGDK_MD5_G, a, b, c, d, x[ 1], 0xf61e2562,  5)
			CGDK_MD5_STEP(CGDK_MD5_G, d, a, b, c, x[ 6], 0xc040b340,  9)
			CGDK_MD5_STEP(CGDK_MD5_G, c, d, a, b, x[11], 0x265e5a51, 14)
			CGDK_MD5_STEP(CGDK_MD5_G, b, c, d, a, x[ 0], 0xe9b6c7aa, 20)
			CGDK_MD5_STEP(CGDK_MD5_G, a, b, c, d, x[ 5], 0xd62f105d,  5)
			CGDK_MD5_STEP(CGDK_MD5_G, d, a, b, c, x[10], 0x02441453,  9)
			CGDK_MD5_STEP(CGDK_MD5_G, c, d, a, b, x[15], 0xd8a1e681, 14)
			CGDK_MD5_STEP(CGDK_MD5_G, b, c, d, a, x[ 4], 0xe7d3fbc8, 20)
			CGDK_MD5_STEP(CGDK_MD5_G, a, b, c, d, x[ 9], 0x21e1cde6,  5)
			CGDK_MD5_STEP(CGDK_MD5_G, d, a, b, c, x[14], 0xc33707d6,  9)
			CGDK_MD5_STEP(CGDK_MD5_G, c, d, a, b, x[ 3], 0xf4d50d87, 14)
			CGDK_MD5_STEP(CGDK_MD5_G, b, c, d, a, x[ 8], 0x455a14ed, 20)
			CGDK_MD5_STEP(CGDK_MD5_G, a, b, c, d, x[13], 0xa9e3e905,  5)
			CGDK_MD5_STEP(CGDK_MD5_G, d, a, b, c, x[ 2], 0xfcefa3f8,  9)
			CGDK_MD5_STEP(CGDK_MD5_G, c, d, a, b, x[ 7], 0x676f02d9, 14)
			CGDK_MD5_STEP(CGDK_MD5_G, b, c, d, a, x[12], 0x8d2a4c8a, 20)

			// 3) round 3
			CGDK_MD5_STEP(CGDK_MD5_H, a, b, c, d, x[ 5], 0xfffa3942,  4)
			CGDK_MD5_STEP(CGDK_MD5_H, d, a, b, c, x[ 8], 0x8771f681, 11)
			CGDK_MD5_STEP(CGDK_MD5_H, c, d, a, b, x[11], 0x6d9d6122, 16)
			CGDK_MD5_STEP(CGDK_MD5_H, b, c, d, a, x[14], 0xfde5380c, 23)
			CGDK_MD5_STEP(CGDK_MD5_H, a, b, c, d, x[ 1], 0xa4beea44,  4)
			CGDK_MD5_STEP(CGDK_MD5_H, d, a, b, c, x[ 4], 0x4bdecfa9, 11)
			CGDK_MD5_STEP(CGDK_MD5_H, c, d, a, b, x[ 7], 0xf6bb4b60, 16)
			CGDK_MD5_STEP(CGDK_MD5_H, b, c, d, a, x[10], 0xbebfbc70, 23)
			CGDK_MD5_STEP(CGDK_MD5_H, a, b, c, d, x[13], 0x289b7ec6,  4)
			CGDK_MD5_STEP(CGDK_MD5_H, d, a, b, c, x[ 0], 0xeaa127fa, 11)
			CGDK_MD5_STEP(CGDK_MD5_H, c, d, a, b, x[ 3], 0xd4ef3085, 16)
			CGDK_MD5_STEP(CGDK_MD5_H, b, c, d, a, x[ 6], 0x04881d05, 23)
			CGDK_MD5_STEP(CGDK_MD5_H, a, b, c, d, x[ 9], 0xd9d4d039,  4)
			CGDK_MD5_STEP(CGDK_MD5_H, d, a, b, c, x[12], 0xe6db99e5, 11)
			CGDK_MD5_STEP(CGDK_MD5_H, c, d, a, b, x[15], 0x1fa27cf8, 16)
			CGDK_MD5_STEP(CGDK_MD5_H, b, c, d, a, x[ 2], 0xc4ac5665, 23)

			// 4) round 4
			CGDK_MD5_STEP(CGDK_MD5_I, a, b, c, d, x[ 0], 0xf4292244,  6)
			CGDK_MD5_STEP(CGDK_MD5_I, d, a, b, c, x[ 7], 0x432aff97, 10)
			CGDK_MD5_STEP(CGDK_MD5_I, c, d, a, b, x[14], 0xab9423a7, 15)
			CGDK_MD5_STEP(CGDK_MD5_I, b, c, d, a, x[ 5], 0xfc93a039, 21)
			CGDK_MD5_STEP(CGDK_MD5_I, a, b, c, d, x[12], 0x655b59c3,  6)
			CGDK_MD5_STEP(CGDK_MD5_I, d, a, b, c, x[ 3], 0x8f0ccc92, 10)
			CGDK_MD5_STEP(CGDK_MD5_I, c, d, a, b, x[10], 0xffeff47d, 15)
			CGDK_MD5_STEP(CGDK_MD5_I, b, c, d, a, x[ 1], 0x85845dd1, 21)
			CGDK_MD5_STEP(CGDK_MD5_I, a, b, c, d, x[ 8], 0x6fa87e4f,  6)
			CGDK_MD5_STEP(CGDK_MD5_I, d, a, b, c, x[15], 0xfe2ce6e0, 10)
			CGDK_MD5_STEP(CGDK_MD5_I, c, d, a, b, x[ 6], 0xa3014314, 15)
			CGDK_MD5_STEP(CGDK_MD5_I, b, c, d, a, x[13], 0x4e0811a1, 21)
			CGDK_MD5_STEP(CGDK_MD5_I, a, b, c, d, x[ 4], 0xf7537e82,  6)
			CGDK_MD5_STEP(CGDK_MD5_I, d, a, b, c, x[11], 0xbd3af235, 10)
			CGDK_MD5_STEP(CGDK_MD5_I, c, d, a, b, x[ 2], 0x2ad7d2bb, 15)
			CGDK_MD5_STEP(CGDK_MD5_I, b, c, d, a, x[ 9], 0xeb86d391, 21)

			a += aa;
			b += bb;
			c += cc;
			d += dd;
		}

		_state[0] = a;
		_state[1] = b;
		_state[2] = c;
		_state[3] = d;
	}

	#undef CGDK_MD5_F
	#undef CGDK_MD5_G
	#undef CGDK_MD5_H
	#undef CGDK_MD5_I
	#undef CGDK_MD5_STEP
}

// 16 x uint32_t (64 bytes) block
void edc::md5::transform_md5(void* _buffer, const void* _psource)
{
	_md5_blocks(reinterpret_cast<uint32_t*>(_buffer), reinterpret_cast<const uint8_t*>(_psource), 1);
}

bool edc::md5::initialize()
{ 
	// 1) MD5 초기화를 한다.
	m_state.data[0]	 = 0x67452301; 
	m_state.data[1]	 = 0xefcdab89; 
	m_state.data[2]	 = 0x98badcfe; 
	m_state.data[3]	 = 0x10325476; 

	m_count[0]			 = 0; 
	m_count[1]			 = 0; 

	m_is_process		 = true;

	// return) 
	return true;
}

void edc::md5::padding()
{ 
	// check) 
	RETURN_IF(m_is_process!=true);

	// 1) padding 초기화
	uint8_t s_padding[64]; 
	std::memset(reinterpret_cast<void*>(&s_padding), 0, sizeof(s_padding));
	s_padding[0] = 0x80;

	// 2) 
	// - message length in bits (little endian, before the padding is added)
	uint8_t s_bits[8];
	for (int i = 0; i < 8; ++i)
		s_bits[i] = static_cast<uint8_t>(m_count[i >> 2] >> ((i & 3) * 8));

	uint32_t s_index = (m_count[0] >> 3) & 0x3f;		// 64Byte의 나머지 구함.
	uint32_t s_padding_size = (s_index < 56) ? (56 - s_index) : (120 - s_index);
	push_md5(reinterpret_cast<void*>(s_padding), s_padding_size); 

	// 3) 
	push_md5(reinterpret_cast<void*>(s_bits), sizeof(s_bits)); 

	// 3) 
	m_is_process = false;
}

bool edc::md5::calculate_md5(const std::string_view _filename)
{ 
	// 1) MD5를 초기화한다.
	initialize();

	// 2) File open한다.
	auto hFile = api::file::open(_filename, eFILE_OPERATION::READ);

	// check) hFile이 nullptr이면 끝낸다.
	RETURN_IF(hFile == INVALID_FILE_HANDLE, false);

	// 3) 임시 File shared_buffer를 만든다.
	auto buf_file = alloc_shared_buffer(SIZE_OF_MD5_FILEBUFFER);
	auto pbuf_file = buf_file.data<uint8_t>();

	// check) 메모리가 제대로 할당되었는지 확인한다.
	if (pbuf_file == nullptr) { api::file::close(hFile); return false; }

	// 4) MD5 값 만들기
	for(;;)
	{ 
		// - File에서 읽어들인다.
		auto s_read_bytes = api::file::read(hFile, SIZE_OF_MD5_FILEBUFFER, pbuf_file);

		// check) 읽어들인 값이 0이면 끝낸다.
		BREAK_IF(s_read_bytes==0);

		// - MD5값을 계산한다.
		push_md5(pbuf_file, static_cast<uint32_t>(s_read_bytes));
	}

	// 5) File을 닫는다.
	api::file::close(hFile);

	// 7) padding
	padding();

	// return) 성공
	return true;
} 

bool edc::md5::calculate_md5(const std::wstring_view _filename)
{
	char	str_buffer[MAX_STRING_LENGTH]{};

	return	calculate_md5(encode_string(str_buffer, _filename.data()));
}

}

//#endif
