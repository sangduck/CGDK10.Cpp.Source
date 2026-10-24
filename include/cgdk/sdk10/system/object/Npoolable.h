//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                               Pool Classes                                *
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
/**

 @class		Npoolable
 @brief		정적 멤버 변수로 자체적인 풀을 가진 풀 대상 객체 클래스이다.
 @todo		
 @ref		
 @details	\n
 IPoolable<T>를 상속받아 풀의 대상이 됨과 동시에 자신의 풀을 자체적으로 정적 멤버 변수로 가진 것이다.
 따라서 풀에서 할당을 받을 때 자체적으로 가진 풀에서 할당받을 수 있고 할당이 해제되면 그 풀로 다시 돌아간다.
 또 이 Npoolable<T>을 상속받으면 make_object<T>()를 사용해 간단히 풀에서 할당받을 수 있다.

 Npoolable<T>을 상속받으면 참조계수가 0이 되어 on_final_release/process_dispose
 함수가 호출되면 할당받은 풀로 자동적으로 돌아가도록 제작되어 있다.

 은 다음과 같이 사용할 수 있다.가장 간단하게는 NET<T>()를 사용해 할당을 받을 수 있다.
 또 정적 멤버 함수인 alloc()함수를 사용해 할당할 수 있다.

 Npoolable<T>의 사용법은 다음과 같다.
 
 일단 아래와 같이 상속받아 foo클래스를 정의했다면

 <pre>
 // foo객체를 Npoolable을 상속받아 정의한다.
 class foo: public Npoolable<foo>
 {
 };
 </pre>

 아래와 같이 foo의 정적 멤버함수인 alloc()함수를 사용해 풀에서 할당받을 수 있다.
 <pre>
 // foo객체를 Pool에서 할당한다.
 object_ptr<foo>  a = foo::alloc();
 ...
 </pre>

 하지만 더 간단한 방법은 그냥 make_object<T>()함수를 사용하는 것이다.
 <pre>
 // foo객체를 Pool에서 할당한다.
 object_ptr<foo>  a = make_object<foo>();

 // Reference count를 통한 자연스러운 할당해제를 수행한다.
 a.reset();
 </pre>

 참조계수가 0이 되면 자동적으로 풀로 되돌아가기 때문에 foo::free()함수를 호출할 필요는 없다.

 Npoolable을 상속받으면 on_final_release가 이미 정의되어 있기 때문에 OnFinalReleass()를 재정의할 필요가 없다. 
 만약 재정의를 하게 된다면 반드시 free()함수를 호출하여 생성된 객체가 Pool로 되돌아가도록 해주어야 한다.

*/
//-----------------------------------------------------------------------------
template <class T>
class CGDK::Npoolable :
	public						Ipoolable<T>,
	public						factory::_traits_has_pool
{
public:
			using factory_t = default_factory_pool_t<T>;
			using object_t = T;

protected:
	virtual	~Npoolable() noexcept {}

public:
	[[nodiscard]] static own_ptr<factory_t>& get_factory() { static own_ptr<factory_t> pool_object = _make_own_raw<factory_t>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA std::string_view(), eFACTORY_TYPE::DEFAULT); return pool_object;}
	[[nodiscard]] static T* alloc(CGNEW_DEBUG_INFO_PARAMETERS_WITH_DEFAULT)
	{
		auto* ppool = get_factory().get();
	
		if (ppool != nullptr)
		{
			return ppool->alloc(CGNEW_DEBUG_INFO_PARAMETERS_PASSING);
		}
		else
		{
		#ifdef _ENABLE_DEBUG_NEW
			#if defined(_WIN32)
				return new(_NORMAL_BLOCK, CGNEW_DEBUG_INFO_PARAMETERS_PASSING) T();
			#else
				return new T();
			#endif
		#else
			return new T();
		#endif
		}
	}
	static	void				dealloc(T* _pitem) noexcept { CGASSERT_ERROR(_pitem != nullptr); get_factory()->dealloc(_pitem);}

protected:
	virtual	void				process_dispose() noexcept override { if (this->m_ppool.exist()) { if (this->m_this == nullptr) { this->m_this = dynamic_cast<T*>(this); } this->m_ppool->dealloc(this->m_this); } else { delete this; } }

private:

			object_ptr<factory_t> m_ppool;
			T*					m_this = nullptr;

			friend	factory_t;
};


// poolable template
template <class T>
class CGDK::POOLABLE : public T, public CGDK::Npoolable<POOLABLE<T>>
{
public:
	virtual ~POOLABLE() noexcept {}
};

namespace CGDK
{
	template <class T> class is_base_of_Nreferenceable : public std::false_type {};
	template <class X> class is_base_of_Nreferenceable<Nreferenceable<X>> : public std::true_type {};

	// case) 'Npoolable<T>' inherited
	template <class T>
	[[nodiscard]] std::enable_if_t<std::is_base_of_v<Npoolable<T>, T> && !is_base_of_Nreferenceable<T>::value, object_ptr<T>>
		_alloc_object()
	{
		return T::alloc();
	}

	// case) No 'Npoolable<T>' inherited
	template <class T>
	[[nodiscard]] std::enable_if_t<!std::is_base_of_v<Npoolable<T>, T>&& is_base_of_Nreferenceable<T>::value, object_ptr<T>>
		_alloc_object()
	{
		return _make_object_raw<T>();
	}

	// case) No 'Npoolable<T>' inherited
	template <class T>
	[[nodiscard]] std::enable_if_t<!std::is_base_of_v<Npoolable<T>, T> && !is_base_of_Nreferenceable<T>::value, object_ptr<T>>
		_alloc_object()
	{
		// check) T must inherit class'Ireferenceable'
		static_assert(std::is_base_of_v<Ireferenceable, T>, "CGObj: T must inherit 'Ireferenceable' class");

		return POOLABLE<T>::alloc();
	}

}