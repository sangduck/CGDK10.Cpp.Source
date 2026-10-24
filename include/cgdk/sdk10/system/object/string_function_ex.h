//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                            string function ex                             *
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
//-----------------------------------------------------------------------------
//
// string function ex
//
//-----------------------------------------------------------------------------
namespace CGDK
{
	
// - Convert String(2) - converting object
template<class TDEST, class TSOURCE> [[nodiscard]] inline std::basic_string<TDEST> encode_string(std::basic_string_view<TSOURCE> _source)
{
	// 1) buffer count (4 per source char is enough for every conversion, +1 for the terminator)
	const std::size_t count_buffer = _source.size() * 4 + 1;

	// 2) small : stack (no heap allocation)
	if (count_buffer * sizeof(TDEST) <= 1024)
	{
		TDEST temp[1024 / sizeof(TDEST)];
		return std::basic_string<TDEST>(encode_string(temp, _source, count_buffer));
	}

	// 3) large : shared_buffer
	auto buf = alloc_shared_buffer(count_buffer * sizeof(TDEST));
	return std::basic_string<TDEST>(encode_string(buf.template data<TDEST>(), _source, count_buffer));
}
// to_string with byte (1000)
[[nodiscard]] std::string to_string_scaled(int64_t  _value);
[[nodiscard]] std::string to_string_scaled(uint64_t _value);
[[nodiscard]] std::string to_string_scaled(float _value);


// to_string with byte scaled (1024)
[[nodiscard]] std::string to_string_scaled_byte(int64_t  _value);
[[nodiscard]] std::string to_string_scaled_byte(uint64_t _value);
[[nodiscard]] std::string to_string_scaled_byte(float _value);

}
