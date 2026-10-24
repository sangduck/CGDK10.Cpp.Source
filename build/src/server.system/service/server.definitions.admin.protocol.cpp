#include "pch.h"
#include <fstream>
#include <random>
#if !defined(_WIN32)
	#include <sys/stat.h>
#endif

using namespace CGDK;


shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_CERTIFY_REQUEST(const CGDK::server_system::sCERTIFY_INFO& _certify_info)
{
	// 1) get size of message
	const std::size_t size_of_message = 8 + get_size_of(_certify_info);

	// 2) alloc shared buffer
	auto temp_buffer = alloc_shared_buffer(size_of_message);

	// 3) message head
	temp_buffer.append<uint32_t>();
	temp_buffer.append<uint32_t>(eMESSAGE::SERVER::ADMIN::CERTIFY_REQUEST);
	temp_buffer.append<CGDK::server_system::sCERTIFY_INFO>(_certify_info);

	// 4) message length
	temp_buffer.front<uint32_t>() = temp_buffer.size<uint32_t>();

	// check)
	CGASSERT_ERROR(size_of_message == temp_buffer.size());

	// return) 
	return temp_buffer;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_CERTIFY_RESPONSE(eRESULT _result)
{
	// 1) get size of message
	const std::size_t size_of_message = 8 + get_size_of(_result);

	// 2) alloc shared buffer
	auto temp_buffer = alloc_shared_buffer(size_of_message);

	// 3) message head
	temp_buffer.append<uint32_t>();
	temp_buffer.append<uint32_t>(eMESSAGE::SERVER::ADMIN::CERTIFY_RESPONSE);
	temp_buffer.append<eRESULT>(_result);

	// 4) message length
	temp_buffer.front<uint32_t>() = temp_buffer.size<uint32_t>();

	// check)
	CGASSERT_ERROR(size_of_message == temp_buffer.size());

	// return) 
	return temp_buffer;
}

shared_buffer CGDK::make_message_SERVER_ADMIN_REQUEST_INIT_SERVICE()
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(8);

	// 2) write message
	buf_temp.append<uint32_t>(8);
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE);

	// return)
	return buf_temp;
}

shared_buffer CGDK::make_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE()
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(8);

	// 2) write message
	buf_temp.append<uint32_t>(8);
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE);

	// return)
	return buf_temp;
}

shared_buffer CGDK::make_message_SERVER_ADMIN_REQUEST_START_SERVICE()
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(8);

	// 2) write message
	buf_temp.append<uint32_t>(8);
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE);

	// return)
	return buf_temp;
}

shared_buffer CGDK::make_message_SERVER_ADMIN_REQUEST_STOP_SERVICE()
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(8);

	// 2) write message
	buf_temp.append<uint32_t>(8);
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE);

	// return)
	return buf_temp;
}

shared_buffer CGDK::make_message_SERVER_ADMIN_REQUEST_PAUSE_SERVICE()
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(8);

	// 2) write message
	buf_temp.append<uint32_t>(8);
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_PAUSE_SERVICE);

	// return)
	return buf_temp;
}

shared_buffer CGDK::make_message_SERVER_ADMIN_REQUEST_CONTINUE_SERVICE()
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(8);

	// 2) write message
	buf_temp.append<uint32_t>(8);
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE);

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_CERTIFY_REQUEST(const CGDK::server_system::sCERTIFY_INFO& _certify_info, std::string_view _certify_key)
{
	// check) key가 없으면 기존 형식으로 보낸다.
	if (_certify_key.empty())
		return make_message_SERVER_ADMIN_CERTIFY_REQUEST(_certify_info);

	// 1) get size of message
	const std::size_t size_of_message = 8 + get_size_of(_certify_info) + get_size_of(_certify_key);

	// 2) alloc shared buffer
	auto temp_buffer = alloc_shared_buffer(size_of_message);

	// 3) message
	temp_buffer.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	temp_buffer.append<uint32_t>(eMESSAGE::SERVER::ADMIN::CERTIFY_REQUEST);
	temp_buffer.append<CGDK::server_system::sCERTIFY_INFO>(_certify_info);
	temp_buffer.append<std::string_view>(_certify_key);

	// check)
	CGASSERT_ERROR(size_of_message == temp_buffer.size());

	// return) 
	return temp_buffer;
}

bool CGDK::server_system::is_certify_key_matched(std::string_view _expected, std::string_view _received) noexcept
{
	// check) key가 설정되지 않았으면 검사하지 않는다.
	if (_expected.empty())
		return true;

	// - 길이와 상관없이 같은 시간이 걸리도록 비교한다.
	std::size_t diff = _expected.size() ^ _received.size();
	for (std::size_t i = 0; i < _expected.size(); ++i)
		diff |= static_cast<std::size_t>(static_cast<unsigned char>(_expected[i]) ^ static_cast<unsigned char>(i < _received.size() ? _received[i] : 0));

	// return)
	return diff == 0;
}

std::string CGDK::server_system::get_certify_key_from_environment()
{
	// - CGDK_ADMIN_CERTIFY_KEY (설정 파일에 key를 쓰지 않을 때 사용한다.)
#if defined(_WIN32)
	char buffer[512];
	auto length = ::GetEnvironmentVariableA("CGDK_ADMIN_CERTIFY_KEY", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (length == 0 || length >= sizeof(buffer))
		return std::string();
	return std::string(buffer, length);
#else
	auto pvalue = std::getenv("CGDK_ADMIN_CERTIFY_KEY");
	return (pvalue != nullptr) ? std::string(pvalue) : std::string();
#endif
}

std::optional<std::string> CGDK::server_system::read_key_file(std::string_view _filename)
{
	// 1) read
	std::ifstream file(std::string(_filename), std::ios::binary);
	if (!file)
		return std::nullopt;

	std::string key((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	// 2) trim (앞뒤 공백/줄바꿈)
	auto f_is_space = [](char _ch) { return _ch == ' ' || _ch == '\t' || _ch == '\r' || _ch == '\n'; };
	while (key.empty() == false && f_is_space(key.back()))
		key.pop_back();
	std::size_t pos_begin = 0;
	while (pos_begin < key.size() && f_is_space(key[pos_begin]))
		++pos_begin;
	key.erase(0, pos_begin);

	// 3) permission (linux. group/other가 읽을 수 있으면 경고)
#if !defined(_WIN32)
	struct stat status_file {};
	if (::stat(std::string(_filename).c_str(), &status_file) == 0 && (status_file.st_mode & (S_IRWXG | S_IRWXO)) != 0)
		LOG_WARNING << "(warn) key file '"sv << _filename << "' can be read by other users. (chmod 600)"sv;
#endif

	// return)
	return key;
}

std::string CGDK::server_system::get_key_from_setting(context& _context, std::string_view _name, std::string_view _environment)
{
	// 1) "<name>_file"
	if (auto& context_file = _context.find(std::string(_name) + "_file"); context_file.is_valid())
	{
		auto filename = context_file.operator std::string();
		auto key = read_key_file(filename);
		if (key.has_value() && key->empty() == false)
			return *key;

		// - 읽지 못했다. key가 없는 것(=검사하지 않음)으로 동작하면 안 되므로 아무도 맞출 수 없는 key로 설정한다.
		LOG_ERROR << "(err ) fail to read key file '"sv << filename << "' ('"sv << _name << "_file'). nobody can be certified"sv;

		std::random_device random_device;
		std::string key_unmatchable = "!unreadable-key-file!";
		for (int i = 0; i < 8; ++i)
			key_unmatchable += std::to_string(random_device());
		return key_unmatchable;
	}

	// 2) "<name>"
	if (auto& context_key = _context.find(_name); context_key.is_valid())
		return context_key.operator std::string();

	// 3) environment
	if (_environment.empty())
		return std::string();

#if defined(_WIN32)
	char buffer[512];
	auto length = ::GetEnvironmentVariableA(std::string(_environment).c_str(), buffer, static_cast<DWORD>(sizeof(buffer)));
	if (length == 0 || length >= sizeof(buffer))
		return std::string();
	return std::string(buffer, length);
#else
	auto pvalue = std::getenv(std::string(_environment).c_str());
	return (pvalue != nullptr) ? std::string(pvalue) : std::string();
#endif
}
