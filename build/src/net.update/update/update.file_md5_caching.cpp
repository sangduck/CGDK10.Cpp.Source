#include "pch.h"


CGDK::update::md5_caching::md5_caching()
{

}

CGDK::update::md5_caching::~md5_caching() noexcept
{

}

CGDK::result_code CGDK::update::md5_caching::start()
{
	context _context;

	return this->start(_context);
}

CGDK::result_code CGDK::update::md5_caching::start(context& _context)
{
	return object::Ninitializable_startable::start(_context);
}

CGDK::result_code CGDK::update::md5_caching::start(std::string_view _filename, int _file_confirm_level, bool _auto_save)
{
	// declare)
	context _context;

	// 1) set values
	SCOPED_LOCK(this->m_lockable_map_prepare_info)
	{
		this->m_file_confirm_level = _file_confirm_level;
		this->m_filename = _filename;
		this->m_is_auto_save = _auto_save;
	}

	// return) 
	return object::Ninitializable_startable::start(_context);
}

CGDK::sUPDATABLE_PREPARE_INFO CGDK::update::md5_caching::get_prepare_info(const std::string& _full_file_path) const noexcept
{
	// declare) 
	CGDK::sUPDATABLE_PREPARE_INFO updatable_prepare_info;

	SCOPED_LOCK(this->m_lockable_map_prepare_info)
	{
		// 1) find cached PREPARE_INFO
		auto iter_find = m_map_prepare_info.find(_full_file_path);

		// check)
		RETURN_IF(iter_find == m_map_prepare_info.end(), CGDK::sUPDATABLE_PREPARE_INFO());

		// 2) .. 
		updatable_prepare_info = iter_find->second.prepare_info;
	}

	// return) 
	return updatable_prepare_info;
}

void CGDK::update::md5_caching::put_prepare_info (const std::string& _full_file_path, const sUPDATABLE_PREPARE_INFO& _prepare_info)
{
	// declare) 
	std::error_code ec;

	// 1) get last write time
	auto temp_time = std::filesystem::last_write_time(_full_file_path, ec);

	// check)
	RETURN_IF(ec);

	SCOPED_LOCK(this->m_lockable_map_prepare_info)
	{
		// 2) find cached PREPARE_INFO
		auto iter_find = m_map_prepare_info.find(_full_file_path);

		// check)
		if (iter_find == m_map_prepare_info.end())
		{
			m_map_prepare_info.insert({ _full_file_path, {_prepare_info, temp_time} });
		}
		else
		{
			iter_find->second = sPREPARE_INFO{ _prepare_info, temp_time };
		}

		// 3) flag changed
		this->m_is_changed = true;
	}
}

void CGDK::update::md5_caching::pop_prepare_info(const std::string& _full_file_path) noexcept
{
	SCOPED_LOCK(this->m_lockable_map_prepare_info)
	{
		// 1) find cached PREPARE_INFO
		auto iter_find = m_map_prepare_info.find(_full_file_path);

		// check)
		RETURN_IF(iter_find == m_map_prepare_info.end());

		// 2) erase
		m_map_prepare_info.erase(iter_find);

		// 2) flag changed
		this->m_is_changed = true;
	}
}

void CGDK::update::md5_caching::save()
{
	process_save_caching_file(this->m_filename);
}

void CGDK::update::md5_caching::save_as(std::string_view _filename)
{
	process_save_caching_file(_filename);
}

void CGDK::update::md5_caching::load()
{
	process_load_caching_file(this->m_filename);
}

void CGDK::update::md5_caching::load_from(std::string_view _filename)
{
	process_load_caching_file(_filename);
}

void CGDK::update::md5_caching::set_filename(const std::string& _filename)
{
	m_filename = _filename;
}

void CGDK::update::md5_caching::set_filename(std::string&& _filename)
{
	m_filename = std::move(_filename);
}

std::filesystem::path CGDK::update::md5_caching::get_filename() const noexcept
{
	return m_filename;
}


CGDK::result_code CGDK::update::md5_caching::_process_starting(context& _context)
{
	// 1) on_starting 함수를 호출한다.
	this->on_starting(_context);

	// trace)
	LOG_PROGRESS << "@ <BEGIN> start MD5 CACHING FOR UPDATE RECEIVER['"sv << /*this->name() <<*/ "']"sv;

	// declare)
	std::string filename;

	// 2) get filename
	SCOPED_LOCK(this->m_lockable_map_prepare_info)
	{
		filename = this->m_filename;
	}

	// 3) load file
	if(filename.empty() == false)
	{
		try
		{
			this->process_load_caching_file(filename);
		}
		catch (...)
		{
			LOG_ERROR << "@ (excp) can't open file "sv << this->m_filename << " '"sv << /*name() <<*/ "' " __CGFUNCTION__ ""sv;
		}
	}

	// return)
	return eRESULT::SUCCESS;
}

CGDK::result_code CGDK::update::md5_caching::_process_stopping() noexcept
{
	// trace)
	LOG_PROGRESS << "@ <BEGIN> stop 'MD5 caching for receiver'['"sv << /*name() <<*/ "']"sv;

	try
	{
		// 1) on_stopping 함수를 호출한다.
		this->on_stopping();

		// 2) auto save
		if (this->m_is_auto_save 
		 && this->m_is_changed == true 
		 && this->m_filename.empty() == false)
		{
			this->process_save_caching_file(this->m_filename);
		}
	}
	catch (...)
	{
		// log) 
		LOG_ERROR << "@ (excp) exception on 'on_stopping' function call MD5 CACHING FOR UPDATE RECEIVER'"sv << /*name() <<*/ "' " __CGFUNCTION__ ""sv;
	}

	// return)
	return eRESULT::SUCCESS;
}

void CGDK::update::md5_caching::process_save_caching_file(std::string_view _filename)
{
	// check)
	CGASSERT_ERROR(_filename.empty() == false);

	// check)
	THROW_IF(_filename.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "file name is empty"sv));

	// declare) 
	eFILE_OPERATION file_operation = eFILE_OPERATION::WRITE;

	// 1) make file
	auto file_handle = CGDK::api::file::open(_filename, file_operation);

	// check)
	THROW_IF(file_handle == INVALID_FILE_HANDLE, throwable::failure(eRESULT::FAIL_TO_OPEN, 0, "fail to to open file"sv));

	try
	{
		// declare)
		shared_buffer buf_write;

		// declare)
		size_t size_write = 0;
		size_t file_info_count = 0;

		SCOPED_LOCK(this->m_lockable_map_prepare_info)
		{
			// 2) get size of data
			size_write = get_size_of(this->m_map_prepare_info);

			// 3) alloc shared_buffer
			buf_write = alloc_shared_buffer(size_write);

			// 3) append data
			buf_write.append(this->m_map_prepare_info);

			// 4) get file info count
			file_info_count = this->m_map_prepare_info.size();
		}

		// 5) write file block
		auto result_write = CGDK::api::file::write(file_handle, buf_write.size(), buf_write.data());

		// check)
		CGASSERT_ERROR(result_write == size_write);

		// check)
		THROW_IF(result_write != size_write, throwable::failure(eRESULT::FAIL_TO_OPERATION, 0, "fail to write"sv));

		// 6) close file
		CGDK::api::file::close(file_handle);

		SCOPED_LOCK(this->m_lockable_map_prepare_info)
		{
			this->m_is_changed = false;
		}

		// log)
		LOG_INFO << "  - md5 caching data saved:"sv << CGDK::stream_string(file_info_count, " file infos.");

		// 7)...
		this->on_caching_file_saved();
	}
	catch (...)
	{
		// - close file
		CGDK::api::file::close(file_handle);

		// - reraise
		throw;
	}
}

void CGDK::update::md5_caching::process_load_caching_file(std::string_view _filename)
{
	// check)
	CGASSERT_ERROR(_filename.empty() == false);

	// check)
	THROW_IF(_filename.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "file name is empty"sv));

	// declare) 
	eFILE_OPERATION file_operation = eFILE_OPERATION::READ;

	// 1) make file
	auto file_handle = CGDK::api::file::open(_filename, file_operation);

	// check)
	THROW_IF(file_handle == INVALID_FILE_HANDLE, throwable::failure(eRESULT::FAIL_TO_OPEN, 0, "fail to to open file"sv));

	try
	{
		// 2) get size of file
		auto size_read = CGDK::api::file::get_length(file_handle);

		// 3) alloc shard_buffer
		auto buf_read = alloc_shared_buffer(size_read);

		// 3) read buffer
		auto result_read = CGDK::api::file::read(file_handle, size_read, buf_read.data());

		// check)
		CGASSERT_ERROR(result_read == size_read);

		// check)
		THROW_IF(result_read != size_read, throwable::failure(eRESULT::FAIL_TO_OPERATION, 0, "fail to write"sv));

		// 4) add length
		buf_read.add_size(size_read);

		// 5) extract data
		auto map_prepare_info = buf_read.extract<std::map<std::string, sPREPARE_INFO>>();

		// log)
		LOG_INFO << "  - md5 caching data loaded:"sv << CGDK::stream_string(map_prepare_info.size(), " file infos.");

		// 6) confirm files
		bool is_changed = false;
		
		if (this->m_file_confirm_level >= 1)
		{
			is_changed = _process_confirm_files(map_prepare_info);
		}

		if (this->m_file_confirm_level >= 2)
		{
		}

		// 7) set!
		SCOPED_LOCK(this->m_lockable_map_prepare_info)
		{
			this->m_map_prepare_info = std::move(map_prepare_info);
			this->m_is_changed = is_changed;
		}

		// 7) close file
		CGDK::api::file::close(file_handle);
	}
	catch (...)
	{
		// - close file
		CGDK::api::file::close(file_handle);

		// - reraise
		throw;
	}

	// 8)...
	this->on_caching_file_loaded();

}

bool CGDK::update::md5_caching::_process_confirm_files(std::map<std::string, sPREPARE_INFO>& _map_cached_file)
{
	// ------------------------------------------
	// 설명) confirm files
	//       - cached된 file info들이 실제 파일과 일치하는지를 확인한다.
	//       - 만약 cache된 최종 저장 시간(last write_tiem)이 실제 파일과 다를 경우 eRESULT::EXPIRED를 설정한다.
	//         cache된 file 정보가 유효하지 않음을 말한다. file이 추후 변경되거나 다른 파일로 교체되었을 가능성이 높다.
	//         -> 이것은 check code가 일치하건 말건 update로 설정된다.
	//       - 만약 file의 크기가 변경되었다면 역시 eRESULT::EXPIRED를 설정한다.
	//         file이 추후 변경되거나 다른 파일로 교체되었을 가능성이 높다.
	//         -> 이것은 check code가 일치하건 말건 update로 설정된다.
	//       - 모두 성공한 경우 eRESULT::DONE으루 설정한다.
	// ------------------------------------------
	// log)
	LOG_INFO << "  - confirm files(L1: check file write time and file length)"sv;

	// declare)
	int deleted_files = 0;
	int invalid_files = 0;

	auto iter = _map_cached_file.begin();

	while(iter != _map_cached_file.end())
	{
		try
		{
			// declare) 
			std::error_code ec;

			// - get last write time
			auto last_write_time = std::filesystem::last_write_time(iter->first, ec);

			// check)
			if (ec)
			{
				++deleted_files;

				iter = _map_cached_file.erase(iter);
				continue;
			}

			// check) is same write time
			if (last_write_time != iter->second.time_write)
			{
				++invalid_files;
				iter->second.prepare_info.result = eRESULT::EXPIRED;

				++iter;
				continue;
			}

			// - get last write time
			auto file_length = std::filesystem::file_size(iter->first, ec);

			// check)
			if (ec)
			{
				++deleted_files;

				iter = _map_cached_file.erase(iter);
				continue;
			}

			// check) is same write time
			if (file_length != iter->second.prepare_info.size)
			{
				++invalid_files;
				iter->second.prepare_info.result = eRESULT::EXPIRED;

				++iter;
				continue;
			}
		}
		catch (...)
		{
			++deleted_files;

			iter = _map_cached_file.erase(iter);
			continue;
		}

		// - DONE
		iter->second.prepare_info.result = eRESULT::DONE;

		// - next
		++iter;
	}

	// log)
	if (deleted_files != 0)
	{
		LOG_INFO << CGDK::stream_string("    ", deleted_files, " file infos removed.");
		LOG_INFO << CGDK::stream_string("    ", invalid_files, " file infos is invalid.");
	}

	//return)
	return deleted_files != 0;
}