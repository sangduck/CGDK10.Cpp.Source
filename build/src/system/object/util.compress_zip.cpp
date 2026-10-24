#include "pch.h"
#include "zlib/zlib.h"

namespace CGDK
{

std::string_view _get_zip_error_string(int _err_no) noexcept
{
	switch (_err_no)
	{
	case	Z_ERRNO:			return "error no"sv;
	case	Z_STREAM_ERROR:		return "stream error"sv;
	case	Z_DATA_ERROR:		return "data error"sv;
	case	Z_MEM_ERROR:		return "memory error"sv;
	case	Z_BUF_ERROR:		return "buffer error"sv;
	case	Z_VERSION_ERROR:	return "version error"sv;
	default:					return "unknowned error"sv;
	}
}

buffer_view compress::zip(buffer_view _buf_dest, const_buffer_view _buf_source)
{
	Bytef* pdata  = _buf_dest.data<Bytef>();
	uLongf length = _buf_dest.size<uLongf>();

	::compress(pdata, &length, _buf_source.data<const Bytef>(), _buf_source.size<uLong>());

	return buffer_view { pdata, length };
}

shared_buffer compress::zip(const_buffer_view _buf_source)
{
	// declare) 
	shared_buffer buf_dest;

	// 1) get size of buffer
	const auto buffer_size = (_buf_source.size() * 1001 / 1000) + 12;

	// 1) alloc buffer (buffer size must be larger than _buf_source.size() x 1.001 + 12 bytes)
	buf_dest = alloc_shared_buffer(buffer_size) ^ buffer_size;

	// 2) decompressed
	Bytef* pdata = buf_dest.data<Bytef>();
	uLongf length = buf_dest.size<uLongf>();

	// 3) 
	auto result = ::compress(pdata, &length, _buf_source.data<const Bytef>(), _buf_source.size<uLong>());

	// check)
	if (result < 0)
		throw throwable::failure(eRESULT::RUNTIME_ERROR, result, _get_zip_error_string(result));

	// return) 
	buf_dest ^= length;
	return buf_dest;
}

buffer_view compress::unzip(buffer_view _buf_dest, const_buffer_view _buf_source)
{
	Bytef* pdata  = _buf_dest.data<Bytef>();
	uLongf length = _buf_dest.size<uLongf>();

	::uncompress(pdata, &length, _buf_source.data<const Bytef>(), _buf_source.size<uLong>());

	return buffer_view { pdata, length };
}


shared_buffer compress::unzip(const_buffer_view _buf_source)
{
	// declare) 
	shared_buffer buf_dest;

	// 1) set values
	auto size_decompressed = _buf_source.size<uLongf>();

	int result;

	do
	{
		// 1) get size
		size_decompressed *= 2;

		// 2) alloc buffer
		buf_dest = alloc_shared_buffer(size_decompressed) ^ size_decompressed;

		// 4) 
		result = ::uncompress(buf_dest.data<Bytef>(), &size_decompressed, _buf_source.data<const Bytef>(), _buf_source.size<uLong>());

		// check)
		if (result < 0 && result != Z_BUF_ERROR)
			throw throwable::failure(eRESULT::RUNTIME_ERROR, result, _get_zip_error_string(result));

	} while (result == Z_BUF_ERROR);

	// return) 
	buf_dest ^= size_decompressed;
	return buf_dest;
}


}