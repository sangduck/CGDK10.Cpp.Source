namespace CGDK
{

template <class TENTITY, class TCREATE>
doss::repository::orm<TENTITY, TCREATE>::orm()
{
	object::Iidentifiable::set_object_type(eOBJECT_TYPE::REPOSITORY_ORM);
}

template <class TENTITY, class TCREATE>
doss::repository::orm<TENTITY, TCREATE>::~orm() noexcept
{
}

template <class TENTITY, class TCREATE>
void doss::repository::orm<TENTITY, TCREATE>::on_initializing(context& _msg)
{
	rdb<TENTITY, TCREATE>::on_initializing(_msg);
}

template <class TENTITY, class TCREATE>
void doss::repository::orm<TENTITY, TCREATE>::on_initialize(context& _msg)
{
	rdb<TENTITY, TCREATE>::on_initialize(_msg);
}

template <class TENTITY, class TCREATE>
void doss::repository::orm<TENTITY, TCREATE>::on_destroying()
{
	rdb<TENTITY, TCREATE>::on_destroying();
}

template <class TENTITY, class TCREATE>
void doss::repository::orm<TENTITY, TCREATE>::on_destroy()
{
	rdb<TENTITY, TCREATE>::on_destroy();
}

template <class TENTITY, class TCREATE>
void doss::repository::orm<TENTITY, TCREATE>::on_starting(context& _msg)
{
	rdb<TENTITY, TCREATE>::on_starting(_msg);
}

template <class TENTITY, class TCREATE>
void doss::repository::orm<TENTITY, TCREATE>::on_start(context& _msg)
{
	rdb<TENTITY, TCREATE>::on_start(_msg);
}

template <class TENTITY, class TCREATE>
void doss::repository::orm<TENTITY, TCREATE>::on_stop_request()
{
	rdb<TENTITY, TCREATE>::on_stop_request();
}

template <class TENTITY, class TCREATE>
void doss::repository::orm<TENTITY, TCREATE>::on_stopping()
{
	rdb<TENTITY, TCREATE>::on_stopping();
}

template <class TENTITY, class TCREATE>
void doss::repository::orm<TENTITY, TCREATE>::on_stop()
{
	rdb<TENTITY, TCREATE>::on_stop();
}

template <class TENTITY, class TCREATE>
void doss::repository::orm<TENTITY, TCREATE>::on_execute()
{

}

}
