#include "pch.h"

using namespace CGDK;


server_system::service_ex::service_ex(const std::string_view _name, const std::string_view _filename_log, const std::string_view _filename_configure, uint32_t /*_server_type*/) :
	service(_name)
{
	// 1) set configure file
	try
	{
		this->m_service_setting.buf_configure = api::file::read(_filename_configure);
	}
	catch (...)
	{
	}

	// check)
	if (this->m_service_setting.buf_configure.empty())
	{
		LOG_ERROR << "fail to load configure file '" << _filename_configure << "'";
	}

	try
	{
		// 1) logger_file
		auto plogger_file = make_own<logger_file>("logger_file");

		if (_filename_log.empty())
		{
			if (this->m_service_setting.buf_configure.size() != 0)
			{
				// - keep buffer 
				auto buf_context = this->m_service_setting.buf_configure;

				// declare) 
				context context_log;

				// - read configure file
				context_log.read_from_json_string(buf_context.data<const char>());

				// - find service classe
				auto& context_service = context_log.find("service");

				if (context_service.is_valid())
				{
					plogger_file->initialize(context_service);
				}
			}
		}
		else
		{
			plogger_file->initialize({ _filename_log });
		}

		// - set
		this->m_plogger_file = std::move(plogger_file);
	}
	catch (...)
	{
		// - rollback
		this->m_service_setting.buf_configure.clear();

		// throw)
		throw;
	}
}

server_system::service_ex::~service_ex()
{
}

void server_system::service_ex::on_server_certified(const shared_buffer& /*_buffer*/)
{
	// 1) notify update...
	this->send_to_admin_server(server_admin::message_SERVER_ADMIN_INSTANCE_UPDATE_STATE(this->m_service_setting.iid, this->get_object_state(), eOBJECT_STATE::NONE));
}

void server_system::service_ex::_process_initializing_pre(context& /*_context*/)
{
	// 1) get logger
	auto plogger = this->get_logger();

	if (plogger.exist())
	{
		plogger->register_log_targetable(this->m_plogger_file);
	}

	// 2) name을 setting한다.
	set_name(this->m_service_setting.nickname);
	this->m_service_setting.version.build = SVN_COMMIT_REVISION;
	this->m_service_setting.version.build_time = from_string<chrono::time::utc::time_point>(std::string_view(SVN_TIME_NOW_STRING), "%d/%d/%d %d:%d:%d"sv);

	// trace) 
	LOG_SYSTEM	<< "-----------------------------------------------------------------"sv << log_level_t::important();
	LOG_SYSTEM	<< " "sv;
	LOG_SYSTEM	<< "                      "sv << this->m_service_setting.nickname << log_level_t::important();
	LOG_SYSTEM	<< " "sv;
	LOG_SYSTEM	<< "@ CGDK: "sv << CGDK_CONFIGURATION;
	LOG_SYSTEM	<< " "sv;
	LOG_SYSTEM	<< "@ SVN_URL: "sv << this->m_service_setting.revision.url;
	LOG_SYSTEM	<< "@ SVN_UPDATE_REVISION: "sv << this->m_service_setting.revision.revision;
	LOG_SYSTEM	<< "@ SVN_DATE: "sv << to_string(this->m_service_setting.revision.time);
	LOG_SYSTEM	<< "@ compile: "sv << __DATE__ << " "sv << __TIME__;
	LOG_SYSTEM	<< " "sv;
	LOG_SYSTEM	<< "@ system uuid: "sv << api::get_system_unique_id();
	LOG_SYSTEM	<< " "sv;
	LOG_SYSTEM	<< "@ paramters"sv;
	LOG_SYSTEM	<< "  + name: "sv << this->m_service_setting.nickname << log_level_t::important();
	LOG_SYSTEM	<< "  + id_server.id: "sv << this->m_service_setting.iid;
	LOG_SYSTEM	<< "  + rank.group: "sv << this->m_service_setting.rank.group;
	LOG_SYSTEM	<< "  + rank.field: "sv << this->m_service_setting.rank.field;
	LOG_SYSTEM	<< "  + rank.role: "sv << this->m_service_setting.rank.role;
	LOG_SYSTEM	<< "  + rank.index: "sv << this->m_service_setting.rank.index;
	LOG_SYSTEM	<< "  + comment: "sv << this->m_service_setting.comment;
	LOG_SYSTEM	<< "-----------------------------------------------------------------"sv << log_level_t::important();
}

void server_system::service_ex::_process_destroy_post() noexcept
{
	// 2) Log File을 unregister한다.
	if (this->m_plogger_file.exist())
	{
		// 1) get logger
		auto plogger = this->get_logger();

		if (plogger.exist())
		{
			plogger->unregister_log_targetable(m_plogger_file);
		}
	}
}
