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

 @class		Ifactory
 
*/
//-----------------------------------------------------------------------------
class Ifactory :
// inherited classes) 
	virtual public				object::Inameable,
	virtual public				object::Idestroyable
{
// constructor/destructor)
public:
			Ifactory() noexcept	{}
	virtual	~Ifactory() noexcept {}

public:
	virtual	eFACTORY_TYPE		type() const noexcept { return this->m_statistics.type;}
	virtual	int					id() const noexcept { return this->m_statistics.id;}

	virtual	size_t				prepare(size_t _amount) PURE;
	virtual	size_t				shrink(size_t _amount) noexcept PURE;
	virtual	void				sustain(eFACTORY_GC_LEVEL _level= eFACTORY_GC_LEVEL::NORMAL) PURE;

	[[nodiscard]] virtual auto	object_size() const noexcept -> size_t PURE;
	[[nodiscard]] virtual auto	stacked_count() const noexcept -> size_t PURE;
	[[nodiscard]] virtual auto	existing_limits() const noexcept -> size_t PURE;
	virtual	void				set_gc_type(eFACTORY_GC_TYPE _gc_type) noexcept PURE;
	[[nodiscard]] auto			gc_type() const noexcept -> eFACTORY_GC_TYPE { return this->m_statistics.gc_type; }

	virtual	void				close() noexcept PURE;

	[[nodiscard]] const auto& 	statistics() const noexcept { return this->m_statistics; }

protected:
	[[nodiscard]] virtual bool	process_is_this(std::string_view _compare) const noexcept override { return this->m_statistics.name == _compare; }
	[[nodiscard]] virtual auto	process_get_name() const noexcept -> std::string override { return this->m_statistics.name; }
	virtual	void				process_set_name(std::string_view _name) override;

	virtual	void				process_tls_garbage_collection() {};

			void				process_register_factory();
			void				process_unregister_factory() noexcept;
			void				process_reset_statistics() noexcept;

			size_t				_get_shrink_count(eFACTORY_GC_LEVEL _level) const noexcept;

protected:
			statistics::factory	m_statistics;
private:
			lock_object			m_lockable_pmanager;
			object_ptr<factory::manager> m_pmanager;
			friend class factory::manager;
};

template<class T>
inline Nreferenceable<T>::Nreferenceable(CGNEW_DEBUG_INFO_PARAMETERS)
{
	this->m_pfactory_object = factory::object<T>::instance(CGNEW_DEBUG_INFO_PARAMETERS_PASSING);
	this->m_pfactory = m_pfactory_object.get();

	this->m_pfactory_object->statistics_on_alloc_create();
}

template<class T>
inline Nreferenceable<T>::~Nreferenceable() noexcept
{
	// check) 
	if (this->m_pfactory_object.empty())
		return;
	
	// statistics) 
	this->m_pfactory_object->statistics_on_free_delete();

	// 1) reset factory object
	this->m_pfactory_object.reset();
	this->m_pfactory = nullptr;
}

inline void Ireferenceable::statistics_on_execute_elapsed_time(chrono::tick::duration _elapsed_time) noexcept
{
	// check)
	if (this->m_pfactory == nullptr)
		return;

	this->m_pfactory->statistics_on_execute_elapsed_time(_elapsed_time);
}


}