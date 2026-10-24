//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                                  Common                                   *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//
// CGDK::Lock::stack<T>
//
// 1. CGDK::Lock::stack란!
//     InterlockedPush/Pop을 사용하여 구현한 stack이다.
//    Thread Safe하게 동작하지만 동작 중 Lock을 걸지 않는다.
//
// 2. CGDK::Lock::stack의 핵심! Function의 기능 설명.
//    1) push		Data를 stack에 push하는 함수.
//    2) pop		stack에서 Data를 하나 빼내는 함수.
//    3) pop_all()	stack의 모든 data를 한꺼번에 빼내는 함수. 데이터는 list혀애로
//                  뽛혀져 나온다.
//
// 3. CGDK::Lock::stack의 주의사항
//    data는 반드시 SLIST_ENTRY를 상속받은 객체여야지 된다.
//
//
//-----------------------------------------------------------------------------
namespace CGDK
{

template <class TDATA>
class locked_self_stack
{
	// ----------------------------------------------------------------------------
	// Type definitions for Standard)
	//
	//  * Common Container Requirement
	//     1) X:value_type			Contain에 담긴 값들의 형.
	//     2) X:reference			X:value_type&.
	//     3) X:const_reference		const X:value_type&.
	//     4) X:iterator			순회할 때 사용되는 반복자.
	//     5) X:const_iterator		순회할 때 사용되는 반복자(const형)
	//     6) X:difference_type		두 iterator에 담김 값의 거리를 나타내는 타입.
	//     7) X:size_type			X의 크기를 나타낼 때 사용되는 형.
	//
	// ----------------------------------------------------------------------------
public:
	// Standard 1) value type.
	using value_type = TDATA;

	// Standard 2) Pointer.
	using pointer = TDATA*;
	using const_pointer = const TDATA*;

	// Standard 3) Reference.
	using reference = TDATA&;
	using const_reference = const TDATA&;

	// Standard 4) size of type & difference type.
	using size_type = size_t;
	using difference_type = ptrdiff_t;

	// Self) 자기 자신(표준 아님.)
	using _mytype = locked_self_stack<TDATA>;

	// Rebinder) 
	template <class TOTHER_DATA>	
	struct rebind
	{
		using other = locked_self_stack<TOTHER_DATA>;
	};

public:
	locked_self_stack() noexcept : m_phead(nullptr), m_depth(0) {}
	~locked_self_stack() noexcept {}

public:
	// 1) 상태 Function
	size_t				size() const noexcept;

	// 2) stack을 모조리 비우는 함수.
	void				clear() noexcept;

	// 3) 동작 Function
	void				push(TDATA _pdata) noexcept;
	void				push(TDATA _pfirst, TDATA _plast, int _count) noexcept;
	bool				push(TDATA _pfirst, TDATA _plast, int _count, int64_t _size_limit) noexcept;	// (size() + _count) <= _size_limit일 때만 push한다.
	TDATA				pop() noexcept;
	TDATA				pop(int _count_max, TDATA& _plast, int& _count) noexcept;
	TDATA				pop_all() noexcept;

	// 4) chain size (객체 묶음의 최대 크기 - 단일 push 및 작은 묶음은 이 크기까지 top chain에 합쳐진다.)
	void				set_chain_size(int _chain_size) noexcept { this->m_chain_size.store((_chain_size < 1) ? 1 : _chain_size, std::memory_order_relaxed); }
	[[nodiscard]] int	chain_size() const noexcept { return this->m_chain_size.load(std::memory_order_relaxed); }

public:
	// lock) 짧은 임계 구역용 lock (4 byte)
	//  - 임계 구역은 항상 O(1)이다. (객체 메모리를 lock 안에서 거의 읽지 않는다.)
	//  - 잠깐 spin한 후 atomic wait(WaitOnAddress/futex)로 대기한다.
	//  - std::mutex(MSVC 80 byte)는 lock/unlock마다 여러 cache line을 쓰므로 쓰지 않는다.
	class spin_lockable
	{
	public:
		void			lock() noexcept { int expected = 0; if (this->m_state.compare_exchange_strong(expected, 1, std::memory_order_acquire, std::memory_order_relaxed)) return; this->_lock_slow(); }
		void			unlock() noexcept { if (this->m_state.exchange(0, std::memory_order_release) == 2) this->m_state.notify_one(); }

	private:
		void			_lock_slow() noexcept;
		static void		_pause() noexcept;

		std::atomic<int> m_state{ 0 };	// 0: unlocked, 1: locked, 2: locked (waiter may exist)
	};

	// - hot members (lock과 함께 매번 쓰이는 멤버를 모아 둔다.)
	spin_lockable		m_lockable_head;
	unsigned int		m_chain_count = 0;
	TDATA				m_phead;
	std::atomic<int>	m_depth;				// lock 안에서만 쓰고 size()는 lock 없이 읽는다. (relaxed)
	unsigned int		m_chain_top = 0;		// (index of top chain) + 1

private:
	// chain) push(_pfirst, _plast, _count)로 들어온 객체 묶음을 chain으로 기억해 둔다.
	//  - pop(_count_max, ...)은 top chain을 list 순회 없이 O(1)로 떼어낸다. (lock 안에서 pointer chasing을 하지 않는다.)
	//    (chain 다음 객체(pafter)도 기억해 두므로 lock 안에서 객체의 Next를 읽지 않는다.)
	//  - top부터 MAX_CHAIN개의 chain을 ring으로 유지한다. 가득 차면 가장 깊은 두 chain을 합친다.
	//  - 항상 'chain 수가 0 <=> stack이 비어 있음', 'chain count의 합 == m_depth'이다.
	static constexpr unsigned int MAX_CHAIN = 32;

	struct CHAIN
	{
		TDATA			plast;					// last object of chain
		TDATA			pafter;					// plast->Next (first object of the chain below)
		int				count;
	};

			CHAIN&		_chain_top() noexcept { return this->m_chain[(this->m_chain_top - 1) & (MAX_CHAIN - 1)]; }
			void		_push_chain(TDATA _plast, TDATA _pafter, int _count) noexcept;
			void		_push_locked(TDATA _pfirst, TDATA _plast, int _count) noexcept;
			void		_pop_chain() noexcept { --this->m_chain_top; --this->m_chain_count; }
			void		_add_depth(int _count) noexcept { this->m_depth.store(this->m_depth.load(std::memory_order_relaxed) + _count, std::memory_order_relaxed); }

	std::atomic<int>	m_chain_size{ 32 };
	CHAIN				m_chain[MAX_CHAIN]{};
};

template <class TDATA>
void locked_self_stack<TDATA>::spin_lockable::_lock_slow() noexcept
{
	// 1) spin
	for (int i = 0; i < 64; ++i)
	{
		_pause();

		if (this->m_state.load(std::memory_order_relaxed) == 0)
		{
			int expected = 0;
			if (this->m_state.compare_exchange_weak(expected, 1, std::memory_order_acquire, std::memory_order_relaxed))
				return;
		}
	}

	// 2) wait
	while (this->m_state.exchange(2, std::memory_order_acquire) != 0)
		this->m_state.wait(2, std::memory_order_relaxed);
}

template <class TDATA>
void locked_self_stack<TDATA>::spin_lockable::_pause() noexcept
{
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
	_mm_pause();
#elif defined(_MSC_VER) && defined(_M_ARM64)
	__yield();
#elif defined(__x86_64__) || defined(__i386__)
	__builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
	__asm__ __volatile__("yield");
#endif
}

template <class TDATA>
size_t locked_self_stack<TDATA>::size() const noexcept
{
	return static_cast<size_t>(this->m_depth.load(std::memory_order_relaxed));
}

template <class TDATA>
void locked_self_stack<TDATA>::clear() noexcept
{
	this->pop_all();
}

template <class TDATA>
void locked_self_stack<TDATA>::_push_chain(TDATA _plast, TDATA _pafter, int _count) noexcept
{
	// 1) merge the deepest two chains if full
	if (this->m_chain_count == MAX_CHAIN)
	{
		const auto& chain_deepest = this->m_chain[this->m_chain_top & (MAX_CHAIN - 1)];
		auto& chain_upper = this->m_chain[(this->m_chain_top + 1) & (MAX_CHAIN - 1)];

		chain_upper.plast = chain_deepest.plast;
		chain_upper.pafter = chain_deepest.pafter;
		chain_upper.count += chain_deepest.count;

		--this->m_chain_count;
	}

	// 2) push new chain
	auto& chain = this->m_chain[this->m_chain_top & (MAX_CHAIN - 1)];
	chain.plast = _plast;
	chain.pafter = _pafter;
	chain.count = _count;

	++this->m_chain_top;
	++this->m_chain_count;
}

template <class TDATA>
void locked_self_stack<TDATA>::push(TDATA _pdata) noexcept
{
	std::lock_guard<spin_lockable> lock(this->m_lockable_head);

	auto phead = this->m_phead;
	_pdata->Next = phead;
	this->m_phead = _pdata;

	this->_add_depth(1);

	// - merge into the top chain
	if (this->m_chain_count != 0 && this->_chain_top().count < this->m_chain_size.load(std::memory_order_relaxed))
		++this->_chain_top().count;
	else
		this->_push_chain(_pdata, phead, 1);
}

template <class TDATA>
void locked_self_stack<TDATA>::_push_locked(TDATA _pfirst, TDATA _plast, int _count) noexcept
{
	auto phead = this->m_phead;
	_plast->Next = phead;
	this->m_phead = _pfirst;

	this->_add_depth(_count);

	// - merge into the top chain if small
	if (this->m_chain_count != 0 && this->_chain_top().count + _count <= this->m_chain_size.load(std::memory_order_relaxed))
		this->_chain_top().count += _count;
	else
		this->_push_chain(_plast, phead, _count);
}

template <class TDATA>
void locked_self_stack<TDATA>::push(TDATA _pfirst, TDATA _plast, int _count) noexcept
{
	std::lock_guard<spin_lockable> lock(this->m_lockable_head);

	this->_push_locked(_pfirst, _plast, _count);
}

template <class TDATA>
bool locked_self_stack<TDATA>::push(TDATA _pfirst, TDATA _plast, int _count, int64_t _size_limit) noexcept
{
	std::lock_guard<spin_lockable> lock(this->m_lockable_head);

	// check)
	if (static_cast<int64_t>(this->m_depth.load(std::memory_order_relaxed)) + _count > _size_limit)
		return false;

	this->_push_locked(_pfirst, _plast, _count);

	// return)
	return true;
}

template <class TDATA>
TDATA locked_self_stack<TDATA>::pop() noexcept
{
	std::lock_guard<spin_lockable> lock(this->m_lockable_head);

	if(this->m_phead == nullptr)
		return nullptr;

	auto data = this->m_phead;
	auto& chain = this->_chain_top();

	if (--chain.count == 0)
	{
		this->m_phead = chain.pafter;
		this->_pop_chain();
	}
	else
	{
		this->m_phead = data->Next;
	}

	this->_add_depth(-1);

	return data;
}

template <class TDATA>
TDATA locked_self_stack<TDATA>::pop(int _count_max, TDATA& _plast, int& _count) noexcept
{
	// check)
	if (_count_max < 1)
		_count_max = 1;

	// declare)
	TDATA pfirst;
	TDATA plast;
	int count;

	{
		// lock)
		std::lock_guard<spin_lockable> lock(this->m_lockable_head);

		// check)
		pfirst = this->m_phead;

		if (pfirst == nullptr)
		{
			_plast = nullptr;
			_count = 0;
			return nullptr;
		}

		// declare)
		auto& chain = this->_chain_top();

		// 1.A) detach the top chain (O(1))
		if (chain.count <= _count_max)
		{
			plast = chain.plast;
			count = chain.count;
			this->m_phead = chain.pafter;

			this->_pop_chain();
		}
		// 1.B) top chain is bigger than _count_max (merged chain) - walk _count_max objects
		else
		{
			plast = pfirst;
			for (count = 1; count < _count_max; ++count)
				plast = plast->Next;

			this->m_phead = plast->Next;
			chain.count -= count;
		}

		this->_add_depth(-count);
	}

	// 2) terminate detached chain (out of lock - the chain is owned by this thread now)
	plast->Next = nullptr;

	// return)
	_plast = plast;
	_count = count;
	return pfirst;
}

template <class TDATA>
TDATA locked_self_stack<TDATA>::pop_all() noexcept
{
	std::lock_guard<spin_lockable> lock(this->m_lockable_head);

	auto data = this->m_phead;
	this->m_phead = nullptr;

	this->m_depth.store(0, std::memory_order_relaxed);
	this->m_chain_count = 0;

	return data;
}
}
