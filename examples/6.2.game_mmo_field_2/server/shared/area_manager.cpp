#include "pch.h"


area::manager ::manager()
{
}

area::manager::~manager() noexcept
{
}

void area::manager::register_area(own_ptr<area::group>&& _pgroup_area)
{
	// check)
	CGASSERT_ERROR(_pgroup_area != nullptr);

	// check)
	THROW_IF(_pgroup_area == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "_pgroup_area is nullptr"sv));

	// check) RUNNING 상태가 아니면 등록을 하지 못한다.
	CGASSERT_ERROR(this->get_object_state() == eOBJECT_STATE::START_PENDING || this->get_object_state() >= eOBJECT_STATE::RUNNING)

	// check) 등록 가능한 상태가 아니라면 exception을 던진다.
	THROW_IF(this->get_object_state() != eOBJECT_STATE::START_PENDING && this->get_object_state() < eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY, 0, "cann't reguster _pgroup_area"sv));

	// 1) get area_index
	auto area_index = _pgroup_area->m_area_info.id;

	// check)
	CGASSERT_ERROR(area_index != 0);

	// check)
	THROW_IF(area_index == 0, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "area_index is zero"sv));

	// declare)
	object_ptr<area::group> pgroup_area = _pgroup_area;

	SCOPED_LOCK(this->m_cs_map_area)
	{
		// 2) get exist
		auto iter_find = this->m_map_area.find(area_index);

		// check)
		CGASSERT_ERROR(iter_find == this->m_map_area.end());

		// check)
		THROW_IF(iter_find != this->m_map_area.end(), throwable::failure(eRESULT::ALREADY_EXIST, 0, "area_index aleady exist"sv));

		// 3) insert
		this->m_map_area.insert({area_index, std::move(_pgroup_area)});
	}

	// log)
	LOG_INFO << "> area["sv << area_index << "] registered"sv;

	// notify)
	transmit_message_to(g_pservice, sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_AREA_GROUP_ADD, (uint64_t)reinterpret_cast<Igroup_user*>(pgroup_area.get())));
}

void area::manager::unregister_area(AREA_INDEX _area_index) noexcept
{
	// check)
	CGASSERT_ERROR(_area_index != 0);

	// check)
	RETURN_IF(_area_index == 0);

	// declare)
	own_ptr<area::group> pgroup_area;

	SCOPED_LOCK(this->m_cs_map_area)
	{
		// 1) is exist
		auto iter_find = this->m_map_area.find(_area_index);

		// check)
		RETURN_IF(iter_find == this->m_map_area.end());

		// 2) set..
		pgroup_area = std::move(iter_find->second);

		// 3) erase
		this->m_map_area.erase(iter_find);
	}

	// log)
	LOG_INFO << "< area["sv << _area_index << "] unregistered"sv;

	// notify)
	transmit_message_to(g_pservice, sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_AREA_GROUP_REMOVE, (uint64_t)reinterpret_cast<Igroup_user*>(pgroup_area.get())));

	// 4) destroy
	pgroup_area.reset();
}

object_ptr<area::group> area::manager::find_area(AREA_INDEX _area_index) const noexcept
{
	// check)
	CGASSERT_ERROR(_area_index != 0);

	// declare) 
	object_ptr<area::group> pgroup_area;

	SCOPED_LOCK(this->m_cs_map_area)
	{
		// 1) is exist
		auto iter_find = this->m_map_area.find(_area_index);

		// check)
		if (iter_find != this->m_map_area.end())
		{
			pgroup_area = iter_find->second;
		}
	}

	// return) 
	return pgroup_area;
}

void area::manager::on_starting(context& /*_context*/)
{
}

void area::manager::on_start(context& /*_context*/)
{
	// 1) 임시로 arae를 생성해서 추가한다.
	{
		// - create group
		auto parea = make_own<group>();

		// - set area info
		parea->m_area_info.id = 1;
		parea->name("test_field_1"sv);

		// - start area
		parea->start();

		// -register
		this->register_area(std::move(parea));
	}

	{
		// - create group
		auto parea = make_own<group>();

		// - set area info
		parea->m_area_info.id = 2;
		parea->name("test_field_2"sv);

		// - start area
		parea->start();

		// -register
		this->register_area(std::move(parea));
	}

	{
		// - create group
		auto parea = make_own<group>();

		// - set area info
		parea->m_area_info.id = 3;
		parea->name("test_field_3"sv);

		// - start area
		parea->start();

		// -register
		this->register_area(std::move(parea));
	}

	{
		// - create group
		auto parea = make_own<group>();

		// - set area info
		parea->m_area_info.id = 4;
		parea->name("test_field_4"sv);

		// - start area
		parea->start();

		// -register
		this->register_area(std::move(parea));
	}
}

void area::manager::on_stopping()
{
	// declar) 
	std::vector<AREA_INDEX> vector_area_index;

	// 1) get area list
	SCOPED_LOCK(m_cs_map_area)
	{
		for (auto& iter : m_map_area)
		{
			vector_area_index.push_back(iter.first);
		}
	}

	// 2) destroy it!
	for (auto& iter : vector_area_index)
	{
		this->unregister_area(iter);
	}
}

void area::manager::on_stop()
{

}

void area::manager::on_execute()
{
}

