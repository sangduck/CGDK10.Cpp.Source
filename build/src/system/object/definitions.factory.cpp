#include "pch.h"

namespace CGDK
{

std::string_view to_string(eFACTORY_TYPE _type)
{
	std::string_view result;

	switch(_type)
	{
	case	eFACTORY_TYPE::POOL_MEMORYBLOCK:	result = "POOL.MEMORY"; break;
	case	eFACTORY_TYPE::POOL_SYSTEM:			result = "POOL.SYSTEM"; break;
	case	eFACTORY_TYPE::POOL_USER:			result = "POOL.USER"; break;
	case	eFACTORY_TYPE::ALLOCATOR_SYSTEM:	result = "SYSTEM"; break;
	case	eFACTORY_TYPE::ALLOCATOR_USER:		result = "USER"; break;
	default:									result = ""; break;
	};

	return result;
}

std::string_view to_string(eFACTORY_GC_TYPE _type)
{
	std::string_view result;

	switch (_type)
	{
	case	eFACTORY_GC_TYPE::NO_POOLING:		result = "NO_POOLING"; break;
	case	eFACTORY_GC_TYPE::POOLING:			result = "POOLING"; break;
	case	eFACTORY_GC_TYPE::TLS_POOLING:		result = "TLS_POOLING"; break;
	case	eFACTORY_GC_TYPE::POOLING_NO_GC:	result = "POOLING_NO_GC"; break;
	case	eFACTORY_GC_TYPE::TLS_POOLING_NO_GC:result = "TLS_POOLING_NO_GC"; break;
	default:									result = ""; break;
	};

	return result;
}

statistics::unit_factory::unit_factory() noexcept
{
}

statistics::unit_factory::unit_factory(const unit_factory& _rhs) noexcept :
	tick_snap(_rhs.tick_snap),
	existing_limits(_rhs.existing_limits),
	tls_stack_depth_threshold(_rhs.tls_stack_depth_threshold),
#if defined(_USE_TLS_FACTORY_STATISTICS)
	alloc_stack(_rhs.alloc_stack),
	alloc_stack_tls(_rhs.alloc_stack_tls),
	alloc_create(_rhs.alloc_create),
	free_stack(_rhs.free_stack),
	free_stack_tls(_rhs.free_stack_tls),
	free_delete(_rhs.free_delete),
	existing(_rhs.existing),
	tls_stack_depth(_rhs.tls_stack_depth),
	in_using(_rhs.in_using),
	execute_count(_rhs.execute_count),
	execute_accumated(_rhs.execute_accumated)
#else
	alloc_stack(_rhs.alloc_stack.load()),
	alloc_stack_tls(_rhs.alloc_stack_tls.load()),
	alloc_create(_rhs.alloc_create.load()),
	free_stack(_rhs.free_stack.load()),
	free_delete(_rhs.free_delete.load()),
	free_stack_tls(_rhs.free_stack_tls.load()),
	existing(_rhs.existing.load()),
	tls_stack_depth(_rhs.tls_stack_depth.load()),
	in_using(_rhs.in_using.load()),
	execute_count(_rhs.execute_count.load()),
	execute_accumated(_rhs.execute_accumated.load())
#endif
{
}

statistics::unit_factory& statistics::unit_factory::operator=(const statistics::unit_factory& _rhs) noexcept
{
	tick_snap = _rhs.tick_snap;
	existing_limits = _rhs.existing_limits;
	tls_stack_depth_threshold = _rhs.tls_stack_depth_threshold;
#if defined(_USE_TLS_FACTORY_STATISTICS)
	alloc_stack = _rhs.alloc_stack;
	alloc_stack_tls = _rhs.alloc_stack_tls;
	alloc_create = _rhs.alloc_create;
	free_stack = _rhs.free_stack;
	free_stack_tls = _rhs.free_stack_tls;
	free_delete = _rhs.free_delete;
	existing = _rhs.existing;
	tls_stack_depth = _rhs.tls_stack_depth;
	in_using = _rhs.in_using;
	execute_accumated = _rhs.execute_accumated;
	execute_count = _rhs.execute_count;

#else
	alloc_stack = _rhs.alloc_stack.load();
	alloc_stack_tls = _rhs.alloc_stack_tls.load();
	alloc_create = _rhs.alloc_create.load();
	free_stack = _rhs.free_stack.load();
	free_stack_tls = _rhs.free_stack_tls.load();
	free_delete = _rhs.free_delete.load();
	existing = _rhs.existing.load();
	tls_stack_depth = _rhs.tls_stack_depth.load();
	in_using = _rhs.in_using.load();
	execute_accumated = _rhs.execute_accumated.load();
	execute_count = _rhs.execute_count.load();
#endif
	return *this;
}

void statistics::unit_factory::reset() noexcept
{
	tick_snap = chrono::tick::time_point::min();
	existing_limits = INT64_MAX;
	tls_stack_depth_threshold = 0;
	alloc_stack = 0;
	alloc_stack_tls = 0;
	alloc_create = 0;
	free_stack = 0;
	free_stack_tls = 0;
	free_delete = 0;
	existing = 0;
	tls_stack_depth = 0;
	in_using = 0;
	execute_accumated = chrono::tick::duration::zero();
	execute_count = 0;
}

void statistics::unit_factory_snap::reset() noexcept
{
	tick_snap = chrono::tick::time_point::min();
	existing_limits = INT64_MAX;
	tls_stack_depth_threshold = 0;
	alloc_stack = 0;
	alloc_stack_tls = 0;
	alloc_create = 0;
	free_stack = 0;
	free_stack_tls = 0;
	free_delete = 0;
	existing = 0;
	tls_stack_depth = 0;
	in_using = 0;
	execute_accumated = chrono::tick::duration::zero();
	execute_count = 0;
}

statistics::unit_factory_snap& statistics::unit_factory_snap::operator=(const statistics::unit_factory& _rhs) noexcept
{
	tick_snap = _rhs.tick_snap;
	existing_limits = _rhs.existing_limits;
	tls_stack_depth_threshold = _rhs.tls_stack_depth_threshold;
#if defined(_USE_TLS_FACTORY_STATISTICS)
	alloc_stack = _rhs.alloc_stack;
	alloc_stack_tls = _rhs.alloc_stack_tls;
	alloc_create = _rhs.alloc_create;
	free_stack = _rhs.free_stack;
	free_stack_tls = _rhs.free_stack_tls;
	free_delete = _rhs.free_delete;
	existing = _rhs.existing;
	tls_stack_depth = _rhs.tls_stack_depth;
	in_using = _rhs.in_using;
	execute_accumated = _rhs.execute_accumated;
	execute_count = _rhs.execute_count;
#else
	alloc_stack = _rhs.alloc_stack.load();
	alloc_stack_tls = _rhs.alloc_stack_tls.load();
	alloc_create = _rhs.alloc_create.load();
	free_stack = _rhs.free_stack.load();
	free_stack_tls = _rhs.free_stack_tls.load();
	free_delete = _rhs.free_delete.load();
	existing = _rhs.existing.load();
	tls_stack_depth = _rhs.tls_stack_depth.load();
	in_using = _rhs.in_using.load();
	execute_accumated = _rhs.execute_accumated.load();
	execute_count = _rhs.execute_count.load();
#endif

	return *this;
}

bool factory::sSTATISTICS_TLS::update_statistics(statistics::unit_factory& _dest, eFACTORY_GC_TYPE _gc_type) noexcept
{
	// check) is unused factory?
	//  (shrink() records free_stack/free_delete only - the thread calling shrink/sustain may not alloc/free at all)
	if(now.alloc == 0 && now.free == 0 && now.free_stack == 0 && now.free_delete == 0) 
		return false;

	// 1) process statistics (must COPY to local variable first)
	statistics::unit_factory_tls temp_now = now;

	// 2) update alloc_stack/alloc_create
	//    (int32 counters wrap around - get the difference with statistics::counter_diff())
	auto diff_alloc_stack = statistics::counter_diff(temp_now.alloc_stack, old.alloc_stack);
	_dest.alloc_stack += diff_alloc_stack;
	auto diff_alloc_create = statistics::counter_diff(temp_now.alloc_create, old.alloc_create);
	_dest.alloc_create += diff_alloc_create;

	// 3) update alloc_stack_tls
	auto diff_alloc = statistics::counter_diff(temp_now.alloc, old.alloc);
	auto diff_alloc_stack_tls = diff_alloc - diff_alloc_stack - diff_alloc_create;
	_dest.alloc_stack_tls += diff_alloc_stack_tls;

	// 4) update free_stack/free_delete
	auto diff_free_stack = statistics::counter_diff(temp_now.free_stack, old.free_stack);
	_dest.free_stack += diff_free_stack;
	auto diff_free_delete = statistics::counter_diff(temp_now.free_delete, old.free_delete);
	_dest.free_delete += diff_free_delete;

	// 5) update alloc_stack_tls
	auto diff_free = statistics::counter_diff(temp_now.free, old.free);
	auto diff_free_stack_tls = diff_free - diff_free_stack - diff_free_delete;
	_dest.free_stack_tls += diff_free_stack_tls;

	// 6) existing and in_using
	auto diff_existing = diff_alloc_create - diff_free_delete;
	_dest.existing += diff_existing;

	// 7) tls_sack existing
	_dest.tls_stack_depth += temp_now.tls_stack_depth;

	int64_t diff_in_using = diff_alloc - diff_free;
	_dest.in_using += diff_in_using;

	// 8) calculate 'tls_stack_depth_threshold'
	if(_gc_type == eFACTORY_GC_TYPE::TLS_POOLING)
	{
		// 설명) 'tls_stack_depth_threshold ' algorithm
		// - 'now.tls_stack_depth_threshold' is increased 1 at alloc() call if not allocated at tls
		//   따라서 now.tls_stack_depth_threshold는 tls에서 미할당횟수만큼 늘어나게 된다.
		// - decrease 12.5% on every udpate(1second)
		// - alloc() is MIN_TLS_STACK(min value) in thread call more than once
		// - tls_stack_depth_threshold value is saturate to 0 in thread alloc() being not executed

		// definitions)
		#define MIN_TLS_STACK	0

		auto tls_stack_depth_threshold = now.tls_stack_depth_threshold;

		// - diminish 12.5% (7/8) of tls_stack_depth_threshold
		tls_stack_depth_threshold = static_cast<int32_t>((static_cast<int64_t>(tls_stack_depth_threshold) * 31) >> 5);

	#if MIN_TLS_STACK > 0
		// - saturate tls_stack_depth_threshold if allocation exist
		if (diff_alloc != 0 && tls_stack_depth_threshold < MIN_TLS_STACK)
		{
			tls_stack_depth_threshold = MIN_TLS_STACK;
		}
	#endif

		// - apply 'tls_stack_depth_threshold'
		now.tls_stack_depth_threshold = tls_stack_depth_threshold;
		_dest.tls_stack_depth_threshold += tls_stack_depth_threshold;
	}

	// 9) store old
	old = temp_now;

	// return)
	return (diff_alloc != 0);
}

#if defined(_WIN32)
std::string factory::_get_name_from_rtti(std::string_view _rtti_name) noexcept
{
	return std::string(_rtti_name);
}

#elif defined(__linux__)
#include <cxxabi.h>
std::string factory::_get_name_from_rtti(std::string_view _rtti_name) noexcept
{
	// declare) 
	int status = -4;

	// 1) get demangled name
	auto res = abi::__cxa_demangle(_rtti_name.data(), nullptr, nullptr, &status);

	// 2) set string
	std::string result((status == 0) ? res : _rtti_name.data());

	// 3) free string
	::free(res);

	// return) 
	return result;
}
#else
	#error	"[CGDK10] No supported platform"
#endif

}