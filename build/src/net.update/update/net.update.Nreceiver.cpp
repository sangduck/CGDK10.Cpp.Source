#include "pch.h"


CGDK::update::Nreceiver::Nreceiver()
{
}

CGDK::update::Nreceiver::~Nreceiver() noexcept
{
	// 1) self destruction
	this->destroy();
}

void CGDK::update::Nreceiver::set_root_path(const std::string& _root_path)
{
	m_path_root = _root_path;
}

void CGDK::update::Nreceiver::set_root_path(std::string&& _root_path) noexcept
{
	m_path_root = std::move(_root_path);
}

CGDK::result_code CGDK::update::Nreceiver::_process_starting(context& _context)
{
	// 1) call 'on_starting'
	this->on_starting(_context);

	// trace)
	LOG_PROGRESS << "@ <BEGIN> start UPDATE_RECEIVER['"sv << /*this->name() <<*/ "']"sv;

	try
	{
		this->m_total_processed_bytes = 0;
		this->m_total_processed_files = 0;
		this->m_total_transfered_bytes = 0;
		this->m_total_transfered_files = 0;

		this->m_period_transfered_bytes = 0;
		this->m_period_transfered_bytes_per_sec = 0.0f;
		this->m_period_tick_last_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			chrono::time::utc::now().time_since_epoch()).count();
	}
	catch (std::exception& /*e*/)
	{
		// log)
		LOG_ERROR << "@ (excp) CGDK::update::receiver - exception complete process_start:'" << /*name() <<*/ __CGFUNCTION__;

		// reraise)
		throw;
	}

	// return)
	return eRESULT::SUCCESS;
}

CGDK::result_code CGDK::update::Nreceiver::_process_stopping() noexcept
{
	// trace)
	LOG_PROGRESS << "@ <BEGIN> stop UPDATE_RECEIVER['"sv << /*name() <<*/ "']"sv;

	try
	{
		// 1) call 'on_stopping'
		this->on_stopping();

		// 2) close all receivable (queued after any work still pending, so it runs in order)
		sWORK work;
		work.type = sWORK::eTYPE::CANCEL_ALL;
		this->process_enqueue(std::move(work));

		// 3) wait until the worker has drained the queue
		this->process_wait_idle();
	}
	catch (...)
	{
		// log)
		LOG_ERROR << "@ (excp) exception on 'on_stopping' function call UPDATE_RECEIVER '"sv << /*name() <<*/ "' " __CGFUNCTION__ ""sv;
	}

	// return)
	return eRESULT::SUCCESS;
}

CGDK::result_code CGDK::update::Nreceiver::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) execute message
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
			ON_CGMESSAGE(CGDK::eMESSAGE::UPDATE::BEGIN_REQUEST,		on_message_UPDATE_BEGIN_REQUEST,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(CGDK::eMESSAGE::UPDATE::END,				on_message_UPDATE_END,				sMESSAGE_NETWORK);
			ON_CGMESSAGE(CGDK::eMESSAGE::UPDATE::UPDATABLE_REQUEST,	on_message_UPDATE_UPDATABLE_REQUEST,sMESSAGE_NETWORK);
			ON_CGMESSAGE(CGDK::eMESSAGE::UPDATE::UPDATABLE_BEGIN,	on_message_UPDATE_UPDATABLE_BEGIN,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(CGDK::eMESSAGE::UPDATE::UPDATABLE_BLOCK,	on_message_UPDATE_UPDATABLE_BLOCK,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(CGDK::eMESSAGE::UPDATE::UPDATABLE_END,		on_message_UPDATE_UPDATABLE_END,	sMESSAGE_NETWORK);
		END_SUB_CGMESSAGE_MAP
	END_NESTED_CGMESSAGE_MAP

	// return)
	return eRESULT::BYPASS;
}

// ----------------------------------------------------------------------------
// message handlers (network thread)
//
//   The receive thread only parses a message and enqueues a work item; md5,
//   decompression, file I/O and every change of the update state happen on the
//   serial worker (see process_enqueue / process_drain_queue), in message order.
// ----------------------------------------------------------------------------
CGDK::result_code CGDK::update::Nreceiver::on_message_UPDATE_BEGIN_REQUEST(sMESSAGE_NETWORK& _msg)
{
	// ------------------------------------------
	// update request
	//   - the server sends it to start an update; sUPDATE_REQUEST_INFO carries the total file count and bytes
	//     (files found identical on this side are not transferred, so the actual amount can be much smaller)
	// ------------------------------------------

	// 1) buffer for message extract
	auto buf = _msg.buf_message + offset(8);

	// 2) get receiver_id
	auto receiver_id = buf.extract<CGDK::update::RECEIVER_ID>();

	// 3) get sender_id
	auto sender_id = buf.extract<CGDK::update::SENDER_ID>();

	// 4) read update_request_info
	auto update_request_info = buf.extract<CGDK::update::sUPDATE_REQUEST_INFO>();

	try
	{
		// check)
		THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY, 0, "receiver is not ready (call 'start')"sv));

		// 5) the receiver id is checked on this thread for every following message, so it is set here
		this->m_receiver_id = receiver_id;

		SCOPED_LOCK(this->m_lockable_map_updatable)
		{
			this->m_update_info = update_request_info;
		}

		// 6) the rest runs on the worker, after whatever is still queued
		sWORK work;
		work.type = sWORK::eTYPE::UPDATE_BEGIN;
		work.sender_id = sender_id;
		work.receiver_id = receiver_id;
		work.update_request_info = update_request_info;
		this->process_enqueue(std::move(work));
	}
	catch (throwable::failure& _e)
	{
		this->send(_message_UPDATE_BEGIN_RESPONSE(sender_id, receiver_id, _e.reason()));
	}
	catch (...)
	{
		this->send(_message_UPDATE_BEGIN_RESPONSE(sender_id, receiver_id, eRESULT::EXCEPTION));
	}

	// return)
	return eRESULT::DONE;
}

CGDK::result_code CGDK::update::Nreceiver::on_message_UPDATE_END(sMESSAGE_NETWORK& _msg)
{
	// ------------------------------------------
	// update end
	//   - the server sends it when the whole update is over, with the overall result
	//   - the client may stop the receiver after this message
	// ------------------------------------------

	// 1) buffer for message extract
	auto buf = _msg.buf_message + offset(8);

	// 2) get receiver_id
	auto receiver_id = buf.extract<CGDK::update::RECEIVER_ID>();

	// check)
	RETURN_IF(receiver_id != this->m_receiver_id, eRESULT::BYPASS);

	try
	{
		// 3) get sender_id
		auto sender_id = buf.extract<CGDK::update::SENDER_ID>();

		// 4) get result
		auto result = buf.extract<eRESULT>();

		// 5) finish on the worker, after the blocks still queued
		sWORK work;
		work.type = sWORK::eTYPE::UPDATE_END;
		work.sender_id = sender_id;
		work.receiver_id = receiver_id;
		work.result = result;
		this->process_enqueue(std::move(work));
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

CGDK::result_code CGDK::update::Nreceiver::on_message_UPDATE_UPDATABLE_REQUEST(sMESSAGE_NETWORK& _msg)
{
	// ------------------------------------------
	// updatable request
	//   - asked before a file is transferred: answers with the local file's length and, when the length matches, its md5
	// ------------------------------------------

	// 1) buffer for message extract
	auto buf = _msg.buf_message + offset(8);

	// 2) get receiver_id
	auto receiver_id = buf.extract<CGDK::update::RECEIVER_ID>();

	// check)
	RETURN_IF(receiver_id != this->m_receiver_id, eRESULT::BYPASS);

	// 3) get sender_id
	auto sender_id = buf.extract<CGDK::update::SENDER_ID>();

	// 4) read file meta info
	auto file_meta_info = buf.extract<CGDK::resource::sUPDATABLE_FILE_META_INFO>();

	try
	{
		// check)
		THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY, 0, "receiver is not ready (call 'start')"sv));

		// 5) prepare on the worker (opens and hashes the existing file)
		sWORK work;
		work.type = sWORK::eTYPE::UPDATABLE_PREPARE;
		work.sender_id = sender_id;
		work.receiver_id = receiver_id;
		work.updatable_id = file_meta_info.id;
		work.file_meta_info = std::move(file_meta_info);
		this->process_enqueue(std::move(work));
	}
	catch (throwable::failure& _e)
	{
		this->send(_message_UPDATE_UPDATABLE_RESPONSE(sender_id, receiver_id, file_meta_info.id, CGDK::sUPDATABLE_PREPARE_INFO{0, MD5(), _e.reason()}));
	}
	catch (...)
	{
		this->send(_message_UPDATE_UPDATABLE_RESPONSE(sender_id, receiver_id, file_meta_info.id, CGDK::sUPDATABLE_PREPARE_INFO{0, MD5(), eRESULT::EXCEPTION}));
	}

	// return)
	return eRESULT::DONE;
}

CGDK::result_code CGDK::update::Nreceiver::on_message_UPDATE_UPDATABLE_BEGIN(sMESSAGE_NETWORK& _msg)
{
	// ------------------------------------------
	// updatable begin
	//   - the transfer of a file starts; the start position was decided from the UPDATABLE_REQUEST answer
	//   - the file flag carries RECREATE on a retry (the file is rewritten from scratch)
	// ------------------------------------------

	// 1) buffer for message extract
	auto buf = _msg.buf_message + offset(8);

	// 2) get receiver_id
	auto receiver_id = buf.extract<CGDK::update::RECEIVER_ID>();

	// check)
	RETURN_IF(receiver_id != this->m_receiver_id, eRESULT::BYPASS);

	// 3) get sender_id
	auto sender_id = buf.extract<CGDK::update::SENDER_ID>();

	// 4) get updatable_id
	auto updatable_id = buf.extract<CGDK::update::UPDATABLE_ID>();

	try
	{
		// check)
		THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY, 0, "receiver is not ready (call 'start')"sv));

		// 5) get start pos and file flag
		auto pos = buf.extract<uint64_t>();
		auto file_flag = buf.extract<resource::eFILE_FLAG>();

		// 6) open on the worker
		sWORK work;
		work.type = sWORK::eTYPE::UPDATABLE_BEGIN;
		work.sender_id = sender_id;
		work.receiver_id = receiver_id;
		work.updatable_id = updatable_id;
		work.pos_begin = pos;
		work.file_flag = file_flag;
		this->process_enqueue(std::move(work));
	}
	catch (throwable::failure& _e)
	{
		this->send(_message_UPDATE_UPDATABLE_END(sender_id, receiver_id, updatable_id, _e.reason(), CGDK::MD5()));
	}
	catch (...)
	{
		this->send(_message_UPDATE_UPDATABLE_END(sender_id, receiver_id, updatable_id, eRESULT::EXCEPTION, CGDK::MD5()));
	}

	// return)
	return eRESULT::DONE;
}

CGDK::result_code CGDK::update::Nreceiver::on_message_UPDATE_UPDATABLE_BLOCK(sMESSAGE_NETWORK& _msg)
{
	// 1) buffer for message extract
	auto buf = _msg.buf_message + offset(8);

	// 2) get receiver_id
	auto receiver_id = buf.extract<CGDK::update::RECEIVER_ID>();

	// check)
	RETURN_IF(receiver_id != this->m_receiver_id, eRESULT::BYPASS);

	// 3) get sender_id
	auto sender_id = buf.extract<CGDK::update::SENDER_ID>();

	// declare)
	CGDK::update::UPDATABLE_ID updatable_id = 0;

	try
	{
		// check)
		THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY, 0, "receiver is not ready (call 'start')"sv));

		// 4) read block meta info
		auto meta_file_block = buf.extract<CGDK::resource::sMETA_FILE_BLOCK>();
		updatable_id = meta_file_block.id;

		// statistics) (bytes received; the worker counts the files)
		this->m_total_processed_bytes += meta_file_block.size;
		this->m_total_transfered_bytes += meta_file_block.size;
		this->m_period_transfered_bytes += meta_file_block.size;

		// statistics) - update throughput every 1 second
		auto tick_now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			chrono::time::utc::now().time_since_epoch()).count();
		auto last_tick_ms = this->m_period_tick_last_ms.load(std::memory_order_relaxed);
		auto f_duration = static_cast<float>(tick_now_ms - last_tick_ms);
		if (f_duration > 1000.0f)
		{
			auto bytes = this->m_period_transfered_bytes.load(std::memory_order_relaxed);
			this->m_period_transfered_bytes_per_sec.store(
				(f_duration != 0.0f) ? (static_cast<float>(bytes) / f_duration) * 1000.0f : 0.0f,
				std::memory_order_relaxed);
			this->m_period_transfered_bytes.store(0, std::memory_order_relaxed);
			this->m_period_tick_last_ms.store(tick_now_ms, std::memory_order_relaxed);
		}

		// 5) decompress, hash and write on the worker (the message buffer stays alive in the work item)
		sWORK work;
		work.type = sWORK::eTYPE::UPDATABLE_BLOCK;
		work.sender_id = sender_id;
		work.receiver_id = receiver_id;
		work.updatable_id = updatable_id;
		work.meta_file_block = meta_file_block;
		work.buf_data = buf;
		this->process_enqueue(std::move(work));
	}
	catch (throwable::failure& _e)
	{
		this->send(_message_UPDATE_UPDATABLE_END(sender_id, receiver_id, updatable_id, _e.reason(), CGDK::MD5()));
	}
	catch (...)
	{
		this->send(_message_UPDATE_UPDATABLE_END(sender_id, receiver_id, updatable_id, eRESULT::EXCEPTION, CGDK::MD5()));
	}

	// return)
	return eRESULT::DONE;
}

CGDK::result_code CGDK::update::Nreceiver::on_message_UPDATE_UPDATABLE_END(sMESSAGE_NETWORK& _msg)
{
	// 1) buffer for message extract
	auto buf = _msg.buf_message + offset(8);

	// 2) get receiver_id
	auto receiver_id = buf.extract<CGDK::update::RECEIVER_ID>();

	// check)
	RETURN_IF(receiver_id != this->m_receiver_id, eRESULT::BYPASS);

	try
	{
		// check)
		THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY, 0, "receiver is not ready (call 'start')"sv));

		// 3) get sender_id
		auto sender_id = buf.extract<CGDK::update::SENDER_ID>();

		// 4) get updatable id
		auto updatable_id = buf.extract<CGDK::update::UPDATABLE_ID>();

		// 5) get result
		auto result = buf.extract<eRESULT>();

		// 6) close on the worker, after the blocks still queued
		sWORK work;
		work.type = sWORK::eTYPE::UPDATABLE_CLOSE;
		work.sender_id = sender_id;
		work.receiver_id = receiver_id;
		work.updatable_id = updatable_id;
		work.result = result;
		this->process_enqueue(std::move(work));
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	// return)
	return eRESULT::DONE;
}

// ----------------------------------------------------------------------------
// work queue
// ----------------------------------------------------------------------------
void CGDK::update::Nreceiver::process_enqueue(sWORK&& _work)
{
	// declare)
	bool is_post_needed = false;

	SCOPED_LOCK_NAMED(this->m_lockable_queue, lock)
	{
		// 1) backpressure: a block waits (throttling the receive thread) while the worker is behind by more than the limit
		if (_work.type == sWORK::eTYPE::UPDATABLE_BLOCK)
		{
			while (this->m_is_draining && this->m_queue_bytes >= this->m_queue_limit_bytes.load(std::memory_order_relaxed) && std::this_thread::get_id() != this->m_thread_id_worker.load(std::memory_order_relaxed))
			{
				this->m_cv_queue.wait_for(lock, std::chrono::milliseconds(100));
			}

			this->m_queue_bytes += _work.buf_data.size();
		}

		// 2) enqueue
		this->m_queue_work.push_back(std::move(_work));

		// 3) start the worker when it is idle
		if (this->m_is_draining == false)
		{
			this->m_is_draining = true;
			is_post_needed = true;
		}
	}

	// check)
	RETURN_IF(is_post_needed == false);

	// 4) post the drain task (the captured pointer keeps the receiver alive while it runs)
	object_ptr<Nreceiver> preceiver(this);
	bool is_posted = false;

	try
	{
		is_posted = system_executor::post(make_executable([preceiver]() { preceiver->process_drain_queue(); }));
	}
	catch (...)
	{
		is_posted = false;
	}

	// 5) no executor: drain on the calling thread
	if (is_posted == false)
	{
		this->process_drain_queue();
	}
}

void CGDK::update::Nreceiver::process_drain_queue()
{
	// 1) this thread is the worker until the queue is empty
	this->m_thread_id_worker.store(std::this_thread::get_id(), std::memory_order_relaxed);

	for (;;)
	{
		// declare)
		sWORK work;

		// 2) take the next item
		SCOPED_LOCK(this->m_lockable_queue)
		{
			if (this->m_queue_work.empty())
			{
				this->m_is_draining = false;
				this->m_thread_id_worker.store(std::thread::id(), std::memory_order_relaxed);
				this->m_cv_queue.notify_all();
				return;
			}

			work = std::move(this->m_queue_work.front());
			this->m_queue_work.pop_front();

			if (work.type == sWORK::eTYPE::UPDATABLE_BLOCK)
			{
				this->m_queue_bytes -= work.buf_data.size();
				this->m_cv_queue.notify_all();
			}
		}

		// 3) execute it (never throws)
		this->process_work(work);
	}
}

void CGDK::update::Nreceiver::process_wait_idle() noexcept
{
	// check) the worker cannot wait for itself (stop called from a callback)
	RETURN_IF(std::this_thread::get_id() == this->m_thread_id_worker.load(std::memory_order_relaxed));

	SCOPED_LOCK_NAMED(this->m_lockable_queue, lock)
	{
		// 1) wait up to 30 seconds for the queue to drain
		for (int i = 0; i < 300 && (this->m_is_draining || this->m_queue_work.empty() == false); ++i)
		{
			this->m_cv_queue.wait_for(lock, std::chrono::milliseconds(100));
		}

		// log)
		if (this->m_is_draining || this->m_queue_work.empty() == false)
		{
			LOG_ERROR << "@ (!) UPDATE_RECEIVER work queue did not drain in time "sv << __CGFUNCTION__;
		}
	}
}

void CGDK::update::Nreceiver::process_work(sWORK& _work)
{
	try
	{
		switch (_work.type)
		{
		case sWORK::eTYPE::UPDATE_BEGIN:		this->process_work_update_begin(_work); break;
		case sWORK::eTYPE::UPDATE_END:			this->process_work_update_end(_work); break;
		case sWORK::eTYPE::UPDATABLE_PREPARE:	this->process_work_updatable_prepare(_work); break;
		case sWORK::eTYPE::UPDATABLE_BEGIN:		this->process_work_updatable_begin(_work); break;
		case sWORK::eTYPE::UPDATABLE_BLOCK:		this->process_work_updatable_block(_work); break;
		case sWORK::eTYPE::UPDATABLE_CLOSE:		this->process_work_updatable_close(_work); break;
		case sWORK::eTYPE::CANCEL_ALL:			this->cancel_all_updatable(); break;
		default:								CGASSERT_ERROR(false); break;
		}
	}
	catch (...)
	{
		// log)
		LOG_ERROR << "@ (excp) exception on UPDATE_RECEIVER work "sv << static_cast<int>(_work.type) << " "sv << __CGFUNCTION__;
	}
}

// ----------------------------------------------------------------------------
// work (serial worker)
// ----------------------------------------------------------------------------
void CGDK::update::Nreceiver::process_work_update_begin(sWORK& _work)
{
	try
	{
		// 1) reset the updatable map
		SCOPED_LOCK(this->m_lockable_map_updatable)
		{
			this->m_map_updatable_receive.clear();
		}

		// 2) get begin time
		this->m_total_tick_begin = chrono::time::utc::now();

		// 3) call 'on_update_begin'
		this->on_update_begin(_work.update_request_info);

		// 4) send response
		this->send(_message_UPDATE_BEGIN_RESPONSE(_work.sender_id, _work.receiver_id, eRESULT::SUCCESS));
	}
	catch (throwable::failure& _e)
	{
		this->send(_message_UPDATE_BEGIN_RESPONSE(_work.sender_id, _work.receiver_id, _e.reason()));
	}
	catch (...)
	{
		this->send(_message_UPDATE_BEGIN_RESPONSE(_work.sender_id, _work.receiver_id, eRESULT::EXCEPTION));
	}
}

void CGDK::update::Nreceiver::process_work_update_end(sWORK& _work)
{
	// 1) get end time
	this->m_total_tick_end = chrono::time::utc::now();

	// 2) call 'on_update_end'
	this->on_update_end(_work.result);

	// 3) cancel all updatable
	this->cancel_all_updatable();
}

void CGDK::update::Nreceiver::process_work_updatable_prepare(sWORK& _work)
{
	// declare)
	object_ptr<update::Ireceivable> preceivable;
	bool is_registered = false;

	try
	{
		// 1) create new updatable
		preceivable = make_object<update::receivable::file>();

		// 2) set file meta info
		preceivable->m_meta_info = _work.file_meta_info;

		// 3) call 'on_updatable_requested'
		this->on_updatable_requested(preceivable);

		// 4) add updatable
		this->register_updatable(preceivable);
		is_registered = true;

		// 5) md5 caching
		object_ptr<md5_caching> pmd5_caching = m_pmd5_caching;

		// declare)
		sUPDATABLE_PREPARE_INFO updatable_prepare_info;

		// 6) try the cached 'prepare_info' first
		if (pmd5_caching.exist())
		{
			updatable_prepare_info = pmd5_caching->get_prepare_info(preceivable->m_full_file_path);
		}

		// 7) compute it when nothing is cached, or when the cached entry is expired (the file changed since it
		//    was cached), so the sender gets the file's real size and can resume a partial download
		if (updatable_prepare_info.check_code.empty() || updatable_prepare_info.result == eRESULT::EXPIRED)
		{
			// - compute from the file
			updatable_prepare_info = preceivable->process_prepare(preceivable->m_full_file_path);

			// - cache the freshly computed check code (only a complete, size-matched file yields one)
			if (pmd5_caching.exist() && updatable_prepare_info.check_code.exist())
			{
				pmd5_caching->put_prepare_info(preceivable->m_full_file_path, updatable_prepare_info);
			}
		}

		// 8) send response message
		this->send(_message_UPDATE_UPDATABLE_RESPONSE(_work.sender_id, _work.receiver_id, _work.updatable_id, updatable_prepare_info));
	}
	catch (throwable::failure& _e)
	{
		if (preceivable.exist())
		{
			this->on_updatable_end(preceivable, _e.reason());
		}

		if (is_registered)
		{
			this->unregister_updatable(_work.updatable_id);
		}

		this->send(_message_UPDATE_UPDATABLE_RESPONSE(_work.sender_id, _work.receiver_id, _work.updatable_id, CGDK::sUPDATABLE_PREPARE_INFO{0, MD5(), _e.reason()}));
	}
	catch (...)
	{
		if (preceivable.exist())
		{
			this->on_updatable_end(preceivable, eRESULT::EXCEPTION);
		}

		if (is_registered)
		{
			this->unregister_updatable(_work.updatable_id);
		}

		this->send(_message_UPDATE_UPDATABLE_RESPONSE(_work.sender_id, _work.receiver_id, _work.updatable_id, CGDK::sUPDATABLE_PREPARE_INFO{0, MD5(), eRESULT::EXCEPTION}));
	}
}

void CGDK::update::Nreceiver::process_work_updatable_begin(sWORK& _work)
{
	// declare)
	object_ptr<update::Ireceivable> preceivable;

	try
	{
		// 1) get updatable
		preceivable = this->find_updatable(_work.updatable_id);

		// check)
		THROW_IF(preceivable.empty(), throwable::failure(eRESULT::NOT_READY, 0, "receiver not ready')"sv));

		// 2) file flag (RECREATE on a retry: the file is rewritten from scratch)
		SCOPED_LOCK(preceivable->m_lockable_info)
		{
			preceivable->m_meta_info.flag = preceivable->m_meta_info.flag | _work.file_flag;
		}

		// 3) call 'on_updatable_begin'
		this->on_updatable_begin(preceivable, _work.pos_begin);

		// 4) process begin (opens the file and hashes the part kept on a resume)
		preceivable->process_begin(_work.pos_begin);

		// 5) an empty file has no blocks: it is complete right away
		if (preceivable->m_meta_info.size == 0)
		{
			// - md5 of no data
			preceivable->m_md5_check.padding();

			// - send UPDATABLE_END message
			this->send(_message_UPDATE_UPDATABLE_END(_work.sender_id, _work.receiver_id, _work.updatable_id, eRESULT::SUCCESS, preceivable->m_md5_check.get_md5()));
		}
	}
	catch (throwable::failure& _e)
	{
		if (preceivable.exist())
		{
			this->process_close_updatable(preceivable, _e.reason());
		}

		this->send(_message_UPDATE_UPDATABLE_END(_work.sender_id, _work.receiver_id, _work.updatable_id, _e.reason(), CGDK::MD5()));
	}
	catch (...)
	{
		if (preceivable.exist())
		{
			this->process_close_updatable(preceivable, eRESULT::EXCEPTION);
		}

		this->send(_message_UPDATE_UPDATABLE_END(_work.sender_id, _work.receiver_id, _work.updatable_id, eRESULT::EXCEPTION, CGDK::MD5()));
	}
}

void CGDK::update::Nreceiver::process_work_updatable_block(sWORK& _work)
{
	// declare)
	object_ptr<update::Ireceivable> preceivable;

	try
	{
		// 1) get updatable (an updatable closed by an earlier error is gone: its remaining blocks are dropped)
		SCOPED_LOCK(this->m_lockable_map_updatable)
		{
			auto iter_find = this->m_map_updatable_receive.find(_work.updatable_id);

			RETURN_IF(iter_find == this->m_map_updatable_receive.end());

			preceivable = iter_find->second;
		}

		// 2) block data
		shared_buffer buf = _work.buf_data;

		// 3) de-compress
		if ((_work.meta_file_block.flag & resource::eOPTION::BLOCK_COMPRESSED) == resource::eOPTION::BLOCK_COMPRESSED)
		{
			buf = CGDK::compress::unzip(buf);
		}

		// 4) generate md5
		preceivable->m_md5_check.push_md5(buf.data(), buf.size<uint32_t>());

		// 5) call 'on_updatable_process'
		this->on_updatable_process(preceivable, _work.meta_file_block);

		// 6) write
		preceivable->process_process(_work.meta_file_block, buf);

		// 7) check complete
		if (preceivable->m_meta_info.size <= (_work.meta_file_block.pos + _work.meta_file_block.size))
		{
			// check)
			CGASSERT_ERROR(preceivable->m_meta_info.size == (_work.meta_file_block.pos + _work.meta_file_block.size));

			// - md5 padding
			preceivable->m_md5_check.padding();

			// - send UPDATABLE_END message
			this->send(_message_UPDATE_UPDATABLE_END(_work.sender_id, _work.receiver_id, _work.updatable_id, eRESULT::SUCCESS, preceivable->m_md5_check.get_md5()));
		}
	}
	catch (throwable::failure& _e)
	{
		if (preceivable.exist())
		{
			this->process_close_updatable(preceivable, _e.reason());
		}

		this->send(_message_UPDATE_UPDATABLE_END(_work.sender_id, _work.receiver_id, _work.updatable_id, _e.reason(), CGDK::MD5()));
	}
	catch (...)
	{
		if (preceivable.exist())
		{
			this->process_close_updatable(preceivable, eRESULT::EXCEPTION);
		}

		this->send(_message_UPDATE_UPDATABLE_END(_work.sender_id, _work.receiver_id, _work.updatable_id, eRESULT::EXCEPTION, CGDK::MD5()));
	}
}

void CGDK::update::Nreceiver::process_work_updatable_close(sWORK& _work)
{
	// declare)
	CGDK::object_ptr<CGDK::update::Ireceivable> pupdatable;

	// 1) take the updatable out of the map
	SCOPED_LOCK(this->m_lockable_map_updatable)
	{
		auto iter_find = this->m_map_updatable_receive.find(_work.updatable_id);

		// check) already closed by an error
		RETURN_IF(iter_find == this->m_map_updatable_receive.end());

		pupdatable = std::move(iter_find->second);
		this->m_map_updatable_receive.erase(iter_find);
	}

	// check)
	RETURN_IF(pupdatable.empty());

	// statistics)
	if (_work.result == eRESULT::SUCCESS)
	{
		++this->m_total_processed_files;
		++this->m_total_transfered_files;
	}

	// 2) close updatable
	this->process_close_updatable(pupdatable, _work.result);
}

// ----------------------------------------------------------------------------
// updatable map
// ----------------------------------------------------------------------------
void CGDK::update::Nreceiver::register_updatable(update::Ireceivable* _preceivable)
{
	// check)
	CGASSERT_FATAL(_preceivable != nullptr);

	// 1) get meta info
	CGDK::resource::sUPDATABLE_FILE_META_INFO& file_meta_info = _preceivable->m_meta_info;

	// check)
	CGASSERT_ERROR(file_meta_info.id != 0);

	// check)
	CGASSERT_ERROR(file_meta_info.filename.empty() == false);

	// 2) relative path of the file below the root (path and filename are UTF-8 and come from the network)
	std::filesystem::path relative_path = (std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t*>(file_meta_info.path.data()), file_meta_info.path.size()))
										 / std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t*>(file_meta_info.filename.data()), file_meta_info.filename.size()))).lexically_normal();

	// check) the file must stay below the root path
	THROW_IF(relative_path.empty() || relative_path.is_absolute() || relative_path.has_root_name() || relative_path.has_root_directory(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "invalid updatable path"sv));

	for (const auto& iter : relative_path)
	{
		THROW_IF(iter == "..", throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "updatable path escapes the root path"sv));
	}

	// 3) make fullpath
	std::filesystem::path full_file_path;

	if (this->have_root_path())
	{
		const auto root_path = this->get_root_path();
		full_file_path = std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t*>(root_path.data()), root_path.size()));
	}

	full_file_path /= relative_path;

	// 4) create directory
	if (full_file_path.has_parent_path())
	{
		std::filesystem::create_directories(full_file_path.parent_path());
	}

	// 5) initialize updatable
	SCOPED_LOCK(_preceivable->m_lockable_info)
	{
		_preceivable->set_full_file_path(full_file_path.string());
	}

	SCOPED_LOCK(this->m_lockable_map_updatable)
	{
		// - is it already registered?
		auto iter_updatable = this->m_map_updatable_receive.find(file_meta_info.id);

		// check)
		THROW_IF(iter_updatable != this->m_map_updatable_receive.end(), throwable::failure(eRESULT::FAIL, 0, ""sv))

		// - insert
		this->m_map_updatable_receive.insert({ file_meta_info.id, _preceivable });
	}
}

CGDK::object_ptr<CGDK::update::Ireceivable> CGDK::update::Nreceiver::find_updatable(UPDATABLE_ID _id)
{
	// declare)
	CGDK::object_ptr<CGDK::update::Ireceivable> preceivable;

	SCOPED_LOCK(this->m_lockable_map_updatable)
	{
		// 1) find ...
		auto iter_updatable = this->m_map_updatable_receive.find(_id);

		// check)
		THROW_IF(iter_updatable == this->m_map_updatable_receive.end(), throwable::failure(eRESULT::FAIL, 0, ""sv))

		// 2) get
		preceivable = iter_updatable->second;
	}

	// return)
	return preceivable;
}

void CGDK::update::Nreceiver::unregister_updatable(UPDATABLE_ID _id) noexcept
{
	SCOPED_LOCK(this->m_lockable_map_updatable)
	{
		this->m_map_updatable_receive.erase(_id);
	}
}

void CGDK::update::Nreceiver::cancel_all_updatable() noexcept
{
	// declare)
	std::map<UPDATABLE_ID, object_ptr<update::Ireceivable>> map_receivable;

	// 1) get all
	SCOPED_LOCK(this->m_lockable_map_updatable)
	{
		map_receivable = std::move(this->m_map_updatable_receive);
	}

	// 2) close all
	for (auto& iter : map_receivable)
	{
		this->process_close_updatable(iter.second, eRESULT::CANCEL);
	}
}

void CGDK::update::Nreceiver::process_close_updatable(update::Ireceivable* _preceivable, eRESULT _result) noexcept
{
	// check)
	CGASSERT_FATAL(_preceivable != nullptr);

	// 1) call 'on_updatable_end'
	this->on_updatable_end(_preceivable, _result);

	// 2) close file
	_preceivable->process_end();

	// 3) cache the check code of a completely received file
	if (_preceivable->m_md5_check.get_md5().exist())
	{
		// - get caching
		object_ptr<md5_caching> pmd5_caching = m_pmd5_caching;

		if (pmd5_caching.exist())
		{
			sUPDATABLE_PREPARE_INFO temp_prepare_info;
			temp_prepare_info.size = _preceivable->m_meta_info.size;
			temp_prepare_info.check_code = _preceivable->m_md5_check.get_md5();
			temp_prepare_info.result = eRESULT::DONE;

			pmd5_caching->put_prepare_info(_preceivable->m_full_file_path, temp_prepare_info);
		}
	}

	// 4) remove updatable
	this->unregister_updatable(_preceivable->m_meta_info.id);
}
