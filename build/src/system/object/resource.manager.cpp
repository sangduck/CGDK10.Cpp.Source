#include "pch.h"

namespace CGDK
{

char* get_filename_only(char* _dest, const char* _source)
{
	const int	tempLength	 = static_cast<int>(strlen(_source))-1;
	int			tempPP		 = tempLength;
	int			tempStart	 = 0;
	int			tempEnd		 = tempPP-1;
	while(tempPP)
	{
		if(_source[tempPP] == '.')
		{
			tempEnd	 = tempPP;
		}

		if(_source[tempPP] == '\\' || _source[tempPP] == '/')
		{
			tempStart	 = tempPP+1;
			break;
		}

		tempPP--;
	}
	memcpy((void*)_dest, (void*)&_source[tempStart], (static_cast<size_t>(tempEnd)-tempStart+1)*sizeof(char));
	_dest[static_cast<uint64_t>(tempEnd)-tempStart]	 = 0;

	// return) 
	return	_dest;
}

wchar_t* get_filename_only(wchar_t* _dest, const wchar_t* _source)
{
	const int	tempLength	 = static_cast<int>(wcslen(_source))-1;
	int			tempPP		 = tempLength;
	int			tempStart	 = 0;
	int			tempEnd		 = tempPP-1;
	while(tempPP)
	{
		if(_source[tempPP] == L'.')
		{
			tempEnd	 = tempPP;
		}

		if(_source[tempPP] == L'\\' || _source[tempPP] == L'/')
		{
			tempStart	 = tempPP+1;
			break;
		}

		tempPP--;
	}
	memcpy((void*)_dest, (void*)&_source[tempStart], (static_cast<size_t>(tempEnd)-tempStart+1)*sizeof(wchar_t));
	_dest[tempEnd-tempStart]	 = 0;

	// return) 
	return	_dest;
}

char16_t* get_filename_only(char16_t* _dest, const char16_t* _source)
{
	const int	tempLength	 = static_cast<int>(__strlen(_source)) - 1;
	int			tempPP		 = tempLength;
	int			tempStart	 = 0;
	int			tempEnd		 = tempPP - 1;
	while (tempPP)
	{
		if (_source[tempPP] == u'.')
		{
			tempEnd = tempPP;
		}

		if (_source[tempPP] == u'\\' || _source[tempPP] == u'/')
		{
			tempStart = tempPP + 1;
			break;
		}

		tempPP--;
	}
	memcpy((void*)_dest, (void*)&_source[tempStart], (static_cast<size_t>(tempEnd) - tempStart + 1) * sizeof(char16_t));
	_dest[tempEnd - tempStart] = 0;

	// return) 
	return	_dest;
}

char32_t* get_filename_only(char32_t* _dest, const char32_t* _source)
{
	const int	tempLength	 = static_cast<int>(__strlen(_source)) - 1;
	int			tempPP		 = tempLength;
	int			tempStart	 = 0;
	int			tempEnd		 = tempPP - 1;
	while (tempPP)
	{
		if (_source[tempPP] == U'.')
		{
			tempEnd = tempPP;
		}

		if (_source[tempPP] == U'\\' || _source[tempPP] == U'/')
		{
			tempStart = tempPP + 1;
			break;
		}

		tempPP--;
	}
	memcpy((void*)_dest, (void*)&_source[tempStart], (static_cast<size_t>(tempEnd) - static_cast<size_t>(tempStart) + 1) * sizeof(char32_t));
	_dest[tempEnd - tempStart] = 0;

	// return) 
	return	_dest;
}

resource::manager::manager() noexcept :
	object::Nnameable("resource_manager"),
#if defined(__ANDROID__)
	m_activity(nullptr),
#endif
	m_vector_path(),
	m_map_resources()

{
}

resource::manager::~manager() noexcept
{
}

result_code resource::manager::destroy() noexcept
{
	return result_code(eRESULT::SUCCESS);
}

void resource::manager::initialize_instance()
{
	scoped_lock(m_s_lockable_create)
	{
		// check)
		RETURN_IF(m_s_pinstance.exist());
		
		// 1) create manager
		auto pmanager = _make_own_raw<resource::manager>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL); 
		
		// 2) set manager
		m_s_pinstance = std::move(pmanager);
	}
}

void resource::manager::set_instance(own_ptr<manager>&& _pinstance)
{
	// declare)
	own_ptr<manager> pinstance_pre;

	scoped_lock(m_s_lockable_create)
	{
		// 1) move old instance
		pinstance_pre = std::move(m_s_pinstance);

		// 2) set new instance
		m_s_pinstance = std::move(_pinstance);
	}

	// 3) reset old instance
	pinstance_pre.reset();
}

void resource::manager::reset_instance() noexcept
{
	// declare)
	own_ptr<manager> pinstance;

	scoped_lock(m_s_lockable_create)
	{
		// 1) move instance
		pinstance = std::move(m_s_pinstance);

		// check) 
		RETURN_IF(pinstance.empty());
	}

	// 2) reset old instance
	pinstance.reset();
}

bool resource::manager::register_resource_path(const std::string& _path)
{
	// check) _path이 nullptr이면 false를 리턴한다.
	RETURN_IF(_path.empty(), false);

	scoped_lock(m_vector_path)
	{
		// 1) 해당하는 Resource를 찾는다.
		const auto iter_find = std::find(m_vector_path.begin(), m_vector_path.end(), _path);

		// check) 이미 동일한 Path가 등록되어 있으면 여기서 끝낸다.
		RETURN_IF(iter_find!=m_vector_path.end(), false);

		// 2) 해당 Resource를 추가한다.
		m_vector_path.push_back(_path.data());
	}

	// trace) 
	LOG_INFO << "(info) register resource path [file:"sv << _path << "]"sv;

	// return) 
	return true;
}

bool resource::manager::unregister_resource_path(const std::string& _path) noexcept
{
	scoped_lock(m_vector_path)
	{
		// 1) 해당하는 Resource를 찾는다.
		auto iter_find = std::find(m_vector_path.begin(), m_vector_path.end(), _path);

		// check) 찾지 못했을 경우 false를 리턴한다.
		RETURN_IF(iter_find==m_vector_path.end(), false);

		// 2) 해당 Resource를 지운다.
		m_vector_path.erase(iter_find);
	}

	// trace) 
	LOG_INFO << "(info) unregister resource path [file:"sv << _path << "]"sv;

	// return) 
	return true;
}

void resource::manager::register_resource(Iresource* _presource)
{
	// check) _presource이 nullptr이면 Exception을 던진다.
	THROW_IF(_presource == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) 먼저 이름을 얻는다.
	auto str_name = _presource->name();

	// check) name이 nullptr이면 Exception을 던진다.
	THROW_IF(str_name.empty(), throwable::failure(eRESULT::NO_INITIALIZED));

	scoped_lock(m_map_resources)
	{
		// 2) 해당하는 Resource를 찾는다.
		auto iter_find = m_map_resources.find(str_name.data());

		// check) 이미 동일한 것이 존재하면 Exception을 던진다.
		THROW_IF(iter_find != m_map_resources.end(), throwable::failure(eRESULT::ALEADY_EXIST));

		// 3) 해당 Resource를 추가한다.
		m_map_resources.emplace(str_name.data(), _presource);
	}
}
bool resource::manager::unregister_resource(Iresource* _presource) noexcept
{
	scoped_lock(m_map_resources)
	{
		// 1) 해당하는 Resource를 찾는다.
		auto iter_find = m_map_resources.find(_presource->name().data());

		// check) 찾지 못했을 경우 false를 리턴한다.
		RETURN_IF(iter_find == m_map_resources.end(), false);
		
		// 2) 해당 Resource를 지운다.
		m_map_resources.erase(iter_find);
	}

	// return) 
	return true;
}

// 이름으로 찾아 제거한다.   [12/27/2016 BSH]
bool resource::manager::unregister_resource(const std::string& _name) noexcept
{
	scoped_lock(m_map_resources)
	{
		// 1) 해당하는 Resource를 찾는다.
		auto iter_find = m_map_resources.find(_name);

		// check) 찾지 못했을 경우 false를 리턴한다.
		RETURN_IF(iter_find == m_map_resources.end(), false);

		// 2) 해당 Resource를 지운다.
		m_map_resources.erase(iter_find);
	}

	// return) 
	return true;
}

object_ptr<Iresource> resource::manager::find(const std::string& _name) noexcept
{
	// declare) 
	object_ptr<Iresource> pResource;

	scoped_lock(m_map_resources)
	{
		// 1) find 한다.
		auto iter_find = m_map_resources.find(_name);

		// check) 찾았다면 저장
		if(iter_find != m_map_resources.end())
		{
			pResource = iter_find->second;
		}
	}

	// return) 
	return	pResource;
}


}