#include "pch.h"

namespace CGDK
{

void Ilog_source::store_log(std::vector<object_ptr<LOG_RECORD>>&& _vector_plog_record)
{
	this->process_store_log(std::move(_vector_plog_record));
}


}