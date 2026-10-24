#include "pch.h"
#include <charconv>

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
			else if (CGDK::__stricmp(str_find, "folder") == 0
				|| CGDK::__stricmp(str_find, "directory") == 0
				|| CGDK::__stricmp(str_find, "dir") == 0)
			{
				type = CGDK::resource::eTYPE::FOLDER;
			}
			else if (CGDK::__stricmp(str_find, "ref") == 0
				|| CGDK::__stricmp(str_find, "reference") == 0)
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

namespace
{
	// std::filesystem::path -> UTF-8 std::string with '/' separators (what the receiver expects on every platform)
	std::string _to_utf8(const std::filesystem::path& _path)
	{
		const auto u8 = _path.generic_u8string();
		return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
	}
}

void _process_resource_load_folder(std::vector<CGDK::object_ptr<CGDK::resource::Iupdatable>>& _vector_updatable_process, const CGDK::sINITIALIZE_INFO_UPDATABLE_RESOURCE& _parameter)
{
	// check)
	RETURN_IF(_parameter.src.path.empty(), );

	// 1) make full path (path_base and path are UTF-8, like every resource string)
	std::filesystem::path path_base;
	std::filesystem::path full_file_path;

	if (_parameter.src.path_base.empty() == false)
	{
		path_base = std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t*>(_parameter.src.path_base.data()), _parameter.src.path_base.size()));
		full_file_path = path_base;
	}

	full_file_path /= std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t*>(_parameter.src.path.data()), _parameter.src.path.size()));

	// log)
	LOG_INFO << "     folder " << full_file_path.string();

	// 2) path_base as the file resources expect it (UTF-8)
	const std::string& path_base_utf8 = _parameter.src.path_base;

	// 3) one file resource per regular file; its path is relative to path_base (the receiver recreates it below its root)
	auto process_entry = [&](const std::filesystem::directory_entry& _entry)
	{
		// check)
		RETURN_IF(_entry.is_regular_file() == false, );

		// - get entry path
		const auto& entry_path = _entry.path();

		// - initialize parameter
		CGDK::resource::Iupdatable::INITIALIZE_PARAMETER temp_initialize_parameter;
		temp_initialize_parameter.src.path_base = path_base_utf8;
		temp_initialize_parameter.src.path = _to_utf8(path_base.empty() ? entry_path.parent_path() : entry_path.parent_path().lexically_relative(path_base));
		temp_initialize_parameter.src.filename = _to_utf8(entry_path.filename());
		temp_initialize_parameter.options = _parameter.options;

		// - create file resource
		auto presource_file = make_object<CGDK::resource::updatable::file>();

		// - initialize
		presource_file->initialize(temp_initialize_parameter);

		// - add
		_vector_updatable_process.push_back(presource_file);
	};

	// 4) recursive or not
	if ((_parameter.options & CGDK::resource::eOPTION::RECURSIVE) == CGDK::resource::eOPTION::RECURSIVE)
	{
		for (const auto& entry : std::filesystem::recursive_directory_iterator(full_file_path))
		{
			process_entry(entry);
		}
	}
	else
	{
		for (const auto& entry : std::filesystem::directory_iterator(full_file_path))
		{
			process_entry(entry);
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
	// - the resource manager stores its resource map as a pointer value
	auto pcontext_resource_manager = _context.try_find("resource_manager"sv);

	// check)
	THROW_IF(pcontext_resource_manager == nullptr || pcontext_resource_manager->type() != CGDK::context::value_type_t::ptr_, CGDK::throwable::failure(eRESULT::NOT_READY, 0, "'resource_manager' not exist"sv), LOG_ERROR << "     (!) resource manager not exist ");

	auto pmap_updatable_resource = &pcontext_resource_manager->ref_value<std::map<CGDK::update::UPDATE_RESOURCE_ID, CGDK::object_ptr<CGDK::resource::Iupdatable>>>();

	// check)
	THROW_IF(pmap_updatable_resource == nullptr, CGDK::throwable::failure(eRESULT::NOT_READY, 0, "'resource_manager' not exist"sv), LOG_ERROR << "     (!) resource manager not exist ");

	// 3) find resource
	int64_t resource_id = 0;
	std::from_chars(str_find.data(), str_find.data() + str_find.size(), resource_id);
	auto iter_find = pmap_updatable_resource->find(static_cast<CGDK::update::UPDATE_RESOURCE_ID>(resource_id));

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

					// - initialize
					pupdatable_resource->initialize(iter.second);
				}
				break;

		case	CGDK::resource::eTYPE::FOLDER:
				{
					// - 객체를 생성한다.
					pupdatable_resource = make_object<CGDK::resource::updatable::folder>();

					// - initialize
					pupdatable_resource->initialize(iter.second);
				}
				break;

		case	CGDK::resource::eTYPE::REFERENCE:
				{
					// - get ..
					// - the resource manager stores its resource map as a pointer value
					auto pcontext_resource_manager = _context.try_find("resource_manager"sv);

					// check)
					THROW_IF(pcontext_resource_manager == nullptr || pcontext_resource_manager->type() != CGDK::context::value_type_t::ptr_, CGDK::throwable::failure(eRESULT::NOT_READY, 0, "'resource_manager' not exist"sv), LOG_ERROR << "     (!) resource manager not exist ");

					auto pmap_updatable_resource = &pcontext_resource_manager->ref_value<std::map<CGDK::update::UPDATE_RESOURCE_ID, CGDK::object_ptr<CGDK::resource::Iupdatable>>>();

					// check)
					THROW_IF(pmap_updatable_resource == nullptr, CGDK::throwable::failure(eRESULT::NOT_READY, 0, "'resource_manager' not exist"sv), LOG_ERROR << "     (!) 'reference' index not exist");

					// - get refkey
					std::string str_find = iter.second.find("ref"sv, "reference"sv);

					// check)
					THROW_IF(str_find.empty(), CGDK::throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'ref' index not exist"sv), LOG_ERROR << "     (!) 'reference' index not exist");

					// - find resource
					int64_t resource_id = 0;
					std::from_chars(str_find.data(), str_find.data() + str_find.size(), resource_id);
					auto iter_find = pmap_updatable_resource->find(static_cast<CGDK::update::UPDATE_RESOURCE_ID>(resource_id));

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
		if (_context.type() == context::value_type_t::buffer_)
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
		SCOPED_LOCK(this->m_lockable_info)
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
		LOG_DEBUG << "(excp) exception occurred on '_process_starting' ['"sv << pname << "']"sv;

		// reraise)
		throw;
	}
	catch (...)
	{
		// - get name
		auto* pnameable = dynamic_cast<Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string_view();

		// log) 
		LOG_DEBUG << "(excp) exception occurred on '_process_starting' ['"sv << pname << "']"sv;

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
	SCOPED_LOCK(this->m_lockable_info)
	{
		this->m_meta_info = sUPDATABLE_FILE_META_INFO();
		this->m_meta_completion = sUPDATABLE_FILE_COMPLETION_INFO();
		vector_updatable_process = std::move(this->m_vector_updatable_process);
	}
}
