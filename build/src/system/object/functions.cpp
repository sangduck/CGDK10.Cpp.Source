#include "pch.h"
#include <cstring>


// ----------------------------------------------------------------------------
// to_string scaled_dotted
// ----------------------------------------------------------------------------
std::string_view to_string_dot_seperated_char(char* _buffer, size_t /*_size*/, uint64_t _value)
{
	// check)
	if (_value == 0)
	{
		_buffer[0] = '0';
		_buffer[1] = 0;
		return std::string_view(_buffer, 1);
	}

	char result[128];
	int pos = 127;
	result[pos--] = 0;
	while (_value)
	{
		if (!((pos - 3) % 4))
			result[pos--] = ',';
		result[pos--] = (_value % 10) + '0';
		_value /= 10;
	}

	auto len = 126 - pos;
	std::memcpy(_buffer, result + pos + 1, (len + 1) * sizeof(char));

	// return) 
	return std::string_view(_buffer, len);
}

std::string_view to_string_dot_seperated_char(char* _buffer, size_t /*_size*/, float _value)
{
	// check)
	if (_value == 0)
	{
		_buffer[0] = '0';
		_buffer[1] = 0;
		return std::string_view(_buffer, 1);
	}

	uint64_t temp_value = static_cast<uint64_t>(_value);
	char result[128];
	int pos = 127;
	result[pos--] = 0;
	while (temp_value)
	{
		if (!((pos - 3) % 4))
			result[pos--] = ',';
		result[pos--] = (temp_value % 10) + '0';
		temp_value /= 10;
	}

	auto len = 126 - pos;
	std::memcpy(_buffer, result + pos + 1, (len + 1) * sizeof(char));

	// return) 
	return std::string_view(_buffer, len);
}

std::wstring_view to_string_dot_seperated_wchar(wchar_t* _buffer, size_t /*_size*/, uint64_t _value)
{
	// check)
	if (_value == 0)
	{
		_buffer[0] = L'0';
		_buffer[1] = 0;
		return std::wstring_view(_buffer, 1);
	}

	wchar_t result[128];
	int pos = 127;
	result[pos--] = 0;
	while (_value)
	{
		if (!((pos - 3) % 4))
			result[pos--] = L',';
		result[pos--] = (_value % 10) + L'0';
		_value /= 10;
	}

	auto len = 126 - pos;
	std::memcpy(_buffer, result + pos + 1, (len + 1) * sizeof(wchar_t));

	// return) 
	return std::wstring_view(_buffer, len);
}

std::wstring_view to_string_dot_seperated_wchar(wchar_t* _buffer, size_t /*_size*/, float _value)
{
	// check)
	if (_value == 0)
	{
		_buffer[0] = L'0';
		_buffer[1] = 0;
		return std::wstring_view(_buffer, 1);
	}

	uint64_t temp_value = static_cast<uint64_t>(_value);
	wchar_t result[128];
	int pos = 127;
	result[pos--] = 0;
	while (temp_value)
	{
		if (!((pos - 3) % 4))
			result[pos--] = L',';
		result[pos--] = (temp_value % 10) + L'0';
		temp_value /= 10;
	}

	auto len = 126 - pos;
	std::memcpy(_buffer, result + pos + 1, (len + 1) * sizeof(wchar_t));

	// return) 
	return std::wstring_view(_buffer, len);
}

// ----------------------------------------------------------------------------
// to_string scaled
// ----------------------------------------------------------------------------
template<>
std::string_view CGDK::to_string_scaled<char, int32_t>(char* _buffer, size_t _size, int32_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::string_view());

	if (_value >> 20)
		__sprintf(_buffer, _size, "%.2fM"sv, ((float)_value) / 1000000.0f);
	else if (_value >> 10)
		__sprintf(_buffer, _size, "%.2fK"sv, ((float)_value) / 1000.0f);
	else
		__sprintf(_buffer, _size, "%.2f"sv, ((float)_value));

	return std::string_view(_buffer);
}
template<>
std::string_view CGDK::to_string_scaled<char, uint32_t>(char* _buffer, size_t _size, uint32_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::string_view());

	if (_value >> 20)
		__sprintf(_buffer, _size, "%.2fM"sv, ((float)_value) / 1000000.0f);
	else if (_value >> 10)
		__sprintf(_buffer, _size, "%.2fK"sv, ((float)_value) / 1000.0f);
	else
		__sprintf(_buffer, _size, "%.2f"sv, ((float)_value));

	return	std::string_view(_buffer);
}
template<>
std::string_view CGDK::to_string_scaled<char, int64_t>(char* _buffer, size_t _size, int64_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::string_view());

	if (_value >> 40)
		__sprintf(_buffer, _size, "%.2fT"sv, ((float)_value) / 1000000000000.0f);
	else if (_value >> 30)
		__sprintf(_buffer, _size, "%.2fG"sv, ((float)_value) / 1000000000.0f);
	else if (_value >> 20)
		__sprintf(_buffer, _size, "%.2fM"sv, ((float)_value) / 1000000.0f);
	else if (_value >> 10)
		__sprintf(_buffer, _size, "%.2fK"sv, ((float)_value) / 1000.0f);
	else
		__sprintf(_buffer, _size, "%.2f"sv, ((float)_value));

	return	std::string_view(_buffer);
}
template<>
std::string_view CGDK::to_string_scaled<char, uint64_t>(char* _buffer, size_t _size, uint64_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::string_view());

	if (_value >> 40)
		__sprintf(_buffer, _size, "%.2fT"sv, ((float)_value) / 1000000000000.0f);
	else if (_value >> 30)
		__sprintf(_buffer, _size, "%.2fG"sv, ((float)_value) / 1000000000.0f);
	else if (_value >> 20)
		__sprintf(_buffer, _size, "%.2fM"sv, ((float)_value) / 1000000.0f);
	else if (_value >> 10)
		__sprintf(_buffer, _size, "%.2fK"sv, ((float)_value) / 1000.0f);
	else
		__sprintf(_buffer, _size, "%.2f"sv, ((float)_value));

	return	std::string_view(_buffer);
}
template<>
std::string_view CGDK::to_string_scaled<char, float>(char* _buffer, size_t _size, float _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::string_view());

	if (_value > (1024.0f*1024.0f*1024.0f*1024.0f))
		__sprintf(_buffer, _size, "%.2fT"sv, _value / 1000000000000.0f);
	else if (_value > (1024.0f*1024.0f*1024.0f))
		__sprintf(_buffer, _size, "%.2fG"sv, _value / 1000000000.0f);
	else if (_value > (1024.0f*1024.0f))
		__sprintf(_buffer, _size, "%.2fM"sv, _value / 1000000.0f);
	else if (_value > 1024.0f)
		__sprintf(_buffer, _size, "%.2fK"sv, _value / 1000.0f);
	else
		__sprintf(_buffer, _size, "%.2f"sv, _value);

	return	std::string_view(_buffer);
}

template<>
std::wstring_view CGDK::to_string_scaled<wchar_t, int32_t>(wchar_t* _buffer, size_t _size, int32_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::wstring_view());

	if (_value >> 20)
	{
		__sprintf(_buffer, _size, L"%.2fM"sv, ((float)_value) / 1000000.0f);
	}
	else if (_value >> 10)
	{
		__sprintf(_buffer, _size, L"%.2fK"sv, ((float)_value) / 1000.0f);
	}
	else
	{
		__sprintf(_buffer, _size, L"%.2f"sv, ((float)_value));
	}

	return	std::wstring_view(_buffer);
}
template<>
std::wstring_view CGDK::to_string_scaled<wchar_t, uint32_t>(wchar_t* _buffer, size_t _size, uint32_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::wstring_view());

	if (_value >> 20)
	{
		__sprintf(_buffer, _size, L"%.2fM"sv, ((float)_value) / 1000000.0f);
	}
	else if (_value >> 10)
	{
		__sprintf(_buffer, _size, L"%.2fK"sv, ((float)_value) / 1000.0f);
	}
	else
	{
		__sprintf(_buffer, _size, L"%.2f"sv, ((float)_value));
	}

	return	std::wstring_view(_buffer);
}
template<>
std::wstring_view CGDK::to_string_scaled<wchar_t, int64_t>(wchar_t* _buffer, size_t _size, int64_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::wstring_view());

	if (_value >> 40)
	{
		__sprintf(_buffer, _size, L"%.2fT"sv, ((float)_value) / 1000000000000.0f);
	}
	else if (_value >> 30)
	{
		__sprintf(_buffer, _size, L"%.2fG"sv, ((float)_value) / 1000000000.0f);
	}
	else if (_value >> 20)
	{
		__sprintf(_buffer, _size, L"%.2fM"sv, ((float)_value) / 1000000.0f);
	}
	else if (_value >> 10)
	{
		__sprintf(_buffer, _size, L"%.2fK"sv, ((float)_value) / 1000.0f);
	}
	else
	{
		__sprintf(_buffer, _size, L"%.2f"sv, ((float)_value));
	}

	return	std::wstring_view(_buffer);
}
template<>
std::wstring_view CGDK::to_string_scaled<wchar_t, uint64_t>(wchar_t* _buffer, size_t _size, uint64_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::wstring_view());

	if (_value >> 40)
	{
		__sprintf(_buffer, _size, L"%.2fT"sv, ((float)_value) / 1000000000000.0f);
	}
	else if (_value >> 30)
	{
		__sprintf(_buffer, _size, L"%.2fG"sv, ((float)_value) / 1000000000.0f);
	}
	else if (_value >> 20)
	{
		__sprintf(_buffer, _size, L"%.2fM"sv, ((float)_value) / 1000000.0f);
	}
	else if (_value >> 10)
	{
		__sprintf(_buffer, _size, L"%.2fK"sv, ((float)_value) / 1000.0f);
	}
	else
	{
		__sprintf(_buffer, _size, L"%.2f"sv, ((float)_value));
	}

	return	std::wstring_view(_buffer);
}
template<>
std::wstring_view CGDK::to_string_scaled<wchar_t, float>(wchar_t* _buffer, size_t _size, float _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::wstring_view());

	if (_value > 1000000000000.0f)
		__sprintf(_buffer, _size, L"%.2fT"sv, _value / 1000000000000.0f);
	else if (_value > 1000000000.0f)
		__sprintf(_buffer, _size, L"%.2fG"sv, _value / 1000000000.0f);
	else if (_value > (1000000.0f))
		__sprintf(_buffer, _size, L"%.2fM"sv, _value / 1000000.0f);
	else if (_value > 1000.0f)
		__sprintf(_buffer, _size, L"%.2fK"sv, _value / 1000.0f);
	else
		__sprintf(_buffer, _size, L"%.2f"sv, _value);

	return	std::wstring_view(_buffer);
}

template<>
std::string CGDK::to_string_scaled<char, int32_t>(int32_t _value)
{
	char temp[256];
	return std::string(CGDK::to_string_scaled<char, int32_t>(temp, 256, _value));
}
template<>
std::string CGDK::to_string_scaled<char, uint32_t>(uint32_t _value)
{
	char temp[256];
	return std::string(CGDK::to_string_scaled<char, uint32_t>(temp, 256, _value));
}
template<>
std::string CGDK::to_string_scaled<char, int64_t>(int64_t _value)
{
	char temp[256];
	return std::string(CGDK::to_string_scaled<char, int64_t>(temp, 256, _value));
}
template<>
std::string CGDK::to_string_scaled<char, uint64_t>(uint64_t _value)
{
	char temp[256];
	return std::string(CGDK::to_string_scaled<char, uint64_t>(temp, 256, _value));
}
template<>
std::string CGDK::to_string_scaled<char, float>(float _value)
{
	char temp[256];
	return std::string(CGDK::to_string_scaled<char, float>(temp, 256, _value));
}


template<>
std::wstring CGDK::to_string_scaled<wchar_t, int32_t>(int32_t _value)
{
	wchar_t	temp[256];
	return std::wstring(CGDK::to_string_scaled<wchar_t, int32_t>(temp, 256, _value));
}
template<>
std::wstring CGDK::to_string_scaled<wchar_t, uint32_t>(uint32_t _value)
{
	wchar_t	temp[256];
	return std::wstring(CGDK::to_string_scaled<wchar_t, uint32_t>(temp, 256, _value));
}
template<>
std::wstring CGDK::to_string_scaled<wchar_t, int64_t>(int64_t _value)
{
	wchar_t	temp[256];
	return std::wstring(CGDK::to_string_scaled<wchar_t, int64_t>(temp, 256, _value));
}
template<>
std::wstring CGDK::to_string_scaled<wchar_t, uint64_t>(uint64_t _value)
{
	wchar_t	temp[256];
	return std::wstring(CGDK::to_string_scaled<wchar_t, uint64_t>(temp, 256, _value));
}
template<>
std::wstring CGDK::to_string_scaled<wchar_t, float>(float _value)
{
	wchar_t	temp[256];
	return std::wstring(CGDK::to_string_scaled<wchar_t, float>(temp, 256, _value));
}



// ----------------------------------------------------------------------------
// to_string scaled
// ----------------------------------------------------------------------------
template<>
std::string_view CGDK::to_string_scaled_byte<char, int32_t>(char* _buffer, size_t _size, int32_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::string_view());

	if (_value >> 20)
		__sprintf(_buffer, _size, "%.2fM"sv, ((float)_value) / (1024.0f*1024.0f));
	else if (_value >> 10)
		__sprintf(_buffer, _size, "%.2fK"sv, ((float)_value) / 1024.0f);
	else
		__sprintf(_buffer, _size, "%.2f"sv, ((float)_value));

	return	std::string_view(_buffer);
}
template<>
std::string_view CGDK::to_string_scaled_byte<char, uint32_t>(char* _buffer, size_t _size, uint32_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::string_view());

	if (_value >> 20)
		__sprintf(_buffer, _size, "%.2fM"sv, ((float)_value) / (1024.0f*1024.0f));
	else if (_value >> 10)
		__sprintf(_buffer, _size, "%.2fK"sv, ((float)_value) / 1024.0f);
	else
		__sprintf(_buffer, _size, "%.2f"sv, ((float)_value));

	return	std::string_view(_buffer);
}
template<>
std::string_view CGDK::to_string_scaled_byte<char, int64_t>(char* _buffer, size_t _size, int64_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::string_view());

	if (_value >> 40)
		__sprintf(_buffer, _size, "%.2fT"sv, ((float)_value) / (1024.0f*1024.0f*1024.0f*1024.0f));
	else if (_value >> 30)
		__sprintf(_buffer, _size, "%.2fG"sv, ((float)_value) / (1024.0f*1024.0f*1024.0f));
	else if (_value >> 20)
		__sprintf(_buffer, _size, "%.2fM"sv, ((float)_value) / (1024.0f*1024.0f));
	else if (_value >> 10)
		__sprintf(_buffer, _size, "%.2fK"sv, ((float)_value) / 1024.0f);
	else
		__sprintf(_buffer, _size, "%.2f"sv, ((float)_value));

	return	std::string_view(_buffer);
}
template<>
std::string_view CGDK::to_string_scaled_byte<char, uint64_t>(char* _buffer, size_t _size, uint64_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::string_view());

	if (_value >> 40)
		__sprintf(_buffer, _size, "%.2fT"sv, ((float)_value) / (1024.0f*1024.0f*1024.0f*1024.0f));
	else if (_value >> 30)
		__sprintf(_buffer, _size, "%.2fG"sv, ((float)_value) / (1024.0f*1024.0f*1024.0f));
	else if (_value >> 20)
		__sprintf(_buffer, _size, "%.2fM"sv, ((float)_value) / (1024.0f*1024.0f));
	else if (_value >> 10)
		__sprintf(_buffer, _size, "%.2fK"sv, ((float)_value) / 1024.0f);
	else
		__sprintf(_buffer, _size, "%.2f"sv, ((float)_value));

	return	std::string_view(_buffer);
}
template<>
std::string_view CGDK::to_string_scaled_byte<char, float>(char* _buffer, size_t _size, float _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::string_view());

	if (_value > (1024.0f*1024.0f*1024.0f*1024.0f))
		__sprintf(_buffer, _size, "%.2fT"sv, _value / (1024.0f*1024.0f*1024.0f*1024.0f));
	else if (_value > (1024.0f*1024.0f*1024.0f))
		__sprintf(_buffer, _size, "%.2fG"sv, _value / (1024.0f*1024.0f*1024.0f));
	else if (_value > (1024.0f*1024.0f))
		__sprintf(_buffer, _size, "%.2fM"sv, _value / (1024.0f*1024.0f));
	else if (_value > 1024.0f)
		__sprintf(_buffer, _size, "%.2fK"sv, _value / 1024.0f);
	else
		__sprintf(_buffer, _size, "%.2f"sv, _value);

	return	std::string_view(_buffer);
}

template<>
std::wstring_view CGDK::to_string_scaled_byte<wchar_t, int32_t>(wchar_t* _buffer, size_t _size, int32_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::wstring_view());

	if (_value >> 20)
	{
		__sprintf(_buffer, _size, L"%.2fM"sv, ((float)_value) / (1024.0f*1024.0f));
	}
	else if (_value >> 10)
	{
		__sprintf(_buffer, _size, L"%.2fK"sv, ((float)_value) / 1024.0f);
	}
	else
	{
		__sprintf(_buffer, _size, L"%.2f"sv, ((float)_value));
	}

	return	std::wstring_view(_buffer);
}
template<>
std::wstring_view CGDK::to_string_scaled_byte<wchar_t, uint32_t>(wchar_t* _buffer, size_t _size, uint32_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::wstring_view());

	if (_value >> 20)
	{
		__sprintf(_buffer, _size, L"%.2fM"sv, ((float)_value) / (1024.0f*1024.0f));
	}
	else if (_value >> 10)
	{
		__sprintf(_buffer, _size, L"%.2fK"sv, ((float)_value) / 1024.0f);
	}
	else
	{
		__sprintf(_buffer, _size, L"%.2f"sv, ((float)_value));
	}

	return	std::wstring_view(_buffer);
}
template<>
std::wstring_view CGDK::to_string_scaled_byte<wchar_t, int64_t>(wchar_t* _buffer, size_t _size, int64_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::wstring_view());

	if (_value >> 40)
	{
		__sprintf(_buffer, _size, L"%.2fT"sv, ((float)_value) / (1024.0f*1024.0f*1024.0f*1024.0f));
	}
	else if (_value >> 30)
	{
		__sprintf(_buffer, _size, L"%.2fG"sv, ((float)_value) / (1024.0f*1024.0f*1024.0f));
	}
	else if (_value >> 20)
	{
		__sprintf(_buffer, _size, L"%.2fM"sv, ((float)_value) / (1024.0f*1024.0f));
	}
	else if (_value >> 10)
	{
		__sprintf(_buffer, _size, L"%.2fK"sv, ((float)_value) / 1024.0f);
	}
	else
	{
		__sprintf(_buffer, _size, L"%.2f"sv, ((float)_value));
	}

	return	std::wstring_view(_buffer);
}
template<>
std::wstring_view CGDK::to_string_scaled_byte<wchar_t, uint64_t>(wchar_t* _buffer, size_t _size, uint64_t _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::wstring_view());

	if (_value >> 40)
	{
		__sprintf(_buffer, _size, L"%.2fT"sv, ((float)_value) / (1024.0f*1024.0f*1024.0f*1024.0f));
	}
	else if (_value >> 30)
	{
		__sprintf(_buffer, _size, L"%.2fG"sv, ((float)_value) / (1024.0f*1024.0f*1024.0f));
	}
	else if (_value >> 20)
	{
		__sprintf(_buffer, _size, L"%.2fM"sv, ((float)_value) / (1024.0f*1024.0f));
	}
	else if (_value >> 10)
	{
		__sprintf(_buffer, _size, L"%.2fK"sv, ((float)_value) / 1024.0f);
	}
	else
	{
		__sprintf(_buffer, _size, L"%.2f"sv, ((float)_value));
	}

	return	std::wstring_view(_buffer);
}
template<>
std::wstring_view CGDK::to_string_scaled_byte<wchar_t, float>(wchar_t* _buffer, size_t _size, float _value)
{
	// check) _buffer가 nullptr이면 안됀다.
	CGASSERT_ERROR(_buffer != nullptr);

	// check) _buffer가 nullptr이면 그냥 되돌린다.
	RETURN_IF(_buffer == nullptr, std::wstring_view());

	if (_value > (1024.0f*1024.0f*1024.0f*1024.0f))
		__sprintf(_buffer, _size, L"%.2fT"sv, _value / (1024.0f*1024.0f*1024.0f*1024.0f));
	else if (_value > (1024.0f*1024.0f*1024.0f))
		__sprintf(_buffer, _size, L"%.2fG"sv, _value / (1024.0f*1024.0f*1024.0f));
	else if (_value > (1024.0f*1024.0f))
		__sprintf(_buffer, _size, L"%.2fM"sv, _value / (1024.0f*1024.0f));
	else if (_value > 1024.0f)
		__sprintf(_buffer, _size, L"%.2fK"sv, _value / 1024.0f);
	else
		__sprintf(_buffer, _size, L"%.2f"sv, _value);

	return	std::wstring_view(_buffer);
}

template<>
std::string CGDK::to_string_scaled_byte<char, int32_t>(int32_t _value)
{
	char	temp[256];
	return std::string(CGDK::to_string_scaled_byte<char, int32_t>(temp, 256, _value));
}
template<>
std::string CGDK::to_string_scaled_byte<char, uint32_t>(uint32_t _value)
{
	char	temp[256];
	return std::string(CGDK::to_string_scaled_byte<char, uint32_t>(temp, 256, _value));
}
template<>
std::string CGDK::to_string_scaled_byte<char, int64_t>(int64_t _value)
{
	char	temp[256];
	return std::string(CGDK::to_string_scaled_byte<char, int64_t>(temp, 256, _value));
}
template<>
std::string CGDK::to_string_scaled_byte<char, uint64_t>(uint64_t _value)
{
	char	temp[256];
	return std::string(CGDK::to_string_scaled_byte<char, uint64_t>(temp, 256, _value));
}
template<>
std::string CGDK::to_string_scaled_byte<char, float>(float _value)
{
	char	temp[256];
	return std::string(CGDK::to_string_scaled_byte<char, float>(temp, 256, _value));
}


template<>
std::wstring CGDK::to_string_scaled_byte<wchar_t, int32_t>(int32_t _value)
{
	wchar_t	temp[256];
	return std::wstring(CGDK::to_string_scaled_byte<wchar_t, int32_t>(temp, 256, _value));
}
template<>
std::wstring CGDK::to_string_scaled_byte<wchar_t, uint32_t>(uint32_t _value)
{
	wchar_t	temp[256];
	return std::wstring(CGDK::to_string_scaled_byte<wchar_t, uint32_t>(temp, 256, _value));
}
template<>
std::wstring CGDK::to_string_scaled_byte<wchar_t, int64_t>(int64_t _value)
{
	wchar_t	temp[256];
	return std::wstring(CGDK::to_string_scaled_byte<wchar_t, int64_t>(temp, 256, _value));
}
template<>
std::wstring CGDK::to_string_scaled_byte<wchar_t, uint64_t>(uint64_t _value)
{
	wchar_t	temp[256];
	return std::wstring(CGDK::to_string_scaled_byte<wchar_t, uint64_t>(temp, 256, _value));
}
template<>
std::wstring CGDK::to_string_scaled_byte<wchar_t, float>(float _value)
{
	wchar_t	temp[256];
	return std::wstring(CGDK::to_string_scaled_byte<wchar_t, float>(temp, 256, _value));
}
