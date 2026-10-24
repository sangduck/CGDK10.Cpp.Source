//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                              object classes                               *
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

namespace CGDK
{
//-----------------------------------------------------------------------------
/**

 @class		threads
 

*///-----------------------------------------------------------------------------
class api::threads :
	virtual public				thread_list,
	virtual public				Ireferenceable
{
public:
			threads() noexcept{}
	virtual	~threads() noexcept{}

public:
			// - if _function takes std::stop_token as the first parameter, the stop_token of each thread is passed
			template<class TFN, class... TARGS>
			std::enable_if_t<!std::is_arithmetic<std::decay_t<TFN>>::value, void>
								begin_thread(TFN&& _function, TARGS&&... _arguments);
			template<class TFN, class... TARGS>
			void				begin_threads(size_t _count, TFN&& _function, TARGS&&... _arguments);

			// - requests stop to all threads, then waits until all of them end
			void				stop();
			bool				wait();

			size_t				get_existing_thread_count() const { return this->m_count_thread_exist;}
	[[nodiscard]] bool			empty() const { return this->get_existing_thread_count()==0;}
	[[nodiscard]] bool			exist() const { return this->get_existing_thread_count()!=0;}

// implementation)
protected:
	//! @brief 스레드 함수
	template <class TTYPE>
	static	void				fn_process_thread_pool(std::stop_token _stop_token, threads* _ppool, thread* _pthread, TTYPE* _pfunctor);

	virtual	void				process_begin_thread_object(thread* _pthread) noexcept;
	virtual	void				process_end_thread_object(thread* _pthread) noexcept;

private:
	template <class TFN, class... TARGS>
	static	auto*				_new_functor(TFN&& _function, TARGS&&... _arguments);
	template <class TFUNCTOR>
			void				_begin_thread_object(TFUNCTOR* _pfunctor);

			std::atomic<int>	m_count_thread_exist{ 0 };
};


template <class TTYPE>
void api::threads::fn_process_thread_pool(std::stop_token _stop_token, threads* _ppool, thread* _pthread, TTYPE* _pfunctor)
{
	// 1) Hold
	object_ptr<thread> pthread = _pthread;
	object_ptr<threads>	ppool = _ppool;

	// 2) Functor 호출전 사전처리
	ppool->process_begin_thread_object(pthread);

	// 3) Functor를 호출한다. (a functor without std::placeholders::_1 ignores the stop_token)
	try
	{
		(*_pfunctor)(std::move(_stop_token));
	}
	catch (...)
	{
	}

	// 4) Functor를 지운다.
	delete _pfunctor;

	// 5) Functor 호출 후 사전처리
	ppool->process_end_thread_object(pthread);
}

template <class TFN, class... TARGS>
auto* api::threads::_new_functor(TFN&& _function, TARGS&&... _arguments)
{
	if constexpr (std::is_invocable_v<std::decay_t<TFN>&, std::stop_token, std::decay_t<TARGS>&...>)
	{
		// - the function takes the stop_token of the thread as the first parameter
		using TFUNCTOR = decltype(std::bind(DECAY_COPY(std::forward<TFN>(_function)), std::placeholders::_1, DECAY_COPY(std::forward<TARGS>(_arguments))...));
		return new TFUNCTOR(std::bind(DECAY_COPY(std::forward<TFN>(_function)), std::placeholders::_1, DECAY_COPY(std::forward<TARGS>(_arguments))...));
	}
	else
	{
		using TFUNCTOR = decltype(std::bind(DECAY_COPY(std::forward<TFN>(_function)), DECAY_COPY(std::forward<TARGS>(_arguments))...));
		return new TFUNCTOR(std::bind(DECAY_COPY(std::forward<TFN>(_function)), DECAY_COPY(std::forward<TARGS>(_arguments))...));
	}
}

template <class TFUNCTOR>
void api::threads::_begin_thread_object(TFUNCTOR* _pfunctor)
{
	// lock)
	std::lock_guard lock(this->m_lockable_vector_thread);

	// declare)
	char temp_string[MAX_STRING_LENGTH]{};

	// 1) 이름을 작성~ (index = number of thread objects in the list)
	const auto index = static_cast<int>(this->m_vector_thread.size());
#ifdef __cpp_rtti
	auto* pnameable = dynamic_cast<object::Inameable*>(this);
	std::snprintf(temp_string, std::size(temp_string), std::string_view("%s[%d]").data(), (pnameable != nullptr) ? pnameable->name().data() : "", index);
#else
	std::snprintf(temp_string, std::size(temp_string), std::string_view("ThreadPool[%d]").data(), index);
#endif

	// 2) Thread를 생성한다.
	object_ptr<thread> pthread;
	try
	{
		pthread = make_object<thread>();
	}
	catch (...)
	{
		delete _pfunctor;
		throw;
	}

	// 3) 이름을 설정한다.
	pthread->name(temp_string);

	// 4) Thread List에 추가한다.
	this->m_vector_thread.emplace_back(pthread);

	// 5) add_ref (released by process_end_thread_object())
	this->add_ref();

	try
	{
		// 6) Thread를 시작한다.
		pthread->begin_thread(fn_process_thread_pool<TFUNCTOR>, this, pthread.get(), _pfunctor);
	}
	catch(...)
	{
		// - Release
		this->release();

		// - Thread List에서 제거한다.
		this->m_vector_thread.pop_back();

		// - the thread was not started => the functor was not handed over
		delete _pfunctor;

		// reraise)
		throw;
	}
}

template<class TFN, class... TARGS>
std::enable_if_t<!std::is_arithmetic<std::decay_t<TFN>>::value, void>
api::threads::begin_thread(TFN&& _function, TARGS&&... _arguments)
{
	this->_begin_thread_object(_new_functor(std::forward<TFN>(_function), std::forward<TARGS>(_arguments)...));
}

template<class TFN, class... TARGS>
void api::threads::begin_threads(size_t _count, TFN&& _function, TARGS&&... _arguments)
{
	// - every thread gets its own copy of the function and the arguments (never moved from)
	for (size_t i=0; i<_count; ++i)
	{
		this->_begin_thread_object(_new_functor(_function, _arguments...));
	}
}


}
