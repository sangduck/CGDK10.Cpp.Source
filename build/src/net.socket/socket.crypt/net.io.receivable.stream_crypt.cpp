#include "pch.h"

using namespace CGDK;


shared_buffer net::io::receivable::Nstream_crypt::process_decryptor(shared_buffer& _buffer)
{
	// 1) Receive Cryptor를 얻는다.
	object_ptr<Icryptor> pcryptor = this->get_cryptor();

	// check) 그냥 리턴한다.
	if (pcryptor.empty())
		return std::move(_buffer);

	// 2) receive Cryptor가 있으면 복호화후 리턴한다.
	return pcryptor->decrypt(_buffer);
}
