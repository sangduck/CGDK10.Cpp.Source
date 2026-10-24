#include "pch.h"

namespace
{
	// closes a file handle when leaving the scope (normal return or exception)
	struct file_handle_closer
	{
		CGDK::FILE_HANDLE handle;
		~file_handle_closer() { if (this->handle != CGDK::INVALID_FILE_HANDLE) CGDK::api::file::close(this->handle); }
	};

	// configuration string (native narrow encoding) -> UTF-8 (the encoding every resource string carries)
	std::string to_utf8_from_native(const std::string& _native)
	{
		const auto u8 = std::filesystem::path(_native).u8string();
		return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
	}
}


CGDK::update::UPDATABLE_ID _get_unique_updatable_id() noexcept
{
	static std::atomic<CGDK::update::UPDATABLE_ID> g_updatable_id{ 0 };

	auto v = g_updatable_id.fetch_add(1, std::memory_order_relaxed) + 1;

	// check)
	CGASSERT_ERROR(v != 0);

	return v;
}

CGDK::resource_pos_src _get_updatable_resource_pos_src(CGDK::context& _context)
{
	// declare)
	CGDK::resource_pos_src pos;

	// 1) path_base
	if (std::string str_find = _context.find("path_base"sv, "pathbase"sv); str_find.empty() == false)
	{
		pos.path_base = to_utf8_from_native(str_find);
	}

	// 2) path_base
	if (std::string str_find = _context.find("path"sv); str_find.empty() == false)
	{
		pos.path = to_utf8_from_native(str_find);
	}

	// 3) path_base
	if (std::string str_find = _context.find("filename"sv, "file_name"sv); str_find.empty() == false)
	{
		pos.filename = to_utf8_from_native(str_find);
	}

	// return)
	return pos;
}

CGDK::resource_pos_dst _get_updatable_resource_pos_dst(CGDK::context& _context)
{
	// declare)
	CGDK::resource_pos_dst pos;

	// 2) path_base
	if (std::string str_find = _context.find("path"sv); str_find.empty() == false)
	{
		pos.path = to_utf8_from_native(str_find);
	}

	// 3) path_base
	if (std::string str_find = _context.find("filename"sv, "file_name"sv); str_find.empty() == false)
	{
		pos.filename = to_utf8_from_native(str_find);
	}

	// return)
	return pos;
}

CGDK::resource::Iupdatable::INITIALIZE_PARAMETER get_parameter_resource_updatable_file(CGDK::context& _context)
{
	// declare)
	CGDK::resource::Iupdatable::INITIALIZE_PARAMETER parameter;

	// 1) src find
	{
		auto context_src = _context.find("src"sv, "source"sv);

		if (context_src.is_valid())
		{
			parameter.src = _get_updatable_resource_pos_src(context_src);
		}
	}

	// 2) dst find
	{
		auto context_dst = _context.find("dst"sv, "destination"sv);

		if (context_dst.is_valid())
		{
			parameter.dst = _get_updatable_resource_pos_dst(context_dst);
		}
	}

	// 3) options
	{
		auto context_options = _context.find("option"sv, "options"sv);

		if (context_options.is_valid())
		{
			// declare)
			CGDK::resource::eOPTION options = CGDK::resource::eOPTION::NONE;

			for (auto& iter : context_options)
			{
				// - get string
				std::string str_name = iter.second;

				if (CGDK::__stricmp(str_name, "block_compressed") == 0
				 || CGDK::__stricmp(str_name, "block_compress") == 0
				 || CGDK::__stricmp(str_name, "blockcompressed") == 0
				 || CGDK::__stricmp(str_name, "blockcompress") == 0
				 || CGDK::__stricmp(str_name, "compressed") == 0
				 || CGDK::__stricmp(str_name, "compress") == 0)
				{
					options = options | CGDK::resource::eOPTION::BLOCK_COMPRESSED;
				}
				else if (CGDK::__stricmp(str_name, "stream") == 0)
				{
					options = options | CGDK::resource::eOPTION::STREAM;
				}
				else if (CGDK::__stricmp(str_name, "recursive") == 0)
				{
					options = options | CGDK::resource::eOPTION::RECURSIVE;
				}
				else if (CGDK::__stricmp(str_name, "remove") == 0)
				{
					options = options | CGDK::resource::eOPTION::REMOVE;
				}
				else if (CGDK::__stricmp(str_name, "remove_folder") == 0
					  || CGDK::__stricmp(str_name, "removefolder") == 0
					  || CGDK::__stricmp(str_name, "folder_remove") == 0
					  || CGDK::__stricmp(str_name, "folderremove") == 0)
				{
					options = options | CGDK::resource::eOPTION::REMOVE_FOLDER;
				}
				else if (CGDK::__stricmp(str_name, "find") == 0)
				{
					options = options | CGDK::resource::eOPTION::FIND;
				}
				else if (CGDK::__stricmp(str_name, "move") == 0)
				{
					options = options | CGDK::resource::eOPTION::MOVE;
				}
				else if (CGDK::__stricmp(str_name, "none") == 0)
				{
					options = options | CGDK::resource::eOPTION::NONE;
				}
				else
				{

				}
			}
		}
	}

	// 4) blocksize
	{
		// - get value
		auto context_value = _context.find("block_size"sv, "blocksize"sv);

		// - get block size
		parameter.block_size = (context_value.is_valid()) ? context_value : CGDK::update::_FILE_BLOCK_SIZE;

		// - saturate MINIMUM block size
		if (parameter.block_size < 256)
		{
			parameter.block_size = 256;
		}
	}

	// return)
	return parameter;
}

CGDK::result_code CGDK::resource::updatable::file::_process_initializing(context& _context)
{
	// 1) call 'on_initialize'
	this->on_initializing(_context);

	try
	{
		// declare)
		INITIALIZE_PARAMETER parameter;

		// 1) parsing socket setting options
		if (_context.type() == context::value_type_t::buffer_)
		{
			parameter = _context.v.value_buffer.front<INITIALIZE_PARAMETER>();
		}
		else
		{
			parameter = get_parameter_resource_updatable_file(_context);
		}

		// log) 
		LOG_INFO << "     file (" << parameter.src.path_base << "/" << parameter.src.path << "/" << parameter.src.filename << ")";

		// 2) file load
		this->process_resource_load(parameter);
	}
	catch (std::exception& _exception)
	{
		// - call exception handler
		throwable::execute_exception_handler(this, _exception, __FUNCTION__);

		// - get name
		auto* pnameable = dynamic_cast<Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string_view();

		// log) 
		LOG_ERROR << "(excp) exception occurred on '_process_starting' ['"sv << pname << "']"sv;

		// reraise)
		throw;
	}
	catch (...)
	{
		// - get name
		auto* pnameable = dynamic_cast<Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string_view();

		// log) 
		LOG_ERROR << "(excp) exception occurred on '_process_starting' ['"sv << pname << "']"sv;

		// reraise)
		throw;
	}

	return eRESULT::SUCCESS;
}

CGDK::result_code CGDK::resource::updatable::file::_process_initialize(context& _context)
{
	this->on_initialize(_context);
	
	return eRESULT::SUCCESS;
}

CGDK::result_code CGDK::resource::updatable::file::_process_destroying() noexcept
{
	try
	{
		this->on_destroying();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}
	
	return eRESULT::SUCCESS;
}

CGDK::result_code CGDK::resource::updatable::file::_process_destroy() noexcept
{
	try 
	{ 
		this->on_destroy();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	return eRESULT::SUCCESS;
}


CGDK::result_code CGDK::resource::updatable::file::process_message(sMESSAGE& /*_msg*/)
{
	return eRESULT::SUCCESS;
}

CGDK::shared_buffer CGDK::resource::updatable::file::get_block(uint64_t _pos)
{
	// declare)
	CGDK::shared_buffer buf;

	SCOPED_LOCK(this->m_lockable_info)
	{
		// check)
		RETURN_IF(_pos >= this->m_meta_info.size, buf);

		// 1) get index
		auto index = _pos / m_meta_info.block_size;

		// check)  
		THROW_IF(index >= this->m_vector_buffer.size(), throwable::failure(eRESULT::OUT_OF_RANGE, 0, "out of range"sv));

		// 2) get buffer
		buf = this->m_vector_buffer.at(index);
	}

	// check) 
	CGASSERT_ERROR(buf.exist());

	// return) 
	return buf;
}

void CGDK::resource::updatable::file::process_resource_load(const sINITIALIZE_INFO_UPDATABLE_RESOURCE& _initlaize_updable_parameter)
{
	// check)
	CGASSERT_ERROR(_initlaize_updable_parameter.src.filename.empty() == false);

	// check)
	THROW_IF(_initlaize_updable_parameter.src.filename.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'filename' is empty"sv), LOG_ERROR << "     (!) 'filename' is empty");

	// check)
	CGASSERT_ERROR(_initlaize_updable_parameter.block_size >= 4096);

	// check)
	THROW_IF(_initlaize_updable_parameter.block_size < 4096, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "_block_size is too few number. it must be over 4096(>=4096) "sv), LOG_ERROR << "     (!) _block_size is too few number. it must be over 4096(>=4096");

	// 1) FILE_META_INFO
	sUPDATABLE_FILE_META_INFO file_meta_info;
	file_meta_info.id = _get_unique_updatable_id(); // update::UPDATABLE_ID
	file_meta_info.size = 0;
	file_meta_info.block_size = _initlaize_updable_parameter.block_size;
	file_meta_info.type = eTYPE::FILE;
	file_meta_info.path = _initlaize_updable_parameter.src.path;
	file_meta_info.filename = _initlaize_updable_parameter.src.filename;
	//file_meta_info.time_write;

	// 2) FILE_META_COMPLETION
	sUPDATABLE_FILE_COMPLETION_INFO file_meta_completion;
	file_meta_completion.id = file_meta_info.id;

	// 3) make full path
	std::filesystem::path full_file_path;
#if _WIN32
	{
		// declare) 
		wchar_t temp_dest[256];

		if (_initlaize_updable_parameter.src.path_base.empty() == false)
		{
			full_file_path += encode_string(temp_dest, reinterpret_cast<const char8_t*>(_initlaize_updable_parameter.src.path_base.c_str())); ;
			full_file_path += "/";
		}
		if (_initlaize_updable_parameter.src.path.empty() == false)
		{
			full_file_path += encode_string(temp_dest, reinterpret_cast<const char8_t*>(_initlaize_updable_parameter.src.path.c_str()));
			full_file_path += "/";
		}
		full_file_path += encode_string(temp_dest, reinterpret_cast<const char8_t*>(_initlaize_updable_parameter.src.filename.c_str()));
	}
#else
	{
		if (_initlaize_updable_parameter.src.path_base.empty() == false)
		{
			full_file_path += _initlaize_updable_parameter.src.path_base;
			full_file_path += "/";
		}
		if (_initlaize_updable_parameter.src.path.empty() == false)
		{
			full_file_path += _initlaize_updable_parameter.src.path;
			full_file_path += "/";
		}
		full_file_path.append(_initlaize_updable_parameter.src.filename);
	}
#endif

	// 4) open file
	auto file_handle = CGDK::api::file::open(full_file_path.string(), eFILE_OPERATION::READ);

	// check) 
	THROW_IF(file_handle == INVALID_FILE_HANDLE, throwable::failure(eRESULT::FAIL_TO_OPEN, 0, "fail to to open file"sv), LOG_ERROR << "     (!) fail to to open file");

	// - close the file when leaving this function
	file_handle_closer file_closer{ file_handle };

	// 5) get file length
	file_meta_info.size = CGDK::api::file::get_length(file_handle);

	// 6) file blocks (the whole file is read and, if requested, compressed once here; get_block() only hands out ready blocks)
	std::vector<shared_buffer> vector_buffer;
	vector_buffer.reserve(static_cast<size_t>((file_meta_info.size + file_meta_info.block_size - 1) / file_meta_info.block_size));

	// 7) block meta
	sMETA_FILE_BLOCK meta_file_block;
	meta_file_block.id = file_meta_info.id;
	meta_file_block.pos = 0;
	meta_file_block.size = 0;
	meta_file_block.flag = _initlaize_updable_parameter.options;

	// 8) compression flag
	const bool is_bcompress = (_initlaize_updable_parameter.options & resource::eOPTION::BLOCK_COMPRESSED) == resource::eOPTION::BLOCK_COMPRESSED;

	// - scratch buffer reused for every block's compression (zlib may need slightly more than the source size)
	shared_buffer buf_compress_scratch;

	if (is_bcompress)
	{
		buf_compress_scratch = alloc_shared_buffer(file_meta_info.block_size * 2 + 64);
	}

	// declare)
	edc::md5 md5_file;

	// 9) initialize md5
	md5_file.initialize();

	// 10) read all blocks
	while (meta_file_block.pos < file_meta_info.size)
	{
		// - get block size
		meta_file_block.size = (meta_file_block.pos + file_meta_info.block_size <= file_meta_info.size) ? file_meta_info.block_size : (file_meta_info.size - meta_file_block.pos);

		// - alloc shared buffer
		auto buf_block = alloc_shared_buffer(meta_file_block.size + CGDK::get_size_of(meta_file_block));

		// - reserve
		buf_block.append<sMETA_FILE_BLOCK>();

		// - read file block
		auto size_read = CGDK::api::file::read(file_handle, meta_file_block.size, buf_block.data() + CGDK::get_size_of(meta_file_block));

		// check)
		CGASSERT_ERROR(size_read == meta_file_block.size);

		// check)
		THROW_IF(size_read != meta_file_block.size, throwable::failure(eRESULT::FAIL, 0, "fail to read file"sv), LOG_ERROR << "     (!) fail to read file");

		// - add md5
		md5_file.push_md5(buf_block.data(CGDK::get_size_of(meta_file_block)), static_cast<uint32_t>(meta_file_block.size));

		if (is_bcompress == false)
		{
			// - add size
			buf_block.add_size(size_read);
		}
		else
		{
			// declare)
			buffer_view buf_temp_source(buf_block.data() + sizeof(sMETA_FILE_BLOCK), size_read);

			// - compress block into the scratch buffer
			auto temp_compressed = CGDK::compress::zip(buffer_view(buf_compress_scratch.data(), buf_compress_scratch.get_remained_size()), buf_temp_source);

			// - keep the compressed block only when it is smaller than 90% of the original
			if ((buf_temp_source.size() / 10 * 9) > temp_compressed.size())
			{
				// - add resource::eOPTION::BLOCK_COMPRESSED
				meta_file_block.flag = meta_file_block.flag | resource::eOPTION::BLOCK_COMPRESSED;

				// - copy compressed data
				buf_block.append(temp_compressed.size(), temp_compressed.data());
			}
			else
			{
				// - remove resource::eOPTION::BLOCK_COMPRESSED
				meta_file_block.flag = meta_file_block.flag & (~resource::eOPTION::BLOCK_COMPRESSED);

				// - add size
				buf_block.add_size(size_read);
			}
		}

		// - write block head info
		buf_block.front<sMETA_FILE_BLOCK>() = meta_file_block;

		// - update
		meta_file_block.pos += size_read;

		// - push back
		vector_buffer.push_back(std::move(buf_block));
	}

	// 11) get last file time
	file_meta_info.time_write = std::filesystem::last_write_time(full_file_path);

	// 12) 理쒖쥌 md5 援ы븯湲?
	md5_file.padding();
	md5_file.get_md5(&file_meta_completion.check_code);

	// log) 
	LOG_INFO << "      - file size: "sv << file_meta_info.size << " bloco size:"sv << file_meta_info.block_size;
	LOG_INFO << "        md5: "sv
		<< CGDK::stream_hex(file_meta_completion.check_code.data.at(0), 8) << " "
		<< CGDK::stream_hex(file_meta_completion.check_code.data.at(1), 8) << " "
		<< CGDK::stream_hex(file_meta_completion.check_code.data.at(2), 8) << " "
		<< CGDK::stream_hex(file_meta_completion.check_code.data.at(3), 8);

	// 13) 
	SCOPED_LOCK(this->m_lockable_info)
	{
		this->m_meta_info = std::move(file_meta_info);
		this->m_meta_completion = std::move(file_meta_completion);
		this->m_full_file_path = std::move(full_file_path);
		this->m_vector_buffer = std::move(vector_buffer);
	}
}

void CGDK::resource::updatable::file::process_resource_clear() noexcept
{

}
