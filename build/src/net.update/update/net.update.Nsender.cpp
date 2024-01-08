#include "pch.h"


CGDK::update::Nsender::Nsender()
{
}

CGDK::update::Nsender::~Nsender() noexcept
{
}

void CGDK::update::Nsender::generate_and_set_sender_id() noexcept
{
	// static)
	static uint64_t s_receiver_id = 1;

	// 1) generate and set
	m_receiver_id = s_receiver_id++;
}

bool CGDK::update::Nsender::add(const object_ptr<resource::Iupdatable>& _resource_updatable)
{
	// 1) get meta info
	const auto& meta_info = _resource_updatable->_get_meta_info();

	// check)
	THROW_IF(meta_info.id == 0, throwable::failure(eRESULT::INVALID_PARAMETER_HANDLE, 0, ""sv));

	// 2) file or folder
	auto presource_updatable_folder = dynamic_pointer_cast<resource::updatable::folder>(_resource_updatable);

	if (presource_updatable_folder.exist())
	{
		// declare) 
		std::vector<object_ptr<resource::Iupdatable>> vector_updatable_process;

		// - copy
		scoped_lock(presource_updatable_folder->m_cs_info)
		{
			vector_updatable_process = presource_updatable_folder->m_vector_updatable_process;
		}

		for (auto& iter : vector_updatable_process)
		{
			this->add(iter);
		}
	}
	else
	{
		// - 추가한다.
		scoped_lock(this->m_cs_vector_updatable)
		{
			process_add_updatable(object_ptr<resource::Iupdatable>(_resource_updatable));
		}
	}

	// return)
	return true;
}

bool CGDK::update::Nsender::add(object_ptr<resource::Iupdatable>&& _resource_updatable)
{
	// 1) get meta info
	const auto& meta_info = _resource_updatable->_get_meta_info();

	// check)
	THROW_IF(meta_info.id == 0, throwable::failure(eRESULT::INVALID_PARAMETER_HANDLE, 0, ""sv));

	// 2) file or folder
	auto presource_updatable_folder = dynamic_pointer_cast<resource::updatable::folder>(_resource_updatable);

	if (presource_updatable_folder.exist())
	{
		// declare) 
		std::vector<object_ptr<resource::Iupdatable>> vector_updatable_process;

		// - copy
		scoped_lock(presource_updatable_folder->m_cs_info)
		{
			vector_updatable_process = presource_updatable_folder->m_vector_updatable_process;
		}

		for (auto& iter : vector_updatable_process)
		{
			this->add(std::move(iter));
		}

		// - reset
		_resource_updatable.reset();
	}
	else
	{
		// - 추가한다.
		scoped_lock(this->m_cs_vector_updatable)
		{
			process_add_updatable(std::move(_resource_updatable));
		}
	}

	// return)
	return true;
}

void CGDK::update::Nsender::clear() noexcept
{
	scoped_lock(this->m_cs_vector_updatable)
	{
		this->m_list_updatable_process.clear();
	}
}

CGDK::result_code CGDK::update::Nsender::_process_starting(context& _context)
{
	// 1) on_starting 함수를 호출한다.
	this->on_starting(_context);

	// trace)
	LOG_PROGRESS << "@ <BEGIN> start UPDATE_SENDER['"sv << /*this->name() <<*/ "']"sv;

	try
	{
		// declare)
		sUPDATE_REQUEST_INFO temp_update_request_info;

		// 2) get first resource
		scoped_lock(this->m_cs_vector_updatable)
		{
			// check)
			THROW_IF(this->m_list_updatable_process.empty(), throwable::failure(eRESULT::ALEADY_DONE, 0, "updatable file is empty!!"sv));

			// 3) calculate totals 
			for (auto& iter : this->m_list_updatable_process)
			{
				const auto& meta_info = iter->_get_meta_info();

				temp_update_request_info.total_count++;
				temp_update_request_info.total_bytes += meta_info.size;
			}

			// 4) reset
			this->m_bytes_now = 0;
			this->m_referenceable_send_completor.psender = this;

			// statistics)
			this->m_total_required_bytes = 0;
			this->m_total_required_files = 0;
			this->m_total_processed_bytes = 0;
			this->m_total_processed_files = 0;
			this->m_total_transfered_bytes = 0;
			this->m_total_transfered_files = 0;
			this->m_total_tick_begin = chrono::time::utc::now();

			this->m_period_transfered_bytes = 0;
			this->m_period_transfered_bytes_per_sec = 0.0f;
			this->m_period_tick_last = chrono::time::utc::now();
		}

		// 5) ...
		on_update_begin(temp_update_request_info);

		// 6) start resource
		this->send(_message_UPDATE_BEGIN_REQUEST(this->m_receiver_id, this->m_sender_id, temp_update_request_info));
	}
	catch (std::exception& /*e*/)
	{
		// log) prepare receive에 실패했다!
		LOG_ERROR << "@ (excp) CGDK::update::sender - exception complete process_start:'" << /*name() <<*/ __CGFUNCTION__;

		// reraise)
		throw;
	}

	// return)
	return eRESULT::SUCCESS;
}

CGDK::result_code CGDK::update::Nsender::_process_stopping() noexcept
{
	// trace)
	LOG_PROGRESS << "@ <BEGIN> stop UPDATE_SENDER['"sv << /*name() <<*/ "']"sv;

	try
	{
		// 1) on_stopping 함수를 호출한다.
		this->on_stopping();

		// 2) clear infos
		this->m_referenceable_send_completor.psender.reset();
	}
	catch (...)
	{
		// log) 
		LOG_ERROR << "@ (excp) exception on 'on_stopping' function call ACCEPTOR '"sv << /*name() <<*/ "' " __CGFUNCTION__ ""sv;
	}

	// return)
	return eRESULT::SUCCESS;
}

void CGDK::update::Nsender::process_add_updatable(object_ptr<resource::Iupdatable>&& _updatable)
{
	scoped_lock(this->m_cs_vector_updatable)
	{
		// 1) 이미 등록되어 있는지 확인한다.
		auto iter_find = std::find(this->m_list_updatable_process.begin(), m_list_updatable_process.end(), _updatable);

		// 2) 등록되어 있다면 종료
		RETURN_IF(iter_find != this->m_list_updatable_process.end(), );

		// 3) push_back
		this->m_list_updatable_process.push_back(std::move(_updatable));
	}
}

void CGDK::update::Nsender::process_updatable_prepare()
{
	// ------------------------------------------
	// 설명) prepare upgradable 
	//       - 처리할 upgradable이 존재하는지 확인해 본다.
	//       - 처리할 upgradable이 존재하면 해당 upgradable의 UPDATABLE_PREPARE 메세지를 클라이언트에 전송한다.
	//       - 처리할 upgradable이 없는 경우 전체 upgrade process를 종료한다.
	// ------------------------------------------
	try
	{
		// declare) 
		object_ptr<resource::Iupdatable> pupdatable;

		// 1) get first updatable
		scoped_lock(this->m_cs_vector_updatable)
		{
			if (this->m_list_updatable_process.empty() == false)
			{
				// - get front
				pupdatable = this->m_list_updatable_process.front();
			}
		}

		// 2-1) updatable exist => go ahead
		if (pupdatable.exist())
		{
			// - get meta info
			auto meta_info = pupdatable->_get_meta_info();

			// 3) reset retry
			this->m_count_try = 0;

			// - send updatable begin message
			auto is_result = this->send(_message_UPDATE_UPDATABLE_REQUEST(this->m_receiver_id, this->m_sender_id, meta_info));

			// check) 
			THROW_IF(is_result == false, throwable::failure(eRESULT::NOT_READY, 0, ""sv));
		}
		// 2-2) updatable is empty => update is ended
		else
		{
			this->process_update_end(eRESULT::DONE);
		}
	}
	catch (throwable::failure& _e)
	{
		this->process_update_end(_e.reason());
	}
	catch (...)
	{
		this->process_update_end(eRESULT::EXCEPTION);
	}
}

void CGDK::update::Nsender::process_updatable_begin(resource::Iupdatable* _pupdatable, CGDK::update::UPDATABLE_ID _updatable_id, uint64_t _pos_begin)
{
	// ------------------------------------------
	// 설명) start upgradable 
	//       - upgradable의 전송 시작할 위치를 _pos_begin으로 전달 받아 설졍한다.
	//       - 첫번째 전송일 경우 'on_updatable_begin'를 호출한 후 UPDATABLE_BEGIN를 전송한다.
	//       - 재전송일 경우 'on_updatable_begin'를 호출하지 않으며 flag에 RECREATE 옵션을 붙혀 UPDATABLE_BEGIN 메시지를 전송한다.
	// ------------------------------------------
	// check)
	CGASSERT_ERROR(_pupdatable != nullptr);

	// 1) set pos_begin
	this->m_bytes_now = _pos_begin;

	// 2) get meta info
	auto meta_info = _pupdatable->_get_meta_info();

	// 3) first-try or re-try?
	if (m_count_try == 0)
	{
		// - call 'on_updatable_begin'()'
		this->on_updatable_begin(_pupdatable, meta_info);
	}
	else
	{
		// - set option
		meta_info.flag = meta_info.flag | resource::eFILE_FLAG::RECREATE;
	}

	// 4) set updatable
	this->m_referenceable_send_completor.pupdatable = _pupdatable;

	// 5) send updatable begin message
	auto is_result = this->send(_message_UPDATE_UPDATABLE_BEGIN(this->m_receiver_id, this->m_sender_id, _updatable_id, _pos_begin));

	// check) 
	THROW_IF(is_result == false, throwable::failure(eRESULT::NOT_READY, 0, ""sv));

	// 6) send block first
	process_updatable_next(_pupdatable);
}

void CGDK::update::Nsender::process_updatable_next(resource::Iupdatable* _pupdatable)
{
	// ------------------------------------------
	// 설명) process upgradable 
	//       - upgradable의 다음 fle block을 전송한다.
	//       - 전송한 만큼 m_bytes_now 값을 증가시킨다.
	//       - 전송에 따른 통계처리를 한다.
	//       - 더 이상 전송할 fle block이 존재하지 않는 경우 그냥 리턴한다.
	//       - buf_block[0]에는 메시지 헤더 정보를 넣는다.
	//       - buf_block[1]에는 file block을 넣는다.
	//       - 전송에 실패하면 updata_end를 효출하며 처리를 종료한다.
	// ------------------------------------------
	// check)
	CGASSERT_ERROR(this->m_bytes_now <= _pupdatable->_get_meta_info().size);

	// check)
	THROW_IF(this->m_bytes_now > _pupdatable->_get_meta_info().size, throwable::failure(eRESULT::INVALID_REQUEST, 0, ""sv));

	// 1) get next block
	auto buf_next = _pupdatable->get_block(this->m_bytes_now);

	// check) return of end of file
	RETURN_IF(buf_next.empty());

	// check) 
	CGASSERT_ERROR(buf_next.exist());

	// check) return of end of file
	THROW_IF(buf_next.empty(), throwable::failure(eRESULT::RUNTIME_ERROR, 0, ""sv));

	// declare) 
	shared_buffer buf_blocks[2];

	// 2) make message head and context
	buf_blocks[0] = _message_UPDATE_UPDATABLE_BLOCK(this->m_receiver_id, this->m_sender_id, buf_next.size<uint32_t>());
	buf_blocks[1] = std::move(buf_next);
	m_referenceable_send_completor.buf_original = buf_blocks[1].move_out_source();
	buf_blocks[1].set_source(&m_referenceable_send_completor);

	// 3) get metadata
	const auto& meta_file_block = buf_blocks[1].front<resource::sMETA_FILE_BLOCK>();

	// 4) get meta size
	this->m_bytes_now += meta_file_block.size;

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

	// 5) on_updatable_process
	this->on_updatable_process(_pupdatable, meta_file_block);

	// 6) send block
	auto result = this->send(buf_blocks);

	// check) 
	if (result == false)
	{
		//throw throwable::failure(eRESULT::NOT_READY, 0, ""sv));
	}
}

void CGDK::update::Nsender::process_updatable_end(resource::Iupdatable* _pupdatable, CGDK::update::UPDATABLE_ID _updatable_id, eRESULT _result)
{
	// ------------------------------------------
	// 설명) end upgradable 
	//       - upgradable의 종료 처리를 하고 다음 작업을 시작한다.
	//       - 성공적 전송 (eRESULT::SUCCESS)
	//         다음 updatable을 시작한다.
	//         다음 updatable이 없을 경우 완전 종료 처리한다.
	//       - 전송은 끝났으나 파일 check code가 다를 경우 (eRESULT::INVALID_CHECK_KEY)
	//         파일을 재전송 시도한다.
	//       - 그냥 실패한 경우
	//         파일을 재전송 시도한다.
	// ------------------------------------------
	// 1-1) on success
	if(_result == eRESULT::SUCCESS)
	{
		// 1) end now updatable
		this->on_updatable_end(_pupdatable, _result);

		// statistics) 
		++this->m_total_transfered_files;
		++this->m_total_processed_files;

		// - remove ended updatable
		scoped_lock(this->m_cs_vector_updatable)
		{
			this->m_list_updatable_process.pop_front();
		}

		// - send end message
		this->send(_message_UPDATE_UPDATABLE_END(this->m_receiver_id, this->m_sender_id, _updatable_id, eRESULT::SUCCESS));

		// - begin next updatable
		this->process_updatable_prepare();
	}
	// 1-2) bypass
	else if (_result == eRESULT::ALEADY_DONE)
	{
		// statistics) 
		++this->m_total_processed_files;

		// - remove ended updatable
		scoped_lock(this->m_cs_vector_updatable)
		{
			this->m_list_updatable_process.pop_front();
		}

		// - send end message
		this->send(_message_UPDATE_UPDATABLE_END(this->m_receiver_id, this->m_sender_id, _updatable_id, eRESULT::ALEADY_DONE));

		// - begin next updatable
		this->process_updatable_prepare();
	}
	// 1-3) retry
	else if (_result == eRESULT::INVALID_CHECK_KEY)
	{
		// - try count
		++this->m_count_try;

		if (this->m_count_try > 3)
		{
			// - end now updatable
			this->on_updatable_end(_pupdatable, _result);

			// - send end message
			this->send(_message_UPDATE_UPDATABLE_END(this->m_receiver_id, this->m_sender_id, _updatable_id, eRESULT::RETRY_OUT));
		}

		// - retry now updatable
		this->process_updatable_begin(_pupdatable, _updatable_id, 0);
	}
	// 1-4) on fail
	else
	{
		// throw) 
		throw throwable::failure(eRESULT::RETRY_OUT, 0, ""sv);
	}
}

void CGDK::update::Nsender::process_update_end(eRESULT _result) noexcept
{
	// 1) get end time
	this->m_total_tick_end = chrono::time::utc::now();

	// 2) ...
	this->on_update_end(_result);

	// 3) cancel all updatable
	this->process_cancel_all_updatable();

	// 4) send complete message
	this->send(_message_UPDATE_END(this->m_receiver_id, this->m_sender_id, _result));
}

void CGDK::update::Nsender::process_cancel_all_updatable() noexcept
{
	// declare) 
	std::list<object_ptr<resource::Iupdatable>> list_updatable_process;;

	// 1) move all updatable
	scoped_lock(this->m_cs_vector_updatable)
	{
		list_updatable_process = std::move(this->m_list_updatable_process);
	}

	// check)
	RETURN_IF(list_updatable_process.empty());

	// 2) 진행 중인 것은 취소한다.
	this->on_updatable_end(list_updatable_process.front(), eRESULT::CANCEL);

	// 4) 나머지 모든 updatable을 취소한다.
	//for (auto& iter : list_updatable_process)
	//{
	//	
	//}

	// 5) reset internal variables
	this->m_count_try = 0;
}

CGDK::object_ptr<CGDK::resource::Iupdatable> CGDK::update::Nsender::_process_get_first_updatable(CGDK::update::UPDATABLE_ID _updatable_id) const
{
	// ------------------------------------------
	// 설명) get first updatable
	//       - m_list_updatable_process에 있는 첫번째 updatable을 얻어낸다.
	//       - 첫번째 updatable은 updatable_id가 일치해야 한다.
	//         일치 하지 않을 경우 excpetion을 던진다.
	//       - updatable이 하나도 없는 경우 잘못된 것이므로 exception을 던진다.
	// ------------------------------------------
	// declare) 
	object_ptr<resource::Iupdatable> pupdatable;

	// check) 현재 진행중인 것을 종료하는 처리이므로 updatable 객체가 하나도 없을 수는 없다.
	CGASSERT_ERROR(this->m_list_updatable_process.empty() == false);

	// check) updatable이 하나도 없으면 예외를 던진다.
	THROW_IF(this->m_list_updatable_process.empty(), throwable::failure(eRESULT::INVALID_STATE, 0, ""sv));

	// - get now updatable 
	pupdatable = this->m_list_updatable_process.front();

	// check) 현재 진행중인 것인지 확인한다.
	CGASSERT_ERROR(_updatable_id == pupdatable->_get_meta_info().id);

	// check) 현재 진행중인 id와 다르면 예외를 던진다.
	THROW_IF(_updatable_id != pupdatable->_get_meta_info().id, throwable::failure(eRESULT::INVALID_STATE, 0, ""sv));

	// return)
	return pupdatable;
}

CGDK::result_code CGDK::update::Nsender::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) execute messge
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
			ON_CGMESSAGE(CGDK::eMESSAGE::UPDATE::BEGIN_RESPONSE,		on_message_UPDATE_BEGIN_RESPONSE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(CGDK::eMESSAGE::UPDATE::UPDATABLE_RESPONSE,	on_message_UPDATE_UPDATABLE_RESPONSE,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(CGDK::eMESSAGE::UPDATE::UPDATABLE_END,			on_message_UPDATE_UPDATABLE_END,		sMESSAGE_NETWORK);
		END_SUB_CGMESSAGE_MAP
	END_NESTED_CGMESSAGE_MAP
	
	// return)
	return eRESULT::BYPASS;
}

CGDK::result_code CGDK::update::Nsender::on_message_UPDATE_BEGIN_RESPONSE(sMESSAGE_NETWORK& _msg)
{
	// ------------------------------------------
	// 설명) update begining
	//       - CGDK::eMESSAGE::UPDATE::BEGIN_REQUEST의 응답 메시지
	//       - 전체 updata 시작 요청에 응답하는 메시지이다.
	//       - 이때 sender_id가 같아야 한다. (아닐 경우 다른 Nsender에 전달하려는 메시지로 간주 무시한다.)
	//       - receive_id는 응답하는 쪽의 id로 사용하지는 않는다.
	//       - eRESULT::SUCCESS 결과값을 받으면 m_list_updatable_process에 쌓여 있는 updatable부터 차례로 업데이트를 시작한다
	//       - 결과값이 eRESULT::SUCCESS이 아닐 경우 updata를 바로 종료한다.
	// ------------------------------------------

	// 1) buffer for message extract
	auto buf = _msg.buf_message + offset(8);

	// 3) get sender_id
	auto sender_id = buf.extract<CGDK::update::SENDER_ID>();

	// check)
	RETURN_IF(sender_id != this->m_sender_id, eRESULT::BYPASS);

	// 3) get sender_id
	/*auto receive_id =*/ buf.extract<CGDK::update::RECEIVER_ID>();

	// 4) result
	auto result = buf.extract<eRESULT>();

	// 5) process
	if (result == eRESULT::SUCCESS)
	{
		// - begin
		this->process_updatable_prepare();
	}
	else
	{
		this->process_update_end(result);
	}

	// return)
	return eRESULT::DONE;
}

CGDK::result_code CGDK::update::Nsender::on_message_UPDATE_UPDATABLE_RESPONSE(sMESSAGE_NETWORK& _msg)
{
	// ------------------------------------------
	// 설명) updatable begining
	//       - CGDK::eMESSAGE::UPDATE::UPDATABLE_REQUEST의 응답 메시지
	//       - 개별 updatable의 시작 응답 메시지이다.
	//       - 현재 파일의 크기를 받아서 전송을 시작한다.
	//       - 만약 현재 파일과 md5값이 동일하다면 동일 파일로 인식하고 다음 updatable을 시작한다.
	//       - 동일한 파일이 아닐 경우 본격적인 전송을 시작한다.
	// ------------------------------------------

	// 1) buffer for message extract
	auto buf = _msg.buf_message + offset(8);

	// 2) get sender_id
	auto sender_id = buf.extract<CGDK::update::SENDER_ID>();

	// check)
	RETURN_IF(sender_id != this->m_sender_id, eRESULT::BYPASS);

	// 3) get sender_id
	/*auto receive_id =*/ buf.extract<CGDK::update::RECEIVER_ID>();

	// 4) get sender_id
	auto updatable_id = buf.extract<CGDK::update::UPDATABLE_ID>();

	//  declare) 
	object_ptr<resource::Iupdatable> pupdatable;

	try
	{
		// 5) get updtable
		pupdatable = this->_process_get_first_updatable(updatable_id);

		// 7) get pos begin
		auto updatable_prepare_info = buf.extract<sUPDATABLE_PREPARE_INFO>();

		// 8 get md5
		auto updatable_file_completion_info = pupdatable->get_meta_completion();

		// 9) 
		switch(updatable_prepare_info.result)
		{
		case	eRESULT::DONE: // cache hit - cache file info 
		case	eRESULT::ALEADY_EXIST: // no cache hit - file exist
				{
					// 9) compare md5 if file size is same
					if (updatable_prepare_info.size == pupdatable->_get_meta_info().size)
					{
						// - md5 is same => next~
						if (updatable_prepare_info.check_code == updatable_file_completion_info.check_code)
						{
							// - process updatable end
							this->process_updatable_end(pupdatable, updatable_id, eRESULT::ALEADY_DONE);
						}
						else
						{
							// - process updatable next
							this->process_updatable_begin(pupdatable, updatable_id, 0);
						}
					}
					else
					{
						// check) 
						CGASSERT_ERROR(updatable_prepare_info.check_code != updatable_file_completion_info.check_code);

						// check) 
						THROW_IF(updatable_prepare_info.check_code == updatable_file_completion_info.check_code, throwable::failure(eRESULT::INVALID_STATE, 0, ""sv));

						// - pos는 이미 전송 완료가 된 파일일 경우 0부터 아니면 이어서 전송한다.
						auto pos_begin = (updatable_prepare_info.result != eRESULT::NOT_EXIST) ? 0 : updatable_prepare_info.size;

						// - process updatable next
						this->process_updatable_begin(pupdatable, updatable_id, pos_begin);
					}
				}
				break;

		case	eRESULT::NOT_EXIST: // no cache hit - file not exist
				{
					// check) 
					CGASSERT_ERROR(updatable_prepare_info.size == 0);

					// check) 
					THROW_IF(updatable_prepare_info.size != 0, throwable::failure(eRESULT::INVALID_STATE, 0,"size must be 0"sv));

					// - process updatable next
					this->process_updatable_begin(pupdatable, updatable_id, 0);
				}
				break;

		case	eRESULT::EXPIRED: // cache hit but invalid state (cached file info expired)
				{
					// - process updatable next
					this->process_updatable_begin(pupdatable, updatable_id, 0);
				}
				break;

		default:
				{
					// - failed - process updatable end
					this->process_updatable_end(pupdatable, updatable_id, updatable_prepare_info.result);
				}
				break;
		}
	}
	catch (throwable::failure& _e)
	{
		// - end all process
		this->process_update_end(_e.reason());
	}
	catch (...)
	{
		// - end all process
		this->process_update_end(eRESULT::EXCEPTION);
	}

	// return)
	return eRESULT::DONE;
}

CGDK::result_code CGDK::update::Nsender::on_message_UPDATE_UPDATABLE_END(sMESSAGE_NETWORK& _msg)
{
	// ------------------------------------------
	// 설명) updatable end
	//       - 개별 updatable의 종료를 알리는 메시지다.
	//       - 전체 file의 md5를 전송해 오는데 이것을 사용해 오류 없이 전달되었는지 확인한다.
	//       - 만약 md5가 다를 경우 재전송을 다시 한번 실행한다.
	//       - 
	// ------------------------------------------

	// 1) buffer for message extract
	auto buf = _msg.buf_message + offset(8);

	// 2) extract  sender_id
	auto sender_id = buf.extract<CGDK::update::SENDER_ID>();

	// check)
	RETURN_IF(sender_id != this->m_sender_id, eRESULT::BYPASS);

	// 3) extract  receiver_id
	/*auto receive_id =*/ buf.extract<CGDK::update::RECEIVER_ID>();

	// 4) extract updatable_id
	auto updatable_id = buf.extract<CGDK::update::UPDATABLE_ID>();

	try
	{
		// 5) extract result
		auto result = buf.extract<eRESULT>();

		// 6) extract md5
		auto md5 = buf.extract<MD5>();

		// declare) 
		object_ptr<resource::Iupdatable> pupdatable;

		// 7) get resource_updatable
		scoped_lock(this->m_cs_vector_updatable)
		{
			// 8) get first updtable
			pupdatable = this->_process_get_first_updatable(updatable_id);

			// 9) 성공했는가? (result가 SUCCESS이고 md5가 동일해야 성공이다.)
			if (result == eRESULT::SUCCESS)
			{
				if (pupdatable->get_meta_completion().check_code != md5)
				{
					result = eRESULT::INVALID_CHECK_KEY;
				}
			}
		}

		// 10) process updatable end
		this->process_updatable_end(pupdatable, updatable_id, result);
	}
	catch (throwable::failure& _e)
	{
		this->process_update_end(_e.reason());
	}
	catch (...)
	{
		// - end...
		this->process_update_end(eRESULT::EXCEPTION);
	}

	// return)
	return eRESULT::DONE;
}
