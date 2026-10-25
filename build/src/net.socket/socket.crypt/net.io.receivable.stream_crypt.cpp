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

void net::io::receivable::Nstream_crypt::process_decrypt_received_after(const void* _pos)
{
	// check) no bytes taken as plain are being processed
	RETURN_IF(this->m_preceived_plain_end == nullptr);

	// 1) the range : from _pos (the end of the message that set the cryptor) to the end of the received bytes
	auto pbegin = static_cast<char*>(const_cast<void*>(_pos));
	auto pend = this->m_preceived_plain_end;

	if (pbegin < this->m_preceived_plain_begin)
		pbegin = this->m_preceived_plain_begin;

	// - once (the bytes are decrypted now)
	this->m_preceived_plain_begin = nullptr;
	this->m_preceived_plain_end = nullptr;

	// check) nothing after the message
	RETURN_IF(pbegin >= pend);

	// 2) get the cryptor (set just before)
	auto pcryptor = this->get_cryptor();

	// check)
	RETURN_IF(pcryptor.empty());

	// 3) decrypt in place (a stream cipher : the size does not change)
	const auto size = static_cast<std::size_t>(pend - pbegin);
	auto buf_decrypt = pcryptor->decrypt(buffer_view(pbegin, size));

	// check)
	if (buf_decrypt.size() != size)
		throw throwable::failure(eRESULT::FAIL, 0, "decrypted size of the received bytes changed"sv);

	memcpy(pbegin, buf_decrypt.data(), size);
}
