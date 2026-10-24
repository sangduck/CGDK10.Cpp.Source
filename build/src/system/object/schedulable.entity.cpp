#include "pch.h"

namespace CGDK
{


schedulable::entity::entity() noexcept
{
	this->set_object_type(eOBJECT_TYPE::SCHEDULABLE_ENTITY);
}

schedulable::entity::~entity() noexcept
{
}

result_code schedulable::entity::start()
{
	// 1) set parameters
	context context_setting;
	//context_setting["tick"] = _tick.count();

	// 2) start
	return this->object::Nstartable::start(context_setting);
}

result_code schedulable::entity::_process_initialize(context& _context)
{
	// 1) parameter - tick
	get_parameter_execute_interval(this, _context);

	// 2) ...
	return this->schedulable::Ninitializable_startable::_process_initialize(_context);
}

result_code schedulable::entity::_process_start(context& _context)
{
	// 1) parameter - tick
	get_parameter_execute_interval(this, _context);

	// 2) ...
	return this->schedulable::Ninitializable_startable::_process_start(_context);
}


}