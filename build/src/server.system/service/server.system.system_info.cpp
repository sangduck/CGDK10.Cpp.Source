#include "pch.h"

#if defined(_WIN32)
	#include "psapi.h"
	#include <tlhelp32.h>
#endif

using namespace CGDK;


//bool GetComputerName(LPSTR lpBuffer, uint32_t* lpnsize)
//{
//	return true;
//}
//
//bool GetUserName(LPSTR lpBuffer, uint32_t* lpnsize)
//{
//	return true;
//}
//
//
//bool GetOSVersionInfo(OSVERSIONINFOEX& p_rOSVersionInfo)
//{
//	// declare) 
//	BOOL bOsVersionInfoEx;
//
//	// 1) 먼저 메모리를 reset한다.
//	__zero_memory(&p_rOSVersionInfo, sizeof(OSVERSIONINFOEX));
//
//	// 1) 먼저 GetVersionEx을 얻을려고 시도해 본다.
//	p_rOSVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
//
//	// 2) Ex Version으러 버전 정보를 먼저 얻는다.
//	if(!(bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*)&p_rOSVersionInfo)))
//	{
//		// - Ex Version으로 구하지 못했을 경우 새로 일반 버전으로 구한다.
//		p_rOSVersionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
//		if (!GetVersionEx((OSVERSIONINFO*)&p_rOSVersionInfo)) 
//		{
//			// return) 이것도 실패했을 때는 false를 리턴한다.
//			return	false;
//		}
//	}
//
//	// return) 
//	return true;
//}
//
//bool GetinfoSystem(SYSTEM_INFO& p_rinfoSystem)
//{
//	// declare) 
//	typedef void (CALLBACK *PGNSI)(LPSYSTEM_INFO);
//
//	// 1) 먼저 메모리를 reset한다.
//	__zero_memory(&p_rinfoSystem, sizeof(SYSTEM_INFO));
//
//	// 2) ProcessAddress를 먼저 얻는다.
//	PGNSI pGNSI	 = (PGNSI)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeinfoSystem");
//
//	// 3) System Information을 얻는다.
//	if(pGNSI != NULL)
//	{
//		pGNSI(&p_rinfoSystem);
//	}
//	else 
//	{
//		GetinfoSystem(&p_rinfoSystem);
//	}
//
//	// return) 
//	return true;
//}
//
//bool GetProcessName(uint32_t _ProcessID)
//{
//	// declare) 
//    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
//
//    // 1) Get a handle to the process.
//    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, _ProcessID);
//
//	// check) 
//    RETURN_IF(hProcess==NULL, false);
//
//	// declare) 
//    HMODULE	hMod;
//    uint32_t	cbNeeded;
//
//    // 2) Get the process name.
//    if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded))
//    {
//        GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName)/sizeof(TCHAR));
//    }
//
//    // Print the process name and identifier.
//    CloseHandle( hProcess );
//
//    //_tprintf( TEXT("%s  (PID: %u)\n"), szProcessName, processID );
//}
//
//bool GetAllProcessName()
//{
//	// declare) 
//    uint32_t	aProcesses[MAX_STRING_LENGTH], cbNeeded;
//
//    if (!EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded))
//        return	false;
//
//    // Calculate how many process identifiers were returned.
//    uint32_t	cProcesses = cbNeeded / sizeof(uint32_t);
//
//    // Print the name and process identifier for each process.
//    for (int i = 0; i<cProcesses; i++)
//	{
//        if( aProcesses[i] != 0 )
//            GetProcessName(aProcesses[i]);
//	}
//
//	return true;
//}
//
//
//
//
//








//#define BUFSIZE 80
//
//bool GetinfoSystem(SYSTEM_INFO& p_rinfoSystem)
//{
//   SYSTEM_INFO siSysInfo;
// 
//   // Copy the hardware information to the SYSTEM_INFO structure. 
// 
//   GetinfoSystem(&siSysInfo); 
// 
//   // Display the contents of the SYSTEM_INFO structure. 
//
//   printf("Hardware information: \n");  
//   printf("  OEM sID: %u\n", siSysInfo.dwOemId);
//   printf("  Number of processors: %u\n", 
//      siSysInfo.dwNumberOfProcessors); 
//   printf("  Page size: %u\n", siSysInfo.dwPageSize); 
//   printf("  Processor type: %u\n", siSysInfo.dwProcessorType); 
//   printf("  Minimum application address: %lx\n", 
//      siSysInfo.lpMinimumApplicationAddress); 
//   printf("  Maximum application address: %lx\n", 
//      siSysInfo.lpMaximumApplicationAddress); 
//   printf("  Active processor mask: %u\n", 
//      siSysInfo.dwActiveProcessorMask); 
//}

//	// declare) 
//	BOOL bOsVersionInfoEx;
//
//   switch (osvi.dwPlatformId)
//   {
//      // Test for the Windows NT product family.
//      case VER_PLATFORM_WIN32_NT:
//
//      // Test for the specific product.
//
//      if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 )
//      {
//         if( osvi.wProductType == VER_NT_WORKSTATION )
//             printf ("Windows Vista ");
//         else printf ("Windows Server \"Longhorn\" " );
//      }
//
//      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
//      {
//         if( GetSystemMetrics(SM_SERVERR2) )
//            printf( "Microsoft Windows Server 2003 \"R2\" ");
//         else if( osvi.wProductType == VER_NT_WORKSTATION &&
//            si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
//         {
//            printf( "Microsoft Windows XP Professional x64 Edition ");
//         }
//         else printf ("Microsoft Windows Server 2003, ");
//      }
//
//      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
//         printf ("Microsoft Windows XP ");
//
//      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
//         printf ("Microsoft Windows 2000 ");
//
//      if ( osvi.dwMajorVersion <= 4 )
//         printf ("Microsoft Windows NT ");
//
//      // Test for specific product on Windows NT 4.0 SP6 and later.
//      if( bOsVersionInfoEx )
//      {
//         // Test for the workstation type.
//         if ( osvi.wProductType == VER_NT_WORKSTATION &&
//              si.wProcessorArchitecture!=PROCESSOR_ARCHITECTURE_AMD64)
//         {
//            if( osvi.dwMajorVersion == 4 )
//               printf ( "Workstation 4.0 " );
//            else if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
//               printf ( "Home Edition " );
//            else printf ( "Professional " );
//         }
//            
//         // Test for the server type.
//         else if ( osvi.wProductType == VER_NT_SERVER || 
//                   osvi.wProductType == VER_NT_DOMAIN_CONTROLLER )
//         {
//            if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion==2)
//            {
//               if (si.wProcessorArchitecture ==PROCESSOR_ARCHITECTURE_IA64)
//               {
//                   if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
//                      printf ( "Datacenter Edition for Itanium-based Systems" );
//                   else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
//                      printf ( "Enterprise Edition for Itanium-based Systems" );
//               }
//
//               else if ( si.wProcessorArchitecture ==
//                         PROCESSOR_ARCHITECTURE_AMD64 )
//               {
//                   if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
//                      printf ( "Datacenter x64 Edition " );
//                   else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
//                      printf ( "Enterprise x64 Edition " );
//                   else printf( "Standard x64 Edition " );
//               }
//
//               else
//               {
//                   if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
//                      printf ( "Datacenter Edition " );
//                   else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
//                      printf ( "Enterprise Edition " );
//                   else if ( osvi.wSuiteMask & VER_SUITE_BLADE )
//                      printf ( "Web Edition " );
//                   else printf ( "Standard Edition " );
//               }
//            }
//            else if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion==0)
//            {
//               if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
//                  printf ( "Datacenter Server " );
//               else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
//                  printf ( "Advanced Server " );
//               else printf ( "Server " );
//            }
//            else  // Windows NT 4.0 
//            {
//               if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
//                  printf ("Server 4.0, Enterprise Edition " );
//               else printf ( "Server 4.0 " );
//            }
//         }
//      }
//      // Test for specific product on Windows NT 4.0 SP5 and earlier
//      else  
//      {
//         HKEY hKey;
//         TCHAR szProductType[BUFSIZE];
//         uint32_t dwBufLen=BUFSIZE*sizeof(TCHAR);
//         LONG lRet;
//
//         lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
//            TEXT("SYSTEM\\CurrentControlSet\\control\\"
//                 "ProductOptions"), 0, KEY_QUERY_VALUE, &hKey );
//         if( lRet != ERROR_SUCCESS )
//            return FALSE;
//
//         lRet = RegQueryValueEx( hKey, TEXT("ProductType"),
//            NULL, NULL, (LPBYTE) szProductType, &dwBufLen);
//         RegCloseKey( hKey );
//
//         if( (lRet != ERROR_SUCCESS) ||
//             (dwBufLen > BUFSIZE*sizeof(TCHAR)) )
//            return FALSE;
//
//         if ( lstrcmpi( TEXT("WINNT"), szProductType) == 0 )
//            printf( "Workstation " );
//         if ( lstrcmpi( TEXT("LANMANNT"), szProductType) == 0 )
//            printf( "Server " );
//         if ( lstrcmpi( TEXT("SERVERNT"), szProductType) == 0 )
//            printf( "Advanced Server " );
//         printf( "%d.%d ", osvi.dwMajorVersion, osvi.dwMinorVersion );
//      }
//
//      // Display service pack (if any) and build number.
//
//      if( osvi.dwMajorVersion == 4 && 
//          lstrcmpi( osvi.szCSDVersion, TEXT("Service Pack 6") ) == 0 )
//      { 
//         HKEY hKey;
//         LONG lRet;
//
//         // Test for SP6 versus SP6a.
//         lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
//            TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\"
//                 "Hotfix\\Q246009"), 0, KEY_QUERY_VALUE, &hKey );
//         if( lRet == ERROR_SUCCESS )
//            printf( "Service Pack 6a (Build %d)\n", 
//            osvi.dwBuildNumber & 0xFFFF );         
//         else // Windows NT 4.0 prior to SP6a
//         {
//            _tprintf( TEXT("%s (Build %d)\n"),
//               osvi.szCSDVersion,
//               osvi.dwBuildNumber & 0xFFFF);
//         }
//
//         RegCloseKey( hKey );
//      }
//      else // not Windows NT 4.0 
//      {
//         _tprintf( TEXT("%s (Build %d)\n"),
//            osvi.szCSDVersion,
//            osvi.dwBuildNumber & 0xFFFF);
//      }
//
//      break;
//
//      // Test for the Windows Me/98/95.
//      case VER_PLATFORM_WIN32_WINDOWS:
//
//      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
//      {
//          printf ("Microsoft Windows 95 ");
//          if (osvi.szCSDVersion[1]=='C' || osvi.szCSDVersion[1]=='B')
//             printf("OSR2 " );
//      } 
//
//      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
//      {
//          printf ("Microsoft Windows 98 ");
//          if ( osvi.szCSDVersion[1]=='A' || osvi.szCSDVersion[1]=='B')
//             printf("SE " );
//      } 
//
//      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
//      {
//          printf ("Microsoft Windows Millennium Edition\n");
//      } 
//      break;
//
//      case VER_PLATFORM_WIN32s:
//
//      printf ("Microsoft Win32s\n");
//      break;
//   }
//   return TRUE; 
//}

















//
////  Forward declarations:
//BOOL GetProcessList( );
//BOOL ListProcessModules( uint32_t dwPID );
//BOOL ListProcessThreads( uint32_t dwOwnerPID );
//void printError( TCHAR* msg );

//
//bool GetProcessList( )
//{
//	HANDLE hProcessSnap;
//	HANDLE hProcess;
//	PROCESSENTRY32 pe32;
//	uint32_t dwPriorityClass;
//
//	// 1) Take a snapshot of all processes in the system.
//	hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
//
//	// check) 
//	RETURN_IF(hProcessSnap==INVALID_HANDLE_VALUE, false);
//
//	// 2) Set the size of the structure before using it.
//	pe32.dwSize = sizeof( PROCESSENTRY32);
//
//	// 3) Retrieve information about the first process, and exit if unsuccessful
//	if( !Process32First( hProcessSnap, &pe32 ) )
//	{
//		CloseHandle( hProcessSnap );
//		return( FALSE );
//	}
//
//  // Now walk the snapshot of processes, and
//  // display information about each process in turn
//	do
//	{
//		// PROCESS NAME: pe32.szExeFile
//
//		// Retrieve the priority class.
//		dwPriorityClass = 0;
//		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
//
//		if( hProcess == NULL )
//		{
//			printError( "OpenProcess" );
//		}
//		else
//		{
//			dwPriorityClass = GetPriorityClass( hProcess );
//			if( !dwPriorityClass )
//				printError( "GetPriorityClass" );
//			CloseHandle( hProcess );
//		}
//
//		printf( "\n  process sID        = 0x%08X",	pe32.th32ProcessID );
//		printf( "\n  thread count      = %d",		pe32.cntThreads );
//		printf( "\n  parent process sID = 0x%08X",	pe32.th32ParentProcessID );
//		printf( "\n  Priority Base     = %d",		pe32.pcPriClassBase );
//		if( dwPriorityClass )
//			printf( "\n  Priority Class    = %d", dwPriorityClass );
//
//		// List the modules and threads associated with this process
//		//ListProcessModules( pe32.th32ProcessID );
//		//ListProcessThreads( pe32.th32ProcessID );
//	} while( Process32Next( hProcessSnap, &pe32));
//
//	CloseHandle( hProcessSnap );
//
//	// return)
//	return true;
//}
//
//
//BOOL ListProcessModules( uint32_t dwPID )
//{
//  HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
//  MODULEENTRY32 me32;
//
//  // Take a snapshot of all modules in the specified process.
//  hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, dwPID );
//  if( hModuleSnap == INVALID_HANDLE_VALUE )
//  {
//    printError( "CreateToolhelp32Snapshot (of modules)" );
//    return( FALSE );
//  }
//
//  // Set the size of the structure before using it.
//  me32.dwSize = sizeof( MODULEENTRY32 );
//
//  // Retrieve information about the first module,
//  // and exit if unsuccessful
//  if( !Module32First( hModuleSnap, &me32 ) )
//  {
//    printError( "Module32First" ); // Show cause of failure
//    CloseHandle( hModuleSnap );    // Must clean up the
//                                   //   snapshot object!
//    return( FALSE );
//  }
//
//  // Now walk the module list of the process,
//  // and display information about each module
//  do
//  {
//    printf( "\n\n     MODULE NAME:     %s",
//      me32.szModule );
//    printf( "\n     executable     = %s",
//      me32.szExePath );
//    printf( "\n     process sID     = 0x%08X",
//      me32.th32ProcessID );
//    printf( "\n     ref count (g)  =     0x%04X",
//      me32.GlblcntUsage );
//    printf( "\n     ref count (p)  =     0x%04X",
//      me32.ProccntUsage );
//    printf( "\n     base address   = 0x%08X",
//      (uint32_t) me32.modBaseAddr );
//    printf( "\n     base size      = %d",
//      me32.modBaseSize );
//
//  } while( Module32Next( hModuleSnap, &me32 ) );
//
//  CloseHandle( hModuleSnap );
//  return( TRUE );
//}
//
//BOOL ListProcessThreads( uint32_t dwOwnerPID ) 
//{ 
//  HANDLE hThreadSnap = INVALID_HANDLE_VALUE; 
//  THREADENTRY32 te32; 
// 
//  // Take a snapshot of all running threads  
//  hThreadSnap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 ); 
//  if( hThreadSnap == INVALID_HANDLE_VALUE ) 
//    return( FALSE ); 
// 
//  // Fill in the size of the structure before using it. 
//  te32.dwSize = sizeof(THREADENTRY32 ); 
// 
//  // Retrieve information about the first thread,
//  // and exit if unsuccessful
//  if( !Thread32First( hThreadSnap, &te32 ) ) 
//  {
//    printError( "Thread32First" ); // Show cause of failure
//    CloseHandle( hThreadSnap );    // Must clean up the
//                                   //   snapshot object!
//    return( FALSE );
//  }
//
//  // Now walk the thread list of the system,
//  // and display information about each thread
//  // associated with the specified process
//  do 
//  { 
//    if( te32.th32OwnerProcessID == dwOwnerPID )
//    {
//      printf( "\n\n     THREAD sID      = 0x%08X",
//        te32.th32ThreadID ); 
//      printf( "\n     base priority  = %d", te32.tpBasePri ); 
//      printf( "\n     delta priority = %d", te32.tpDeltaPri ); 
//    }
//  } while( Thread32Next(hThreadSnap, &te32 ) ); 
//
//  CloseHandle( hThreadSnap );
//  return( TRUE );
//}
