namespace CGDK
{

template <class TENTITY, class TCREATE>
doss::repository::rdb<TENTITY, TCREATE>::rdb()
{
	object::Iidentifiable::set_object_type(eOBJECT_TYPE::REPOSITORY_DB);

	doss::manager_repository::get_instance()->register_repository(this);
}

template <class TENTITY, class TCREATE>
doss::repository::rdb<TENTITY, TCREATE>::~rdb() noexcept
{

	doss::manager_repository::get_instance()->unregister_repository(this);
}

template <class TENTITY, class TCREATE>
void doss::repository::rdb<TENTITY, TCREATE>::on_initializing(context& )
{

	Irepository::m_id_repository = sREPOSITORY_ID::generate();
}

template <class TENTITY, class TCREATE>
void doss::repository::rdb<TENTITY, TCREATE>::on_initialize(context& )
{
}

template <class TENTITY, class TCREATE>
void doss::repository::rdb<TENTITY, TCREATE>::on_destroying()
{
	this->checkout_all();
}

template <class TENTITY, class TCREATE>
void doss::repository::rdb<TENTITY, TCREATE>::on_destroy()
{
	// - break the references between entities, transactions and sessions
	this->clear_entities();
}

template <class TENTITY, class TCREATE>
void doss::repository::rdb<TENTITY, TCREATE>::on_starting(context& )
{
}

template <class TENTITY, class TCREATE>
void doss::repository::rdb<TENTITY, TCREATE>::on_start(context& )
{
	set_object_state(eOBJECT_STATE::ESTABLISHED);

	this->m_cv_complete.notify_one();
}

template <class TENTITY, class TCREATE>
void doss::repository::rdb<TENTITY, TCREATE>::on_stop_request()
{
	set_object_state_if(eOBJECT_STATE::ESTABLISHED, eOBJECT_STATE::RUNNING);
}

template <class TENTITY, class TCREATE>
void doss::repository::rdb<TENTITY, TCREATE>::on_stopping()
{
}

template <class TENTITY, class TCREATE>
void doss::repository::rdb<TENTITY, TCREATE>::on_stop()
{
}

template <class TENTITY, class TCREATE>
void doss::repository::rdb<TENTITY, TCREATE>::on_execute()
{

}

template <class TENTITY, class TCREATE>
object_ptr<TENTITY> doss::repository::rdb<TENTITY, TCREATE>::process_alloc_entity(sENTITY_ID _entity_id)
{
	auto pentity = make_object<TCREATE>();

	pentity->m_eid = _entity_id;

	return pentity;
}

}
