#include "pch.h"
#include <atomic>
#include <filesystem>
#include <mutex>
#include <thread>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

//-----------------------------------------------------------------------------
// ssl (net::io::Nsocket_ssl) : loopback echo
//
//  - acceptor(ssl_context : a self-signed certificate made by the test. valid for 1 day : it does not expire) <- tcp_ssl client
//  - the client sends messages of several sizes (small ~ several TLS records) at once,
//    the server echoes them. the client checks the order and the contents.
//  - the encrypted receive path (decrypt + framing) of both sides is used.
//-----------------------------------------------------------------------------
namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	// - [uint32 size][uint32 index][pattern...]
	std::string make_ssl_message(uint32_t _index, std::size_t _size)
	{
		std::string message(_size, '\0');
		const uint32_t size = static_cast<uint32_t>(_size);
		memcpy(message.data(), &size, 4);
		memcpy(message.data() + 4, &_index, 4);
		for (std::size_t i = 8; i < _size; ++i)
			message[i] = static_cast<char>('a' + (i + _index) % 26);
		return message;
	}

	// - the port of the test (CGDK_TEST_SSL_PORT or 29400 + pid % 500)
	uint16_t ssl_test_port()
	{
		return static_cast<uint16_t>(29400 + api::get_process_id() % 500);
	}

	// - self-signed certificate (RSA 2048, CN=localhost, valid for 1 day) -> _file_crt, _file_key (PEM)
	bool make_test_certificate(const std::filesystem::path& _file_crt, const std::filesystem::path& _file_key)
	{
		// 1) key
		EVP_PKEY* pkey = EVP_RSA_gen(2048);
		if (pkey == nullptr)
			return false;

		// 2) certificate
		X509* px509 = X509_new();
		ASN1_INTEGER_set(X509_get_serialNumber(px509), 1);
		X509_gmtime_adj(X509_getm_notBefore(px509), -60);
		X509_gmtime_adj(X509_getm_notAfter(px509), 60 * 60 * 24);
		X509_set_pubkey(px509, pkey);
		X509_NAME* pname = X509_get_subject_name(px509);
		X509_NAME_add_entry_by_txt(pname, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>("localhost"), -1, -1, 0);
		X509_set_issuer_name(px509, pname);
		bool result = X509_sign(px509, pkey, EVP_sha256()) > 0;

		// 3) write (PEM)
		if (result)
		{
			BIO* pbio_crt = BIO_new_file(_file_crt.string().c_str(), "w");
			BIO* pbio_key = BIO_new_file(_file_key.string().c_str(), "w");
			result = pbio_crt != nullptr && pbio_key != nullptr
				&& PEM_write_bio_X509(pbio_crt, px509) == 1
				&& PEM_write_bio_PrivateKey(pbio_key, pkey, nullptr, nullptr, 0, nullptr, nullptr) == 1;
			BIO_free(pbio_crt);
			BIO_free(pbio_key);
		}

		X509_free(px509);
		EVP_PKEY_free(pkey);
		return result;
	}
}

class ssl_echo_server_socket : public net::socket::tcp_ssl<>
{
protected:
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		this->send(_msg.buf_message);
		return eRESULT::DONE;
	}
};

class ssl_test_client_socket : public net::socket::tcp_ssl<>, public net::io::connect_requestable::tcp::Ndefault
{
public:
			std::atomic<bool>	is_connected{ false };
			std::mutex			lockable;
			std::vector<std::string> received;

protected:
	virtual void				on_connect() override { this->is_connected = true; }
	virtual void				on_fail_connect(uint64_t /*_disconnect_reason*/) override {}
	virtual void				on_disconnect(uint64_t /*_disconnect_reason*/) override { this->is_connected = false; }
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		std::lock_guard lock(this->lockable);
		this->received.emplace_back(reinterpret_cast<const char*>(_msg.buf_message.data()), _msg.buf_message.size());
		return eRESULT::DONE;
	}
};

TEST(ssl_loopback, echo)
{
	// 0) certificate (made by the test : temp/cgdk_ssl_test_<pid>)
	const auto path_temp = std::filesystem::temp_directory_path() / ("cgdk_ssl_test_" + std::to_string(api::get_process_id()));
	std::filesystem::create_directories(path_temp);
	const auto file_crt = (path_temp / "test_server.crt").string();
	const auto file_key = (path_temp / "test_server.key").string();
	ASSERT_TRUE(make_test_certificate(file_crt, file_key)) << "make the test certificate (" << file_crt << ")";

	// 1) server
	auto pssl_context = make_object<crypt::ssl_context>(std::string_view(file_crt), std::string_view(file_key));
	auto pacceptor = make_own<net::acceptor<ssl_echo_server_socket>>();
	const auto port = ssl_test_port();
	pacceptor->start({ { net::ip::address_v4::loopback(), port }, pssl_context });

	// 2) client (handshake)
	auto psocket = make_own<ssl_test_client_socket>();
	psocket->start({ { net::ip::address_v4::loopback(), port } });

	for (int i = 0; i < 100 && psocket->is_connected == false; ++i)
		std::this_thread::sleep_for(50ms);
	ASSERT_TRUE(psocket->is_connected) << "ssl connection (port " << port << ")";

	// 3) send : small ~ larger than a TLS record (16KB)
	const std::vector<std::size_t> sizes = { 8, 100, 1000, 16384, 16385, 40000, 60000, 9, 32768 };
	for (std::size_t i = 0; i < sizes.size(); ++i)
	{
		auto message = make_ssl_message(static_cast<uint32_t>(i), sizes[i]);
		auto buf = alloc_shared_buffer(message.size());
		buf.append(message.size(), message.data());
		ASSERT_TRUE(psocket->send(buf)) << "send " << i;
	}

	// 4) receive (echo)
	for (int i = 0; i < 200; ++i)
	{
		{
			std::lock_guard lock(psocket->lockable);
			if (psocket->received.size() >= sizes.size())
				break;
		}
		std::this_thread::sleep_for(50ms);
	}

	{
		std::lock_guard lock(psocket->lockable);
		ASSERT_EQ(psocket->received.size(), sizes.size());
		for (std::size_t i = 0; i < sizes.size(); ++i)
			EXPECT_EQ(psocket->received[i], make_ssl_message(static_cast<uint32_t>(i), sizes[i])) << "message " << i << " (size " << sizes[i] << ")";
	}

	// 5) close
	psocket->stop();
	pacceptor->stop();

	std::error_code error_code;
	std::filesystem::remove_all(path_temp, error_code);
}

}
