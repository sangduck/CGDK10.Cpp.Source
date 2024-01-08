#include "pch.h"

#if defined(__ANDROID__)

CGDK::sDEVICE_INFO		g_device_info;

#include <thread>
#include <sys/system_properties.h>
#include <jni.h>

namespace CGDK
{


uint64_t api::get_process_id()
{
	return ::getpid();
}

std::size_t	api::get_core_count()
{
	// 1) open한다.
	auto fp = fopen("/sys/devices/system/cpu/present", "r");

	// check) fp가 nullptr이면 끝낸다.
	if(fp == nullptr)
		return 0;

	// declare) 
	int i = -1, j = -1;

	// 2) 읽어들인다.
	auto res = fscanf(fp, "%d-%d", &i, &j);

	// 3) file을 닫는다.
	fclose(fp);

	std::size_t count_core = 0;

	// 4) Dual Core 이상일 경우
	if (res == 2 && i == 0)
	{
		count_core = static_cast<size_t>(j + 1);
	}
	else
	{
		count_core = static_cast<size_t>(1);
	}

	// return) 
	return count_core;
}

int64_t	api::get_memory_usage()
{
	//unsigned long	vsize;
	long rss;

	std::string ignore;
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

//
//{
//	JNIEnv &env = ...
//		jobject activity = ... (android.app.Activity object)
//
//		//TelephonyManager telephony_manager =
//	getSystemService(Context.TELEPHONY_SERVICE);
//
//	jmethodID	mid					 = env.GetMethodID(env.GetObjectClass(activity), "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
//	jobject		telephony_manager	 = env.CallObjectMethod(activity, mid, env.NewStringUTF("phone"));
//
//	//String s_imei = tm.getDeviceId();
//	mid				 = env.GetMethodID(env.GetObjectClass(telephony_manager), "getDeviceId", "()Ljava/lang/String;");
//
//	jstring s_imei	 = (jstring)env.CallObjectMethod(telephony_manager, mid);
//
//	if (env.ExceptionCheck()) 
//	{
//		env.ExceptionClear();
//		return false;
//	}
//
//	const jchar *imei = env.GetStringCritical(s_imei, NULL);
//
//	// copy imei string anywhere you need
//	env.ReleaseStringCritical(s_imei, imei);
//
//
//}

//void xxxx()
//{
//	//returns the string length of the value.
//	int ir = __system_property_get("ro.gsm.imei", imei_start);
//
//	if (ir > 0)
//	{
//		imei_start[15] = 0;//strz end      
//		printf("method1 got imei %s len %d\r\n", imei_start, strlen(imei_start));
//		strcpy(g_imei, imei_start);
//	}
//	else
//	{
//		printf("method1 imei failed - trying method2\r\n");
//		//old dumpsys imei getter
//		char* res = exec_get_out("dumpsys iphonesubinfo");
//		const char* imei_start_match = "ID = ";
//		int imei_start_match_len = strlen(imei_start_match);
//		char* imei_start = strstr(res, imei_start_match);
//		if (imei_start && strlen(imei_start) >= 15 + imei_start_match_len)
//		{
//			imei_start += imei_start_match_len;
//			imei_start[15] = 0;
//			printf("method2 IMEI [%s] len %d\r\n", imei_start, strlen(imei_start));
//			strcpy(g_imei, imei_start);
//		}
//	}
//}
//
//void Temp()
//{
//	jclass context_cls;
//
//	/* assuming you already have */
//	JNIEnv *env;
//	jobject context;
//
//	/* then call (with error-checking) */
//	auto	cls = (*env)->FindClass(env, "android/context/Context");
//	auto	mid = (*env)->GetMethodID(env, context_cls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
//	auto	fid = (*env)->GetStaticFieldID(env, cls, "TELEPHONY_SERVICE", "Ljava/lang/String;");
//	auto	str = (*env)->GetStaticObjectField(env, cls, fid);
//	auto	telephony = (*env)->CallObjectMethod(env, context, mid, str);
//
//	cls = (*env)->FindClass(env, "android/telephony/TelephonyManager");
//	mid = (*env)->GetMethodID(env, cls, "getDeviceId", "()Ljava/lang/String;");
//	str = (*env)->CallObjectMethod(env, telephony, mid);
//	jsize len = (*env)->GetStringUTFLength(env, str);
//	char* deviceId = calloc(len + 1, 1);
//	(*env)->GetStringUTFRegion(env, str, 0, len, deviceId);
//	(*env)->DeleteLocalRef(env, str);
//	/* to get a string in deviceId */
//}
//

//char * _GetDeviceID(JNIEnv *env, jobject context)
//{   
//	jclass		cls			 = (*env)->FindClass(env, "android/context/Context");   
//
//	jmethodID	mid			 = (*env)->GetMethodID(env, cls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
//	jfieldID	fid			 = (*env)->GetStaticFieldID(env, cls, "TELEPHONY_SERVICE", "Ljava/lang/String;");
//	jstring		str			 = (*env)->GetStaticObjectField(env, cls, fid);
//	jobject		telephony	 = (*env)->CallObjectMethod(env, context, mid, str);
//
//	cls			 = (*env)->FindClass(env, "android/telephony/TelephonyManager");
//	mid			 = (*env)->GetMethodID(env, cls, "getDeviceId", "()Ljava/lang/String;");
//	str			 = (*env)->CallObjectMethod(env, telephony, mid);
//	jsize len	 = (*env)->GetStringUTFLength(env, str);
//
//	char* deviceId	 = calloc(len + 1, 1);
//
//	(*env)->GetStringUTFRegion(env, str, 0, len, deviceId);
//	(*env)->DeleteLocalRef(env, str);
//
//	/* to get a string in deviceId */
//
//	return deviceId;
//}

sDEVICE_INFO _get_device_info()
{
	sDEVICE_INFO temp;

	// 1) Device 정보를 얻는다.
	char str_temp[PROP_VALUE_MAX + 1]{ 0 };

	// 2) manufacturer를 쓴다.
	/*auto	len = */ __system_property_get("ro.product.manufacturer", str_temp);
	temp.manufacturer = str_temp;

	// 3) model을 얻는다.
	/*len = */ __system_property_get(".product.model", str_temp);
	temp.model = str_temp;

	// return) 
	return temp;
}

const sDEVICE_INFO&	api::get_device_info() noexcept
{
	if (g_device_info.os_type == eOS_TYPE::NOT_DEFINED)
	{
		g_device_info = _get_device_info();
	}

	// return) 
	return g_device_info;
}

std::array<int, 4> api::get_device_unique_id() noexcept
{
	//// 4) Device ID를 얻는다.
	//temp.device_id;

	/* assuming you already have */
	//TelephonyManager manager = (TelephonyManager)context.getSystemService(Context.TELEPHONY_SERVICE);
	//if (manager != null)
	//{
	//	str_uniqueID = manager.getDeviceId();
	//}

	//JNIEnv*		env;
	//jobject		context;
	///* then call (with error-checking) */
	////jclass(*FindClass)(JNIEnv*, const char*);

	//auto		cls			 = env->FindClass(env, "android/context/Context");
	//auto		mid			 = env->GetMethodID(env, cls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
	//auto		fid			 = env->GetStaticFieldID(env, cls, "TELEPHONY_SERVICE", "Ljava/lang/String;");
	//auto		str			 = env->GetStaticObjectField(env, cls, fid);
	//auto		telephony	 = env->CallObjectMethod(env, context, mid, str);

	//			cls			 = env->FindClass(env, "android/telephony/TelephonyManager");
	//			mid			 = env->GetMethodID(env, cls, "getDeviceId", "()Ljava/lang/String;");
	//			str			 = env->CallObjectMethod(env, telephony, mid);
	//auto		length		 = env->GetStringUTFLength(env, str);
	//char*		deviceId	 = (char*)malloc(length + 1);
	//(*env).GetStringUTFRegion(env, str, 0, length, deviceId);
	//(*env).DeleteLocalRef(env, str);
	///* to get a string in deviceId */

	return std::array<int, 4>();
}

std::string api::get_system_unique_id() noexcept
{
	// declare) 
	std::string result;

	// return)
	return result;
}


}

#endif
