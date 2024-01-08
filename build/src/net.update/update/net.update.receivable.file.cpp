#include "pch.h"


void _get_md5(std::string_view _full_file_path, CGDK::edc::md5& _md5, uint64_t _end)
{
	// check)
	CGASSERT_ERROR(_full_file_path.empty() == false);

	// 1) make file
	auto file_handle = CGDK::api::file::open(_full_file_path, CGDK::eFILE_OPERATION::READ);

	// check)
	THROW_IF(file_handle == CGDK::INVALID_FILE_HANDLE, CGDK::throwable::failure(eRESULT::INVALID_HANDLE, 0, "invalid handle (file is not opened)"sv));

	// 2) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(CGDK::update::_FILE_BLOCK_SIZE * 2);

	// declare) 
	uint64_t pos_now = 0;

	// 3) ...
	while (pos_now < _end)
	{
		// - set pos
		auto result = CGDK::api::file::seek(file_handle, pos_now);

		// check)
		THROW_IF(result != pos_now, CGDK::throwable::failure(eRESULT::FAIL_TO_OPERATION, 0, "file is not opened"sv));

		// - read file block
		uint64_t size_read = CGDK::api::file::read(file_handle, CGDK::update::_FILE_BLOCK_SIZE, buf_temp.data());

		// - push md5
		_md5.push_md5(buf_temp.data(), static_cast<uint32_t>(size_read));

		// - pos_now...
		pos_now += size_read;
	}

	// 4) close file
	CGDK::api::file::close(file_handle);
}

void _get_md5(CGDK::FILE_HANDLE _file_handle, CGDK::edc::md5& _md5, uint64_t _end)
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(CGDK::update::_FILE_BLOCK_SIZE * 2);

	// declare) 
	uint64_t pos_now = 0;

	// 2) ...
	while (pos_now < _end)
	{
		// - set pos
		auto result = CGDK::api::file::seek(_file_handle, pos_now);

		// check)
		THROW_IF(result != pos_now, CGDK::throwable::failure(eRESULT::FAIL_TO_OPERATION, 0, "file is not opened"sv));

		// - read file block
		uint64_t size_read = CGDK::api::file::read(_file_handle, CGDK::update::_FILE_BLOCK_SIZE, buf_temp.data());

		// - push md5
		_md5.push_md5(buf_temp.data(), static_cast<uint32_t>(size_read));

		// - pos_now...
		pos_now += size_read;
	}
}

void _get_md5_to_end (CGDK::FILE_HANDLE _file_handle, CGDK::edc::md5& _md5, uint64_t _pos)
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(CGDK::update::_FILE_BLOCK_SIZE * 2);

	// 2) set pos
	auto result = CGDK::api::file::seek(_file_handle, _pos);

	// check)
	THROW_IF(result != _pos, CGDK::throwable::failure(eRESULT::FAIL_TO_OPERATION, 0, "file is not opened"sv));

	// declare)
	size_t size_read;

	// 3) read data and push to md5 until end of file
	do
	{
		// - read file block
		size_read = CGDK::api::file::read(_file_handle, CGDK::update::_FILE_BLOCK_SIZE, buf_temp.data());

		// - push md5
		_md5.push_md5(buf_temp.data(), static_cast<uint32_t>(size_read));
	} while (size_read != 0);
}

CGDK::sUPDATABLE_PREPARE_INFO CGDK::update::receivable::file::process_prepare(std::string_view _full_file_path)
{
	// check) 
	CGASSERT_ERROR(this->m_meta_info.id != 0);

	// check) 
	CGASSERT_ERROR(this->m_meta_info.type == resource::eTYPE::FILE);

	// check) 
	CGASSERT_ERROR(this->m_meta_info.filename.empty() == false);

	// declare) 
	CGDK::sUPDATABLE_PREPARE_INFO updatable_prepare_info;

	// declare) 
	std::error_code ec;

	// 1) get last write time
	auto file_length = std::filesystem::file_size(_full_file_path, ec);

	// 2) 
	if (!ec)
	{
		// 설명) 기존의 파일이 있는 경우이다.
		//       - 파일의 길이가 완료 길이와 동일한 경우 md5값도 구해서 보낸다.

		// - get md5
		this->m_md5_check.initialize();

		// - ...
		if (file_length == this->m_meta_info.size)
		{
			// declare) 
			CGDK::edc::md5 temp_md5;

			// - get md5
			temp_md5.initialize();
			_get_md5(_full_file_path, temp_md5, file_length);
			temp_md5.padding();

			// - set md5
			updatable_prepare_info.check_code = temp_md5.get_md5();
		}

		// - ...
		updatable_prepare_info.size = file_length;
		updatable_prepare_info.result = eRESULT::ALEADY_EXIST;
	}
	else
	{
		// 설명) 파일이 없는 경우이다. 
		//       - size를 0으로 설정하고 result는 NOT_EXIST로 설정한다.

		updatable_prepare_info.size = 0;
		updatable_prepare_info.result = eRESULT::NOT_EXIST;
	}

	// 2) set meta info
	scoped_lock(this->m_cs_info)
	{
		this->m_full_file_path = std::move(_full_file_path);
	}

	// return) 
	return updatable_prepare_info;
}

void CGDK::update::receivable::file::process_begin(uint64_t _pos_begin)
{
	// check)
	CGASSERT_ERROR(m_file_handle == INVALID_FILE_HANDLE);

	// declare) 
	eFILE_OPERATION file_operation = eFILE_OPERATION::UPDATE;

	// 1) resource::eFILE_FLAG::RECREATE flag
	if ((this->m_meta_info.flag & resource::eFILE_FLAG::RECREATE) == resource::eFILE_FLAG::RECREATE)
	{
		file_operation = eFILE_OPERATION::WRITE;
	}

	// 2) make file
	auto file_handle = CGDK::api::file::open(this->m_full_file_path, file_operation);

	// check)
	THROW_IF(file_handle == INVALID_FILE_HANDLE, throwable::failure(eRESULT::FAIL_TO_OPEN, 0, "fail to to open file"sv));

	// check)
	CGASSERT_ERROR(this->m_meta_info.block_size >= 4096);

	// check)
	CGASSERT_ERROR(this->m_meta_info.block_size >= 4096);

	// 3) get md5
	CGDK::edc::md5 temp_md5;
	temp_md5.initialize();
	if (_pos_begin != 0)
	{
		_get_md5(file_handle, temp_md5, _pos_begin);
	}

	// 4) set result
	scoped_lock(this->m_cs_info) 
	{
		// - file handle
		this->m_file_handle = file_handle;
		this->m_md5_check = temp_md5;
	}
}

CGDK::result_code CGDK::update::receivable::file::process_process(const  CGDK::resource::sMETA_FILE_BLOCK& _meta_file_block, buffer_view _buf_data)
{
	// check)
	CGASSERT_ERROR(_meta_file_block.size == _buf_data.size());

	// declare) 
	FILE_HANDLE file_handle;

	// 1) get file_handle
	scoped_lock(this->m_cs_info)
	{
		file_handle = this->m_file_handle;
	}

	// check)
	THROW_IF(file_handle == INVALID_FILE_HANDLE, throwable::failure(eRESULT::INVALID_HANDLE, 0, "invalid handle (file is not opened)"sv));

	// 2) set pose
	{
		// - set pos
		auto result = CGDK::api::file::seek(file_handle, _meta_file_block.pos);

		// check)
		CGASSERT_ERROR(result == _meta_file_block.pos);

		// check)
		THROW_IF(result != _meta_file_block.pos, throwable::failure(eRESULT::FAIL_TO_OPERATION, 0, "file is not opened"sv));
	}

	// 3) write data
	{
		// declare)
		shared_buffer buffer_decompressed;

		if ((_meta_file_block.flag & resource::eOPTION::BLOCK_COMPRESSED) == resource::eOPTION::BLOCK_COMPRESSED)
		{

		}

		// - write file block
		auto result = CGDK::api::file::write(file_handle, _meta_file_block.size, _buf_data.data());

		// check)
		CGASSERT_ERROR(result == _meta_file_block.size);

		// check)
		THROW_IF(result != _meta_file_block.size, throwable::failure(eRESULT::FAIL_TO_OPERATION, 0, "fail to write"sv));
	}

	//return)
	return eRESULT::DONE;
}

CGDK::result_code CGDK::update::receivable::file::process_end()
{
	// declare) 
	FILE_HANDLE file_handle;
	std::filesystem::path full_file_path;

	// 1) reset meta info
	scoped_lock(this->m_cs_info)
	{
		// - move file handle
		file_handle = this->m_file_handle;
		this->m_file_handle = INVALID_FILE_HANDLE;

		// - move file file path
		full_file_path = std::move(this->m_full_file_path);
	}

	// check)
	RETURN_IF(file_handle == INVALID_FILE_HANDLE, eRESULT::FAIL_TO_OPERATION);

	// check)
	CGASSERT_ERROR(full_file_path.empty() == false);

	// 2) close file
	CGDK::api::file::close(file_handle);

	// reuturn)
	return eRESULT::DONE;
}
