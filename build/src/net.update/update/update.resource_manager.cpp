#include "pch.h"
#include <charconv>

using namespace CGDK;

object_ptr<resource::Iupdatable> update::resource_manager::get_resource(UPDATE_RESOURCE_ID _update_resource_id) const
{
	// declare) 
	object_ptr<resource::Iupdatable> pupdate_resource;

	SCOPED_LOCK(this->m_lockable_map_resource)
	{
		// 1) find resource 
		auto iter_find = this->m_map_resource.find(_update_resource_id);

		// check)
		RETURN_IF(iter_find == m_map_resource.end(), pupdate_resource);

		// 2) ...
		pupdate_resource = iter_find->second;
	}

	// return)
	return pupdate_resource;
}


resource::eTYPE _get_updatable_resource_type(context& _context);

result_code update::resource_manager::_process_initializing(context& _context)
{
	// log) 
	LOG_PROGRESS << "@ <BEGIN> initialize 'UPDATE RESOURCE MANAGER'['"sv << name() << "']"sv;

	// declare) 
	std::string root_path;
	std::map<UPDATE_RESOURCE_ID, object_ptr<resource::Iupdatable>> map_updatable_resource;

	// 1) root path
	if (auto context_root_path = _context.find("root_path"sv); context_root_path.is_valid())
	{
		root_path = (std::string&&)context_root_path;
	}

	// 2) update resource list
	if (auto context_system_executor = _context.find("resource_list"sv); context_system_executor.is_valid())
	{
		// 설명) resource들을 생성한다.
		//       - updatable resource의 설정에 따라 생성을 한다.
		//       - 문제는 reference 속성을 가진 것도 존재하기 때문에 3번의 pass를 거처 생성을 한다.
		// 
		//         pass1, 객체의 종류를 확인해 객체만 생성한다.
		//         pass2, 모든 객체를 initialize한다. 
		//
		//      - 

		// definitions) 
		struct _TEMP_INFO
		{
			UPDATE_RESOURCE_ID	id;
			resource::eTYPE type;
			context* pcontext;
			object_ptr<resource::Iupdatable> pupdatable_resource;
		};

		// declare)
		std::vector<_TEMP_INFO> vector_updatable_resource_info;

		// - pass I - create first
		for (auto& iter : context_system_executor)
		{
			// declare)
			int64_t value_id = 0;
			std::from_chars(iter.first.data(), iter.first.data() + iter.first.size(), value_id);
			auto temp_id = static_cast<UPDATE_RESOURCE_ID>(value_id);

			// - 이미 존재하는지 확인한다.
			auto iter_find = std::find_if(vector_updatable_resource_info.begin(), vector_updatable_resource_info.end(), [=](const _TEMP_INFO& _i) { return _i.id == temp_id; });

			// check) 이미 존재한다.
			THROW_IF(iter_find != vector_updatable_resource_info.end(), throwable::failure(eRESULT::ALREADY_EXIST, 0, "resource id already exist"sv));

			// declare)
			_TEMP_INFO temp_info;
			temp_info.id = temp_id;
			temp_info.type = _get_updatable_resource_type(iter.second);
			temp_info.pcontext = &iter.second;

			// - create 
			switch (temp_info.type)
			{
			case	resource::eTYPE::FILE:
					temp_info.pupdatable_resource = make_object<resource::updatable::file>();
					break;

			case	resource::eTYPE::FOLDER:
					temp_info.pupdatable_resource = make_object<resource::updatable::folder>();
					break;

			case	resource::eTYPE::REFERENCE:
					temp_info.pupdatable_resource = make_object<resource::updatable::folder>(); // reference == folder
					break;

			default:
					throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0,"fail to create"sv);
			}

			// check) 
			CONTINUE_IF(temp_info.pupdatable_resource.empty());

			// - insert
			auto result_insert = map_updatable_resource.insert({ temp_info.id, temp_info.pupdatable_resource });

			// check)
			CGASSERT_ERROR(result_insert.second == true);

			// check) 
			THROW_IF(result_insert.second == false, throwable::failure(eRESULT::ALREADY_EXIST, 0, "duplicated 'updatable resource id'"sv));

			// - 추가
			vector_updatable_resource_info.push_back(temp_info);
		}

		// - pass 2 - reference initialize
		for (auto& iter : vector_updatable_resource_info)
		{
			// log) 
			LOG_PROGRESS << "   [id " << iter.id << "] ..."sv;

			// - set resource manager
			(*iter.pcontext)["resource_manager"sv] = &map_updatable_resource;

			// - initialize
			iter.pupdatable_resource->initialize(*iter.pcontext);
		}
	}

	// 2) setting
	SCOPED_LOCK(this->m_lockable_map_resource)
	{
		this->m_map_resource = std::move(map_updatable_resource);
	}

	// 3) on_initialize
	this->on_initializing(_context);
	
	// return)
	return eRESULT::SUCCESS;
}

result_code update::resource_manager::_process_initialize(context& _context)
{
	// log) 
	LOG_PROGRESS << "@ <END> initialize 'UPDATE RESOURCE MANAGER'['"sv << name() << "']"sv;

	// 3) on_initialize
	this->on_initialize(_context);

	// return)
	return eRESULT::SUCCESS;
}

result_code update::resource_manager::_process_destroying() noexcept
{
	// log) 
	LOG_PROGRESS << "@ <BEGIN> destroying 'UPDATE RESOURCE MANAGER'['"sv << name() << "']"sv;

	try 
	{
		this->on_destroying();
	}
	catch (...) 
	{
		CGASSERT_ERROR(false);
	}
	
	// return)
	return eRESULT::SUCCESS; 
}

result_code update::resource_manager::_process_destroy() noexcept
{
	try
	{
		// declare) 
		std::map<UPDATE_RESOURCE_ID, object_ptr<resource::Iupdatable>> map_resource;

		// 1) 
		SCOPED_LOCK(m_lockable_map_resource)
		{
			map_resource = std::move(this->m_map_resource);
		}

		// 2) ...


		// 3) on_destroy
		this->on_destroy();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}
	
	// log) 
	LOG_PROGRESS << "@ <END> destroying 'UPDATE RESOURCE MANAGER'['"sv << name() << "']"sv;

	// return)
	return eRESULT::SUCCESS;
}
