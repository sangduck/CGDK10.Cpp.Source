#include "pch.h"

CGDK::update::UPDATABLE_ID _get_unique_updatable_id() noexcept;


CGDK::result_code CGDK::resource::updatable::folder::process_message(sMESSAGE& /*_msg*/)
{
	return eRESULT::SUCCESS;
}

CGDK::shared_buffer CGDK::resource::updatable::folder::get_block(uint64_t /*_pos*/)
{
	// check)
	CGASSERT_ERROR(false);

	// return) 
	return CGDK::shared_buffer();
}

CGDK::resource::Iupdatable::INITIALIZE_PARAMETER get_parameter_resource_updatable_file(CGDK::context& _context);
CGDK::resource::Iupdatable::INITIALIZE_PARAMETER get_parameter_resource_updatable_folder(CGDK::context& _context)
{
	// declare)
	CGDK::resource::Iupdatable::INITIALIZE_PARAMETER parameter;

	// 1) read ...
	parameter = get_parameter_resource_updatable_file(_context);

	// 2) read 

	// return)
	return parameter;
}

CGDK::resource::eTYPE _get_updatable_resource_type(CGDK::context & _context)
{
	// declare)
	CGDK::resource::eTYPE type = CGDK::resource::eTYPE::NONE;

	// 1) get type
	{
		if (std::string str_find = _context.find("type"sv); str_find.empty() == false)
		{
			if (CGDK::__stricmp(str_find, "file") == 0)
			{
				type = CGDK::resource::eTYPE::FILE;
			}
			else if (CGDK::__stricmp(str_find, "folder"sv) == 0
				|| CGDK::__stricmp(str_find, "directory"sv) == 0
				|| CGDK::__stricmp(str_find, "dir"sv) == 0)
			{
				type = CGDK::resource::eTYPE::FOLDER;
			}
			else if (CGDK::__stricmp(str_find, "ref"sv) == 0
				|| CGDK::__stricmp(str_find, "reference"sv) == 0)
			{
				type = CGDK::resource::eTYPE::REFERENCE;
			}
			else
			{
				type = CGDK::resource::eTYPE::NONE;
			}
		}
	}

	// return)
	return type;
}

void _process_resource_load_folder(std::vector<CGDK::object_ptr<CGDK::resource::Iupdatable>>& _vector_updatable_process, const CGDK::sINITIALIZE_INFO_UPDATABLE_RESOURCE& _parameter)
{
	// check)
	RETURN_IF(_parameter.src.path.empty(), );

	// 1) make full path
	std::filesystem::path full_file_path;

#if _WIN32
	{
		// declare) 
		wchar_t temp_dest[256];

		if (_parameter.src.path_base.empty() == false)
		{
			full_file_path += CGDK::encode_string(temp_dest, reinterpret_cast<const char8_t*>(_parameter.src.path_base.c_str()));
			full_file_path += "/";
		}
		if (_parameter.src.path.empty() == false)
		{
			full_file_path += CGDK::encode_string(temp_dest, reinterpret_cast<const char8_t*>(_parameter.src.path.c_str()));
			full_file_path += "/";
		}
		if (_parameter.src.filename.empty() == false)
		{
			//full_file_path += CGDK::encode_string(temp_dest, reinterpret_cast<const char8_t*>(_parameter.src.filename.c_str()));
			//full_file_path += "/";
		}
	}
#else
	if (_parameter.src.path_base.empty() == false)
	{
		full_file_path += _parameter.src.path_base;
		full_file_path += "/";
	}
	if (_parameter.src.path.empty() == false)
	{
		full_file_path += _parameter.src.path;
		full_file_path += "/";
	}
	if (_parameter.src.filename.empty() == false)
	{
		//full_file_path += _parameter.src.filename;
		//full_file_path += "/";
	}
#endif

	// log) 
	LOG_INFO << "     folder " << full_file_path.string();

	// 2) path만 골라내기 위해 patah_base의 길이를 얻어 놓는다.
	auto path_base_size = _parameter.src.path_base.size() + 1; // '/'도 제거해야하기 때문에 +!을 한다.

	// 3) 재귀적 옵션이 설정되어 있을 경우 => folger를 읽어들인다.
	if ((_parameter.options & CGDK::resource::eOPTION::RECURSIVE) == CGDK::resource::eOPTION::RECURSIVE)
	{
		for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(full_file_path))
		{
			if (std::filesystem::is_regular_file(entry.path()))
			{
				// - get entry path
				auto entry_path = entry.path();

				// - path를 구한다.(path에서 path_base만큼을 제거한 문자열)
				CGDK::resource::Iupdatable::INITIALIZE_PARAMETER temp_initialize_prarameter;
				temp_initialize_prarameter.src.path_base = _parameter.src.path_base;
				temp_initialize_prarameter.src.path = entry_path.parent_path().string().substr(path_base_size);
				temp_initialize_prarameter.src.filename = entry_path.filename().string();
				temp_initialize_prarameter.options = _parameter.options;

				// - windows일 경우 ucs8로 변환해서 저장한다.
			#if defined(_WIN32)
				{
					char temp_dest[1024];
					{
						[[maybe_unused]]auto result = CGDK::encode_string(reinterpret_cast<char8_t*>(temp_dest), reinterpret_cast<const char*>(temp_initialize_prarameter.src.path_base.c_str()));
						temp_initialize_prarameter.src.path_base = std::string(temp_dest);
					}
					{
						[[maybe_unused]] auto result = CGDK::encode_string(reinterpret_cast<char8_t*>(temp_dest), reinterpret_cast<const char*>(temp_initialize_prarameter.src.path.c_str()));
						temp_initialize_prarameter.src.path = std::string(temp_dest);
					}
					{
						[[maybe_unused]] auto result = CGDK::encode_string(reinterpret_cast<char8_t*>(temp_dest), reinterpret_cast<const char*>(temp_initialize_prarameter.src.filename.c_str()));
						temp_initialize_prarameter.src.filename = std::string(temp_dest);
					}
				}
			#endif

				// - 객체를 생성한다.
				auto presource_file = make_object<CGDK::resource::updatable::file>();

				// - initlaizie
				presource_file->initialize(temp_initialize_prarameter);

				// - add 
				_vector_updatable_process.push_back(presource_file);
			}
		}
	}
	// 4) 재귀적 옵션이 설정되지 않은 경우 => file만 읽어들인다.
	else
	{
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(full_file_path))
		{
			if (std::filesystem::is_regular_file(entry.path()))
			{
				// - get entry path
				auto entry_path = entry.path();

				// - path를 구한다.(path에서 path_base만큼을 제거한 문자열)
				CGDK::resource::Iupdatable::INITIALIZE_PARAMETER temp_initialize_prarameter;
				temp_initialize_prarameter.src.path_base = _parameter.src.path_base;
				temp_initialize_prarameter.src.path = entry_path.parent_path().string().substr(path_base_size);
				temp_initialize_prarameter.src.filename = entry_path.filename().string();
				temp_initialize_prarameter.options = _parameter.options;

			#if defined(_WIN32)
				{
					char temp_dest[1024];
					{
						[[maybe_unused]] auto result = CGDK::encode_string(reinterpret_cast<char8_t*>(temp_dest), reinterpret_cast<const char*>(temp_initialize_prarameter.src.path_base.c_str()));
						temp_initialize_prarameter.src.path_base = std::string(temp_dest);
					}
					{
						[[maybe_unused]] auto result = CGDK::encode_string(reinterpret_cast<char8_t*>(temp_dest), reinterpret_cast<const char*>(temp_initialize_prarameter.src.path.c_str()));
						temp_initialize_prarameter.src.path = std::string(temp_dest);
					}
					{
						[[maybe_unused]] auto result = CGDK::encode_string(reinterpret_cast<char8_t*>(temp_dest), reinterpret_cast<const char*>(temp_initialize_prarameter.src.filename.c_str()));
						temp_initialize_prarameter.src.filename = std::string(temp_dest);
					}
				}
			#endif

				// - 객체를 생성한다.
				auto presource_file = make_object<CGDK::resource::updatable::file>();

				// - initlaizie
				presource_file->initialize(temp_initialize_prarameter);

				// - add 
				_vector_updatable_process.push_back(presource_file);
			}
		}
	}
}

void _process_resource_load_index(std::vector<CGDK::object_ptr<CGDK::resource::Iupdatable>>& _vector_updatable_process, CGDK::context& _context)
{
	// 1) get refkey
	std::string str_find = _context.find("ref"sv, "reference"sv);

	// check)
	THROW_IF(str_find.empty(), CGDK::throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'reference' index not exist"sv), LOG_ERROR << "     (!) 'reference' index not exist");

	// log) 
	LOG_INFO << "     reference index " << str_find;

	// 2) get ..
	auto pmap_updatable_resource = static_cast<std::map<CGDK::update::UPDATE_RESOURCE_ID, CGDK::object_ptr<CGDK::resource::Iupdatable>>*>(_context["resource_manager"sv]);

	// check)
	THROW_IF(pmap_updatable_resource == nullptr, CGDK::throwable::failure(eRESULT::NOT_READY, 0, "'resource_manager' not exist"sv), LOG_ERROR << "     (!) resource manager not exist ");

	// 3) find resource
	auto iter_find = pmap_updatable_resource->find(CGDK::__from_string<char>(str_find));

	// check)
	THROW_IF(iter_find == pmap_updatable_resource->end(), CGDK::throwable::failure(eRESULT::NOT_EXIST, 0, "'resource' not exist"sv), LOG_ERROR << "     (!) resource not exist ");

	// 4) add update_resource
	_vector_updatable_process.push_back(iter_find->second);
}

void _process_resource_load_list(
	std::vector<CGDK::object_ptr<CGDK::resource::Iupdatable>>& _vector_updatable_process,
	CGDK::context& _context_list,
	CGDK::context& _context)
{
	for (auto& iter : _context_list)
	{
		// declare)
		CGDK::object_ptr<CGDK::resource::Iupdatable> pupdatable_resource;

		auto type = _get_updatable_resource_type(iter.second);

		// - create 
		switch (type)
		{
		case	CGDK::resource::eTYPE::FILE:
				{
					// - 객체를 생성한다.
					pupdatable_resource = make_object<CGDK::resource::updatable::file>();

					// - initlaize
					pupdatable_resource->initialize(iter.second);
				}
				break;

		case	CGDK::resource::eTYPE::FOLDER:
				{
					// - 객체를 생성한다.
					pupdatable_resource = make_object<CGDK::resource::updatable::folder>();

					// - initlaize
					pupdatable_resource->initialize(iter.second);
				}
				break;

		case	CGDK::resource::eTYPE::REFERENCE:
				{
					// - get ..
					auto pmap_updatable_resource = static_cast<std::map<CGDK::update::UPDATE_RESOURCE_ID, CGDK::object_ptr<CGDK::resource::Iupdatable>>*>(_context["resource_manager"sv]);

					// check)
					THROW_IF(pmap_updatable_resource == nullptr, CGDK::throwable::failure(eRESULT::NOT_READY, 0, "'resource_manager' not exist"sv), LOG_ERROR << "     (!) 'reference' index not exist");

					// - get refkey
					std::string str_find = iter.second.find("ref"sv, "reference"sv);

					// check)
					THROW_IF(str_find.empty(), CGDK::throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'ref' index not exist"sv), LOG_ERROR << "     (!) 'reference' index not exist");

					// - find resource
					auto iter_find = pmap_updatable_resource->find(CGDK::__from_string<char>(str_find));

					// check)
					THROW_IF(iter_find == pmap_updatable_resource->end(), CGDK::throwable::failure(eRESULT::NOT_EXIST, 0, "'resource' not exist"sv), LOG_ERROR << "     (!) resource not exist ");

					// - get reference
					pupdatable_resource = iter_find->second;
				}
				break;

		default:
			break;
		}

		// - insert
		_vector_updatable_process.push_back(std::move(pupdatable_resource));
	}
}

CGDK::result_code CGDK::resource::updatable::folder::_process_initializing(context& _context)
{
	// 1) call 'on_initialize'
	this->on_initializing(_context);

	try
	{
		// declare)
		std::vector<object_ptr<resource::Iupdatable>> vector_updatable_process;

		// declare)
		CGDK::resource::Iupdatable::INITIALIZE_PARAMETER parameter;

		// 2) parsing socket setting options
		if (_context.type() == eCONTEXT_VALUE_TYPE::BUFFER_)
		{
			parameter = _context.v.value_buffer.front<CGDK::resource::Iupdatable::INITIALIZE_PARAMETER>();
		}
		else
		{
			parameter = get_parameter_resource_updatable_folder(_context);
		}

		// 3) FILE_META_INFO
		sUPDATABLE_FILE_META_INFO file_meta_info;
		file_meta_info.id = _get_unique_updatable_id(); // update::UPDATABLE_ID
		file_meta_info.size = 0;
		file_meta_info.block_size = 0;
		file_meta_info.type = eTYPE::FILE;
		file_meta_info.path = parameter.src.path;
		file_meta_info.filename.clear();

		// 4) FILE_META_COMPLETION
		sUPDATABLE_FILE_COMPLETION_INFO file_meta_completion;
		file_meta_completion.id = file_meta_info.id;

		// 5) loading
		{
			// - get type
			auto type = _get_updatable_resource_type(_context);

			// - load ...
			switch(type)
			{
			case	resource::eTYPE::FOLDER:
					_process_resource_load_folder(vector_updatable_process, parameter);
					break;

			case	resource::eTYPE::REFERENCE:
					_process_resource_load_index(vector_updatable_process, _context);
					break;
		
			default:
					throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'invalid resource type"sv);
			}
		}

		// 3) update resource list
		{
			auto context_system_executor = _context.find("resource_list"sv);

			if (context_system_executor.is_valid())
			{
				_process_resource_load_list(vector_updatable_process, context_system_executor, _context);
			}
		}

		// 4) setting
		scoped_lock(this->m_cs_info)
		{
			this->m_meta_info = std::move(file_meta_info);
			this->m_meta_completion = std::move(file_meta_completion);
			this->m_vector_updatable_process = std::move(vector_updatable_process);
		}
	}
	catch (std::exception& _exception)
	{
		// - call exception handler
		throwable::execute_exception_handler(this, _exception, __FUNCTION__);

		// - get name
		auto* pnameable = dynamic_cast<Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string_view();

		// log) 
		LOG_DEBUG << "(excp) exception occure on '_process_starting' ['"sv << pname << "']"sv;

		// reraise)
		throw;
	}
	catch (...)
	{
		// - get name
		auto* pnameable = dynamic_cast<Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string_view();

		// log) 
		LOG_DEBUG << "(excp) exception occure on '_process_starting' ['"sv << pname << "']"sv;

		// reraise)
		throw;
	}

	// return) 
	return eRESULT::SUCCESS;
}

CGDK::result_code CGDK::resource::updatable::folder::_process_initialize(context& _context)
{
	this->on_initialize(_context);

	return eRESULT::SUCCESS;
}

CGDK::result_code CGDK::resource::updatable::folder::_process_destroying() noexcept
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

CGDK::result_code CGDK::resource::updatable::folder::_process_destroy() noexcept
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

void CGDK::resource::updatable::folder::process_resource_clear() noexcept
{
	// declare)
	std::vector<object_ptr<resource::Iupdatable>> vector_updatable_process;

	// 1) reset info
	scoped_lock(this->m_cs_info)
	{
		this->m_meta_info = sUPDATABLE_FILE_META_INFO();
		this->m_meta_completion = sUPDATABLE_FILE_COMPLETION_INFO();
		vector_updatable_process = std::move(this->m_vector_updatable_process);
	}
}
