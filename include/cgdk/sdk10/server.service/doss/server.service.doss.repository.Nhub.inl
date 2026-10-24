namespace CGDK
{

template <class TENTITY>
doss::repository::Nhub<TENTITY>::Nhub()
{

	object::Iidentifiable::set_object_type(eOBJECT_TYPE::REPOSITORY_HUB);

	doss::manager_repository::get_instance()->register_repository(this);
}

template <class TENTITY>
doss::repository::Nhub<TENTITY>::~Nhub() noexcept
{

	doss::manager_repository::get_instance()->unregister_repository(this);
}

template <class TENTITY>
void doss::repository::Nhub<TENTITY>::on_initializing(context& )
{

	m_object_id	 = sOBJECT_ID::generate();
}

template <class TENTITY>
void doss::repository::Nhub<TENTITY>::on_initialize(context& _msg)
{

	SCOPED_LOCK(this->m_lockable_list_repository)
	{
		for(auto& iter:this->m_list_repository)
		{
			iter->Initialize(_msg);
		}
	}
}

template <class TENTITY>
void doss::repository::Nhub<TENTITY>::on_destroying()
{

	SCOPED_LOCK(this->m_lockable_list_repository)
	{
		for(auto& iter:this->m_list_repository)
		{
			iter->destroy();
		}
	}
}

template <class TENTITY>
void doss::repository::Nhub<TENTITY>::on_destroy()
{

	m_object_id.reset();
}

template <class TENTITY>
void doss::repository::Nhub<TENTITY>::on_starting(context& _msg)
{

	SCOPED_LOCK(this->m_lockable_list_repository)
	{
		for(auto& iter:this->m_list_repository)
		{
			iter->start(_msg);
		}
	}
}

template <class TENTITY>
void doss::repository::Nhub<TENTITY>::on_start(context& )
{

	set_object_state(eOBJECT_STATE::ESTABLISHED);

	this->m_cv_complete.notify_one();
}

template <class TENTITY>
void doss::repository::Nhub<TENTITY>::on_stop_request()
{

	set_object_state_if(eOBJECT_STATE::ESTABLISHED, eOBJECT_STATE::RUNNING);
}

template <class TENTITY>
void doss::repository::Nhub<TENTITY>::on_stopping()
{

	SCOPED_LOCK(this->m_lockable_list_repository)
	{
		for(auto& iter:this->m_list_repository)
		{
			iter->Stop();
		}
	}
}

template <class TENTITY>
void doss::repository::Nhub<TENTITY>::on_stop()
{
}

template <class TENTITY>
coroutine::task<result_code> doss::repository::Nhub<TENTITY>::co_request_transaction(object_ptr<Itransactable> _ptransactable)
{
	// 1) find the sub-repository for this transaction
	auto prepository = process_find_repository(_ptransactable->m_buf_request);

	// check)
	if (prepository.empty())
		co_return eRESULT::NOT_READY;

	// 2) route the transaction to it
	co_return co_await prepository->co_request_transaction(std::move(_ptransactable));
}

}
