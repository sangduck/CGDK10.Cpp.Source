#include "pch.h"

#if defined(_WIN32)
	#include "WinInet.h"
#elif defined(__ANDROID__)
#elif defined(__APPLE__)
#elif defined(__linux__)
#endif

using namespace CGDK;


#define	DEFAULT_SLACK_ADDRESS	 "trollgames.slack.com"

logger_slack::logger_slack()
{
	//const wchar_t*	str_token = L"services/hooks/subversion?token=ed0ekZxgakbc94kTFxyYiYpo";		// Live
	//const char*	str_token	 = L"services/hooks/subversion?token=S8YFTFZLyxqWMd3gkuhiUrSd";	// Alpha
}

logger_slack::logger_slack(const std::string_view _name) :
	Nlogger(_name)
{
}

logger_slack::~logger_slack() noexcept
{
	this->destroy();
}

result_code logger_slack::initialize(const INITIALIZE_PARAMETER& _initialize_param)
{
	// check)
	RETURN_IF(_initialize_param.token.empty(), eRESULT::NOT_READY);

	// declare) 
	context context_setting;

	// 1) Address를 설정한다.
	if (_initialize_param.address.empty() == false)
	{
		context_setting["address"sv] = _initialize_param.address;
	}

	// 2) Token을 설정한다.
	context_setting["token"sv] = _initialize_param.token;

	// 3) Channel을 설정한다.
	context_setting["channel"sv] = _initialize_param.channel;

	// 4) Init
	return this->Ninitializable::initialize(context_setting);
}

result_code logger_slack::initialize(const std::string_view _token, const std::string_view _channel, const std::string_view _address)
{
	// check)
	RETURN_IF(_token.empty(), eRESULT::NOT_READY);

	// declare) 
	context context_setting;

	// 1) Address를 설정한다.
	if (_address.empty() == false)
	{
		context_setting["address"sv] = _address;
	}

	// 2) Token을 설정한다.
	context_setting["token"sv] = _token;

	// 3) Channel을 설정한다.
	context_setting["channel"sv] = _channel;

	// 4) Init
	return this->Ninitializable::initialize(context_setting);
}

result_code logger_slack::_process_initializing(context& _context)
{
	// trace) 
	LOG_PROGRESS << "@ <BEGIN> initializing LOGGER_SLACK["sv << name() << "]"sv;

	// 1) on_initializing 함수를 호출한다.
	on_initializing(_context);

	// 2) Address값을 읽는다.
	std::string str_address = _context["address"sv];

	// 3) Token값을 읽는다.
	std::string str_token = _context["token"sv];

	// 4) Channel값을 읽는다.
	std::string str_channel = _context["channel"sv];

	// check)
	THROW_IF(str_token.empty() || str_channel.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0));

	// 5) Slack Address를 넣는다.
	if (str_address.empty() == false)
	{
		this->m_str_address = str_address;
	}
	else
	{
		this->m_str_address = DEFAULT_SLACK_ADDRESS;
	}

	// 7) Slack Token을 설정한다.
	this->m_str_token = "services/hooks/subversion?token"s;
	this->m_str_token += str_token;

	// 8) Channel
	this->m_str_channel = str_channel;

	// trace) Event Manager를 생성한다.
	LOG_PROGRESS << "@ <END> initializing LOGGER_SLACK["sv << name() << "]"sv;

	// return)
	return eRESULT::SUCCESS;
}

result_code logger_slack::_process_destroying() noexcept
{
	// 1) Default Tracer로 설정되어 있다면 제거한다.
	reset_default_logger(this);

	// 2) detach from parent logger
	this->detach_from_parent();

	// 3) on_destroying 함수를 호출한다.
	try
	{
		this->on_destroying();
	}
	catch(...)
	{
		// Log) 
		LOG_ERROR << "@ (excp) exception on 'on_destroying' function call ['"sv << name() << "']"sv;
	}

	// 3) destroy처리....
	this->m_str_address.clear();
	this->m_str_token.clear();
	this->m_str_channel.clear();

	// return)
	return eRESULT::SUCCESS;
}

#define SAFE_CLOSE_INTERNET_HANDLE(handle) if(handle) { InternetCloseHandle(handle); handle = NULL;}

void logger_slack::process_log(const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record)
{
	// check) _plog_record must be not null
	RETURN_IF(_vector_log_record.empty());

	// check) 
	RETURN_IF(this->get_object_state() < eOBJECT_STATE::STOPPED || this->get_object_state() > eOBJECT_STATE::RUNNING);

	// check)
	RETURN_IF(this->m_str_token.empty() || this->m_str_channel.empty());


	////-----------------------------------------------------------------
	//// 1) Filtering
	////    - Filter가 존재할 경우 Filtering을 수행하고 그 결과가 false가
	////      나올 경우 Log를 남기지 않는다.
	////    - Filter가 설정되어 있지 않을 경우 그냥 통과한다.
	////-----------------------------------------------------------------
	//if (m_plog_filter.exist())
	//{
	//	RETURN_IF(m_plog_filter->process_filtering(_plog_record) == false);
	//}


	////-----------------------------------------------------------------
	//// 2. Log Count
	////-----------------------------------------------------------------
	//// statistics) 
	//process_count_log(_plog_record->type);


	////-----------------------------------------------------------------
	//// 3. Slack에 Message를 전달한다.
	////-----------------------------------------------------------------
	//system_executor::post([=]
	//{
	//#if !defined(__linux__)

	//	// declare) 
	//	HINTERNET hSession		 = NULL;
	//	HINTERNET hConnection	 = NULL;
	//	HINTERNET hRequest		 = NULL;

	//	// {"channel": "#server_status", "text" : " %s\n Log : %s\n" }

	//	//// check) message must be not empty
	//	//RETURN_IF(_plog_record->buf_message.empty());

	//	try
	//	{
	//		// definitions) 
	//		const char* str_header = "Content-type: application/x-www-form-urlencoded;\r\n";
	//		char message[4096] { 0, };

	//		// 1) Internet을 open한다.
	//		hSession = InternetOpenW(L"Request", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

	//		// check) Fail on create session
	//		THROW_IF(hSession == NULL, throwable::failure(eRESULT::FAIL_TO_OPEN));

	//		// 2) Connect to Slack
	//		hConnection = InternetConnectA(hSession, m_str_address.c_str(), 443, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);

	//		// check) Fail on Connection
	//		THROW_IF(hConnection == NULL, throwable::failure());

	//		// 3) open Channel
	//		hRequest = HttpOpenRequestA(hConnection, "POST", m_str_token.c_str(), "HTTP/1.1", NULL, NULL, INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);

	//		// check) Fail on Openning request
	//		THROW_IF(hRequest == NULL, throwable::failure());

	//		// 4) send Request
	//		auto bResult = HttpSendRequestA(hRequest, str_header, (DWORD)__strlen(str_header), (LPVOID)message, (DWORD)__strlen(message));

	//		// check) 
	//		THROW_IF(bResult == 0, throwable::failure());

	//		// 5) Read Result
	//		DWORD dwBytesRead = 0;
	//		InternetReadFile(hRequest, (char*)message, sizeof(message)-1, &dwBytesRead);
	//	}
	//	catch (...)
	//	{
	//	}

	//	// 6) close
	//	SAFE_CLOSE_INTERNET_HANDLE(hRequest);
	//	SAFE_CLOSE_INTERNET_HANDLE(hConnection);
	//	SAFE_CLOSE_INTERNET_HANDLE(hSession);
	//#else
	//#endif
	//});
}


//const wchar_t*	str_token = L"services/hooks/subversion?token=ed0ekZxgakbc94kTFxyYiYpo";
//const char*	str_token	 = L"services/hooks/subversion?token=S8YFTFZLyxqWMd3gkuhiUrSd";	// Alpha
//const int	headLen		 = (sizeof(int) * 7 + sizeof(SYSTEMTIME) + sizeof(WORD));
//WCHAR*		pstrBuffer	 = (WCHAR*)(_msg.Buffer.buf + headLen);

//int isSlackSend = Code::instance().exist("Server") == true ?	Code::instance()["Server"sv]["UseSlack"sv][0].asInt() : 1;

//if (isSlackSend)
//{
//	INT			isReal		 = 0;
//	std::string		message	 = MakeSlackMsg(pstrBuffer, isReal);
//		std::string res;
//		SendMsgSlack(message.c_str(), res, isReal);
//	});
//}


//void logger_slack::set_server_info(const WCHAR* pServerName, const WCHAR* pServerVersion)
//{
//	if (!pServerName)
//		pServerName = L"Unnamed";
//	if (!pServerVersion)
//		pServerVersion = L"0";
//
//	CHAR host[kMaximumLineLength];
//
//	gethostname(host, sizeof(host));
//
//	struct hostent *host_entry;
//
//	host_entry = gethostbyname(host);
//
//	char * szLocalIP;
//
//	szLocalIP = inet_ntoa(*(struct in_addr *)*host_entry->h_addr_list);
//
//
//	m_kCustomInfoEntries[0] = CustomInfoEntry(L"ServerHost", std::string(CA2W(host)).c_str());
//	m_kCustomInfoEntries[1] = CustomInfoEntry(L"ServerIP", std::string(CA2W(szLocalIP)).c_str());
//	m_kCustomInfoEntries[2] = CustomInfoEntry(L"ServerName", pServerName);
//	m_kCustomInfoEntries[3] = CustomInfoEntry(L"pServerVersion", pServerVersion);
//}

//char logger_slack::to_hex2(char code)
//{
//	static CHAR hex[] = "0123456789ABCDEF";
//	return hex[code & 15];
//}

//std::string logger_slack::MakeSlackMsg(const wchar_t* _msg)
//{
//	wchar_t		sendtext[4096];
//	//WCHAR		msg[kMaximumLineLength] = { NULL, };
//
//	//std::string str_line;
//
//	//for (size_t i = 0; i < INFO_SIZE; ++i) 
//	//{
//	//	if (i > 0) 
//	//	{
//	//		str_line += L"\\n";
//	//	}
//	//	str_line += m_kCustomInfoEntries[i].name;
//	//	str_line += L": ";
//	//	str_line += m_kCustomInfoEntries[i].value;
//	//}
//
//	//WCHAR* pStr = NULL;
//
//	//isReal = 1;
//
//	//for (size_t i = 0; i < 5; ++i) 
//	//{
//	//	if ((pStr = wcsstr(m_kCustomInfoEntries[1].value, g_serverip[i])) != NULL)
//	//	{
//	//		isReal = 0;
//	//		break;
//	//	}
//	//	if ((pStr = wcsstr(m_kCustomInfoEntries[0].value, L"-PC")) != NULL)
//	//	{
//	//		isReal = 0;
//	//		break;
//	//	}
//	//}
//
//	//__sprintf(msg, kMaximumLineLength, TEXT("{\"channel\": \"#server_status\", \"text\": \"") TEXT("%s\\n") TEXT("Log : %s\\n") TEXT("\" }"), str_line.c_str(), p_Msg);
//
//	//strcpy_s(sendtext, "payload=");
//	//url_encode(strchr(sendtext, '=') + 1, std::string(CW2A(msg, CP_UTF8)).c_str());
//
//	return sendtext;
//}

//inline int logger_slack::url_encode(char* dst, const char *str)
//{
//	int encodedCnt = 0;
//	char *pstr = (char*)str, *pbuf = dst;
//	while (*pstr) 
//	{
//		if (*pstr > 0x00 && (isalnum(*pstr) || *pstr == '-'
//			|| *pstr == '_' || *pstr == '.' || *pstr == '~'))
//			*pbuf++ = *pstr,
//			encodedCnt++;
//		else
//			*pbuf++ = '%',
//			*pbuf++ = to_hex2(*pstr >> 4),
//			*pbuf++ = to_hex2(*pstr & 15),
//			encodedCnt += 3;
//		pstr++;
//	}
//
//	*pbuf = '\0';
//
//	return encodedCnt;
//}
//
