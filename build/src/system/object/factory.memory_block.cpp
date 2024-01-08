#include "pch.h"

namespace CGDK
{

extern bool g_is_initialize;

factory::memory_block::memory::memory(size_t _size)
{
	// 1) 메모리를 생성합니다.(Aligned된 memory_block로 할당받는다.)
	data_ = reinterpret_cast<char*>(::malloc(_size));

	// check) 할당에 실패했으면 던진다.
	//THROW_IF(data_ == nullptr, throwable::failure(eRESULT::BAD_ALLOC));
	if(data_ == nullptr)
		throw throwable::failure(eRESULT::BAD_ALLOC);

	// 2) Len을 설정한다.
	size_ = static_cast<uint32_t>(_size);
}

factory::memory_block::memory::~memory() noexcept
{
	// 1) 할당해제한다.
	::free(data_);
}

class factory::memory_block::memoryblock_pool : public factory::memory_block::pool_t
{
public:
		#if defined(_USE_TLS_FACTORY_STATISTICS)
			memoryblock_pool(int _index, size_t _block_size, const std::string_view _name) noexcept : factory::memory_block::pool_t(_name, eFACTORY_TYPE::POOL_MEMORYBLOCK), m_block_size(_block_size), m_attributes(0)
			{
				this->m_local_data_index = _index;
			}
		#else
			memoryblock_pool(int, size_t _block_size, const std::string_view _name) noexcept : factory::memory_block::pool_t(_name, eFACTORY_TYPE::POOL_MEMORYBLOCK), m_block_size(_block_size), m_attributes(0)
			{
			}
		#endif

	virtual	~memoryblock_pool() noexcept {}

public:
	virtual	size_t		object_size() const noexcept override { return this->m_block_size;}
	virtual	factory::memory_block::memoryblock_t* create_object(CGNEW_DEBUG_INFO_PARAMETERS) override
			{
			#ifdef _ENABLE_DEBUG_NEW
				#if defined(_WIN32)
					return new(_NORMAL_BLOCK, CGNEW_DEBUG_INFO_PARAMETERS_PASSING) factory::memory_block::memoryblock_t(this->m_block_size);
				#else
					return new factory::memory_block::memoryblock_t(this->m_block_size);
				#endif
			#else
				return new factory::memory_block::memoryblock_t(this->m_block_size);
			#endif
			}

			uint32_t	get_attribute() const noexcept { return this->m_attributes;}
			void		set_attribute(uint32_t _attribute) noexcept { this->m_attributes=_attribute; }

private:
			size_t		m_block_size;
			uint32_t	m_attributes;
};

factory::memory_block::memory_block()
{
	// 1) initialize
	this->initialize();
}

factory::memory_block::~memory_block() noexcept
{
	// 1) destroy
	this->destroy();
}

result_code factory::memory_block::initialize(const INITIALIZE_PARAMETER& _parameter)
{
	// check)
	RETURN_IF(g_is_initialize == false, eRESULT::ALEADY_EXIST);

	// check)
	RETURN_IF(m_s_instance_pool.m_vector_memory_block_pool.empty() == false, eRESULT::ALEADY_DONE);

	// 1) create as type
	switch(_parameter.type)
	{
	// - create as 'tiny'
	case	1:
			m_s_instance_pool.process_create_as_scaled_tiny();
			break;

	// - create as 'normal'
	case	2:
			m_s_instance_pool.process_create_as_scaled_normal();
			break;

	// - create as 'large'
	case	3:
			m_s_instance_pool.process_create_as_scaled_large();
			break;

	// - create as 'huge'
	case	4:
			m_s_instance_pool.process_create_as_scaled_huge();
			break;
	};

	// 2) create 'general size block pool'
	m_s_instance_pool.process_create_general_size_pool();

	// return) 
	return result_code(eRESULT::SUCCESS);
}

result_code factory::memory_block::destroy() noexcept
{
	// 1) reset all pool
	this->m_vector_memory_block_pool.clear();

	// return)
	return result_code(eRESULT::SUCCESS);
}

void factory::memory_block::detach() noexcept
{
	// 1) reset all pools
	for (auto& iter : m_s_instance_pool.m_vector_memory_block_pool)
	{
		iter.detach();
	}

	// 2) clear pools
	m_s_instance_pool.m_vector_memory_block_pool.clear();
}

void factory::memory_block::process_create_as_scaled_tiny()
{
	// 1) under 1k byte.. (128 Byte 단위).
	/*00*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA  0,  1 * 128, "memory_block 128 Byte"));
	/*01*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA  1,  2 * 128, "memory_block 256 Byte"));
	/*02*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA  2,  3 * 128, "memory_block 384 Byte"));
	/*03*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA  3,  4 * 128, "memory_block 512 Byte"));
	/*04*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA  4,  5 * 128, "memory_block 640 Byte"));
	/*05*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA  5,  6 * 128, "memory_block 768 Byte"));
	/*06*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA  6,  7 * 128, "memory_block 896 Byte"));
	/*07*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA  7,  8*  128, "memory_block 1K Byte"));
																																			
	// 2) under 8K bytes (1K Byte 단위)							    																		
	/*08*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA  8, 2 * 1024, "memory_block 2K Byte"));
	/*09*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA  9, 3 * 1024, "memory_block 3K Byte"));
	/*10*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 10, 4 * 1024, "memory_block 4K Byte"));
	/*11*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 11, 5 * 1024, "memory_block 5K Byte"));
	/*12*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 12, 6 * 1024, "memory_block 6K Byte"));
	/*13*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 13, 7 * 1024, "memory_block 7K Byte"));
	/*14*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 14,  8* 1024, "memory_block 8K Byte"));

	// check) 
	CGASSERT_ERROR(this->m_vector_memory_block_pool.size() == 15);
}

void factory::memory_block::process_create_as_scaled_normal()
{
	// 1) call 'create scaled tiny pools'
	this->process_create_as_scaled_tiny();

	// 2) under 64K bytes... (4KByte 단위)
	/*15*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 15,  3 * 4096, "memory_block 12K Byte"));
	/*16*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 16,  4 * 4096, "memory_block 16K Byte"));
	/*17*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 17,  5 * 4096, "memory_block 20K Byte"));
	/*18*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 18,  6 * 4096, "memory_block 24K Byte"));
	/*19*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 19,  7 * 4096, "memory_block 28K Byte"));
	/*20*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 20,  8 * 4096, "memory_block 32K Byte"));
	/*21*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 21,  9 * 4096, "memory_block 36K Byte"));
	/*22*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 22, 10 * 4096, "memory_block 40K Byte"));
	/*23*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 23, 11 * 4096, "memory_block 44K Byte"));
	/*24*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 24, 12 * 4096, "memory_block 48K Byte"));
	/*25*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 25, 13 * 4096, "memory_block 52K Byte"));
	/*26*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 26, 14 * 4096, "memory_block 56K Byte"));
	/*27*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 27, 15 * 4096, "memory_block 60K Byte"));
	/*28*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 28, 16 * 4096, "memory_block 64K Byte"));

	// check) 
	CGASSERT_ERROR(this->m_vector_memory_block_pool.size() == 29);
}

void factory::memory_block::process_create_as_scaled_large()
{
	// 1) call 'create scaled normal pools'
	this->process_create_as_scaled_normal();

	// 2) under 256K bytes... (16K Byte 단위)
	/*29*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 29,  5 * 16384, "memory_block 80K Byte"));
	/*30*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 30,  6 * 16384, "memory_block 96K Byte"));
	/*31*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 31,  7 * 16384, "memory_block 112K Byte"));
	/*32*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 32,  8 * 16384, "memory_block 128K Byte"));
	/*33*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 33,  9 * 16384, "memory_block 144K Byte"));
	/*34*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 34, 10 * 16384, "memory_block 160K Byte"));
	/*35*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 35, 11 * 16384, "memory_block 176K Byte"));
	/*36*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 36, 12 * 16384, "memory_block 192K Byte"));
	/*37*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 37, 13 * 16384, "memory_block 208K Byte"));
	/*38*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 38, 14 * 16384, "memory_block 224K Byte"));
	/*39*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 39, 15 * 16384, "memory_block 240K Byte"));
	/*40*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 40, 16 * 16384, "memory_block 256K Byte"));

	// check) 
	CGASSERT_ERROR(m_vector_memory_block_pool.size() == 41);
}

void factory::memory_block::process_create_as_scaled_huge()
{
	// 1) call 'create scaled large pools'
	this->process_create_as_scaled_large();

	// 2) under 1M bytes.. (64K단위)
	/*41*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 41,  5*65536, "memory_block 320K Byte"));
	/*42*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 42,  6*65536, "memory_block 384K Byte"));
	/*43*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 43,  7*65536, "memory_block 448K Byte"));
	/*44*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 44,  8*65536, "memory_block 512K Byte"));
	/*45*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 45,  9*65536, "memory_block 576K Byte"));
	/*46*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 46, 10*65536, "memory_block 640K Byte"));
	/*47*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 47, 11*65536, "memory_block 704K Byte"));
	/*48*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 48, 12*65536, "memory_block 768K Byte"));
	/*49*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 49, 13*65536, "memory_block 832K Byte"));
	/*50*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 50, 14*65536, "memory_block 896K Byte"));
	/*51*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 51, 15*65536, "memory_block 896K Byte"));
	/*52*/this->m_vector_memory_block_pool.push_back(_make_own_raw <memoryblock_pool>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA 52, 16*65536, "memory_block 1M Byte"));

	// check) 
	CGASSERT_ERROR(this->m_vector_memory_block_pool.size() == 53);
}

void factory::memory_block::process_create_general_size_pool()
{
	// 1) create pool
	this->m_memory_block_extra_pool = _make_own_raw<pool_extra_t>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA "memory_block general");
	this->m_memory_block_extra_pool->set_gc_type(eFACTORY_GC_TYPE::NO_POOLING);
}

size_t factory::memory_block::prepare(int _index, size_t _amount)
{
	// check)
	CGASSERT_ERROR(_index >= 0 && _index < static_cast<int>(this->m_vector_memory_block_pool.size()));

	// check)
	RETURN_IF(_index < 0 || _index >= static_cast<int>(this->m_vector_memory_block_pool.size()), 0);

	// check)
	RETURN_IF(_amount < 1, 0);

	// 1) prepare한다.
	return this->m_vector_memory_block_pool.at(_index)->prepare(_amount);
}

size_t factory::memory_block::shrink(int _index, size_t _amount) noexcept
{
	// check)
	CGASSERT_ERROR(_index >= 0 && _index < static_cast<int>(this->m_vector_memory_block_pool.size()));

	// check)
	RETURN_IF(_index < 0 || _index >= static_cast<int>(this->m_vector_memory_block_pool.size()), 0);

	// check)
	RETURN_IF(_amount < 1, 0);

	// 1) shrink한다.
	return this->m_vector_memory_block_pool.at(_index)->shrink(_amount);
}

size_t factory::memory_block::object_size(int _index) const noexcept
{
	return this->m_vector_memory_block_pool.at(_index)->object_size();
}

size_t factory::memory_block::stacked_count(int _index) const noexcept
{
	return this->m_vector_memory_block_pool.at(_index)->stacked_count();
}

int64_t factory::memory_block::existing_limits(int _index) const noexcept
{
	return this->m_vector_memory_block_pool.at(_index)->existing_limits();
}

int64_t factory::memory_block::statistics_allocated_stack(int _index) const noexcept
{
	return this->m_vector_memory_block_pool.at(_index)->statistics().now.alloc_stack;
}

int64_t factory::memory_block::statistics_allocated_create(int _index) const noexcept
{
	return this->m_vector_memory_block_pool.at(_index)->statistics().now.alloc_create;
}

int64_t factory::memory_block::statistics_allocated(int _index) const noexcept
{
	const auto& ps = this->m_vector_memory_block_pool.at(_index)->statistics().now;
	return ps.alloc_stack + ps.alloc_create;
}

int64_t factory::memory_block::statistics_freed_stack(int _index) const noexcept
{
	return this->m_vector_memory_block_pool.at(_index)->statistics().now.free_stack;
}

int64_t factory::memory_block::statistics_freed_delete(int _index) const noexcept
{
	return this->m_vector_memory_block_pool.at(_index)->statistics().now.free_delete;
}

int64_t factory::memory_block::statistics_freed(int _index) const noexcept
{
	const auto& ps = this->m_vector_memory_block_pool.at(_index)->statistics().now;
	return ps.free_stack + ps.free_delete;
}

int64_t factory::memory_block::statistics_existing(int _index) const noexcept
{
	return this->m_vector_memory_block_pool.at(_index)->statistics().now.existing;
}

int64_t factory::memory_block::statistics_in_using(int _index) const noexcept
{
	return this->m_vector_memory_block_pool.at(_index)->statistics().now.in_using;
}


}