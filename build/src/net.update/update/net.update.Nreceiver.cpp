#include "pch.h"


CGDK::update::Nreceiver::Nreceiver()
{
}

CGDK::update::Nreceiver::~Nreceiver() noexcept
{
	// 1) self destruction
	this->destroy();
}

void CGDK::update::Nreceiver::set_root_path(std::string_view _root_path)
{
	m_path_root = _root_path;
}

void CGDK::update::Nreceiver::set_root_path(std::string&& _root_path) noexcept
{
	m_path_root = std::move(_root_path);
}

CGDK::result_code CGDK::update::Nreceiver::_process_starting(context& _context)
{
	// 1) on_starting 함수를 호출한다.
	this->on_starting(_context);

	// trace)
	LOG_PROGRESS << "@ <BEGIN> start UPDATE_RECEIVER['"sv << /*this->name() <<*/ "']"sv;

	// declare)
	object_ptr<resource::Iupdatable> request_now;

	try
	{
		this->m_total_processed_bytes = 0;
		this->m_total_processed_files = 0;
		this->m_total_transfered_bytes = 0;
		this->m_total_transfered_files = 0;

		this->m_period_transfered_bytes = 0;
		this->m_period_transfered_bytes_per_sec = 0.0f;
		this->m_period_tick_last = chrono::time::utc::now();

	}
	catch (std::exception& /*e*/)
	{
		// log) prepare receive에 실패했다!
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
		// 1) on_stopping 함수를 호출한다.
		this->on_stopping();

		// 2) close all sendable
		cancel_all_updatable();
	}
	catch (...)
	{
		// log) 
		LOG_ERROR << "@ (excp) exception on 'on_stopping' function call ACCEPTOR '"sv << /*name() <<*/ "' " __CGFUNCTION__ ""sv;
	}

	// return)
	return eRESULT::SUCCESS;
}

CGDK::result_code CGDK::update::Nreceiver::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) execute messge
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

CGDK::result_code CGDK::update::Nreceiver::on_message_UPDATE_BEGIN_REQUEST(sMESSAGE_NETWORK& _msg)
{
	// ------------------------------------------
	// 설명) update request
	//       - server에서 update 시작을 요청할때 전송하는 메시지다.
	//       - 이때 CGDK::update::sUPDATE_REQUEST_INFO를 통해 전체 update 정보를 전달한다.
	//       - 전체 update할 파일과 update할 용량이 전송되어 온다.
	//         하지만 실제 전송은 팔이 동일한 것이 판단되면 전송하지 않으므로 훨씬 적게 될수 있다.
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

		// 5) ...
		scoped_lock(this->m_cs_map_updatable)
		{
			// - set total update info
			this->m_update_info = update_request_info;
			this->m_receiver_id = receiver_id;

			// - map info
			this->m_map_updatable_receive.clear();
		}

		// 6) get begin time
		this->m_total_tick_begin = chrono::time::utc::now();

		// 7) ...
		on_update_begin(update_request_info);

		// 8) send response info
		this->send(_message_UPDATE_BEGIN_RESPONSE(receiver_id, sender_id, eRESULT::SUCCESS));
	}
	catch (throwable::failure& _e)
	{
		this->send(_message_UPDATE_BEGIN_RESPONSE(receiver_id, sender_id, _e.reason()));
	}
	catch (...)
	{
		this->send(_message_UPDATE_BEGIN_RESPONSE(receiver_id, sender_id, eRESULT::EXCEPTION));
	}

	// return)
	return eRESULT::DONE;
}

CGDK::result_code CGDK::update::Nreceiver::on_message_UPDATE_END(sMESSAGE_NETWORK& _msg)
{
	// ------------------------------------------
	// 설명) update end
	//       - server에서 update 종를 할 때 전송하는 메시지다.
	//       - 이 메시지가 오면 일단의 update는 완전 종료된 것이다.
	//       - 또 전송의 성공과 실패 여부가 전달된다.
	//       - 클라이언트는 이 메시지 수신 후 receiver를 종료하면 된다.
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
		/*auto sender_id =*/ buf.extract<CGDK::update::SENDER_ID>();

		// 4) get result
		auto result = buf.extract<eRESULT>();

		// 5) 
		m_total_tick_end = chrono::time::utc::now();

		// 6) on complete
		on_update_end(result);

		// 7) cancel all updatable
		cancel_all_updatable();
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
	// 설명) updatable request
	//       - 본 update를 시작하기전 사전 정보를 얻는 과정이다.
	//       - file의 길이와 md5를 되돌려 준다.
	//       - 업데이트하려는 파일과 크기가 같을 경우에만 md5를 전달하면 
	//         아닐 경우 그냥 empty값을 전달한다.
	// ------------------------------------------

	// 1) buffer for message extract
	auto buf = _msg.buf_message + offset(8);

	// 2) get receiver_id
	auto receiver_id = buf.extract<CGDK::update::RECEIVER_ID>();

	// check) 
	RETURN_IF(receiver_id != this->m_receiver_id, eRESULT::BYPASS);

	// 3) get sender_id
	auto sender_id = buf.extract<CGDK::update::SENDER_ID>();

	// 4) read update_request_info
	auto file_meta_info = buf.extract<CGDK::resource::sUPDATABLE_FILE_META_INFO>();

	// check) 
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY, 0, "receiver is not ready (call 'start')"sv));

	// declare) 
	object_ptr<update::Ireceivable> preceivable;

	try
	{
		// 1) create new updatable
		preceivable = make_object<update::receivable::file>();

		// 2) set file meta info
		preceivable->m_meta_info = file_meta_info;

		// 3) call 'on_updatable_requested'
		this->on_updatable_requested(preceivable);

		// 4) add updatable
		this->register_updatable(preceivable);

		// 5) md5 caching 객체를 얻는다.
		object_ptr<md5_caching> pmd5_caching = m_pmd5_caching;

		// declare) 
		sUPDATABLE_PREPARE_INFO updatable_prepare_info;

		// 6) 캐싱된 prepare info를 먼저 확인한다.(try to get cached 'prepare_info')
		if (pmd5_caching.exist())
		{
			updatable_prepare_info = pmd5_caching->get_prepare_info(preceivable->m_full_file_path);
		}

		// 7) 캐싱된 것이 없으면 직접 구한다.
		if (updatable_prepare_info.check_code.empty())
		{
			// - 파일의 sUPDATABLE_FILE_META_INFO 값을 새로 계산한다.
			updatable_prepare_info = preceivable->process_prepare(preceivable->m_full_file_path);

			// - 신규 생성한 파일의 md5를 캐싱한다.
			if (updatable_prepare_info.check_code.empty())
			{
				pmd5_caching->put_prepare_info(preceivable->m_full_file_path, updatable_prepare_info);
			}
		}

		// 8) send response message
		this->send(_message_UPDATE_UPDATABLE_RESPONSE(sender_id, receiver_id, file_meta_info.id, updatable_prepare_info));
	}
	catch (throwable::failure& _e)
	{
		if (preceivable.exist())
		{
			this->on_updatable_end(preceivable, _e.reason());
		}

		this->send(_message_UPDATE_UPDATABLE_RESPONSE(sender_id, receiver_id, file_meta_info.id, CGDK::sUPDATABLE_PREPARE_INFO{0, MD5(), _e.reason()}));
	}
	catch (...)
	{
		if (preceivable.exist())
		{
			this->on_updatable_end(preceivable, eRESULT::EXCEPTION);
		}

		this->send(_message_UPDATE_UPDATABLE_RESPONSE(sender_id, receiver_id, file_meta_info.id, CGDK::sUPDATABLE_PREPARE_INFO(0, MD5(), eRESULT::EXCEPTION)));
	}

	// return)
	return eRESULT::DONE;
}

CGDK::result_code CGDK::update::Nreceiver::on_message_UPDATE_UPDATABLE_BEGIN(sMESSAGE_NETWORK& _msg)
{
	// ------------------------------------------
	// 설명) updatable begin
	//       - file의 전송을 시작함으로 알리는 메시지다.
	//       - UPDATABLE_REQUEST에서 전송 여부가 결정이 되며 
	//         그 결과에 따라 전송의 시작위치가 결정된다.
	//       - 만약 client에 이미 전송된 파일이라면
	//         이 메시지의 전송후 바로 UPDATABLE_END 메시지를 전송한다.
	//         이때 MD5 정보를 server에 전달하며 server에서는 md5를 검사 후
	//         재전송할 것인지 다음 updatable로 진행할 것인지를 결정한다.
	// ------------------------------------------

	// 1) buffer for message extract
	auto buf = _msg.buf_message + offset(8);

	// 2) get receiver_id
	auto receiver_id = buf.extract<CGDK::update::RECEIVER_ID>();

	// check) 
	RETURN_IF(receiver_id != this->m_receiver_id, eRESULT::BYPASS);

	// 3) get sender_id
	auto sender_id = buf.extract<CGDK::update::SENDER_ID>();

	// declare) 
	object_ptr<update::Ireceivable> preceivable;

	// check) 
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY, 0, "receiver is not ready (call 'start')"sv));

	// 4) get updatable_id
	auto updatable_id = buf.extract<CGDK::update::UPDATABLE_ID>();

	try
	{
		// 5) get updatable
		preceivable = this->find_updatable(updatable_id);

		// check)
		CGASSERT_ERROR(preceivable.exist())

		// check)
		THROW_IF(preceivable.empty(), throwable::failure(eRESULT::NOT_READY, 0, "receiver not ready')"sv));

		// 6) get start pos
		auto pos = buf.extract<uint64_t>();

		// 7) call 'on_updatable_begin'
		this->on_updatable_begin(preceivable, pos);

		// 8) process begin
		preceivable->process_begin(pos);
	}
	catch (throwable::failure& _e)
	{
		// - close receivable
		this->process_close_updatable(preceivable, _e.reason());

		// - send messaage
		this->send(_message_UPDATE_UPDATABLE_END(sender_id, receiver_id, preceivable->m_meta_info.id, _e.reason(), CGDK::MD5()));
	}
	catch (...)
	{
		// - close receivable
		this->process_close_updatable(preceivable, eRESULT::EXCEPTION);

		// - send messaage
		this->send(_message_UPDATE_UPDATABLE_END(sender_id, receiver_id, preceivable->m_meta_info.id, eRESULT::EXCEPTION, CGDK::MD5()));
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

	// declare) 
	object_ptr<update::Ireceivable> preceivable;

	// check)
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY, 0, "receiver is not ready (call 'start')"sv));

	// 3) get sender_id
	auto sender_id = buf.extract<CGDK::update::SENDER_ID>();

	try
	{ 
		// 4) read update_request_info
		auto meta_file_block = buf.extract<CGDK::resource::sMETA_FILE_BLOCK>();

		// 5) get updatable
		preceivable = this->find_updatable(meta_file_block.id);

		// check)
		THROW_IF(preceivable.empty(), throwable::failure(eRESULT::FAIL, 0, ""sv));

		// statistics)
		this->m_total_processed_bytes += meta_file_block.size;
		this->m_total_transfered_bytes += meta_file_block.size;
		this->m_period_transfered_bytes += meta_file_block.size;

		// statistics) 
		auto tick_now = chrono::time::utc::now();
		auto f_duration = static_cast<float>(std::chrono::duration_cast<chrono::milliseconds>(tick_now - this->m_period_tick_last).count());
		if (f_duration > 1000.0f)
		{
			this->m_period_transfered_bytes_per_sec = (f_duration != 0.0f) ? (static_cast<float>(this->m_period_transfered_bytes) / f_duration) * 1000.0f : 0.0f;
			this->m_period_transfered_bytes = 0;
			this->m_period_tick_last = tick_now;
		}

		// 6) de-compress
		if ((meta_file_block.flag & resource::eOPTION::BLOCK_COMPRESSED) == resource::eOPTION::BLOCK_COMPRESSED)
		{
			buf = CGDK::compress::unzip(buf);
		}

		// 7) generate md5
		preceivable->m_md5_check.push_md5(buf.data(), buf.size<uint32_t>());

		// 8) call 'on_updatable_
		on_updatable_process(preceivable, meta_file_block);

		// 9) process update
		preceivable->process_process(meta_file_block, buf);

		// 10) check complete
		if (preceivable->m_meta_info.size <= (meta_file_block.pos + meta_file_block.size))
		{
			// check)
			CGASSERT_ERROR(preceivable->m_meta_info.size == (meta_file_block.pos + meta_file_block.size));

			// - md5 padding
			preceivable->m_md5_check.padding();

			// - send...UPDATABPE END message
			this->send(_message_UPDATE_UPDATABLE_END(sender_id, receiver_id, meta_file_block.id, eRESULT::SUCCESS, preceivable->m_md5_check.get_md5()));
		}
	}
	catch (throwable::failure& _e)
	{
		// - close receivable
		this->process_close_updatable(preceivable, _e.reason());

		// - send messaage
		this->send(_message_UPDATE_UPDATABLE_END(sender_id, receiver_id, preceivable->m_meta_info.id, _e.reason(), CGDK::MD5()));
	}
	catch (...)
	{
		// - close receivable
		this->process_close_updatable(preceivable, eRESULT::EXCEPTION);

		// - send messaage
		this->send(_message_UPDATE_UPDATABLE_END(sender_id, receiver_id, preceivable->m_meta_info.id, eRESULT::EXCEPTION, CGDK::MD5()));
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

	// check)
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY, 0, "receiver is not ready (call 'start')"sv));

	// 3) get sender_id
	/*auto sender_id =*/ buf.extract<CGDK::update::SENDER_ID>();

	// 3) get updatable id
	auto updatable_id = buf.extract<CGDK::update::UPDATABLE_ID>();

	try
	{
		// declare) 
		CGDK::object_ptr<CGDK::update::Ireceivable> pupdatable;

		// 4) get updatable
		scoped_lock(this->m_cs_map_updatable)
		{
			// - find updatable
			auto iter_find = this->m_map_updatable_receive.find(updatable_id);

			// check) 
			RETURN_IF(iter_find == this->m_map_updatable_receive.end(), eRESULT::DONE);

			// - get updatable
			pupdatable = std::move(iter_find->second);

			// - erase
			this->m_map_updatable_receive.erase(iter_find);
		}

		// check)
		CGASSERT_ERROR(pupdatable.exist());

		// check)
		THROW_IF(pupdatable.empty(), throwable::failure(eRESULT::NOT_EXIST, 0, ""sv));

		// 5) get updatable id
		auto result = buf.extract<eRESULT>();

		if (result == eRESULT::SUCCESS)
		{
			// statistics) 
			++this->m_total_processed_files;
			++this->m_total_transfered_files;
		}

		// 6) close updatable;
		this->process_close_updatable(pupdatable, result);
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	// return) 
	return eRESULT::DONE;
}

void CGDK::update::Nreceiver::register_updatable(update::Ireceivable* _preceivable)
{
	// check)
	CGASSERT_ERROR(_preceivable != nullptr);

	// 1) get meta info
	CGDK::resource::sUPDATABLE_FILE_META_INFO& file_meta_info = _preceivable->m_meta_info;

	// check)
	CGASSERT_ERROR(file_meta_info.id != 0);

	// check)
	CGASSERT_ERROR(file_meta_info.filename.empty() == false);

	// declare) 
	std::filesystem::path full_file_path;

	// 1) make fullpath
#if _WIN32
	{
		// declare) 
		wchar_t temp_dest[256];

		// - root path
		if (this->have_root_path())
		{

			full_file_path += encode_string(temp_dest, reinterpret_cast<const char8_t*>(this->get_root_path().c_str()));
			full_file_path += "/";
		}
		// - append path
		if (file_meta_info.path.empty() == false)
		{
			full_file_path += encode_string(temp_dest, reinterpret_cast<const char8_t*>(file_meta_info.path.c_str()));
			full_file_path += "/";
		}

		// - create directory
		if (full_file_path.empty() == false)
		{
			std::filesystem::create_directories(full_file_path);
		}
		
		// - make full path
		full_file_path += encode_string(temp_dest, reinterpret_cast<const char8_t*>(file_meta_info.filename.c_str()));
	}
#else
	{
		// - root path
		if (this->have_root_path())
		{
			full_file_path.append(this->get_root_path());
			full_file_path += "/";
		}
		// - append path
		if (file_meta_info.path.empty() == false)
		{
			full_file_path.append(file_meta_info.path);
			full_file_path += "/";
		}

		// - create directory
		if (full_file_path.empty() == false)
		{
			std::filesystem::create_directories(full_file_path);
		}

		// - make full path
		full_file_path += file_meta_info.filename;
	}
#endif
	// 2) initialize updatable
	scoped_lock(_preceivable->m_cs_info)
	{
		_preceivable->set_full_file_path(full_file_path.string());
	}

	scoped_lock(this->m_cs_map_updatable)
	{
		// 1) is aleady exist?
		auto iter_updatable = this->m_map_updatable_receive.find(file_meta_info.id);

		// check) throw exception if aleady exist??
		THROW_IF(iter_updatable != this->m_map_updatable_receive.end(), throwable::failure(eRESULT::FAIL, 0, ""sv))

		// 3) insert
		this->m_map_updatable_receive.insert({ file_meta_info.id, _preceivable });
	}
}

CGDK::object_ptr<CGDK::update::Ireceivable> CGDK::update::Nreceiver::find_updatable(UPDATABLE_ID _id)
{
	// declare) 
	CGDK::object_ptr<CGDK::update::Ireceivable> preceivable;

	scoped_lock(this->m_cs_map_updatable)
	{
		// 1) find ...
		auto iter_updatable = this->m_map_updatable_receive.find(_id);

		// check)
		THROW_IF(iter_updatable == this->m_map_updatable_receive.end(), throwable::failure(eRESULT::FAIL, 0, ""sv))

		// return) 
			preceivable = iter_updatable->second;
	}

	// return)
	return preceivable;
}

void CGDK::update::Nreceiver::unregister_updatable(UPDATABLE_ID _id) noexcept
{
	scoped_lock(this->m_cs_map_updatable)
	{
		this->m_map_updatable_receive.erase(_id);
	}
}

void CGDK::update::Nreceiver::cancel_all_updatable() noexcept
{
	// declare0 
	std::map<UPDATABLE_ID, object_ptr<update::Ireceivable>> map_receivable;

	// 1) get all
	scoped_lock(this->m_cs_map_updatable)
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
	CGASSERT_ERROR(_preceivable != nullptr);

	// 1) call 'on_updatable_end'
	this->on_updatable_end(_preceivable, _result);

	// 2) close file
	_preceivable->process_end();

	// 3) cache updata
	if(_preceivable->m_md5_check.get_md5().exist())
	{
		// - check from caching
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
	
	// 3) remove updatable
	this->unregister_updatable(_preceivable->m_meta_info.id);
}
