#include "pch.h"

#if defined(__linux__)

CGDK::sDEVICE_INFO		g_device_info;

#include <thread>
#include <sys/utsname.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <fstream>
#include <stdio.h>
#include <fcntl.h>
#include <cpuid.h>

// - for _kbhit, _getch
#include <unistd.h>
#include <termios.h>  


namespace CGDK
{

std::array<unsigned int, 4> _get_cpu_id() noexcept
{
	std::array<unsigned int, 4> cpui{ 0,0,0,0 };

	__get_cpuid(1, &cpui[0], &cpui[1], &cpui[2], &cpui[3]);

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

		// ignore rest of the line (Not sure if benefitical)
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

		// ignore rest of the line (Not sure if benefitical)
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

	if (__stricmp(os_name.machine, "x86"sv) == 0)
	{
	return ePROCESSOR_TYPE::x86_64;
	}
	else if(__stricmp(os_name.machine, "x86-64"sv)==0)
	{
		return ePROCESSOR_TYPE::x86_64;
	}

	else if (__stricmp(os_name.machine, "i86pc"sv) == 0)
	{
		return ePROCESSOR_TYPE::x86_64;
	}
	else if (__stricmp(os_name.machine, "i386"sv) == 0)
	{
		return ePROCESSOR_TYPE::x86_64;
	}

	else if (__stricmp(os_name.machine, "i686"sv) == 0)
	{
		return ePROCESSOR_TYPE::x86_64;
	}
	else if (__stricmp(os_name.machine, "i686-64"sv) == 0)
	{
		return ePROCESSOR_TYPE::x86_64;
	}

	else if (__stricmp(os_name.machine, "amd64"sv) == 0)
	{
		return ePROCESSOR_TYPE::x86_64;
	}

	else if (__stricmp(os_name.machine, "armv6l"sv) == 0)
	{
		return ePROCESSOR_TYPE::ANDROID;
	}
	else if (__stricmp(os_name.machine, "armv7l"sv) == 0)
	{
		return ePROCESSOR_TYPE::ANDROID;
	}

	else if (__stricmp(os_name.machine, "aarch64"sv) == 0)
	{
		return ePROCESSOR_TYPE::ANDROID;
	}

	else if (__stricmp(os_name.machine, "IP22"sv) == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}
	else if (__stricmp(os_name.machine, "IP30"sv) == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}
	else if (__stricmp(os_name.machine, "IP35"sv) == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}

	else if (__stricmp(os_name.machine, "RM600"sv) == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}

	else if (__stricmp(os_name.machine, "sun4u"sv) == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}

	else if (__stricmp(os_name.machine, "alpha"sv) == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}

	else if (__stricmp(os_name.machine, "ppc64"sv) == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}
	else if (__stricmp(os_name.machine, "sparc"sv) == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}
	else if (__stricmp(os_name.machine, "sparc64"sv) == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}

	else if (__stricmp(os_name.machine, "VAX"sv) == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}
	else if (__stricmp(os_name.machine, "vax-6340"sv) == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}

	else if (__stricmp(os_name.machine, "2097"sv) == 0)
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}

	else
	{
		return ePROCESSOR_TYPE::NOT_DEFINED;
	}
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

SYSTEM::sSTATUS api::get_system_status() noexcept
{
	// declare)
	SYSTEM::sSTATUS temp_system_status;

	// 1) 값을 설정한다.
	temp_system_status.processor_usage = 0;
	temp_system_status.thread_count = 0;
	temp_system_status.process_count = 0;
	temp_system_status.handle_count = 0;
	temp_system_status.bytes_memory_usage = _get_memory_status();
	temp_system_status.bytes_traffic = 0;
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
		temp_version.major = __from_string<int>(str_1);

		// 2) minor
		for (iter_pre = ++iter; iter != iter_end; ++iter) if (*iter == '.') break;
		std::string_view str_2(iter_pre, iter - iter_pre);
		temp_version.minor = __from_string<int>(str_2);

		// 3) build
		for (iter_pre = ++iter; iter != iter_end; ++iter) if (*iter < '0' || *iter > '9') break;
		std::string_view str_3(iter_pre, iter - iter_pre);
		temp_version.build = __from_string<int>(str_3);

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


size_t	api::get_core_count()
{
	return std::thread::hardware_concurrency();
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

		// check) aleady exist
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

		// 9) get thrid character
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