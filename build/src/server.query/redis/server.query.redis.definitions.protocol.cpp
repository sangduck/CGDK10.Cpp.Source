#include "pch.h"

using namespace CGDK;


bool sMESSAGE_REDIS_RESPONSE::is_success() const noexcept
{
	return pquery.exist() && pquery->m_result.type != eRESP_DATA_TYPE::ERROR_;
}
