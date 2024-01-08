#include "pch.h"

using namespace CGDK;

//
bool net::io::sendable::Ntcp_crypt::process_initialize_encoder( const shared_buffer& _buffer)
{
	// check) _buffer.data_가 0이어서는 안된다.
	CGASSERT(_buffer.data() != nullptr, false)

	// check) _array_buffer->size_가 0이어서는 안된다.
	CGASSERT(_buffer.size() != 0, false)

	// 1) Encryption Code를 설정한다.
	eCRYPTION_TYPE encoder_type = _buffer.front<eCRYPTION_TYPE>(0);	// Offset

	// 2) Cryptor를 생성한다.
	auto pencoder = this->process_create_encoder(encoder_type);

	// 3) Cryptor가 있을 경우 초기화한다.
	if(pencoder.exist())
	{
		int seed_length = _buffer.size<int>();

		// check) Seed의 크기가 0보다 커야한다.
		RETURN_IF(seed_length <= 0, false);

		// - 초기화한다.
		pencoder->initialize_encoder(buffer_view{ _buffer.get_front_ptr(), static_cast<std::size_t>(seed_length) });
	}

	// 4) Cryptor를 설정한다.
	this->set_encoder(pencoder);

	// return) 성공!
	return true;
}

void net::io::sendable::Ntcp_crypt::process_reset_sendable() noexcept
{
	// 1) Encoder를 reset한다.
	this->reset_encoder();

	// 2) sendable을 reset한다.
	this->net::io::sendable::tcp::Ndefault::process_reset_sendable();
}

object_ptr<Iencoder> net::io::sendable::Ntcp_crypt::process_create_encoder( eCRYPTION_TYPE /*_type*/)
{
	// declare) 
	object_ptr<Iencoder> pencoder;

	//switch(_type)
	//{
	//case	eCRYPTION_TYPE::PNCODE_BYTE:
	//		pencoder = make_object<crypt::CPNCodeBYTE>();
	//		break;

	//case	eCRYPTION_TYPE::PNCODE_DWORD:
	//		pencoder = make_object<crypt::CPNCodeDWORD>();
	//		break;

	//default:
	//		break;
	//};

	// return) 생성한 Cryptor를 Return한다.
	return	pencoder;
}
