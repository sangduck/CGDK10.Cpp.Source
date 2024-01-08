#include "pch.h"

using namespace CGDK;


void net::io::receivable::Nstream_crypt::initialize_encoder()
{
	// declare)
	auto temp_buffer = alloc_shared_buffer(4096);

	// 1) message의 head부분을 설정한다.
	temp_buffer.append<uint32_t>();
	temp_buffer.append<uint32_t>(eMESSAGE::NETWORK::SECURITY::SEND_DECODER_INFO);

	// 2) Decoder를 생성한다.
	object_ptr<Idecoder> pdecoder = this->create_decoder();

	// 3) Decoder를 초기화한다.
	if(pdecoder.exist())
	{
		// 4) Key생성을 위한 값을 Random하게 생성해 낸다..
		temp_buffer += pdecoder->initialize_decoder();
	}
	else
	{
		temp_buffer.append<eCRYPTION_TYPE>(eCRYPTION_TYPE::NONE);
	}

	// 5) Decoder를 설정한다.
	this->m_pdecoder = pdecoder;

	// 6) message를 마무리한다.(Size를 써넣고 CRC를 붙힌다.)
	temp_buffer.front<uint32_t>() = static_cast<uint32_t>(temp_buffer.size() + SIZE_OF_CRC);
	temp_buffer.append<CRC_T>(edc::default_crc.get_crc(temp_buffer));

	// 7) Client에게 보냄.
	this->send(temp_buffer);
}

void net::io::receivable::Nstream_crypt::process_decoding( buffer_view& _buffer)
{
	// 1) Recieve Cryptor를 얻는다.
	object_ptr<Idecoder> pdecoder = this->get_decoder();

	// 2) receive Cryptor가 있으면 복호화한다.
	if(pdecoder.exist())
	{
		pdecoder->decode(_buffer, _buffer);
	}
}

void net::io::receivable::Nstream_crypt::process_reset_receivable()
{
	// 1) Decoder를 reset한다.
	this->reset_decoder();

	// 2) receivable을 reset한다.
	this->net::io::receivable::Nstream::process_reset_receivable();
}