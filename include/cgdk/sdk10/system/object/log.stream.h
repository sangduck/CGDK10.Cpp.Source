//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                              object classes                               *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
#include <format>

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// _log_stream
//
// << 연산자를 사용해 로그를 C++ iostream과 같이 남길 수 있도록 지원해주는 클래스이다.
//
//   _log_stream		Stream 객체로 c++ iostream의 std::cout 등에 해당하는 객체이다.
//	 _log_end		Stream의 끝을 나타의미하는 객체로 std::endl 과 같은 객체다.
//
//  example)
//       -> 기존 _log함수 사용 예 
//			LOG(plogger, log_type_t::INFO) << "Test Log {} {}", x, t.c_str());
//
//       -> 기본 log_target을 사용 시 log_target을 생략
//			LOG(log_type_t::info) << "Test Log";
//
//       -> fmt::format 사용예
//			LOG(log_type_t::info) << fmt::format("Test Log {} {}", x, t.c_str());
//
//       -> definition을 사용
//			LOG_INFO << "Test Log"sv;
//          (LOG(log_type_t::info) << "Test Log"와 동일한다.
//
//-----------------------------------------------------------------------------
struct log_continue_t
{
};

//-----------------------------------------------------------------------------
// CGDK::format
//
//  - LOG_INFO << CGDK::format("{} {}", a, b);
//    formats into the log buffer directly with std::format_to_n
//    (std::format makes a temporary std::string and it is copied again)
//  - the format string is checked at compile time like std::format
//  - holds references to the arguments : use it only in the same expression
//-----------------------------------------------------------------------------
template <class... TARGS>
struct log_format_t
{
	std::format_string<TARGS...>	fmt;
	std::tuple<TARGS&&...>			args;

	// - expected size (format string + strings, 24 for others) : to format once
	[[nodiscard]] std::size_t		estimate() const noexcept { return std::apply([this](const auto&... _a) { return fmt.get().size() + (_estimate(_a) + ... + 0); }, args); }

	template <class T>
	[[nodiscard]] static std::size_t _estimate(const T& _value) noexcept
	{
		if constexpr (std::is_convertible_v<const T&, std::string_view> && !std::is_pointer_v<std::decay_t<T>>)
			return std::string_view(_value).size();
		else
			return 24;
	}

	template <std::size_t... I>
	[[nodiscard]] std::format_to_n_result<char*> _format_to_n(char* _dest, std::size_t _size, std::index_sequence<I...>)
	{
		return std::format_to_n<char*, TARGS...>(_dest, static_cast<std::ptrdiff_t>(_size), fmt, std::forward<TARGS>(std::get<I>(args))...);
	}
};

template <class... TARGS>
[[nodiscard]] inline log_format_t<TARGS...> format(std::format_string<TARGS...> _format, TARGS&&... _args)
{
	return { _format, std::forward_as_tuple(std::forward<TARGS>(_args)...) };
}

class _log_stream
{
public:
	_log_stream();
	_log_stream(Ilogger* _plogger, log_type_t _type);
	_log_stream(log_type_t _type);
	~_log_stream() noexcept		{ CGASSERT_ERROR(plog_record.empty()); }

public:
	_log_stream&				operator << (char _rhs);
	_log_stream&				operator << (int8_t _rhs);
	_log_stream&				operator << (uint8_t _rhs);
	_log_stream&				operator << (int16_t _rhs);
	_log_stream&				operator << (uint16_t _rhs);
	_log_stream&				operator << (int32_t _rhs);
	_log_stream&				operator << (uint32_t _rhs);
	_log_stream&				operator << (int64_t _rhs);
	_log_stream&				operator << (uint64_t _rhs);
#if defined(_MSC_VER)
	_log_stream&				operator << (long _rhs);
	_log_stream&				operator << (unsigned long _rhs);
#else
	_log_stream&				operator << (long long _rhs);
	_log_stream&				operator << (unsigned long long _rhs);
#endif
	_log_stream&				operator << (float _rhs);
	_log_stream&				operator << (double _rhs);
	_log_stream&				operator << (const char* _rhs);
	_log_stream&				operator << (const std::string& _rhs);
	_log_stream&				operator << (std::string&& _rhs);
	_log_stream&				operator << (std::string_view _rhs);
	_log_stream&				operator << (const wchar_t* _rhs);
	_log_stream&				operator << (const std::wstring& _rhs);
	_log_stream&				operator << (std::wstring&& _rhs);
	_log_stream&				operator << (std::wstring_view _rhs);
	_log_stream&				operator << (void(*_pfunction)(_log_stream&));

	_log_stream&				operator << (log_continue_t /*_rhs*/);
	_log_stream&				operator << (log_level_t _rhs);
	_log_stream&				operator << (log_attribute_t _rhs);
	_log_stream&				operator << (log_origin_t _rhs);
	_log_stream&				operator << (log_source_t _rhs);
	_log_stream&				operator << (Ilog_source* _rhs);
	_log_stream&				operator << (log_destination_t _rhs);

	template <class... TARGS>
	_log_stream&				operator << (log_format_t<TARGS...>&& _rhs);
public:
			object_ptr<LOG_RECORD> plog_record;
	// - not referenced : the caller (or the cache of the default logger on this thread) keeps the logger alive
			Ilogger*			plogger{ nullptr };
			int64_t				pos{ 0 };
protected:
			template <class T>
			void				process_stream_in(T _rhs);
public:
	// - CGDK::format (log.stream.cpp)
	//   process_format_prepare : tail pointer + writable bytes (terminator excluded), nullptr if not logged
			std::pair<char*, std::size_t> process_format_prepare(std::size_t _size);
			void				process_format_commit(char* _dest, std::size_t _written) noexcept;
};

template <class... TARGS>
_log_stream& _log_stream::operator << (log_format_t<TARGS...>&& _rhs)
{
	try
	{
		// 1) room (expected size)
		auto [dest, size] = this->process_format_prepare(_rhs.estimate());
		if (dest == nullptr)
			return *this;

		// 2) format
		auto result = _rhs._format_to_n(dest, size, std::index_sequence_for<TARGS...>{});

		// 3) didn't fit : grow and format again
		if (static_cast<std::size_t>(result.size) > size)
		{
			auto [dest_grown, size_grown] = this->process_format_prepare(static_cast<std::size_t>(result.size));
			if (size_grown > size)
			{
				dest = dest_grown;
				size = size_grown;
				result = _rhs._format_to_n(dest, size, std::index_sequence_for<TARGS...>{});
			}
		}

		// 4) terminator + size (cut to fit)
		this->process_format_commit(dest, (static_cast<std::size_t>(result.size) < size) ? static_cast<std::size_t>(result.size) : size);
	}
	catch (...)
	{
	}

	// return)
	return *this;
}

//-----------------------------------------------------------------------------
// LOG_MESSAGE_EMPTY
//-----------------------------------------------------------------------------
class LOG_MESSAGE_EMPTY
{
public:
			LOG_MESSAGE_EMPTY&	operator << (char ) noexcept { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (int8_t ) noexcept { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (uint8_t ) noexcept { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (int16_t ) noexcept { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (uint16_t ) noexcept { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (int32_t ) noexcept { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (uint32_t ) noexcept { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (int64_t ) noexcept { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (uint64_t ) noexcept { return *this; }
		#if defined(_MSC_VER)
			LOG_MESSAGE_EMPTY&	operator << (long) noexcept { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (unsigned long) noexcept { return *this; }
		#else 
			LOG_MESSAGE_EMPTY&	operator << (long long) noexcept { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (unsigned long long) noexcept { return *this; }
		#endif
			LOG_MESSAGE_EMPTY&	operator << (float ) noexcept { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (double ) noexcept { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (const char* ) noexcept { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (std::string_view ) noexcept { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (std::wstring_view ) noexcept { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (void(*)(_log_stream&)) { return *this; }

			LOG_MESSAGE_EMPTY&	operator << (log_continue_t ) { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (log_level_t ) { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (log_attribute_t ) { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (log_origin_t ) { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (log_source_t ) { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (Ilog_source* ) { return *this; }
			LOG_MESSAGE_EMPTY&	operator << (log_destination_t ) { return *this; }
	template <class... TARGS>
			LOG_MESSAGE_EMPTY&	operator << (log_format_t<TARGS...>&& ) noexcept { return *this; }
};

//-----------------------------------------------------------------------------
// _log_endl
//-----------------------------------------------------------------------------
void log_endl(_log_stream& _log_stream);

struct _log_end
{
public:
	_log_stream&		operator = (_log_stream&& _log) { return _log_end::operator = (_log); }
	_log_stream&		operator = (_log_stream& _log);
	LOG_MESSAGE_EMPTY&	operator = (LOG_MESSAGE_EMPTY&& _log) { return _log; }
	LOG_MESSAGE_EMPTY&	operator = (LOG_MESSAGE_EMPTY& _log) { return _log; }
};

}
