template <class TEXECUTOR>
executor::threads<TEXECUTOR>::threads() :
	TEXECUTOR(),
	api::threads(),
	m_is_active(false),
	m_enable_auto_expand(false),
	m_count_thread_wait(0)
{
}

template <class TEXECUTOR>
executor::threads<TEXECUTOR>::~threads() noexcept
{
	this->executor::threads<TEXECUTOR>::stop();
}

template <class TEXECUTOR>
uint32_t executor::threads<TEXECUTOR>::fn_thread(executor::threads<TEXECUTOR>* _this)
{

	CGASSERT(_this != nullptr, 0);

	return _this->process_thread_run();
}

template <class TEXECUTOR>
result_code executor::threads<TEXECUTOR>::start(context& _context)
{

	auto* pInitializable = dynamic_cast<object::Iinitializable*>(this);

	if(pInitializable != nullptr)
	{
		pInitializable->initialize();
	}

	size_t thread_count = CGEXECUTOR_AUTO;
	bool enable_auto_expand = false;

	if (_context.type() == context::value_type_t::buffer_)
	{

		auto& parameter = _context.v.value_buffer.front<START_PARAMETER>();

		thread_count = parameter.thread_count;
	}
	else
	{

		const auto& context_thread_count = _context.find("thread_count"sv, "thread count"sv, "threadcount"sv);

		if (context_thread_count.type() != context::value_type_t::null_)
		{
			thread_count = static_cast<decltype(thread_count)>(static_cast<uint64_t>(context_thread_count));
		}
	}

	if(thread_count == CGEXECUTOR_AUTO)
	{
		thread_count = api::get_core_count() * DEFAULT_THREAD_COUNT_PER_PROCESSOR + DEFAULT_BASE_THREAD_COUNT;
	}

	SATURATE_MAX(thread_count, 1024);

	TEXECUTOR::start(_context);

	this->m_is_active = true;
	this->m_enable_auto_expand = enable_auto_expand;
	this->m_count_thread_wait = 0;
	this->m_thread_index = 0;

	try
	{

		this->begin_thread(thread_count, fn_thread, this);

		LOG_INFO << "  - start "sv << thread_count << " threads on executor["sv << name() << "]"sv;
	}
	catch(...)
	{

		this->TEXECUTOR::stop();

		this->m_is_active = false;

		throw;
	}

	return result_code(eRESULT::SUCCESS);
}

template <class TEXECUTOR>
result_code executor::threads<TEXECUTOR>::stop() noexcept
{

	this->TEXECUTOR::stop();

	this->m_is_active = false;

	auto result = this->wait();

	if(result == false)
		return result_code(eRESULT::FAIL);

	return result_code(eRESULT::SUCCESS);
}

template <class TEXECUTOR>
uint32_t executor::threads<TEXECUTOR>::process_thread_run()
{

	int thread_index = this->m_thread_index++;

	while(this->m_is_active)
	{

		++this->m_statistics.now.thread_count;

		try
		{

			while(this->m_is_active)
			{

				this->execute(1s, thread_index);
			}
		}
		catch (throwable::failure& )
		{
		}
		catch(...)
		{
		}

		--this->m_statistics.now.thread_count;

		if(this->m_is_active)
		{

		}
	}

	return 0;
}

template <class TEXECUTOR>
bool executor::threads<TEXECUTOR>::process_control(uint64_t _command, uint64_t ) noexcept
{
	switch (_command)
	{

	case	1:
			this->process_control_block();
			break;

	case	2:
			this->process_control_unblock();
			break;
	default:
			break;
	}

	return true;
}

template <class TEXECUTOR>
bool executor::threads<TEXECUTOR>::process_control_block() noexcept
{

	++this->m_count_thread_wait;

	return true;
}

template <class TEXECUTOR>
bool executor::threads<TEXECUTOR>::process_control_unblock() noexcept
{

	--this->m_count_thread_wait;

	return true;
}

template <class TEXECUTOR>
void executor::threads<TEXECUTOR>::process_begin_thread_object(api::thread* _pthread) noexcept
{
	{
		std::lock_guard cs(_pthread->m_lockable_executor);

		_pthread->m_pexecutor = this;
	}

	api::threads::process_begin_thread_object(_pthread);
}

template <class TEXECUTOR>
void executor::threads<TEXECUTOR>::process_end_thread_object(api::thread* _pthread) noexcept
{
	api::threads::process_end_thread_object(_pthread);

	{
		std::lock_guard cs(_pthread->m_lockable_executor);

		_pthread->m_pexecutor = nullptr;
	}
}
