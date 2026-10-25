#include "pch.h"

//-----------------------------------------------------------------------------
// linux : the io backend an io_context got is logged (info) when the first io_context is created and when it changes
//  - "io_uring enabled - backend 'io_uring completion'", "io_uring not used - backend 'epoll' (asked ...)" or
//    "io_uring disabled - backend 'epoll' instead of ..." (not available : the kernel, kernel.io_uring_disabled, seccomp)
//-----------------------------------------------------------------------------
#if defined(_SYS_EPOLL_H)

#include <mutex>
#include <sstream>

namespace CGDK
{
namespace
{
	class iobl_logger : public Ilogger
	{
	public:
				std::mutex			lockable;
				std::vector<std::string> messages;

	protected:
		virtual	void				process_log(object_ptr<LOG_RECORD>&& _plog_record) override
		{
			std::ostringstream stream;
			_plog_record->stream_out(stream);

			std::lock_guard lock(this->lockable);
			this->messages.push_back(stream.str());
		}
	};

	// - the backend line logged while an executor of _backend is made
	std::string iobl_log_of(iobl_logger* _plogger, executor::io_backend _backend)
	{
		{
			std::lock_guard lock(_plogger->lockable);
			_plogger->messages.clear();
		}

		const auto backend_before = executor::default_io_backend();
		executor::set_default_io_backend(_backend);
		auto pexecutor = executor::make_io_context_threads("io backend log", 1);
		executor::set_default_io_backend(backend_before);

		if (pexecutor.exist())
			pexecutor->stop();

		std::lock_guard lock(_plogger->lockable);
		for (const auto& iter : _plogger->messages)
		{
			if (iter.find("io_context: io_uring") != std::string::npos)
				return iter;
		}
		return std::string();
	}
}

TEST(io_backend_log, logged_when_the_backend_changes)
{
	auto plogger = make_object<iobl_logger>();
	auto plogger_before = set_default_logger(object_ptr<Ilogger>(plogger.get()));

	// - only a change is logged : completion asked first (whatever the tests before left), then epoll and completion again
	static_cast<void>(iobl_log_of(plogger.get(), executor::io_backend::io_uring_completion));

	// - epoll asked : "io_uring not used"
	const auto log_epoll = iobl_log_of(plogger.get(), executor::io_backend::epoll);

	// - completion asked : "io_uring enabled" where available, "io_uring disabled ... instead of" where not
	const auto log_completion = iobl_log_of(plogger.get(), executor::io_backend::io_uring_completion);

	reset_default_logger(plogger_before.get());

	EXPECT_NE(log_epoll.find("io_uring not used - backend 'epoll'"), std::string::npos) << log_epoll;

	if (executor::is_io_uring_completion_available())
	{
		EXPECT_NE(log_completion.find("io_uring enabled - backend 'io_uring completion'"), std::string::npos) << log_completion;
	}
	else
	{
		EXPECT_TRUE(log_completion.find("io_uring disabled - backend 'epoll' instead of 'io_uring completion'") != std::string::npos) << log_completion;
	}
}

}

#endif
