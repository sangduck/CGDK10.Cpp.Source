#include "pch.h"
#include <charconv>

#if defined(__linux__)

CGDK::sDEVICE_INFO		g_device_info;

#include <thread>
#include <pthread.h>
#include <sched.h>
#include <sys/utsname.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <fstream>
#include <stdio.h>
#include <fcntl.h>
#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

// - for _kbhit, _getch
#include <unistd.h>
#include <termios.h>  
#include <dirent.h>
#include <sstream>
#include <mutex>


namespace CGDK
{

std::array<unsigned int, 4> _get_cpu_id() noexcept
{
	std::array<unsigned int, 4> cpui{ 0,0,0,0 };

#if defined(__x86_64__) || defined(__i386__)
	__get_cpuid(1, &cpui[0], &cpui[1], &cpui[2], &cpui[3]);
#endif

	return cpui;
}

void _get_system_memory_info(SYSTEM::sMEMORY& _memory) noexcept
{
	_memory.bytes_physical_usage = 0;
	_memory.bytes_physical_total = 0;

	/* Declare 'info[]' static so it doesn't have to reassign each time the function runs? Or does the compiler take care of that? */
	static const std::string info[] = { "MemTotal:"s, "MemFree:"s, "Buffers:"s, "Cached:"s };
	int64_t intInfo[4];
	std::string token;
	std::ifstream file("/proc/meminfo");

	while (file >> token)
	{
		for (std::size_t i = 0; i != 4; ++i)
		{
			if (token == info[i])
			{
				file >> intInfo[i];

				if (i == 3)
				{
					file.close();

					_memory.bytes_physical_usage = 0;
					_memory.bytes_physical_total = intInfo[0] * 1024;;

					// Went with kB output. Just a personal thing.
					return;
				}
			}
		}

		// ignore rest of the line (Not sure if beneficial)
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
}

uint64_t _get_memory_status() noexcept
{
	uint64_t size_memory_use = 0;

	/* Declare 'info[]' static so it doesn't have to reassign each time the function runs? Or does the compiler take care of that? */
	static const std::string info[] = { "MemTotal:"s, "MemFree:"s, "Buffers:"s, "Cached:"s };
	int64_t intInfo[4];
	std::string token;
	std::ifstream file("/proc/meminfo");

	while (file >> token)
	{
		for (std::size_t i = 0; i != 4; ++i)
		{
			if (token == info[i])
			{
				file >> intInfo[i];

				if (i == 3)
				{
					file.close();

					size_memory_use = (intInfo[0] - intInfo[1] - (intInfo[2] + intInfo[3])) * 1024;

					// Went with kB output. Just a personal thing.
					break;
				}
			}
		}

		// ignore rest of the line (Not sure if beneficial)
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}

	// return)
	return size_memory_use;
}

ePROCESSOR_TYPE _get_processor_type() noexcept
{
	// declare) 
	struct utsname os_name;

	// 2) set os version & name
	RETURN_IF(uname(&os_name) != 0, ePROCESSOR_TYPE::NOT_DEFINED)

	if (CGDK::__stricmp(os_name.machine, "x86") == 0)
	{
	return ePROCESSOR_TYPE::x86_64;
	}
	else if(CGDK::__stricmp(os_name.machine, "x86-64")==0)
	{
		return ePROCESSOR_TYPE::x86_64;
	}

	else if (CGDK::__stricmp(os_name.machine, "i86pc") == 0)
	{
		return ePROCESSOR_TYPE::x86_64;
	}
	else if (CGDK::__stricmp(os_name.machine, "i386") == 0)
	{
		return ePROCESSOR_TYPE::x86_64;
	}

	else if (CGDK::__stricmp(os_name.machine, "i686") == 0)
	{
		return ePROCESSOR_TYPE::x86_64;
	}
	else if (CGDK::__stricmp(os_name.machine, "i686-64") == 0)
	{
		return ePROCESSOR_TYPE::x86_64;
	}

	else if (CGDK::__stricmp(os_name.machine, "amd64") == 0)
	{
		return ePROCESSOR_TYPE::x86_64;
	}

	else if (CGDK::__stricmp(os_name.machine, "armv6l") == 0)
	{
		return ePROCESSOR_TYPE::ANDROID;
	}
	else if (CGDK::__stricmp(os_name.machine, "armv7l") == 0)
	{
		return ePROCESSOR_TYPE::ANDROID;
	}

	else if (CGDK::__stricmp(os_name.machine, "aarch64") == 0)
	{
		return ePROCESSOR_TYPE::ANDROID;
	}

	else if (CGDK::__stricmp(os_name.machine, "IP22") == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}
	else if (CGDK::__stricmp(os_name.machine, "IP30") == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}
	else if (CGDK::__stricmp(os_name.machine, "IP35") == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}

	else if (CGDK::__stricmp(os_name.machine, "RM600") == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}

	else if (CGDK::__stricmp(os_name.machine, "sun4u") == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}

	else if (CGDK::__stricmp(os_name.machine, "alpha") == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}

	else if (CGDK::__stricmp(os_name.machine, "ppc64") == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}
	else if (CGDK::__stricmp(os_name.machine, "sparc") == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}
	else if (CGDK::__stricmp(os_name.machine, "sparc64") == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}

	else if (CGDK::__stricmp(os_name.machine, "VAX") == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}
	else if (CGDK::__stricmp(os_name.machine, "vax-6340") == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}

	else if (CGDK::__stricmp(os_name.machine, "2097") == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}

	else
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}

	return ePROCESSOR_TYPE::NOT_DEFINED;
}

SYSTEM::sINFO api::get_system_info() noexcept
{
	// declare) 
	SYSTEM::sINFO system_resource_info;

	//// declare) 
	//SYSTEM_INFO ssi;

	//// 1) 값을 초기화
	//__zero_memory(&ssi, sizeof(ssi));

	//// 2) 생성되면 먼저 System Info를 설정한다.
	//GetSystemInfo(&ssi);

	// 3) 값을 설정한다.
	system_resource_info.processor.type = ePROCESSOR_TYPE::x86_64;
	system_resource_info.processor.level = 0;
	system_resource_info.processor.revision = 0;
	system_resource_info.processor.number_of_processor = std::thread::hardware_concurrency();
	_get_system_memory_info(system_resource_info.memory);
	system_resource_info.network.dummy = 0;

	// return) 
	return system_resource_info;
}

//-----------------------------------------------------------------------------
// system status (/proc)
//-----------------------------------------------------------------------------
namespace
{
	// - cpu 사용률 (%x1000). /proc/stat의 첫 줄(cpu)을 이전 값과 비교한다. (처음 부르면 부팅 후 평균)
	uint64_t _get_processor_usage() noexcept
	{
		static std::mutex lockable;
		static uint64_t total_last = 0;
		static uint64_t idle_last = 0;

		std::ifstream file("/proc/stat");
		std::string token;
		if (!(file >> token) || token != "cpu")
			return 0;

		// - user nice system idle iowait irq softirq steal (guest는 user에 포함되어 있다.)
		uint64_t values[8]{ 0 };
		for (auto& iter : values)
		{
			if (!(file >> iter))
				break;
		}

		uint64_t total = 0;
		for (auto iter : values)
			total += iter;
		uint64_t idle = values[3] + values[4];

		std::lock_guard lock(lockable);
		uint64_t total_delta = total - total_last;
		uint64_t idle_delta = idle - idle_last;
		total_last = total;
		idle_last = idle;

		if (total_delta == 0 || idle_delta > total_delta)
			return 0;

		return (total_delta - idle_delta) * 100000 / total_delta;
	}

	// - thread 수 (/proc/loadavg의 'running/total')
	uint64_t _get_thread_count() noexcept
	{
		std::ifstream file("/proc/loadavg");
		std::string load1, load5, load15, entities;
		if (!(file >> load1 >> load5 >> load15 >> entities))
			return 0;

		auto pos = entities.find('/');
		return (pos == std::string::npos) ? 0 : std::strtoull(entities.c_str() + pos + 1, nullptr, 10);
	}

	// - process 수 (/proc의 숫자 directory)
	uint64_t _get_process_count() noexcept
	{
		auto pdir = opendir("/proc");
		if (pdir == nullptr)
			return 0;

		uint64_t count = 0;
		while (auto pentry = readdir(pdir))
		{
			if (pentry->d_name[0] >= '1' && pentry->d_name[0] <= '9')
				++count;
		}
		closedir(pdir);

		return count;
	}

	// - 열린 file 수 (/proc/sys/fs/file-nr의 첫 값)
	uint64_t _get_handle_count() noexcept
	{
		std::ifstream file("/proc/sys/fs/file-nr");
		uint64_t count = 0;
		file >> count;
		return count;
	}

	// - network 누적 송수신 byte (/proc/net/dev. lo 제외)
	uint64_t _get_network_bytes() noexcept
	{
		std::ifstream file("/proc/net/dev");
		std::string line;
		uint64_t bytes_total = 0;

		// - 앞의 두 줄은 제목
		std::getline(file, line);
		std::getline(file, line);

		while (std::getline(file, line))
		{
			auto pos = line.find(':');
			if (pos == std::string::npos)
				continue;

			auto name = line.substr(0, pos);
			name.erase(0, name.find_first_not_of(' '));
			if (name == "lo")
				continue;

			// - receive: bytes packets errs drop fifo frame compressed multicast / transmit: bytes ...
			std::istringstream stream(line.substr(pos + 1));
			uint64_t values[9]{ 0 };
			for (auto& iter : values)
			{
				if (!(stream >> iter))
					break;
			}
			bytes_total += values[0] + values[8];
		}

		return bytes_total;
	}
}

SYSTEM::sSTATUS api::get_system_status() noexcept
{
	// declare)
	SYSTEM::sSTATUS temp_system_status;

	// 1) 값을 설정한다.
	try
	{
		temp_system_status.processor_usage = _get_processor_usage();
		temp_system_status.thread_count = _get_thread_count();
		temp_system_status.process_count = _get_process_count();
		temp_system_status.handle_count = _get_handle_count();
		temp_system_status.bytes_memory_usage = _get_memory_status();
		temp_system_status.bytes_traffic = _get_network_bytes();
	}
	catch (...)
	{
	}
	temp_system_status.time_update = chrono::time::utc::now();

	// return) 
	return temp_system_status;
}

sVERSION_EX _get_os_version() noexcept
{
	// declare)
	sVERSION_EX temp_version;

	// declare) 
	struct utsname os_name;

	// 2) set os version & name
	if (uname(&os_name) == 0)
	{
		std::string_view temp = os_name.release;
		auto iter = temp.data();
		auto iter_pre = iter;
		auto iter_end = iter + temp.size();

		// 1) major
		for (; iter != iter_end; ++iter) if (*iter == '.') break;
		std::string_view str_1(iter_pre, iter - iter_pre);
		std::from_chars(str_1.data(), str_1.data() + str_1.size(), temp_version.major);

		// 2) minor
		for (iter_pre = ++iter; iter != iter_end; ++iter) if (*iter == '.') break;
		std::string_view str_2(iter_pre, iter - iter_pre);
		std::from_chars(str_2.data(), str_2.data() + str_2.size(), temp_version.minor);

		// 3) build
		for (iter_pre = ++iter; iter != iter_end; ++iter) if (*iter < '0' || *iter > '9') break;
		std::string_view str_3(iter_pre, iter - iter_pre);
		std::from_chars(str_3.data(), str_3.data() + str_3.size(), temp_version.build);

		// 4) string_version
		temp_version.str_version = os_name.release;
	}
	else
	{
		temp_version.clear();
	}

	// return)
	return temp_version;
}

std::string _get_os_release_id() noexcept
{
	return "0.0"s;
}

std::string _get_os_product_name() noexcept
{
	// declare)
	std::string temp;

	// declare) 
	struct utsname os_name;

	// 1) set os version & name
	temp = (uname(&os_name) == 0) ? os_name.version : ""s;

	// return) 
	return temp;
}

sOS_INFO api::get_os_info() noexcept
{
	//struct utsname
	//{
	//	char sysname[];    // "Linux" /* Operating system name (e.g., "Linux") */
	//	char nodename[];   // "HUKPUNG" /* Name within "some implementation-defined network" */
	//	char release[];    // "5.4.0-52-generic" /* OS release (e.g., "2.6.28") */
	//	char version[];    // "#57-Ubuntu SMP Thu oct 15 10:57:00 UTC 2020" /* OS version */
	//	char machine[];    // "x86-64" /* Hardware identifier */
	//#ifdef _GNU_SOURCE
	//	char domainname[]; /* NIS or YP domain name */
	//#endif
	//};

	// declare) 
	sOS_INFO os_info;

	// 1) set values
	os_info.type = eOS_TYPE::LINUX;
	os_info.name = "Linux"s;
	os_info.version = _get_os_version();
	os_info.product_name = _get_os_product_name();
	os_info.language = eLANGUAGE::KOREAN;

	// return) ...
	return os_info;
}

std::string api::get_os_unique_id() noexcept
{
	return "0.0"s;
}

uint64_t api::get_process_id()
{
	return ::getpid();
}


// - cgroup 의 CPU 한도 (quota / period 올림). 한도가 없거나 읽지 못하면 0
static size_t get_cgroup_cpu_limit() noexcept
{
	try
	{
		// declare)
		size_t limit = 0;
		auto apply = [&](double _quota, double _period)
		{
			if (_quota <= 0 || _period <= 0)
				return;
			auto count = static_cast<size_t>((_quota + _period - 1) / _period);
			if (count == 0)
				count = 1;
			if (limit == 0 || count < limit)
				limit = count;
		};

		// 1) cgroup v2 : 이 process 의 cgroup 부터 root 까지의 cpu.max ("max 100000" 이면 한도 없음)
		std::string path_cgroup;
		{
			std::ifstream file("/proc/self/cgroup");
			std::string line;
			while (std::getline(file, line))
			{
				if (line.compare(0, 3, "0::") == 0)
				{
					path_cgroup = line.substr(3);
					break;
				}
			}
		}
		for (bool is_first = true; is_first || path_cgroup.empty() == false; is_first = false)
		{
			std::ifstream file("/sys/fs/cgroup" + path_cgroup + "/cpu.max");
			std::string quota;
			double period = 0;
			if (file >> quota >> period && quota != "max")
				apply(std::atof(quota.c_str()), period);

			auto pos = path_cgroup.rfind('/');
			if (pos == std::string::npos)
				break;
			path_cgroup.resize(pos);
		}

		// 2) cgroup v1 : cpu.cfs_quota_us (-1 이면 한도 없음)
		{
			std::ifstream file_quota("/sys/fs/cgroup/cpu/cpu.cfs_quota_us");
			std::ifstream file_period("/sys/fs/cgroup/cpu/cpu.cfs_period_us");
			double quota = 0, period = 0;
			if (file_quota >> quota && file_period >> period)
				apply(quota, period);
		}

		// return)
		return limit;
	}
	catch (...)
	{
		return 0;
	}
}

// - 이 process 가 쓸 수 있는 CPU 수 : affinity(taskset, cpuset) 와 cgroup 의 CPU 한도(컨테이너) 중 작은 것
//   (hardware_concurrency() 는 machine 의 CPU 수라 컨테이너에서 thread 를 너무 많이 만든다)
static size_t get_available_core_count() noexcept
{
	// 1) machine
	size_t count = std::thread::hardware_concurrency();

	// 2) affinity
	cpu_set_t set_cpu;
	CPU_ZERO(&set_cpu);
	if (::sched_getaffinity(0, sizeof(set_cpu), &set_cpu) == 0)
	{
		auto count_affinity = static_cast<size_t>(CPU_COUNT(&set_cpu));
		if (count_affinity != 0 && (count == 0 || count_affinity < count))
			count = count_affinity;
	}

	// 3) cgroup
	auto count_cgroup = get_cgroup_cpu_limit();
	if (count_cgroup != 0 && (count == 0 || count_cgroup < count))
		count = count_cgroup;

	// return)
	return (count != 0) ? count : 1;
}

size_t	api::get_core_count() noexcept
{
	static const size_t s_count = get_available_core_count();
	return s_count;
}

bool api::set_current_thread_affinity(int _cpu) noexcept
{
	// check)
	RETURN_IF(_cpu < 0 || _cpu >= CPU_SETSIZE, false);

	// 1) the CPU set with the one CPU
	cpu_set_t set_cpu;
	CPU_ZERO(&set_cpu);
	CPU_SET(_cpu, &set_cpu);

	// 2) pin this thread
	return ::pthread_setaffinity_np(::pthread_self(), sizeof(set_cpu), &set_cpu) == 0;
}

bool api::set_current_thread_priority(eTHREAD_PRIORITY _priority) noexcept
{
	// check) DEFAULT : not changed
	RETURN_IF(_priority == eTHREAD_PRIORITY::DEFAULT, true);

	// - the nice of this thread (linux : the nice of PRIO_PROCESS with a tid is the one of the thread)
	const auto tid = static_cast<id_t>(::syscall(SYS_gettid));
	auto set_nice = [tid](int _nice) noexcept { return ::setpriority(PRIO_PROCESS, tid, _nice) == 0; };

	// 1) TIME_CRITICAL : real-time round robin
	//    - priority 10 : above every SCHED_OTHER thread, below the threaded IRQ handlers of the kernel (50 : the network keeps being served)
	//    - no permission (CAP_SYS_NICE / RLIMIT_RTPRIO) : the highest nice
	if (_priority == eTHREAD_PRIORITY::TIME_CRITICAL)
	{
		sched_param param{};
		param.sched_priority = (std::min)(10, ::sched_get_priority_max(SCHED_RR));

		if (::pthread_setschedparam(::pthread_self(), SCHED_RR, &param) == 0)
			return true;

		return set_nice(-20);
	}

	// 2) the others : a normal thread (SCHED_OTHER) with a nice
	{
		sched_param param{};
		::pthread_setschedparam(::pthread_self(), SCHED_OTHER, &param);
	}

	switch (_priority)
	{
	case eTHREAD_PRIORITY::LOWEST:			return set_nice(19);
	case eTHREAD_PRIORITY::LOW:				return set_nice(10);
	case eTHREAD_PRIORITY::NORMAL:			return set_nice(0);
	case eTHREAD_PRIORITY::HIGH:			return set_nice(-10);
	case eTHREAD_PRIORITY::HIGHEST:			return set_nice(-20);
	default:								return true;
	}
}

int64_t	api::get_memory_usage() noexcept
{
	//unsigned long	vsize;
	long rss;

	std::string ignore;
	ignore.reserve(1024);

	std::ifstream ifs("/proc/self/stat", std::ios_base::in);
	ifs >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore
		>> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore
		//>> ignore >> ignore >> vsize >> rss;
		>> ignore >> ignore >> ignore >> rss;

	long page_size_kb = sysconf(_SC_PAGE_SIZE); // in case x86-64 is configured to use 2MB pages

	//auto	vm_usage		 = vsize / 1024.0;
	auto resident_set = rss * page_size_kb;

	// return) 
	return resident_set;
}

uint64_t api::get_physical_memory_max() noexcept
{
	// declare) 
	struct sysinfo temp_system_info;

	// 1) get physical memory size
	auto result = sysinfo(&temp_system_info);

	// check)
	RETURN_IF(result == -1, 0);
	
	// return) 
	return temp_system_info.totalram;
}

uint64_t api::get_max_open_files() noexcept
{
	rlimit64 lim;
	getrlimit64(RLIMIT_NOFILE, &lim);

	return lim.rlim_cur;
}

void api::set_max_open_files(uint64_t _max_open_files) noexcept
{
	rlimit64 lim;
	getrlimit64(RLIMIT_NOFILE, &lim);

	lim.rlim_cur = _max_open_files;

	setrlimit64(RLIMIT_NOFILE, &lim);
}

void api::reserve_max_open_files() noexcept
{
	// declare) 
	rlimit64 lim_set;

	// 1) get hard limit of open files count
	getrlimit64(RLIMIT_NOFILE, &lim_set);

	// 2) set max
	lim_set.rlim_cur = lim_set.rlim_max;

	// 3) set!
	setrlimit64(RLIMIT_NOFILE, &lim_set);
}

int32_t	api::get_last_error() noexcept
{
	return errno;
}

std::string_view api::get_error_string(int32_t _errno) noexcept
{
	return std::string_view(strerror(_errno));
}

std::string_view api::get_last_error_string() noexcept
{
	return get_error_string(get_last_error());
}

sDEVICE_INFO _get_device_info()
{
	// declare) 
	sDEVICE_INFO temp;

	// 1) manufacturer/mode/device_id (for mobile)
	temp.device_id = "any"sv; // android:IMEI, ios:UDID
	temp.model = "Linux"sv;
	//temp.hardware_serial;
	//temp.sim_id;
	//temp.sim_serial;
	//temp.aaid;

	// return) 
	return temp;
}

const sDEVICE_INFO&	api::get_device_info() noexcept
{
	if (g_device_info.device_id.empty())
	{
		g_device_info = _get_device_info();
	}

	// return) 
	return g_device_info;
}

std::array<unsigned int, 4> api::get_device_unique_id() noexcept
{
	return _get_cpu_id();
}

std::string api::get_machine_name() noexcept
{
	// declare) 
	static utsname u;

	// 1) get machine name
	auto result = uname(&u);

	// check)
	RETURN_IF(result < 0, std::string());

	// return) 
	return std::string(u.nodename);
}

std::string get_smbios_string(const std::string_view _path)
{
	// declare)
	std::string result;

	// 1) open path
	std::ifstream fs(_path.data(), std::ios::in);

	// check) 
	if (fs.is_open() == false)
	{
		// - set error message 
		result = "'product_uuid' permission denied"s;

		// return) 
		return result;
	}

	// 2) get line
	std::getline(fs, result);

	// 3) close file stream
	fs.close();

	// return) 
	return result;
}

std::string api::get_system_unique_id() noexcept
{
	return get_smbios_string("/sys/class/dmi/id/product_uuid"sv);
}

void enable_cin_echo(bool _enable)
{
	termios mode;
	tcgetattr(STDIN_FILENO, &mode);

	if (_enable)
		mode.c_lflag |= ECHO;
	else
		mode.c_lflag &= (~ECHO);

	tcsetattr(STDIN_FILENO, TCSANOW, &mode);
}

}

class _keyboard_init
{
public:
	_keyboard_init()
	{
		tcgetattr(0, &initial_settings);
		new_settings = initial_settings;
		new_settings.c_lflag &= ~ICANON;
		new_settings.c_lflag &= ~ECHO;
		new_settings.c_cc[VMIN] = 1;
		new_settings.c_cc[VTIME] = 0;
		tcsetattr(0, TCSANOW, &new_settings);
	}

	~_keyboard_init()
	{
		tcsetattr(0, TCSANOW, &initial_settings);
	}

	int _kbhit()
	{
		unsigned char ch;
		int nread;

		// check) already exist
		if (peek_character != -1 && peek_character != 27)
			return 1;

		// 1) set nonblock
		new_settings.c_cc[VMIN] = 0;
		tcsetattr(0, TCSANOW, &new_settings);

		if (peek_character == -1)
		{
			// 2) get first character
			nread = read(0, &ch, 1);

			// 3) if not exist
			if (nread == 0)
			{
				// - restore 
				new_settings.c_cc[VMIN] = 1;
				tcsetattr(0, TCSANOW, &new_settings);

				// return)
				return 0;
			}

			// 4) set character
			peek_character = ch;

			// 4) if not ESCAPE character
			if (ch != 27)
			{
				// - restore 
				new_settings.c_cc[VMIN] = 1;
				tcsetattr(0, TCSANOW, &new_settings);

				// return)
				return 1;
			}
		}

		// 5) get second character
		nread = read(0, &ch, 1);

		// 6) not exist
		if (nread == 0)
		{
			// - restore 
			new_settings.c_cc[VMIN] = 1;
			tcsetattr(0, TCSANOW, &new_settings);

			// return)
			return 1;
		}

		// 7) 
		if (ch != 91 && ch != 79)
		{
			// - set
			peek_character_sub = ch;

			// - restore 
			new_settings.c_cc[VMIN] = 1;
			tcsetattr(0, TCSANOW, &new_settings);

			// return)
			return 1;
		}

		// 8) 
		peek_character = (peek_character << 8) | ch;

		// 9) get third character
		nread = read(0, &ch, 1);

		// 10) set 
		peek_character = (peek_character << 8) | ch;

		// - restore 
		new_settings.c_cc[VMIN] = 1;
		tcsetattr(0, TCSANOW, &new_settings);

		// return)
		return 1;
	}

	int _getch()
	{
		char ch;

		if (peek_character != -1)
		{
			ch = peek_character;
			peek_character = peek_character_sub;
			peek_character_sub = -1;
			return ch;
		}
		int nread = read(0, &ch, 1);

		return (nread == 1) ? ch : 0;
	}

protected:
	struct termios initial_settings, new_settings;
	int peek_character = -1;
	int peek_character_sub = -1;
};


_keyboard_init g_key_;

int _kbhit()
{
	return g_key_._kbhit();
}

int _getch()
{
	return g_key_._getch();
}

void _fflush()
{
	fseek(stdin, 0, SEEK_END);
}
#endif