

//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                           Server Admin Classes                            *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------
// admin_client info (Server의 정보)
//-----------------------------------------------------------------
#pragma pack(1)


#if defined(__linux__)
using  LPSERVICE_MAIN_FUNCTIONA = void*;
#endif

namespace CGDK
{
//-----------------------------------------------------------------
// definitions
//-----------------------------------------------------------------
#if defined(_WIN32)

#elif defined(__linux)
	#define	WINAPI
	#if !defined(CALLBACK)
		#define CALLBACK __stdcall
	#endif
	#define	__callback

	#define MAX_PATH								260

	// Controls
	#define SERVICE_CONTROL_STOP					0x00000001
	#define SERVICE_CONTROL_PAUSE					0x00000002
	#define SERVICE_CONTROL_CONTINUE				0x00000003
	#define SERVICE_CONTROL_INTERROGATE				0x00000004
	#define SERVICE_CONTROL_SHUTDOWN				0x00000005
	#define SERVICE_CONTROL_PARAMCHANGE				0x00000006
	#define SERVICE_CONTROL_NETBINDADD				0x00000007
	#define SERVICE_CONTROL_NETBINDREMOVE			0x00000008
	#define SERVICE_CONTROL_NETBINDENABLE			0x00000009
	#define SERVICE_CONTROL_NETBINDDISABLE			0x0000000A
	#define SERVICE_CONTROL_DEVICEEVENT				0x0000000B
	#define SERVICE_CONTROL_HARDWAREPROFILECHANGE	0x0000000C
	#define SERVICE_CONTROL_POWEREVENT				0x0000000D
	#define SERVICE_CONTROL_SESSIONCHANGE			0x0000000E
	#define SERVICE_CONTROL_PRESHUTDOWN				0x0000000F
	#define SERVICE_CONTROL_TIMECHANGE				0x00000010
	#define SERVICE_CONTROL_TRIGGEREVENT			0x00000020

	// begin_wdm
	#define DELETE									(0x00010000L)
	#define READ_CONTROL							(0x00020000L)
	#define WRITE_DAC								(0x00040000L)
	#define WRITE_OWNER								(0x00080000L)
	#define SYNCHRONIZE								(0x00100000L)
	#define STANDARD_RIGHTS_REQUIRED				(0x000F0000L)
	#define STANDARD_RIGHTS_READ					(READ_CONTROL)
	#define STANDARD_RIGHTS_WRITE					(READ_CONTROL)
	#define STANDARD_RIGHTS_EXECUTE					(READ_CONTROL)
	#define STANDARD_RIGHTS_ALL						(0x001F0000L)
	#define SPECIFIC_RIGHTS_ALL						(0x0000FFFFL)

	// Service object specific access type
	#define SERVICE_QUERY_CONFIG					0x0001
	#define SERVICE_CHANGE_CONFIG					0x0002
	#define SERVICE_QUERY_STATUS					0x0004
	#define SERVICE_ENUMERATE_DEPENDENTS			0x0008
	#define SERVICE_START							0x0010
	#define SERVICE_STOP							0x0020
	#define SERVICE_PAUSE_CONTINUE					0x0040
	#define SERVICE_INTERROGATE						0x0080
	#define SERVICE_USER_DEFINED_CONTROL			0x0100
	#define SERVICE_ALL_ACCESS						(STANDARD_RIGHTS_REQUIRED     | \
													 SERVICE_QUERY_CONFIG         | \
													 SERVICE_CHANGE_CONFIG        | \
													 SERVICE_QUERY_STATUS         | \
													 SERVICE_ENUMERATE_DEPENDENTS | \
													 SERVICE_START                | \
													 SERVICE_STOP                 | \
													 SERVICE_PAUSE_CONTINUE       | \
													 SERVICE_INTERROGATE          | \
													 SERVICE_USER_DEFINED_CONTROL)

	// Service Types (Bit Mask)
	#define SERVICE_KERNEL_DRIVER					0x00000001
	#define SERVICE_FILE_SYSTEM_DRIVER				0x00000002
	#define SERVICE_ADAPTER							0x00000004
	#define SERVICE_RECOGNIZER_DRIVER				0x00000008
	#define SERVICE_DRIVER							(SERVICE_KERNEL_DRIVER | \
													 SERVICE_FILE_SYSTEM_DRIVER | \
													 SERVICE_RECOGNIZER_DRIVER)
	#define SERVICE_WIN32_OWN_PROCESS				0x00000010
	#define SERVICE_WIN32_SHARE_PROCESS				0x00000020
	#define SERVICE_WIN32							 (SERVICE_WIN32_OWN_PROCESS | \
													  SERVICE_WIN32_SHARE_PROCESS)
	#define SERVICE_INTERACTIVE_PROCESS				0x00000100
	#define SERVICE_TYPE_ALL			            (SERVICE_WIN32  | \
													 SERVICE_ADAPTER | \
													 SERVICE_DRIVER  | \
													 SERVICE_INTERACTIVE_PROCESS)

	// start type
	#define SERVICE_BOOT_START						0x00000000
	#define SERVICE_SYSTEM_START					0x00000001
	#define SERVICE_AUTO_START						0x00000002
	#define SERVICE_DEMAND_START					0x00000003
	#define SERVICE_DISABLED						0x00000004

	// Error control type
	#define SERVICE_ERROR_IGNORE					0x00000000
	#define SERVICE_ERROR_NORMAL					0x00000001
	#define SERVICE_ERROR_SEVERE					0x00000002
	#define SERVICE_ERROR_CRITICAL					0x00000003

	//struct SYSTEM_INFO
	//{
	//};

	// temporary) 
	#define	PDH_FMT_1000	0

	using HQUERY = int;
	using LPHANDLER_FUNCTION_EX = int*;
	using LPSERVICE_STATUS = int*;
	using SERVICE_STATUS_HANDLE = int;
#endif


namespace server_system
{
	//-----------------------------------------------------------------------------
	/**

	@class		sCERTIFY_INFO
	@brief		네트워크로 송수신 시 사용되는 구조체
	@todo
	@ref		sSERVICE_INSTANCE_INFO, SERVICE_SETTING
	@details	\n

	*/
	//-----------------------------------------------------------------------------
	// 2) 인증 종류
	struct sCERTIFY_INFO
	{
		int					dummy{ 0 };
	};

	// - certify key
	//   CERTIFY_REQUEST = [sCERTIFY_INFO][std::string_view certify_key (optional)]
	//   key가 설정된 쪽(admin server/host agent)은 같은 key를 보낸 접속만 받는다.
	[[nodiscard]] bool	is_certify_key_matched(std::string_view _expected, std::string_view _received) noexcept;
	[[nodiscard]] std::string get_certify_key_from_environment();

	// - key file (앞뒤 공백/줄바꿈은 뺀다. linux: group/other가 읽을 수 있으면 경고한다. 읽지 못하면 std::nullopt)
	[[nodiscard]] std::optional<std::string> read_key_file(std::string_view _filename);

	// - setting의 key
	//   1) "<name>_file" : key file (읽지 못하면 아무도 맞출 수 없는 key로 설정하고 error log를 남긴다.)
	//   2) "<name>"      : key
	//   3) _environment  : 환경변수 (빈 문자열이면 쓰지 않는다.)
	[[nodiscard]] std::string get_key_from_setting(context& _context, std::string_view _name, std::string_view _environment = {});
	[[nodiscard]] inline std::string get_certify_key_from_setting(context& _context) { return get_key_from_setting(_context, "certify_key", "CGDK_ADMIN_CERTIFY_KEY"); }


	//-----------------------------------------------------------------------------
	/**


	*/
	//-----------------------------------------------------------------------------
	// 1) factory
	struct factory_update_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		uint64_t				id{ 0};

		int64_t					stacked{ 0 };

		statistics::unit_factory_snap now;
	};

	struct factory_now_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		std::string				name;

		eFACTORY_TYPE			type{ eFACTORY_TYPE::DEFAULT };

		eOBJECT_TYPE			object_type{ eOBJECT_TYPE::NOT_DEFINED };
		int64_t					object_size{ 0 };

		factory_update_snap		updatable;
		statistics::unit_factory_snap now;
	};

	struct factory_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		std::string				name;

		eFACTORY_TYPE			type{ eFACTORY_TYPE::DEFAULT };

		eOBJECT_TYPE			object_type{ eOBJECT_TYPE::NOT_DEFINED };
		int64_t					object_size{ 0 };

		factory_update_snap		updatable;
		std::vector<statistics::unit_factory_snap> period;
	};

	// 2) thread
	struct unit_thread
	{
		ENABLE_STRUCT_SERIALIZABLE

		uint64_t				id_thread{ 0};

		uint64_t				count_executed{ 0 };
		uint64_t				count_error{ 0 };

		chrono::tick::time_point tick_last_execute{ chrono::tick::duration::zero() };
		bool					is_executing{ false };
	};
	unit_thread					get_snap(api::Nthread* _pthread) noexcept;
	std::vector<unit_thread>	get_snap(api::manager_thread* _pthread_manager);


	// 3) executor
	struct executor_update_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		uint64_t				id{ 0 };
		int64_t					count_execute{ 0 };

		std::vector<unit_thread> vector_threads;
	};

	struct executor_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		std::string				name;
		eEXECUTOR_TYPE			type{ eEXECUTOR_TYPE::USER };

		executor_update_snap	updatable;
	};


	// 4) schedulable
	struct schedulable_update_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		uint64_t				id{ 0};

		chrono::tick::duration	duration_interval{ chrono::tick::duration::zero() };
		chrono::tick::time_point tick_next{ (chrono::tick::max)() };

		int64_t					count_execute{ 0 };
	};

	struct schedulable_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		std::string				name;

		eSCHEDULABLE_TYPE		type{ eSCHEDULABLE_TYPE::USER };

		schedulable_update_snap	updatable;
	};

	// 5) connective snap
	struct connective_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		uint64_t				index{ 0 };
		std::vector<statistics::unit_connective_snap> vector_unit_connective_snap;
	};

	// 6) traffic snap
	struct traffic_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		uint64_t				index{ 0 };
		std::vector<statistics::unit_traffic_snap> vector_unit_traffic_snap;
	};


	//-----------------------------------------------------------------------------
	/**


	*/
	//-----------------------------------------------------------------------------
	struct connective_status_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		uint64_t				id{ 0 };			// - id

		chrono::time::utc::time_point time_start;	// - time: start
		chrono::time::utc::time_point time_stop;	// - time: stop
		net::sockaddr			addr_local;			// - address: local(bind)
		eSOCKET_STATE			state{ eSOCKET_STATE::CLOSED };

		int						is_enable{ 0 };		// - 상태정보
	};

	// 3) unit_connective_snap
	struct connective_info_update_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		connective_status_snap	status;
		statistics::unit_connective_snap now;
	};

	// 2) connective_info_snap
	struct connective_info_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		std::string				name;
		chrono::time::utc::time_point time_create;

		connective_status_snap	status;

		uint64_t				index{ 0 };
		statistics::unit_connective_snap now;
	};

	// 3) connective_info_store
	struct connective_info_store
	{
		ENABLE_STRUCT_SERIALIZABLE

		std::string				name;
		chrono::time::utc::time_point time_create;

		connective_status_snap	status;

		uint64_t				index{ 0 };
		std::vector<statistics::unit_connective_snap> period;

		const auto& now() const noexcept { return period[index]; }
	};


	//-----------------------------------------------------------------
	// binder
	//-----------------------------------------------------------------
	// 2) socket info (sSOCKET_INFO)
	struct socket_info
	{
		ENABLE_STRUCT_SERIALIZABLE

		static_string<64>		name;					// - name
		CGSOCKET				socket_handle{ INVALID_SOCKET }; // - socket handle

		net::sockaddr			addr_local;				// - address: local(bind)
		net::sockaddr			addr_remote;			// - address: remote

		chrono::time::utc::time_point time_create;		// - time: create
	};

	// 3) socket status (sSOCKET_STATUS)
	struct socket_status
	{
		ENABLE_STRUCT_SERIALIZABLE

		CGSOCKET				id { INVALID_SOCKET }; // - id (socket handle)
		eSOCKET_STATE			state_socket{ eSOCKET_STATE::CLOSED }; // - Socket State

		chrono::time::utc::time_point time_connect;		// - connect time
		chrono::time::utc::time_point time_close;		// - close time

		statistics::unit_traffic traffic;				// - I/O Status
	};

	// 4) socket info updatable snap
	struct socket_info_update_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		uint64_t				id{ 0 };

		chrono::time::utc::time_point time_bind;
		chrono::time::utc::time_point time_connect;
		chrono::tick::duration	tick_send { chrono::tick::duration::zero() };
		chrono::tick::duration	tick_receive { chrono::tick::duration::zero() };

		net::sockaddr			addr_local;				// - address: local(bind)
		net::sockaddr			addr_remote;			// - address: remote
		net::sockaddr			addr_public;			// - address: public (accept address of acceptor)

		eSOCKET_STATE			state{ eSOCKET_STATE ::CLOSED};
		uint32_t				flag_socket{ 0 };
		statistics::unit_traffic_snap traffic;
	};

	// 6) socket info snap
	struct socket_info_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		std::string				name;

		socket_info_update_snap updatable;
	};


	//-----------------------------------------------------------------
	// time_event
	//-----------------------------------------------------------------
	struct time_event_update_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		uint64_t			id{ 0};

		time_event::sENTITY_SETTING	entity_setting;
		time_event::sENTITY_STATUS	entity_status;
	};

	struct time_event_snap
	{
		ENABLE_STRUCT_SERIALIZABLE

		time_event_update_snap updatable;
	};


	//-----------------------------------------------------------------------------
	/**

	@class		sHOST
	@brief		HOST의 설정 내용
	@todo
	@ref		sSERVICE
	@details	\n

	*/
	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------
	// host instance info
	//-----------------------------------------------------------------
	struct sHOST
	{
		ENABLE_STRUCT_SERIALIZABLE

		// 1) setting
		struct sSETTING
		{
			ENABLE_STRUCT_SERIALIZABLE

			// - host sID
			iid_t				iid{ 0 };

			// - name/comment
			std::string			name;
			std::string			comment;
		} setting;

		// 2) info
		struct sINFO
		{
			ENABLE_STRUCT_SERIALIZABLE

			// - system/OS Info
			SYSTEM::sINFO		system_info;
			sOS_INFO			os_info;

			// - host address
			sMAC_ADDRESS		mac;
			net::sockaddr		addr_host;
			std::string			uid_host;
			std::string			uid_os;

			// - host name
			std::string			host_name;

			// - start Time
			chrono::time::utc::time_point time_start;
		} info;

		// 3) status
		struct sSTATUS
		{
			ENABLE_STRUCT_SERIALIZABLE

			// 1) host instance id
			iid_t				iid{ 0 };

			// 2) usage info
			SYSTEM::sINFO		system_info;
			std::vector<SYSTEM::sSTATUS> vector_status;
			//std::vector<PROCESS::sSTATUS>	vector_process_status;
		} status;
	};


	//-----------------------------------------------------------------------------
	/**

	@class		sSERVICE

	*/
	//-----------------------------------------------------------------------------


	//-----------------------------------------------------------------
	// all in one
	//-----------------------------------------------------------------
	// - server setting/status
	struct sSERVICE_STATUS_UPDATE
	{
		ENABLE_STRUCT_SERIALIZABLE

		PROCESS::sSTATUS							process_status;
		std::vector<factory_update_snap>			vector_factory_update;
		std::vector<executor_update_snap>			vector_executor_update;
		std::vector<schedulable_update_snap>		vector_schedulable_update;
		std::vector<connective_info_update_snap>	vector_connective_update;
		std::vector<socket_info_update_snap>		vector_binder_update;
		statistics::unit_connective_snap			connective_info;
		statistics::unit_traffic_snap				traffic_info;
	};

	//-----------------------------------------------------------------
	// service type (서비스를 실행하는 방법)
	//
	//  - EXECUTE         : 실행 파일을 실행한다. (sSETTING의 filename/path/working_path/arguments/environment)
	//  - SERVICE         : 서비스를 시작/중지한다. (sSETTING::name이 서비스 이름. windows: windows service, linux: systemd service)
	//  - DOCKER          : docker container를 시작/중지한다. (sSETTING::name이 container 이름. 'docker start/stop <name>')
	//-----------------------------------------------------------------
	enum class eSERVICE_TYPE : uint32_t
	{
		EXECUTE			 = 0,
		SERVICE			 = 1,
		DOCKER			 = 2,
		MAX_
	};

	// - 문자열 <-> eSERVICE_TYPE ("execute", "service"("windows_service"도 허용), "docker". 모르는 값은 EXECUTE)
	[[nodiscard]] inline eSERVICE_TYPE to_service_type(std::string_view _text) noexcept
	{
		if (_text == "service" || _text == "windows_service")
			return eSERVICE_TYPE::SERVICE;
		if (_text == "docker")
			return eSERVICE_TYPE::DOCKER;
		return eSERVICE_TYPE::EXECUTE;
	}

	[[nodiscard]] inline std::string_view to_service_type_string(eSERVICE_TYPE _service_type) noexcept
	{
		switch (_service_type)
		{
		case	eSERVICE_TYPE::SERVICE:	return "service";
		case	eSERVICE_TYPE::DOCKER:			return "docker";
		default:								return "execute";
		}
	}

#if defined(_WIN32)
	// - UTF-8 <-> UTF-16 (windows W API에 넘길 때 쓴다.)
	[[nodiscard]] inline std::wstring utf8_to_utf16(std::string_view _text)
	{
		if (_text.empty())
			return std::wstring();

		auto length = ::MultiByteToWideChar(CP_UTF8, 0, _text.data(), static_cast<int>(_text.size()), nullptr, 0);
		std::wstring result(static_cast<std::size_t>(length), L'\0');
		::MultiByteToWideChar(CP_UTF8, 0, _text.data(), static_cast<int>(_text.size()), result.data(), length);
		return result;
	}

	[[nodiscard]] inline std::string utf16_to_utf8(std::wstring_view _text)
	{
		if (_text.empty())
			return std::string();

		auto size = ::WideCharToMultiByte(CP_UTF8, 0, _text.data(), static_cast<int>(_text.size()), nullptr, 0, nullptr, nullptr);
		std::string result(static_cast<std::size_t>(size), '\0');
		::WideCharToMultiByte(CP_UTF8, 0, _text.data(), static_cast<int>(_text.size()), result.data(), size, nullptr, nullptr);
		return result;
	}
#endif

	//-----------------------------------------------------------------
	// service instance info
	//-----------------------------------------------------------------
	struct sSERVER_CERTIFICATION_INFO
	{
		ENABLE_STRUCT_SERIALIZABLE

		std::string			name;						// - application name
		std::string			comment;					// - comment
		sRANK				rank;						// - server rank (서버 소속(group, field), 서버의 역할 및 순번(role, index), 서버속성(attribute)
		sREVISION			revision;					// - repository 정보
		sVERSION_EX			version;					// - version
	};

	struct sSERVICE
	{
		ENABLE_STRUCT_SERIALIZABLE

		// 1) setting
		struct sSETTING
		{
			ENABLE_STRUCT_SERIALIZABLE

			// 1) instance id
			iid_t				iid{ IID_AUTO };			// - instance id값과 같은 값

			// 2) instance id
			std::string			name;						// - application name
			std::string			comment;					// - comment

			// 3) rank & configures
			sRANK				rank;						// - server rank (서버 소속(group, field), 서버의 역할 및 순번(role, index), 서버속성(attribute)

			// 4) version
			sREVISION			revision;					// - repository 정보
			sVERSION_EX			version;					// - version

			// 5) service type & execute file/directory
			eSERVICE_TYPE		service_type{ eSERVICE_TYPE::EXECUTE };	// - 실행 방법 (EXECUTE: 실행 파일, SERVICE: name이 서비스 이름, DOCKER: name이 container 이름)
			uint32_t			reserved{ 0 };					// - (8 byte 정렬. pack(1)이므로 다음 멤버를 8 byte에 맞춘다.)
			std::string			filename;					// - execute filename
			std::string			path;						// - File Path
			std::string			working_path;				// - Working Directory
			std::vector<std::string> arguments;				// - 실행 인자 (UTF-8. 실행 파일 이름은 포함하지 않는다.)
															//   . 채우는 곳
															//     -> host_agent: 설정의 "arguments" (server_admin.instance_manager.cpp)
															//     (실제로 적용된 실행 인자는 info.arguments에 보관한다. (Xservice::run))
															//   . 쓰는 곳 (비어 있지 않으면 실행 인자로 전달한다.)
															//     -> windows: CreateProcessW/CreateProcessAsUserW의 command line (get_arguments_string)
															//     -> linux  : execv/execve의 argv
			std::vector<std::string> environment;			// - 환경 변수 ('name=value' 목록. UTF-8)
															//   . 채우는 곳
															//     -> host_agent: 설정의 "environment" (server_admin.instance_manager.cpp)
															//   . 쓰는 곳 (host_agent: server_admin.instance_service_local.cpp)
															//     -> windows: 기본 환경(콘솔 모드: 부모 프로세스의 환경, 서비스 모드: CreateEnvironmentBlock으로 만든 사용자 기본 환경)에
															//                 추가/덮어쓴 UTF-16 블록(get_environment_block)을 CreateProcessW/CreateProcessAsUserW의 lpEnvironment로 전달
															//                 (이름은 대소문자를 구분하지 않는다.)
															//     -> linux  : 부모 프로세스의 환경에 추가/덮어쓴 후 execve로 실행한다.
															//     -> 비어 있으면 기본 환경을 그대로 쓴다.

			// 6) contexts
			context				context_initialize;			// - context for start
			context				context_start;				// - context for start

			// 7) options
			uint32_t			stop_grace_period{ 0 };		// - 멈출 때 기다리는 시간(초). 0이면 설정하지 않는다. (DOCKER: 'docker stop -t <초>'. 없으면 docker 기본값)

			// functions)
			void				clear() noexcept;

			// - arguments
			[[nodiscard]] std::string get_arguments_string(std::string_view _execute = {}) const;	// - 공백/따옴표가 있는 인자는 따옴표로 감싼다. (_execute가 있으면 맨 앞에 붙인다.)
			[[nodiscard]] static std::string quote_argument(std::string_view _argument);

			// - environment
		#if defined(_WIN32)
			[[nodiscard]] std::wstring get_environment_block(const wchar_t* _pbase_block = nullptr) const;	// - _pbase_block(기본 환경. UTF-16 블록)에 environment를 추가/덮어쓴 UTF-16 블록 ('name=value'마다 NULL, 끝에 NULL 하나 더. environment가 비어 있으면 빈 문자열)
		#endif
		} setting;

		// 2) info
		struct sINFO
		{
			ENABLE_STRUCT_SERIALIZABLE

			// 1) process id
			iid_t				host_iid{ IID_AUTO };		// - host instance id
			uint64_t			pid{ 0 };					// - process id

			// 2) initialize
			context				context_run;				// - context after started
			std::vector<std::string> arguments;				// - 실제로 적용된 실행 인자 (Xservice::run이 보관한다. setting.arguments는 요청한 값이다.)

			// 3) time
			chrono::time::utc::time_point time_register;	// - register time
			chrono::time::utc::time_point time_update;		// - update time
		} info;

		// 3) status
		struct sSTATUS
		{
			ENABLE_STRUCT_SERIALIZABLE

			// - instance id
			iid_t				iid{ 0 };					// - sSERVICE_INSTANCE_INFO의 iid값과 같은 값

			// - service state
			eOBJECT_STATE		state{ eOBJECT_STATE::NONE };
			uint32_t			dummy{ 0 };

			// - process/memory/network ...
			PROCESS::sSTATUS	process_status;

			// - entity status
			std::vector<factory_snap>				vector_factory;
			std::vector<executor_snap>				vector_executor;
			std::vector<schedulable_snap>			vector_schedulable;
			std::vector<connective_info_snap>		vector_connective;
			std::vector<socket_info_snap>			vector_binder;
			std::vector<time_event_snap>			vector_time_event;
			statistics::unit_connective_snap		connective_info;
			statistics::unit_traffic_snap			traffic_info;

			// - log infos
			circular_list<object_ptr<LOG_RECORD>>	vector_log_record;

			// - time updated
			chrono::time::utc::time_point time_state_changed;	// - 서버 정보를 등록한 시간.
			chrono::time::utc::time_point time_update;			// - 데이터가 작성된 시간

			void clear() noexcept
			{
				this->iid = 0;
				this->state = eOBJECT_STATE::NONE;
				this->dummy = 0;
				//process_status.clear();
				this->vector_factory.clear();
				this->vector_executor.clear();
				this->vector_schedulable.clear();
				this->vector_connective.clear();
				this->vector_binder.clear();
				this->vector_time_event.clear();
				this->connective_info = statistics::unit_connective_snap();
				this->traffic_info = statistics::unit_traffic_snap();
				this->vector_log_record.clear();
				this->time_state_changed = (chrono::time::utc::time_point::min)();
				this->time_update = (chrono::time::utc::time_point::min)();
			}
		} status;
	};

	struct sINSTANCE_KEY
	{
		ENABLE_STRUCT_SERIALIZABLE

		uint64_t	key{ 0 };
		chrono::time::utc::time_point time{ (chrono::time::utc::time_point::min)() };

		void initialize() noexcept;
	};

	bool operator==(const sINSTANCE_KEY& _left, const sINSTANCE_KEY& _right) noexcept;
	bool operator!=(const sINSTANCE_KEY& _left, const sINSTANCE_KEY& _right) noexcept;
	bool operator<(const sINSTANCE_KEY& _left, const sINSTANCE_KEY& _right) noexcept;
	bool operator>(const sINSTANCE_KEY& _left, const sINSTANCE_KEY& _right) noexcept;
	bool operator<=(const sINSTANCE_KEY& _left, const sINSTANCE_KEY& _right) noexcept;
	bool operator>=(const sINSTANCE_KEY& _left, const sINSTANCE_KEY& _right) noexcept;

	////-----------------------------------------------------------------------------
	///**

	//@class		sSERVICE_SETTING_INFO
	//@brief		설정 정보의 기본 내용
	//@todo
	//@ref		sHOST_SETTING_INFO, sSERVICE_INSTANCE_INFO, SERVICE_SETTING
	//@details	\n

	//*/
	////-----------------------------------------------------------------------------
	//// 1. 사용자가 설정할 수 있는 instance 정보들
	//struct sSERVICE_SETTING
	//{
	//	ENABLE_STRUCT_SERIALIZABLE

	//		// 1) instance id
	//		iid_t				iid{ 0 };					// - sSERVICE_INSTANCE_INFO의 iid값과 같은 값

	//	// 2) name/comment
	//	std::string			nickname;
	//	std::string			comment;

	//	// 3) setting
	//	sRANK				rank;						// - server rank (서버 소속(group, field), 서버의 역할 및 순번(role, index), 서버속성(attribute)
	//	std::array<uint64_t, 8>	parameter{};			// - options
	//	shared_buffer		buf_configure;				// - configure (JSON)
	//	shared_buffer		buf_setting;				// - setting info

	//	// 4) version
	//	sREVISION			revision;					// - reversion
	//	sVERSION_EX			version;					// - version

	//	// 5) execute info
	//	std::string			filename;				// - execute filename
	//	std::string			path;					// - File Path
	//	std::string			working_path;			// - Working Directory
	//	shared_buffer		buf_command_line;			// - Commandline(UNICODE, 1 NULL Terminate line, 2 NULL is Terminate Block)
	//	shared_buffer		buf_environment;			// - Environment block(UNICODE, 1 NULL Terminate line, 2 NULL is Terminate Block)

	//	// functions)
	//	bool set_configure_json_string(std::string_view _json_string);
	//	bool set_configure_json_string_from_file(std::string_view _filename);
	//	bool save();
	//	void clear() noexcept;
	//};



	//-----------------------------------------------------------------
	// get snap functions
	//-----------------------------------------------------------------
	// 1) factory
	factory_snap							get_snap				(const Ifactory* _pfactory) noexcept;
	factory_update_snap						get_update_snap			(const Ifactory* _pfactory) noexcept;
	std::vector<factory_snap>				get_snap				(factory::manager* _pmanager);
	std::vector<factory_update_snap>		get_update_snap			(factory::manager* _pmanager);
	std::vector<factory_snap>				get_snap				(factory::manager* _pmanager, eOBJECT_TYPE _object_type);
	std::vector<factory_update_snap>		get_update_snap			(factory::manager* _pmanager, eOBJECT_TYPE _object_type);

	// 2) executor
	executor_snap							get_snap				(Iexecutor* _pexector) noexcept;
	executor_update_snap					get_update_snap			(Iexecutor* _pexector) noexcept;
	std::vector<executor_snap>				get_snap				(executor::manager* _pexecutor_manager);
	std::vector<executor_update_snap>		get_update_snap			(executor::manager* _pexecutor_manager);

	// 3) schedulable
	schedulable_snap						get_snap				(const Ischedulable* _pschedulable) noexcept;
	schedulable_update_snap					get_update_snap			(const Ischedulable* _pschedulable) noexcept;
	std::vector<schedulable_snap>			get_snap				(schedulable::manager* _pschedulable_manager);
	std::vector<schedulable_update_snap>	get_update_snap			(schedulable::manager* _pschedulable_manager);

	// 4) connective
	statistics::unit_connective_snap		get_snap				(const statistics::unit_connective& _unit_connective) noexcept;
	statistics::unit_connective_snap		get_update_snap			(const statistics::unit_connective& _connective) noexcept;
	connective_info_snap					get_snap				(const net::io::Iconnective* _pconnective) noexcept;
	connective_info_update_snap				get_update_snap			(const net::io::Iconnective* _pconnective) noexcept;
	std::vector<connective_info_snap>		get_snap				(net::io::connective::manager* _pconnective_manager);
	std::vector<connective_info_update_snap> get_update_snap		(net::io::connective::manager* _pconnective_manager);

	// 5) traffic
	statistics::unit_traffic_snap			get_snap				(const statistics::unit_traffic& _traffic_info) noexcept;
	statistics::unit_traffic_snap			get_update_snap			(const statistics::unit_traffic& _traffic_info) noexcept;

	// 6) socket info (binder)
	socket_info_snap						get_snap				(const net::io::Ibinder* _pbinder) noexcept;
	socket_info_snap						get_snap				(const net::io::Ibinder* _pbinder, net::io::statistics::Nsocket* _pstatistics_socket) noexcept;
	socket_info_update_snap					get_update_snap			(const net::io::Ibinder* _pbinder) noexcept;
	std::vector<socket_info_snap>			get_snap				(net::io::binder::manager* _pbinder_manager);
	std::vector<socket_info_update_snap> get_update_snap			(net::io::binder::manager* _pbinder_manager);

	// 7) time_event
	time_event_snap							get_snap				(const time_event::Ientity* _ptime_event) noexcept;
	time_event_update_snap					get_update_snap			(const time_event::Ientity* _ptime_event) noexcept;
	std::vector<time_event_snap>			get_snap				(time_event::entity_manager* _pentity_manager);
	std::vector<time_event_update_snap>		get_update_snap(time_event::entity_manager * _pentity_manager);

	// 8) logger
	std::vector<object_ptr<LOG_RECORD>>		get_records_of_default_logger();
	circular_list<object_ptr<LOG_RECORD>>	get_records_of_default_logger_circular_list();


	//-----------------------------------------------------------------
	// message functions
	//-----------------------------------------------------------------
	// 1) certify
	shared_buffer make_message_SERVER_ADMIN_CERTIFY_REQUEST				(const sCERTIFY_INFO& _certify_info);
	shared_buffer make_message_SERVER_ADMIN_CERTIFY_REQUEST				(const sCERTIFY_INFO& _certify_info, std::string_view _certify_key);
	shared_buffer make_message_SERVER_ADMIN_CERTIFY_RESPONSE			(eRESULT _result);

	// 2) admin request
	shared_buffer make_message_SERVER_ADMIN_REQUEST_UPDATE_SETTING		(iid_t _instance_iid, iid_t _host_iid, const sSERVICE::sSETTING& _server_setting);
	shared_buffer make_message_SERVER_ADMIN_REQUEST_INIT_SERVICE		(iid_t _instance_iid);
	shared_buffer make_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE		(iid_t _instance_iid);
	shared_buffer make_message_SERVER_ADMIN_REQUEST_KILL_SERVICE		(iid_t _instance_iid);
	shared_buffer make_message_SERVER_ADMIN_REQUEST_START_SERVICE		(iid_t _instance_iid, const context& _setting);
	shared_buffer make_message_SERVER_ADMIN_REQUEST_STOP_SERVICE		(iid_t _instance_iid);
	shared_buffer make_message_SERVER_ADMIN_REQUEST_PAUSE_SERVICE		(iid_t _instance_iid);
	shared_buffer make_message_SERVER_ADMIN_REQUEST_CONTINUE_SERVICE	(iid_t _instance_iid);
	shared_buffer make_message_SERVER_ADMIN_INSTANCE_VERSION_UPDATE		(iid_t _instance_iid);

	// 3) admin instance process
	shared_buffer make_message_SERVER_ADMIN_INSTANCE_CERTIFY_REQUEST	(const host::manager* _phost_instance_manager, const service::manager* _pservice_instance_manager);
	shared_buffer make_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE	(const sSERVICE& _service_remote, const std::map<iid_t, object_ptr<host::Ninstance>>& _map_host_info_confirmed, const std::map<iid_t, object_ptr<service::Ninstance>>& _map_service_info_confirmed);
	shared_buffer make_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE	(eRESULT _result);

	shared_buffer make_message_SERVER_ADMIN_INSTANCE_ENTER				(const std::vector<sSERVICE>& _vector_instance);
	shared_buffer make_message_SERVER_ADMIN_INSTANCE_LEAVE				(iid_t _service_iid);

	// 4) admin instance info
	shared_buffer make_message_SERVER_ADMIN_INSTANCE_REGISTER			(Ibuffer_serializable* _pinstance);
	shared_buffer make_message_SERVER_ADMIN_INSTANCE_UNREGISTER			(iid_t _iid);
	shared_buffer make_message_SERVER_ADMIN_INSTANCE_UPDATE				(iid_t _iid, Ibuffer_serializable& _instance);
	shared_buffer make_message_SERVER_ADMIN_INSTANCE_UPDATE_SETTING		(iid_t _iid, const sSERVICE::sSETTING& _setting);
	shared_buffer make_message_SERVER_ADMIN_INSTANCE_UPDATE_INFO		(iid_t _iid, const sSERVICE& _service_info);
	shared_buffer make_message_SERVER_ADMIN_INSTANCE_UPDATE_STATUS		(iid_t _iid, const sSERVICE::sSTATUS& _service_status);
	shared_buffer make_message_SERVER_ADMIN_INSTANCE_UPDATE_STATUS		(iid_t _iid);
	// - host_agent web ui 상태 (HOST_UPDATE_STATUS의 system status 뒤에 붙는다. [uint32 field 수][uint64 x field 수])
	//   * 없으면(이전 host_agent) 읽지 않고, 모르는 field(이후 버전)는 건너뛴다. (admin/host_agent를 따로 배포할 수 있다.)
	struct sHOST_WEB_STATUS
	{
		uint64_t		web_connections = 0;
		uint64_t		web_timeout_request = 0;
		uint64_t		web_timeout_idle = 0;
		uint64_t		websocket_connections = 0;
		uint64_t		websocket_timeout_request = 0;
		uint64_t		websocket_timeout_idle = 0;
	};
	shared_buffer make_message_SERVER_ADMIN_HOST_UPDATE_STATUS			(iid_t _host_iid, const SYSTEM::sSTATUS& _system_status, const sHOST_WEB_STATUS* _pweb_status = nullptr);
	[[nodiscard]] SYSTEM::sSTATUS extract_system_status					(buffer_view& _buffer);
	[[nodiscard]] std::optional<sHOST_WEB_STATUS> extract_host_web_status	(buffer_view& _buffer);
	shared_buffer make_message_SERVER_ADMIN_INSTANCE_UPDATE_STATE		(iid_t _iid, eOBJECT_STATE _status_new, eOBJECT_STATE _status_old, const sSERVICE* _service_info);
	shared_buffer make_message_SERVER_ADMIN_INSTANCE_UPDATE_PID			(iid_t _iid, uint64_t _pid);
	shared_buffer make_message_SERVER_ADMIN_ERROR						(iid_t _iid, uint32_t _request, int _error_code, const std::string_view _error_message);
	shared_buffer make_message_SERVER_ADMIN_REQUEST_RESULT				(iid_t _iid, uint32_t _request, uint64_t _request_id, eRESULT _result, std::string_view _message);

	// - request id (optional. appended at the end of REQUEST_xxx by admin server, returned in REQUEST_RESULT)
	//   (services ignore the trailing bytes, so the old services work with it)
	[[nodiscard]] shared_buffer append_request_id(const shared_buffer& _message, uint64_t _request_id);
	[[nodiscard]] uint64_t extract_request_id(buffer_view& _remained) noexcept;

	shared_buffer make_message_FACTORY_FACTORY_REGISTER					(iid_t _iid, Ifactory* _pfactory);
	shared_buffer make_message_FACTORY_FACTORY_UNREGISTER				(iid_t _iid, Ifactory* _pfactory);
	shared_buffer make_message_FACTORY_FACTORY_UPDATE					(iid_t _iid, Ifactory* _pfactory);

	shared_buffer make_message_EXECUTOR_REGISTER						(iid_t _iid, Iexecutor* _pexecutor);
	shared_buffer make_message_EXECUTOR_UNREGISTER						(iid_t _iid, Iexecutor* _pexecutor);
	shared_buffer make_message_EXECUTOR_UPDATE							(iid_t _iid, Iexecutor* _pexecutor);

	shared_buffer make_message_NETWORK_SOCKET_CONNECTIVE_REGISTER		(iid_t _iid, const net::io::Iconnective* _pconnective);
	shared_buffer make_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER		(iid_t _iid, const net::io::Iconnective* _pconnective);
	shared_buffer make_message_NETWORK_SOCKET_CONNECTIVE_UPDATE			(iid_t _iid, const net::io::Iconnective* _pconnective);

	shared_buffer make_message_NETWORK_SOCKET_BINDER_REGISTER			(iid_t _iid, net::io::Ibinder* _pbinder, net::io::statistics::Nsocket* _pstatistics_socket);
	shared_buffer make_message_NETWORK_SOCKET_BINDER_UNREGISTER			(iid_t _iid, net::io::Ibinder* _pbinder);
	shared_buffer make_message_NETWORK_SOCKET_BINDER_UPDATE				(iid_t _iid, net::io::Ibinder* _pbinder);

	shared_buffer make_message_SERVER_TIME_EVENT_EVENT_REGISTER			(iid_t _iid, time_event::Ientity* _ptime_event);
	shared_buffer make_message_SERVER_TIME_EVENT_EVENT_UNREGISTER		(iid_t _iid, time_event::Ientity* _ptime_event);
	shared_buffer make_message_SERVER_TIME_EVENT_EVENT_EXECUTE			(iid_t _iid, time_event::Ientity* _ptime_event);

	shared_buffer make_message_SERVER_ADMIN_DUMP_INFO_LIST				(iid_t _iid);
	shared_buffer make_message_SERVER_ADMIN_DUMP_INFO					(iid_t _iid);

	shared_buffer make_message_SYSTEM_LOG								(iid_t _iid, const LOG_RECORD* _plog_record);
	shared_buffer make_message_SYSTEM_LOG								(iid_t _iid, const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record);
}

}

#pragma pack()

#define	CONFIGURE_JSON_FILENAME	"host_agent_info.json"
