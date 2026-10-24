#include "pch.h"

CGDK::update::UPDATABLE_ID _get_unique_updatable_id() noexcept;
void _get_md5(CGDK::FILE_HANDLE _file_handle, CGDK::edc::md5& _md5, uint64_t _end);
void _get_md5_to_end (CGDK::FILE_HANDLE _file_handle, CGDK::edc::md5& _md5, uint64_t _end);

// closes a file handle when leaving the scope (normal return or exception)
struct _file_handle_closer
{
	CGDK::FILE_HANDLE handle;
	~_file_handle_closer() { CGDK::api::file::close(this->handle); }
};


const CGDK::sUPDATABLE_FILE_COMPLETION_INFO& CGDK::resource::updatable::file_stream::get_meta_completion()
{
	SCOPED_LOCK(m_lockable_info)
	{
		if (this->m_md5_pos < this->m_meta_info.size)
		{
			// - make file
			auto file_handle = CGDK::api::file::open(this->m_full_path.string(), eFILE_OPERATION::READ);

			// check) 
			THROW_IF(file_handle == INVALID_FILE_HANDLE, throwable::failure(eRESULT::FAIL_TO_OPEN, 0, "fail to to open file"sv));
			_file_handle_closer file_closer{ file_handle };

			// - get md5
			_get_md5_to_end(file_handle, this->m_md5_file, this->m_md5_pos);
			this->m_md5_file.padding();
			this->m_md5_file.get_md5(&this->m_meta_completion.check_code);

			// - update pos
			this->m_md5_pos = this->m_meta_info.size;
		}
	}

	return this->m_meta_completion;
}

CGDK::result_code CGDK::resource::updatable::file_stream::process_message(sMESSAGE& /*_msg*/)
{
	return eRESULT::SUCCESS;
}

void CGDK::resource::updatable::file_stream::_process_md5_prepare(FILE_HANDLE _file_handle, uint64_t _pos)
{
	_get_md5(_file_handle, this->m_md5_file, _pos);
	this->m_md5_pos = _pos;

	if (this->m_md5_pos >= this->m_meta_info.size)
	{
		// check)
		CGASSERT_ERROR(this->m_md5_pos == this->m_meta_info.size);

		// - close md5
		this->m_md5_file.padding();
		this->m_md5_file.get_md5(&m_meta_completion.check_code);
	}
}

void CGDK::resource::updatable::file_stream::_process_md5_append_md5(const char* _data, size_t _size)
{
	// 1) push_md5
	this->m_md5_file.push_md5(_data, static_cast<uint32_t>(_size));
	this->m_md5_pos += _size;

	// 2) 파일의 끝이면 md5를 마무리 한다.
	if (this->m_md5_pos >= this->m_meta_info.size)
	{
		// check)
		CGASSERT_ERROR(this->m_md5_pos == this->m_meta_info.size);

		// - close md5
		this->m_md5_file.padding();
		this->m_md5_file.get_md5(&m_meta_completion.check_code);
	}
}

CGDK::shared_buffer CGDK::resource::updatable::file_stream::get_block(uint64_t _pos)
{
	// declare)
	CGDK::shared_buffer buf_block;

	SCOPED_LOCK(this->m_lockable_info)
	{
		// check)
		RETURN_IF(_pos > this->m_meta_info.size, buf_block);

		// 1) get index
		auto index = _pos / m_meta_info.block_size;

		// check)  
		THROW_IF(index >= m_vector_buffer.size(), throwable::failure(eRESULT::OUT_OF_RANGE, 0, "out of range"sv));

		// 2) get buffer
		buf_block = m_vector_buffer.at(index);

		// check) 
		RETURN_IF(buf_block.exist(), buf_block);

		// 1) set sMETA_FILE_BLOCK values
		sMETA_FILE_BLOCK meta_file_block;
		meta_file_block.id = this->m_meta_info.id;
		meta_file_block.pos = _pos;
		meta_file_block.size = (_pos + this->m_meta_info.block_size < this->m_meta_info.size) ? this->m_meta_info.block_size : (this->m_meta_info.size - _pos);
		meta_file_block.flag = this->m_flag;

		// 2) get flag
		bool is_bcompress = (this->m_flag & resource::eOPTION::BLOCK_COMPRESSED) == resource::eOPTION::BLOCK_COMPRESSED;

		// 3) alloc shared_buffer
		buf_block = alloc_shared_buffer(sizeof(sMETA_FILE_BLOCK) + meta_file_block.size);

		// 4) reserver room
		buf_block.append<sMETA_FILE_BLOCK>();

		// 5) read block using cached file handle
		{
			// - ensure cached file is open
			FILE_HANDLE file_handle;
			{
				SCOPED_LOCK(this->m_lock_file_handle)
				{
					file_handle = this->m_cached_file_handle;
					THROW_IF(file_handle == INVALID_FILE_HANDLE,
							 throwable::failure(eRESULT::FAIL_TO_OPEN, 0, "cached file not open"sv));
				}
			}

			// - prepare md5
			if (this->m_md5_pos < _pos)
			{
				this->_process_md5_prepare(file_handle, _pos);
			}

			// check)
			if (_pos == this->m_meta_info.size)
			{
				buf_block.clear();
				return buf_block;
			}

			// - set file pos
			{
				// - fseek
				auto pos_read = CGDK::api::file::seek(file_handle, _pos);

				// check)
				CGASSERT_ERROR(pos_read == _pos);

				// check)
				THROW_IF(pos_read != _pos, throwable::failure(eRESULT::FAIL, 0, ""sv));
			}

			// - set meta info
			{
				// - read file
				auto size_read = CGDK::api::file::read(file_handle, meta_file_block.size, buf_block.data() + CGDK::get_size_of(meta_file_block));

				// check)
				CGASSERT_ERROR(size_read == meta_file_block.size);

				// check)
				THROW_IF(size_read != meta_file_block.size, throwable::failure(eRESULT::FAIL, 0, ""sv));
			}
		}

		// 6) add md5
		if(this->m_md5_pos == _pos)
		{
			this->_process_md5_append_md5(buf_block.data(CGDK::get_size_of(meta_file_block)), static_cast<uint32_t>(meta_file_block.size));
		}

		// 7) compress if compress flag is set
		if (is_bcompress == false)
		{
			// - add size
			buf_block.add_size(meta_file_block.size);
		}
		else
		{
			// - reuse compression buffer instead of allocating new one each time
			if (this->m_compression_buffer.get_remained_size() < meta_file_block.size * 2)
			{
				this->m_compression_buffer = alloc_shared_buffer(meta_file_block.size * 2);
			}
			this->m_compression_buffer.clear();

			// declare)
			buffer_view buf_temp_source(buf_block.data() + sizeof(sMETA_FILE_BLOCK), meta_file_block.size);

			// - compress block
			auto temp_compressed = CGDK::compress::zip(buffer_view(this->m_compression_buffer.data(), this->m_compression_buffer.get_remained_size()), buf_temp_source);

			// - 만약 압축된 크기가 원본의 90%보다 작을 경우에만 압축을 적용한다.
			if ((buf_temp_source.size() / 10 * 9) > temp_compressed.size())
			{
				// - adde resource::eOPTION::BLOCK_COMPRESSED
				meta_file_block.flag = meta_file_block.flag | resource::eOPTION::BLOCK_COMPRESSED;

				// - copy compressed data
				buf_block.append(temp_compressed.size(), temp_compressed.data());
			}
			else
			{
				// - remove resource::eOPTION::BLOCK_COMPRESSED
				meta_file_block.flag = meta_file_block.flag & (~resource::eOPTION::BLOCK_COMPRESSED);

				// - add size
				buf_block.add_size(meta_file_block.size);
			}
		}

		// 8) write block head info
		buf_block.front<sMETA_FILE_BLOCK>() = meta_file_block;

		// 9) caching loaded block
		{
		}
	}

	// check) 
	CGASSERT_ERROR(buf_block.exist());

	// return) 
	return buf_block;
}

void CGDK::resource::updatable::file_stream::_open_cached_file()
{
	SCOPED_LOCK(this->m_lock_file_handle)
	{
		if (this->m_cached_file_handle == INVALID_FILE_HANDLE)
		{
			this->m_cached_file_handle = CGDK::api::file::open(this->m_full_path.c_str(), eFILE_OPERATION::READ);
			THROW_IF(this->m_cached_file_handle == INVALID_FILE_HANDLE,
					 throwable::failure(eRESULT::FAIL_TO_OPEN, 0, "fail to open cached file"sv));
		}
	}
}

void CGDK::resource::updatable::file_stream::_close_cached_file() noexcept
{
	SCOPED_LOCK(this->m_lock_file_handle)
	{
		if (this->m_cached_file_handle != INVALID_FILE_HANDLE)
		{
			CGDK::api::file::close(this->m_cached_file_handle);
			this->m_cached_file_handle = INVALID_FILE_HANDLE;
		}
	}
}

void CGDK::resource::updatable::file_stream::process_resource_prepare(std::string_view _path_base, std::string_view _path, std::string_view _filename, resource::eOPTION _flag, uint64_t _block_size)
{
	// check)
	CGASSERT_ERROR(_filename.empty() == false);

	// check)
	THROW_IF(_filename.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "filename is empty"sv));

	// check)
	CGASSERT_ERROR(_block_size >= 4096);

	// check)
	THROW_IF(_block_size < 4096, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "_block_size is too few number. it must be over 4096(>=4096) "sv));

	// 1) make full path
	std::filesystem::path full_file_path;
	if (_path_base.empty() == false)
	{
		full_file_path += _path_base;
		full_file_path += "/";
	}
	if (_path.empty() == false)
	{
		full_file_path += _path;
		full_file_path += "/";
	}
	full_file_path.append(_filename);

	// declare)
	size_t size_file = 0;

	// 2) open file and get length
	{
		// - open file
		auto file_handle = CGDK::api::file::open(full_file_path.c_str(), eFILE_OPERATION::READ);

		// check)
		THROW_IF(file_handle == INVALID_FILE_HANDLE, throwable::failure(eRESULT::FAIL_TO_OPEN, 0, "fail to to open file"sv));
		_file_handle_closer file_closer{ file_handle };

		// - get file length
		size_file = CGDK::api::file::get_length(file_handle);
	}

	// 3)
	CGDK::resource::sUPDATABLE_FILE_META_INFO file_meta_info;
	file_meta_info.id = _get_unique_updatable_id(); // update::UPDATABLE_ID
	file_meta_info.size = size_file;
	file_meta_info.block_size = _block_size;
	file_meta_info.type = eTYPE::FILE;
	file_meta_info.path = _path;
	file_meta_info.filename = _filename;
	file_meta_info.time_write = std::filesystem::last_write_time(full_file_path);

	CGDK::sUPDATABLE_FILE_COMPLETION_INFO file_meta_completion;
	file_meta_completion.id = file_meta_info.id;
	//file_meta_completion.check_code;

	// decl0are)
	std::vector<shared_buffer> vector_buffer;

	// 4) file block
	vector_buffer.resize(file_meta_info.size / file_meta_info.block_size + 1);

	// 5) set!
	SCOPED_LOCK(this->m_lockable_info)
	{
		this->m_full_path = std::move(full_file_path);
		this->m_path = std::move(_path);
		this->m_filename = std::move(_filename);
		this->m_flag = _flag;

		// -
		this->m_md5_pos = 0;
		this->m_md5_file.initialize();

		this->m_meta_info = std::move(file_meta_info);
		this->m_meta_completion = std::move(file_meta_completion);

		this->m_vector_buffer = std::move(vector_buffer);
	}

	// 6) open cached file for streaming reads
	this->_open_cached_file();
}

void CGDK::resource::updatable::file_stream::process_resource_load()
{
	// declare) 
	uint64_t size_file;
	uint64_t block_size;

	// 1) move
	SCOPED_LOCK(this->m_lockable_info)
	{
		// check)
		CGASSERT_ERROR(this->m_meta_info.filename.empty());

		// check)
		THROW_IF(this->m_meta_info.filename.empty() == false, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "filename is empty"sv));

		// check) 
		CGASSERT_ERROR(this->m_vector_buffer.empty() == false);

		// check) 
		THROW_IF(this->m_vector_buffer.empty(), throwable::failure(eRESULT::NOT_READY, 0, "not prepared"sv));

		// - get file meta info
		size_file = m_meta_info.size;
		block_size = m_meta_info.block_size;
	}

	// check)
	CGASSERT_ERROR(block_size >= 4096);

	// check) 
	THROW_IF(block_size < 4096, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "block_size is too few number. it must be over 4096(>=4096) "sv));

	// declare) 
	std::vector<CGDK::shared_buffer> vector_buffer;

	// 2) load file blocks
	for (size_t pos = 0; pos < size_file; pos += block_size)
	{
		vector_buffer.push_back(get_block(pos));
	}

	// 3) get block count
	auto block_count = size_file / block_size + 1;

	// check)
	CGASSERT_ERROR(vector_buffer.size() == block_count);

	// check)
	THROW_IF(vector_buffer.size() != block_count, throwable::failure(eRESULT::FAIL, 0, "invalid operation"sv));

	// 4) succes!
	SCOPED_LOCK(this->m_lockable_info)
	{
		m_vector_buffer = std::move(vector_buffer);
	}
}

void CGDK::resource::updatable::file_stream::process_resource_clear() noexcept
{
	// 1) close cached file handle
	this->_close_cached_file();

	SCOPED_LOCK(this->m_lockable_info)
	{
		// - ....
		this->m_full_path.clear();
		this->m_path.clear();
		this->m_filename.clear();
		this->m_flag = resource::eOPTION::NONE;

		// - md5
		this->m_md5_pos = 0;
		this->m_md5_file.initialize();

		// - meta info
		this->m_meta_info = sUPDATABLE_FILE_META_INFO();
		this->m_meta_completion = sUPDATABLE_FILE_COMPLETION_INFO();

		// -clear blocks
		this->m_vector_buffer.clear();
	}
}
