#include "pch.h"

using namespace CGDK;

void get_parameter_service_setting(server_system::sSERVICE::sSETTING& _service_setting, context& _context_service)
{
	// 1) find setting context
	auto& context_setting = _context_service.find("setting");

	// check) setting context exist?
	RETURN_IF(context_setting.is_valid() == false, );

	// 2) 'setting.nickname'
	{
		// - get 'name'
		auto& context_name = context_setting.find("name"sv, "nickname"sv);

		// - set 'setting name'
		if (context_name.is_valid())
		{
			// - get name
			std::string str_name = context_name;

			// - name을 m_server_setting_info에  설정한다.
			_service_setting.name = str_name;

			// trace) 
			LOG_INFO << "   + setting.name: '"sv << str_name << "'"sv;
		}
	}

	// 3) 'setting.comment'
	{
		// - get 'comment'
		auto& context_comment = context_setting.find("comment"sv);

		// - set 'comment'
		if (context_comment.is_valid())
		{
			// - get 'comment' string
			std::string str_comment = context_comment;

			// - 'comment'를 m_server_setting_info에  설정한다.
			_service_setting.comment = str_comment;

			// trace) 
			LOG_INFO << "   + setting.comment: '"sv << str_comment << "'"sv;
		}
	}

	// 4) 'setting.rank'
	{
		// - get 'rank' context
		auto& context_rank = context_setting.find("rank"sv);

		// - rank 가 있는가?
		if (context_rank.is_valid())
		{
			// declare) 
			server_system::sRANK rank = _service_setting.rank;

			// - get 'group'
			{
				auto& context_group = context_rank.find("group"sv);
				if (context_group.is_valid())
				{
					// - set value
					rank.group = context_group;

					// trace) 
					LOG_INFO << "   + setting.rank.group: "sv << rank.group;
				}
			}

			// - get 'field'
			{
				auto& context_field = context_rank.find("field"sv);
				if (context_field.is_valid())
				{
					// - set value
					rank.field = context_field;

					// trace) 
					LOG_INFO << "   + setting.rank.field: "sv << rank.field;
				}
			}

			// - get 'role'
			{
				auto& context_role = context_rank.find("role"sv);
				if (context_role.is_valid())
				{
					// - set value
					rank.role = context_role;

					// trace) 
					LOG_INFO << "   + setting.rank.role: "sv << rank.role;
				}
			}

			// - get 'index'
			{
				auto& context_index = context_rank.find("index"sv);
				if (context_index.is_valid())
				{
					if (context_index.equal("auto"))
					{
						rank.index = server_system::RANK_AUTO;
					}
					else
					{
						rank.index = context_index;
					}

					// trace) 
					LOG_INFO << "   + setting.rank.index: "sv << rank.index;
				}
			}

			// - set rank
			_service_setting.rank = rank;

			// trace)
			LOG_PROGRESS << "   ! service.rank: "
				<< _service_setting.rank.group
				<< "." << _service_setting.rank.field
				<< "." << _service_setting.rank.role
				<< "." << _service_setting.rank.index;
		}
	}

	// 5) 'setting.revision'
	{
		// - get 'revision' context
		auto& context_rank = context_setting.find("revision"sv);

		// - revision 가 있는가?
		if (context_rank.is_valid())
		{
			// declare) 
			server_system::sREVISION revision = _service_setting.revision;

			// - get 'url'
			{
				auto& context_url = context_rank.find("url"sv);
				if (context_url.is_valid())
				{
					// - set value
					revision.url = (std::string&&)context_url;

					// trace) 
					LOG_INFO << "   + setting.revision.url: "sv << revision.url;
				}
			}

			// - get 'revision'
			{
				auto& context_revision = context_rank.find("revision"sv);
				if (context_revision.is_valid())
				{
					// - set value
					revision.revision = context_revision;

					// trace) 
					LOG_INFO << "   + setting.revision.revision: "sv << revision.revision;
				}
			}

			// - get 'time'
			{
				auto& context_time = context_rank.find("time"sv);
				if (context_time.is_valid())
				{
					// - set value
					revision.time = static_cast<chrono::time::utc::time_point>(context_time);

					// trace) 
					LOG_INFO << "   + setting.revision.time: "sv << to_string(revision.time);
				}
			}

			// - set rank
			_service_setting.revision = revision;

			// trace) 
			LOG_PROGRESS << "   ! service.revision: "
				<< _service_setting.revision.url
				<< "." << _service_setting.revision.revision
				<< "." << to_string(_service_setting.revision.time);
		}
	}

	// 6) 'setting.version'
	{
		// - get 'version' context
		auto& context_version = context_setting.find("version"sv);

		// - version 가 있는가?
		if (context_version.is_valid())
		{
			// declare) 
			sVERSION_EX version = _service_setting.version;

			// - get 'build'
			{
				auto& context_build = context_version.find("build"sv);
				if (context_build.is_valid())
				{
					// - set value
					version.build = context_build;

					// trace) 
					LOG_INFO << "   + setting.version.build: "sv << version.build;
				}
			}

			// - get 'minor'
			{
				auto& context_minor = context_version.find("minor"sv);
				if (context_minor.is_valid())
				{
					// - set value
					version.minor = context_minor;

					// trace) 
					LOG_INFO << "   + setting.version.minor: "sv << version.minor;
				}
			}

			// - get 'major'
			{
				auto& context_major = context_version.find("major"sv);
				if (context_major.is_valid())
				{
					// - set value
					version.major = context_major;

					// trace) 
					LOG_INFO << "   + setting.version.major: "sv << version.major;
				}
			}

			// - get 'major'
			{
				auto& context_str_version = context_version.find("str_version"sv);
				if (context_str_version.is_valid())
				{
					// - set value
					version.str_version = (std::string&&)context_str_version;

					// trace) 
					LOG_INFO << "   + setting.version.str_version: "sv << version.str_version;
				}
			}

			// - get 'time'
			{
				auto& context_build_time = context_version.find("build_time"sv);
				if (context_build_time.is_valid())
				{
					// - set value
					version.build_time = static_cast<chrono::time::utc::time_point>(context_build_time);

					// trace) 
					LOG_INFO << "   + setting.version.build_time: "sv << to_string(version.build_time);
				}
			}

			// - set rank
			_service_setting.version = version;

			// trace)
			LOG_PROGRESS << "   ! service.version: "
				<< _service_setting.version.build
				<< "." << _service_setting.version.minor
				<< "." << _service_setting.version.major
				<< "[" << _service_setting.version.str_version << "]"
				<< "(" << to_string(_service_setting.version.build_time) << ")";
		}
	}

	// 7) execution infos
	{
		// declare) 
		std::string filename;
		std::string str_path;
		std::string working_path;

		// declare)
		sVERSION_EX version = _service_setting.version;

		// - get 'service_type' ("execute"(기본) / "service" / "docker")
		{
			auto& context_service_type = context_setting.find("service_type"sv);
			if (context_service_type.is_valid())
			{
				// - set value
				_service_setting.service_type = server_system::to_service_type(context_service_type.operator std::string());

				// trace)
				LOG_INFO << "   + setting.service_type: "sv << server_system::to_service_type_string(_service_setting.service_type);
			}
		}

		// - get 'stop_grace_period' (초. 없으면 설정하지 않는다.)
		{
			auto& context_stop_grace_period = context_setting.find("stop_grace_period"sv);
			if (context_stop_grace_period.is_valid())
			{
				// - set value
				_service_setting.stop_grace_period = static_cast<uint32_t>(context_stop_grace_period);

				// trace)
				LOG_INFO << "   + setting.stop_grace_period: "sv << _service_setting.stop_grace_period << "s"sv;
			}
		}

		// - get 'filename'
		{
			auto& context_filename = context_setting.find("filename"sv, "file_execute"sv, "file execute"sv);
			if (context_filename.is_valid())
			{
				// - set value
				filename = (std::string&&)context_filename;

				// trace) 
				LOG_INFO << "   + setting.filename: "sv << filename;
			}
		}

		// - get 'str_path'
		{
			auto& context_str_path = context_setting.find("path"sv, "str_path"sv);
			if (context_str_path.is_valid())
			{
				// - set value
				str_path = (std::string&&)context_str_path;

				// trace) 
				LOG_INFO << "   + setting.str_path: "sv << str_path;
			}
		}

		// - get 'working_path'
		{
			auto& context_path_working = context_setting.find("working_path"sv, "str_path_working"sv, "path_working"sv, "path working"sv, "working path"sv);
			if (context_path_working.is_valid())
			{
				// - set value
				working_path = (std::string&&)context_path_working;

				// trace) 
				LOG_INFO << "   + setting.working_path: "sv << working_path;
			}
		}

		// - set rank
		_service_setting.filename = std::move(filename);
		_service_setting.path = std::move(str_path);
		_service_setting.working_path = std::move(working_path);
	}

	// 8) 'setting.iid'
	{
		// declare) 
		server_system::iid_t iid{ 0 };

		// - name 필드를 얻는다.
		auto& context_iid = _context_service.find("iid"sv);

		// - name 필드가 있으면 Name으로 설정한다.
		if (context_iid.is_valid())
		{
			if (context_iid.equal("auto"))
			{
				// - auto로 설정한다.
				iid = server_system::IID_AUTO;

				// trace) 
				LOG_INFO << "   + instance.iid: auto"sv;
			}
			else
			{
				// - context_iid를 uint64_t로 변환한다.
				iid = (uint64_t)context_iid;

				// trace) 
				LOG_INFO << "   + instance.iid: "sv << iid;
			}
		}

		// - sdet iid
		if (iid == 0)
		{
			LOG_INFO << "   (!) iid is 0 (service instance will be unmanaged)"sv;
		}

		// - set iid
		_service_setting.iid = iid;

		// trace)
		if(_service_setting.iid == server_system::IID_AUTO)
			LOG_PROGRESS << "   - instance.iid: auto";
		else
			LOG_PROGRESS << "   - instance.iid: " << iid;
	}
}
