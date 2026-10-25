#include "pch.h"
#include <charconv>
#include <cstring>


#if defined(_WIN32)

CGDK::sDEVICE_INFO		g_device_info;

#include <thread>
#include <Psapi.h>
#include <Iphlpapi.h>
#include <intrin.h>

namespace CGDK
{


std::array<unsigned int, 4> _get_cpu_id()
{
	std::array<unsigned int, 4> cpui{0,0,0,0};

	__cpuid(reinterpret_cast<int*>(cpui.data()), 0);

	return cpui;
}

std::string _get_os_release_id() noexcept
{
	// declare) 
	HKEY handle_key;

	// 1) 
	auto result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &handle_key);

	// check) 
	RETURN_IF(result != ERROR_SUCCESS, std::string());

	// declare) 
	char str_value[256] = {};

	// declare) 
	DWORD size1 = sizeof(str_value) - 1;
	DWORD Type = 0;

	// 2) 
	RegQueryValueExA(handle_key, "ReleaseId", NULL, &Type, reinterpret_cast<BYTE*>(str_value), &size1);

	// 3) Key 닫는다.
	RegCloseKey(handle_key);

	// return) ...
	return std::string((char*)str_value);
}

sVERSION_EX _get_os_version() noexcept
{
	// declare)
	sVERSION_EX temp_version;

	// declare) 
	HKEY handle_key;

	// 1) 
	auto result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &handle_key);

	// check) 
	RETURN_IF(result != ERROR_SUCCESS, temp_version);

	// declare) 
	char str_value[256] = {};

	// declare) 
	DWORD size1 = sizeof(str_value) - 1;
	DWORD Type = 0;

	// 2) MajorVersionNumber
	RegQueryValueExA(handle_key, "CurrentMajorVersionNumber", NULL, &Type, reinterpret_cast<BYTE*>(str_value), &size1);
	temp_version.major = *(uint16_t*)str_value;

	// 3) MinorVersionNumber
	size1 = sizeof(str_value);
	Type = 0;

	RegQueryValueExA(handle_key, "CurrentMinorVersionNumber", NULL, &Type, reinterpret_cast<BYTE*>(str_value), &size1);
	temp_version.minor = *(uint16_t*)str_value;

	// 4) CurrentBuildNumber
	size1 = sizeof(str_value);
	Type = 0;

	RegQueryValueExA(handle_key, "CurrentBuildNumber", NULL, &Type, reinterpret_cast<BYTE*>(str_value), &size1);
	{
		const std::string_view str_build(reinterpret_cast<const char*>(str_value));
		std::from_chars(str_build.data(), str_build.data() + str_build.size(), temp_version.build);
	}

	// 6) close key
	RegCloseKey(handle_key);

	// 7) string version
	temp_version.str_version = CGDK::stream_string(temp_version.major, ".", temp_version.minor, ".", temp_version.build);

	// return)
	return temp_version;
}

sDEVICE_INFO _get_device_info()
{
	// declare)
	sDEVICE_INFO temp;

	// 1) set values
	temp.device_id = "any"sv; // android:IMEI, ios:UDID
	temp.model = "Microsoft"sv;
	temp.hardware_serial;
	temp.sim_id;
	temp.sim_serial;
	temp.aaid;

	// return) 
	return temp;
}

std::string _get_os_product_name() noexcept
{
	// declare) 
	HKEY handle_key;

	// 1) 
	auto result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &handle_key);

	// check) 
	RETURN_IF(result != ERROR_SUCCESS, std::string());

	// declare) 
	char str_value[256] = {};

	// declare) 
	DWORD size1 = sizeof(str_value) - 1;
	DWORD Type = 0;

	// 2) 
	RegQueryValueExA(handle_key, "ProductName", NULL, &Type, reinterpret_cast<BYTE*>(str_value), &size1);

	// 3) Key 닫는다.
	RegCloseKey(handle_key);

	// return) ...
	return std::string((char*)str_value);
}


const sDEVICE_INFO& api::get_device_info() noexcept
{
	if (g_device_info.device_id.empty())
	{
		g_device_info = _get_device_info();
	}

	// return) 
	return g_device_info;
}

//return fmt::format("{:08X}{:08X}{:08X}{:08X}", cpui[0], cpui[1], cpui[2], cpui[3]);


std::array<unsigned int, 4> api::get_device_unique_id() noexcept
{
	return _get_cpu_id();
}

SYSTEM::sINFO api::get_system_info() noexcept
{
	// declare) 
	SYSTEM::sINFO system_resource_info;

	// declare) 
	::SYSTEM_INFO ssi;

	// 1) 값을 초기화
	std::memset(&ssi, 0, sizeof(ssi));

	// 2) 생성되면 먼저 System Info를 설정한다.
	GetSystemInfo(&ssi);

	// 3) 값을 설정한다.
	system_resource_info.processor.type = ePROCESSOR_TYPE::x86_64;
	system_resource_info.processor.level = ssi.wProcessorLevel;
	system_resource_info.processor.revision = ssi.wProcessorRevision;
	system_resource_info.processor.number_of_processor = ssi.dwNumberOfProcessors;
	system_resource_info.memory.bytes_physical_usage = 0;
	system_resource_info.memory.bytes_physical_total = get_physical_memory_max();
	system_resource_info.network.dummy = 0;

	// return) 
	return system_resource_info;
}

SYSTEM::sSTATUS api::get_system_status() noexcept
{
	// declare)
	SYSTEM::sSTATUS temp_system_status;

	// declare) 
	::SYSTEM_INFO ssi;

	// 1) 값을 초기화
	std::memset(&ssi, 0, sizeof(ssi));

	// 2) 생성되면 먼저 System Info를 설정한다.
	GetSystemInfo(&ssi);

	// 3) 값을 설정한다.
	temp_system_status.processor_usage = 0;
	temp_system_status.thread_count = 0;
	temp_system_status.process_count = 0;
	temp_system_status.handle_count = 0;
	temp_system_status.bytes_memory_usage = static_cast<uint64_t>(ssi.dwPageSize) * static_cast<uint64_t>(ssi.dwAllocationGranularity);
	temp_system_status.bytes_traffic = 0;
	temp_system_status.time_update = chrono::time::utc::now();

	// return) 
	return temp_system_status;
}

sOS_INFO api::get_os_info() noexcept
{
	// declare)
	sOS_INFO os_info;

	// 1) set values
	os_info.type = eOS_TYPE::WINDOWS_NATIVE;
	os_info.name = "Windows";
	os_info.version = _get_os_version();
	os_info.product_name = _get_os_product_name();
	os_info.language = eLANGUAGE::KOREAN;

	// return) ...
	return os_info;
}

std::string api::get_os_unique_id() noexcept
{
	// declare) 
	HKEY handle_key;

	// 1) 
	auto result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &handle_key);

	// check) 
	RETURN_IF(result != ERROR_SUCCESS, std::string());

	// declare) 
	char str_value[256] = {};

	// declare) 
	DWORD size1 = sizeof(str_value) - 1;
	DWORD Type = 0;

	// 2) 
	RegQueryValueExA(handle_key, "Productid", NULL, &Type, reinterpret_cast<BYTE*>(str_value), &size1);

	// 3) Key 닫는다.
	RegCloseKey(handle_key);

	// return) ...
	return std::string((char*)str_value);
}

uint64_t api::get_process_id()
{
	return GetCurrentProcessId();
}

// - the CPUs this process may run on : the affinity of the process (start /affinity, a job object with an affinity) and the CPU
//   rate limit of its job object (a container), the smaller (hardware_concurrency() is the CPUs of the machine : too many threads
//   in a container or with an affinity, as on linux with sched_getaffinity / cgroup)
static std::size_t get_available_core_count() noexcept
{
	// 1) machine (all the processor groups)
	std::size_t count = ::GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
	if (count == 0)
		count = std::thread::hardware_concurrency();

	// 2) affinity of the process (one processor group only : with threads in several groups both masks are 0)
	DWORD_PTR mask_process = 0;
	DWORD_PTR mask_system = 0;
	if (::GetProcessAffinityMask(::GetCurrentProcess(), &mask_process, &mask_system) && mask_process != 0 && ::GetActiveProcessorGroupCount() == 1)
	{
		std::size_t count_affinity = 0;
		for (auto mask = static_cast<uint64_t>(mask_process); mask != 0; mask &= mask - 1)
			++count_affinity;

		if (count_affinity != 0 && count_affinity < count)
			count = count_affinity;
	}

	// 3) CPU rate limit of the job object (hard cap : CpuRate is in 1/100 % of all the CPUs)
	JOBOBJECT_CPU_RATE_CONTROL_INFORMATION info_rate{};
	if (::QueryInformationJobObject(nullptr, JobObjectCpuRateControlInformation, &info_rate, sizeof(info_rate), nullptr)
	 && (info_rate.ControlFlags & JOB_OBJECT_CPU_RATE_CONTROL_ENABLE) != 0
	 && (info_rate.ControlFlags & JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP) != 0
	 && (info_rate.ControlFlags & (JOB_OBJECT_CPU_RATE_CONTROL_WEIGHT_BASED | JOB_OBJECT_CPU_RATE_CONTROL_MIN_MAX_RATE)) == 0
	 && info_rate.CpuRate > 0)
	{
		const auto count_machine = static_cast<std::size_t>(::GetActiveProcessorCount(ALL_PROCESSOR_GROUPS));
		const auto count_rate = (count_machine * info_rate.CpuRate + 9999) / 10000;

		if (count_rate != 0 && count_rate < count)
			count = count_rate;
	}

	// return)
	return (count != 0) ? count : 1;
}

std::size_t	api::get_core_count() noexcept
{
	static const std::size_t s_count = get_available_core_count();
	return s_count;
}

bool api::set_current_thread_affinity(int _cpu) noexcept
{
	// check) one processor group (64 CPUs) only
	RETURN_IF(_cpu < 0 || _cpu >= 64, false);

	// 1) pin this thread
	return ::SetThreadAffinityMask(::GetCurrentThread(), static_cast<DWORD_PTR>(1) << _cpu) != 0;
}

bool api::set_current_thread_priority(eTHREAD_PRIORITY _priority) noexcept
{
	// declare)
	int priority;

	// 1) the priority of windows
	switch (_priority)
	{
	case eTHREAD_PRIORITY::LOWEST:			priority = THREAD_PRIORITY_LOWEST; break;
	case eTHREAD_PRIORITY::LOW:				priority = THREAD_PRIORITY_BELOW_NORMAL; break;
	case eTHREAD_PRIORITY::NORMAL:			priority = THREAD_PRIORITY_NORMAL; break;
	case eTHREAD_PRIORITY::HIGH:			priority = THREAD_PRIORITY_ABOVE_NORMAL; break;
	case eTHREAD_PRIORITY::HIGHEST:			priority = THREAD_PRIORITY_HIGHEST; break;
	case eTHREAD_PRIORITY::TIME_CRITICAL:	priority = THREAD_PRIORITY_TIME_CRITICAL; break;
	default:								return true;	// - DEFAULT : not changed
	}

	// 2) set
	return ::SetThreadPriority(::GetCurrentThread(), priority) != FALSE;
}

int64_t	api::get_memory_usage() noexcept
{
	// declare) 
	PROCESS_MEMORY_COUNTERS_EX sInfoMemoryCounters;
	sInfoMemoryCounters.cb = sizeof(sInfoMemoryCounters);

	// 1) memory 정보를 얻는다.
	GetProcessMemoryInfo(GetCurrentProcess(), (PPROCESS_MEMORY_COUNTERS)&sInfoMemoryCounters, sizeof(sInfoMemoryCounters));

	// return) 
	return sInfoMemoryCounters.PrivateUsage;
}

uint64_t api::get_physical_memory_max() noexcept
{
	// declare) 
	PERFORMANCE_INFORMATION performance_info;

	// 1) memory 상태를 얻는다.
	GetPerformanceInfo(&performance_info, sizeof(PERFORMANCE_INFORMATION));

	// return)
	return ((uint64_t)performance_info.PhysicalTotal) * ((uint64_t)performance_info.PageSize);
}

uint64_t api::get_max_open_files() noexcept
{
	//// declare) 
	//HKEY handle_key;

	//// 1) 
	//auto result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\Tcpip\\Parameters", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &handle_key);

	//// check) 
	//RETURN_IF(result != ERROR_SUCCESS, 0);

	//// declare) 
	//DWORD value = 0;
	//DWORD size1 = sizeof(value);
	//DWORD type = 0;

	//// 2) 
	//result = RegQueryValueExA(handle_key, "MaxUserPort", NULL, &type, reinterpret_cast<BYTE*>(&value), &size1);

	//// 3) Key 닫는다.
	//RegCloseKey(handle_key);

	//// return) ...
	//return (type == REG_DWORD) ? value : 0;

	// return
	return std::numeric_limits<uint64_t>::max();
}

void api::set_max_open_files(uint64_t _max_open_files) noexcept
{
	//_setmaxstdio(_max_open_files);

	// declare) 
	LONG status;
	HKEY handle_key;

	// 1) ...
	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\Tcpip\\Parameters", 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &handle_key);

	// check) 
	RETURN_IF((status != ERROR_SUCCESS) || (handle_key == NULL));

	// 2) set value
	status = RegSetValueEx(handle_key, "MaxUserPort", 0, REG_DWORD, (const BYTE*)&_max_open_files, sizeof(DWORD));

	// 3) close key
	RegCloseKey(handle_key);
}

void api::reserve_max_open_files() noexcept
{
}


int32_t	api::get_last_error() noexcept
{
	return ::GetLastError();
}

thread_local char g_str_error[1024];
std::string_view api::get_error_string(int32_t _errno) noexcept
{
	// check) 
	RETURN_IF(_errno == 0, std::string_view());

	// 1) ...
	auto len_string = FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			_errno,
			0,
			(LPTSTR)g_str_error,
			1024,
			NULL
		);

	// check) 
	RETURN_IF(len_string == 0, std::string_view());

	// return) 
	return std::string_view(g_str_error, len_string);
}

std::string_view api::get_last_error_string() noexcept
{
	return api::get_error_string(api::get_last_error());
}

void api::enable_cin_echo(bool _enable)
{
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(hStdin, &mode);

	if (_enable)
		mode |= ENABLE_ECHO_INPUT;
	else
		mode &= (~ENABLE_ECHO_INPUT);

	SetConsoleMode(hStdin, mode);
}

struct dmi_header
{
	BYTE type;
	BYTE length;
	WORD handle;
};

struct RawSMBIOSData
{
	BYTE    Used20CallingMethod;
	BYTE    SMBIOSMajorVersion;
	BYTE    SMBIOSMinorVersion;
	BYTE    DmiRevision;
	DWORD   Length;
	BYTE    SMBIOSTableData;
};

std::string _dmi_system_uuid(const BYTE* _p, short _version)
{
	// declare)
	std::string result;

	// declare) 
	int only0xFF = 1;
	int only0x00 = 1;
	for (int i = 0; i < 16 && (only0x00 || only0xFF); ++i)
	{
		if (_p[i] != 0x00) only0x00 = 0;
		if (_p[i] != 0xFF) only0xFF = 0;
	}
	// check) 
	RETURN_IF(only0xFF, result);
	// check) 
	RETURN_IF(only0x00, result);

	if (_version >= 0x0206)
	{
		char temp[256];
		std::snprintf(temp, sizeof(temp), "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
			_p[3], _p[2], _p[1], _p[0], _p[5], _p[4], _p[7], _p[6], _p[8], _p[9], _p[10], _p[11], _p[12], _p[13], _p[14], _p[15]);

		result = temp;
	}
	else
	{
		char temp[256];

		std::snprintf(temp, sizeof(temp), "-%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
			_p[0], _p[1], _p[2], _p[3], _p[4], _p[5], _p[6], _p[7], _p[8], _p[9], _p[10], _p[11], _p[12], _p[13], _p[14], _p[15]);

		result = temp;
	}

	// return)
	return result;
}

std::string api::get_machine_name() noexcept
{
	// declare) 
	char computerName[256];
	DWORD size = 256;

	// 1) get machine name
	auto result = GetComputerName(computerName, &size);

	// check)
	RETURN_IF(result == false, std::string());

	// return) 
	return std::string(computerName);
}

std::string api::get_system_unique_id() noexcept
{
	// declare) 
	std::string result;

	// 1) get system firmware table size
	int bufsize = GetSystemFirmwareTable('RSMB', 0, 0, 0);

	// check) 
	RETURN_IF(!bufsize, result);

	// 2) alloc temporary buffer
	auto buf_temp = alloc_shared_buffer(bufsize);

	// 3) get temporary buffer
	BYTE* buf = buf_temp.data<BYTE>();

	// 4) reset buf
	buf[0] = 0;

	// 5) get system firmware table data
	auto ret = GetSystemFirmwareTable('RSMB', 0, buf, bufsize);

	// check) 
	RETURN_IF(!ret, result);

	auto* Smbios = (RawSMBIOSData*)buf;
	BYTE* p = &Smbios->SMBIOSTableData;

	// check) 
	RETURN_IF(Smbios->Length != static_cast<DWORD>(bufsize - 8), result);

	for (int i = 0; i < static_cast<int>(Smbios->Length); ++i)
	{
		auto h = (dmi_header*)p;

		if (h->type == 1)
		{
			result = _dmi_system_uuid(p + 0x8, Smbios->SMBIOSMajorVersion * 0x100 + Smbios->SMBIOSMinorVersion);
			break;
		}
		p += h->length;
		while ((*(WORD*)p) != 0) p++;
		p += 2;
	}

	return result;
}


}

void _fflush()
{
	fflush(stdin);
}

#endif