#include "pch.h"


namespace CGDK
{

const std::string& get_base64_string() noexcept
{
	static const std::string charBase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789+/";

	return charBase64;
}

inline bool is_base64(unsigned char c) 
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string crypt::encode_base64(const_buffer_view _source)
{
	const auto& charBase64 = get_base64_string();

	buffer_base<const char> source{ _source.size(), _source.data() };
	std::string ret;
	int i = 0;
	std::array<unsigned char, 3> arrayChar3{};
	std::array<unsigned char, 4> arrayChar4{};

	ret.reserve(source.size_);

	while (source.size_--)
	{
		arrayChar3[i++] = *(source.data_++);

		if (i == 3) 
		{
			arrayChar4[0] = static_cast<unsigned char> ((arrayChar3[0] & 0xfc) >> 2);
			arrayChar4[1] = static_cast<unsigned char>(((arrayChar3[0] & 0x03) << 4) + ((arrayChar3[1] & 0xf0) >> 4));
			arrayChar4[2] = static_cast<unsigned char>(((arrayChar3[1] & 0x0f) << 2) + ((arrayChar3[2] & 0xc0) >> 6));
			arrayChar4[3] = static_cast<unsigned char>  (arrayChar3[2] & 0x3f);

			for(i = 0; (i <4) ; ++i)
				ret += charBase64.at(arrayChar4[i]);
			i = 0;
		}
	}

	if (i)
	{
		int j;

		for(j = i; j < 3; j++)
		{
			arrayChar3[j] = '\0';
		}

		arrayChar4[0] = static_cast<unsigned char> ((arrayChar3[0] & 0xfc) >> 2);
		arrayChar4[1] = static_cast<unsigned char>(((arrayChar3[0] & 0x03) << 4) + ((arrayChar3[1] & 0xf0) >> 4));
		arrayChar4[2] = static_cast<unsigned char>(((arrayChar3[1] & 0x0f) << 2) + ((arrayChar3[2] & 0xc0) >> 6));
		arrayChar4[3] = static_cast<unsigned char>  (arrayChar3[2] & 0x3f);

		for (j = 0; (j < i + 1); j++)
		{
			ret += charBase64.at(arrayChar4[j]);
		}

		while((i++ < 3))
		{
			ret += '=';
		}
	}

	return ret;
}

shared_buffer crypt::decode_base64(const std::string_view _source)
{
	const auto& charBase64 = get_base64_string();

	int	in_len = static_cast<int>(_source.size());
	int	in_	   = 0;
	std::array<unsigned char, 4> arrayChar4{};
	std::array<unsigned char, 3> arrayChar3{};
	shared_buffer ret = alloc_shared_buffer(_source.size() * 4 / 3 + 1);

	int i = 0;
	while (in_len-- && ( _source.at(in_) != '=') && is_base64(_source.at(in_))) 
	{
		arrayChar4[i++] = _source.at(in_); in_++;

		if (i == 4) 
		{
			for (i = 0; i <4; ++i)
			{
				arrayChar4[i] = (unsigned char)charBase64.find(arrayChar4[i]);
			}

			arrayChar3[0] = static_cast<unsigned char> ((arrayChar4[0] << 2)		+ ((arrayChar4[1] & 0x30) >> 4));
			arrayChar3[1] = static_cast<unsigned char>(((arrayChar4[1] & 0xf) << 4) + ((arrayChar4[2] & 0x3c) >> 2));
			arrayChar3[2] = static_cast<unsigned char>(((arrayChar4[2] & 0x3) << 6) +   arrayChar4[3]);

			for (i = 0; (i < 3); ++i)
			{
				ret.append<unsigned char>(arrayChar3[i]);
			}

			i = 0;
		}
	}

	if (i) 
	{
		int	j;

		for (j = i; j <4; j++)
		{
			arrayChar4[j] = 0;
		}

		for (j = 0; j <4; j++)
		{
			arrayChar4[j] = (unsigned char)charBase64.find(arrayChar4[j]);
		}

		arrayChar3[0] = static_cast<unsigned char> ((arrayChar4[0] << 2)		+ ((arrayChar4[1] & 0x30) >> 4));
		arrayChar3[1] = static_cast<unsigned char>(((arrayChar4[1] & 0xf) << 4) + ((arrayChar4[2] & 0x3c) >> 2));
		arrayChar3[2] = static_cast<unsigned char>(((arrayChar4[2] & 0x3) << 6) +   arrayChar4[3]);

		for (j = 0; (j<i-1); j++) 
		{
			ret.append<unsigned char>(arrayChar3[j]);
		}
	}

	return ret;
}


}