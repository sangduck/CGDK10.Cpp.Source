#include "pch.h"

using namespace CGDK;

std::string CGDK::to_string(const sMAC_ADDRESS& _mac_address) noexcept
{
	char temp_string[256];
	auto length = std::snprintf(temp_string, 256, "%02u-%02u-%02u-%02u-%02u-%02u", _mac_address.sll_addr[0], _mac_address.sll_addr[1], _mac_address.sll_addr[2], _mac_address.sll_addr[3], _mac_address.sll_addr[4], _mac_address.sll_addr[5]);

	// return) 
	return std::string(temp_string, length);
}

std::string_view CGDK::to_string(eOBJECT_STATE _state) noexcept
{
	// declare) 
	std::string_view result;

	switch (_state)
	{
	case	eOBJECT_STATE::NONE:					result = "NONE"sv; break;
	case	eOBJECT_STATE::STOPPED:					result = "CLOSED"sv; break;
	case	eOBJECT_STATE::START_PENDING:			result = "OPENNING"sv; break;
	case	eOBJECT_STATE::STOP_PENDING:			result = "CLOSING"sv; break;
	case	eOBJECT_STATE::RUNNING:					result = "RUNNING"sv; break;
	case	eOBJECT_STATE::CONTINUE_PENDING:		result = "CONTINUE PENDING"sv; break;
	case	eOBJECT_STATE::PAUSE_PENDING:			result = "PAUSING PENDING"sv; break;
	case	eOBJECT_STATE::PAUSED:					result = "PAUSED"sv; break;
	case	eOBJECT_STATE::INITIALIZING_PENDING:	result = "INITIALIZING"sv; break;
	case	eOBJECT_STATE::DESTROYING_PENDING:		result = "DESTROYING"sv; break;
	case	eOBJECT_STATE::UPDATE:					result = "UPATE"sv; break;
	case	eOBJECT_STATE::COMMIT:					result = "COMMIT"sv; break;
	case	eOBJECT_STATE::NOT_CONNECT:				result = "NOT CONNECT"sv; break;
	case	eOBJECT_STATE::ESTABLISHED:				result = "ESTABLISHED"sv; break;
	case	eOBJECT_STATE::DEFAULT:					result = "DEFAULT"sv; break;
	default:										result = "Unknowned"sv; break;
	};

	// return)
	return	result;
}

std::string_view CGDK::to_string(eRESULT _result) noexcept
{
	// declare) 
	std::string_view result;

	switch((eRESULT)(static_cast<uint32_t>(_result) & 0xfff00000))
	{
	case	eRESULT::SUCCESS:
			switch(_result)
			{
			case	eRESULT::SUCCESS:		  							result = "SUCCESS"sv; break;
			case	eRESULT::PENDED:		  							result = "PENDED"sv; break;
			case	eRESULT::SUCCESS_CREATE: 							result = "SUCCESS_CREATE"sv; break;
			case	eRESULT::SUCCESS_RESTORE:							result = "SUCCESS_RESTORE"sv; break;
			case	eRESULT::PENDING:									result = "PENDING"sv; break;
			case	eRESULT::TRANSFER:									result = "TRANSFER"sv; break;
			default:  													result = "SUCCESS_USER"sv; break;
			}				
			break;

	case	eRESULT::FAIL:
			switch(_result)
			{
			case	eRESULT::FAIL:										result = "FAIL."sv; break;
			case	eRESULT::FAIL_DISABLED:								result = "FAIL.FAIL_DISABLED"sv; break;
			case	eRESULT::TIME_OUT:									result = "FAIL.TIME_OUT"sv; break;
			case	eRESULT::ALREADY_EXIST:								result = "FAIL.ALREADY_EXIST"sv; break;
			case	eRESULT::EXPIRED:									result = "FAIL.EXPIRED"sv; break;
			case	eRESULT::NOT_EXIST:									result = "FAIL.NOT_EXIST"sv; break;
			case	eRESULT::NOT_READY:									result = "FAIL.NOT_READY"sv; break;
			case	eRESULT::BANNED_MEMBER:								result = "FAIL.BANNED_MEMBER"sv; break;
			case	eRESULT::MEMBER_FULL:								result = "FAIL.MEMBER_FULL"sv; break;
			case	eRESULT::INVALID_MEMBER:							result = "FAIL.INVALID_MEMBER"sv; break;
			case	eRESULT::INVALID_PASSWORD:							result = "FAIL.INVALID_PASSWORD"sv; break;
			case	eRESULT::INVALID_PLATFORM:							result = "FAIL.INVALID_PLATFORM"sv; break;
			case	eRESULT::ALREADY_EXIST_NICKNAME:						result = "FAIL.ALREADY_EXIST_NICKNAME"sv; break;
			case	eRESULT::ALREADY_DONE:								result = "FAIL.ALREADY_DONE"sv; break;
			case	eRESULT::API_FAILURE:								result = "FAIL.API_FAILURE"sv; break;
			case	eRESULT::FAIL_TO_ACCEPT_SOCKET:						result = "FAIL.FAIL_TO_ACCEPT_SOCKET"sv; break;
			case	eRESULT::FAIL_TO_ATTACHING_TO_EXECUTOR:				result = "FAIL.FAIL_TO_ATTACHING_TO_EXECUTOR"sv; break;
			case	eRESULT::CONNECTION_FAILURE:						result = "FAIL.CONNECTION_FAILURE"sv; break;
			case	eRESULT::FAIL_TO_CREATE:							result = "FAIL.FAIL_TO_CREATE"sv; break;
			case	eRESULT::FAIL_TO_OPEN:								result = "FAIL.FAIL_TO_OPEN"sv; break;
			case	eRESULT::FAIL_TO_OPERATION:							result = "FAIL.FAIL_TO_OPERATION"sv; break;
			case	eRESULT::FAIL_TO_CONNECT:							result = "FAIL.FAIL_TO_CONNECT"sv; break;
			case	eRESULT::FAIL_NO_WEBSOCKET_KEY:						result = "FAIL.FAIL_NO_WEBSOCKET_KEY"sv; break;
			case	eRESULT::INVALID_VALUE:								result = "FAIL.INVALID_VALUE"sv; break;
			case	eRESULT::INVALID_SERVER:							result = "FAIL.INVALID_SERVER"sv; break;
			case	eRESULT::INVALID_REQUEST:							result = "FAIL.INVALID_REEQUEST"sv; break;
			case	eRESULT::BUSY:										result = "FAIL.BUSY"sv; break;
			case	eRESULT::EXCEED_RETRY_LIMIT:						result = "FAIL.EXCEED_RETRY_LIMIT"sv; break;
			case	eRESULT::NO_MEMBER_INFO:							result = "FAIL.NO_MEMBER_INFO"sv; break;
			case	eRESULT::RETRY_OUT:									result = "FAIL.RETRY_OUT"sv; break;
			case	eRESULT::EXCEED_THREAD_COUNT:						result = "FAIL.EXCEED_THREAD_COUNT"sv; break;
			case	eRESULT::FULL:										result = "FAIL.FULL"sv; break;
			case	eRESULT::FAIL_TO_INITIALIZE_SSL_CONTEXT:			result = "FAIL.FAIL_TO_INITIALIZE_SSL_CONTEXT"sv; break;
			case	eRESULT::FAIL_TO_USE_SSL_CERTIFICATION_FILE:		result = "FAIL.FAIL_TO_USE_SSL_CERTIFICATION_FILE"sv; break;
			case	eRESULT::FAIL_TO_USE_SSL_PRIVATE_KEY:				result = "FAIL.FAIL_TO_USE_SSL_PRIVATE_KEY"sv; break;
			case	eRESULT::FAIL_TO_CHECK_SSL_PUBLIC_KEY:				result = "FAIL.FAIL_TO_CHECK_SSL_PUBLIC_KEY"sv; break;
			default:  													result = "FAIL.FAIL_USER"sv; break;
			}				
			break;

	case	eRESULT::CANCEL:
			switch(_result)
			{
			case	eRESULT::CANCEL:									result = "CANCEL."sv; break;
			default:  													result = "CANCEL.USER"sv; break;
			}
			break;

	case	eRESULT::ERROR_:
			switch(_result)
			{
			case	eRESULT::ERROR_:									result = "ERROR."sv; break;
			case	eRESULT::LOGIC_ERROR:								result = "ERROR.LOGIC_ERROR"sv; break;
			case	eRESULT::LENGTH_ERROR:								result = "ERROR.LENGTH_ERROR"sv; break;
			case	eRESULT::DOMAIN_ERROR:								result = "ERROR.DOMAIN_ERROR"sv; break;
			case	eRESULT::OUT_OF_RANGE:								result = "ERROR.OUT_OF_RANGE"sv; break;
			case	eRESULT::RUNTIME_ERROR:								result = "ERROR.RUNTIME_ERROR"sv; break;
			case	eRESULT::OVERFLOW_ERROR:							result = "ERROR.OVERFLOW_ERROR"sv; break;
			case	eRESULT::UNDERFLOW_ERROR:							result = "ERROR.UNDERFLOW_ERROR"sv; break;
			case	eRESULT::RANGE_ERROR:								result = "ERROR.RANGE_ERROR"sv; break;
			case	eRESULT::BAD_ALLOC:									result = "ERROR.BAD_ALLOC"sv; break;
			case	eRESULT::BAD_FUNCTION_CALL:							result = "ERROR.BAD_FUNCTION_CALL"sv; break;
			case	eRESULT::CONCURRENT_LOCK_ERROR:						result = "ERROR.CONCURRENT_LOCK_ERROR"sv; break;
			case	eRESULT::ACCESS_DENIED:								result = "ERROR.ACCESS_DENIED"sv; break;
			case	eRESULT::NO_INITIALIZED:							result = "ERROR.NO_INITIALIZED"sv; break;
			case	eRESULT::INVALID_ARGUMENT:							result = "ERROR.INVALID_ARGUMENT"sv; break;
			case	eRESULT::NOT_ALLOWED:								result = "ERROR.NOT_ALLOWED"sv; break;
			case	eRESULT::INVALID_PARAMETER_HANDLE:					result = "ERROR.INVALID_PARAMETER_HANDLE"sv; break;
			case	eRESULT::INVALID_MESSAGE_LENGTH:					result = "ERROR.INVALID_MESSAGE_LENGTH"sv; break;
			case	eRESULT::INVALID_MESSAGE_BUFFER_IS_NULL:			result = "ERROR.INVALID_MESSAGE_BUFFER_IS_NULL"sv; break;
			case	eRESULT::INVALID_MESSAGE_TOO_SHORT_LENGTH:			result = "ERROR.INVALID_MESSAGE_TOO_SHORT_LENGTH"sv; break;
			case	eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH:			result = "ERROR.INVALID_MESSAGE_TOO_LONG_LENGTH"sv; break;
			case	eRESULT::INVALID_STATE:								result = "ERROR.INVALID_STATE"sv; break;
			case	eRESULT::INVALID_HANDLE:							result = "ERROR.INVALID_HANDLE"sv; break;
			case	eRESULT::INVALID_CHECK_KEY:							result = "ERROR.INVALID_CHECK_KEY"sv; break;
			case	eRESULT::INVALID_RESOURCE:							result = "ERROR.INVALID_RESOURCE"sv; break;
			case	eRESULT::FORMAT_ERROR:								result = "ERROR.FORMAT_ERROR"sv; break;
			default:  													result = "ERROR.USER"sv; break;
			}				
			break;

	case	eRESULT::EXCEPTION:
			switch(_result)
			{
			case	eRESULT::EXCEPTION:									result = "EXCEPTION."sv; break;
			case	eRESULT::BAD_EXCEPTION:								result = "EXCEPTION.BAD_EXCEPTION"sv; break;
			case	eRESULT::FORCE_CLOSE:								result = "EXCEPTION.FORCE_CLOSE"sv; break;
			case	eRESULT::NOT_SUPPORT:								result = "EXCEPTION.NOT_SUPPORT"sv; break;
			case	eRESULT::INVALID_CALL:								result = "EXCEPTION.INVALID_CALL"sv; break;
			case	eRESULT::INVALID_LICENSE:							result = "EXCEPTION.INVALID_LICENSE"sv; break;
			case	eRESULT::ABNORMAL_STATE:							result = "EXCEPTION.ABNORMAL_STATE"sv; break;
			case	eRESULT::BUFFER_INVALID:							result = "EXCEPTION.BUFFER_INVALID"sv; break;
			case	eRESULT::BUFFER_INVALID_STRING:						result = "EXCEPTION.BUFFER_INVALID_STRING"sv; break;
			case	eRESULT::UNEXPECTED_EXCEPTION:						result = "EXCEPTION.UNEXPECTED_EXCEPTION"sv; break;
			default:  													result = "EXCEPTION.USER"sv; break;
			}				
			break;

	case	eRESULT::PROCESS:
			switch(_result)
			{
			case	eRESULT::PROCESS:									result = "PROCESS."sv; break;
			case	eRESULT::REQUEST:									result = "PROCESS.REQUEST"sv; break;
			case	eRESULT::COMPLETE:									result = "PROCESS.COMPLETE"sv; break;
			case	eRESULT::RESET:										result = "PROCESS.RESET"sv; break;
			case	eRESULT::RESTORE:									result = "PROCESS.RESTORE"sv; break;
			default:  													result = "PROCESS.USER"sv; break;
			}				
			break;

	case	eRESULT::_USER:												result = "USER_"sv; break;

	case	eRESULT::CGCII_EXPIRED:										result = "CGCII_EXPIRED"sv; break;

	case	eRESULT::NOT_DEFINED:
			switch(_result)
			{
			case	eRESULT::NOT_DEFINED:								result = "NOT_DEFINED"sv; break;
			default:  													result = " - "sv; break;
			}				
			break;

	default:															result = "Unknowned"sv; break;
	}

	// return)
	return	result;
}

std::string_view CGDK::to_string(eOBJECT_TYPE _type) noexcept
{
	// declare) 
	std::string_view result;

	switch(_type)
	{
	// 1) 
	case	eOBJECT_TYPE::NOT_DEFINED:									result = "NOT DEFINED"sv; break;

	// 2) common
	case	eOBJECT_TYPE::EXECUTOR:										result = "EXECUTOR"sv; break;
	case	eOBJECT_TYPE::EXECUTOR_THREAD:								result = "EXECUTOR_THREAD"sv; break;
	case	eOBJECT_TYPE::EXECUTOR_SCHEDULABLE:							result = "EXECUTOR_SCHEDULABLE"sv; break;
	case	eOBJECT_TYPE::EXECUTOR_LIST:								result = "EXECUTOR_LIST"sv; break;
	case	eOBJECT_TYPE::EXECUTOR_LIST_PRIORITY:						result = "EXECUTOR_LIST_PRIORITY"sv; break;
	case	eOBJECT_TYPE::EXECUTOR_LIST_THREAD:							result = "EXECUTOR_LIST_THREAD"sv; break;
	case	eOBJECT_TYPE::EXECUTOR_QUEUED:								result = "EXECUTOR_QUEUED"sv; break;
	case	eOBJECT_TYPE::EXECUTOR_QUEUED_THREAD:						result = "EXECUTOR_QUEUED_THREAD"sv; break;
	case	eOBJECT_TYPE::EXECUTOR_SINGLE_EXECUTE:						result = "EXECUTOR_SINGLE_EXECUTE"sv; break;
	case	eOBJECT_TYPE::EXECUTOR_SINGLE_EXECUTE_THREAD:				result = "EXECUTOR_SINGLE_EXECUTE_THREAD"sv; break;

	case	eOBJECT_TYPE::EXECUTABLE:									result = "EXECUTABLE"sv; break;
	case	eOBJECT_TYPE::EXECUTABLE_BATCH_FUNCTION:					result = "EXECUTABLE_BATCH_FUNCTION"sv; break;
	case	eOBJECT_TYPE::EXECUTABLE_BATCH_FUNCTION_MEMBER:				result = "EXECUTABLE_BATCH_FUNCTION_MEMBER"sv; break;
	case	eOBJECT_TYPE::EXECUTABLE_BATCH_NOTIFY:						result = "EXECUTABLE_BATCH_NOTIFY"sv; break;
	case	eOBJECT_TYPE::EXECUTABLE_BATCH_WAIT:						result = "EXECUTABLE_BATCH_WAIT"sv; break;
	case	eOBJECT_TYPE::EXECUTABLE_FUNCTION:							result = "EXECUTABLE_FUNCTION"sv; break;
	case	eOBJECT_TYPE::EXECUTABLE_FUNCTION_MEMBER:					result = "EXECUTABLE_FUNCTION_MEMBER"sv; break;
	case	eOBJECT_TYPE::EXECUTABLE_LIST:								result = "EXECUTABLE_LIST"sv; break;
	case	eOBJECT_TYPE::EXECUTABLE_EXECUTABLE:						result = "EXECUTABLE_EXECUTABLE"sv; break;

	case	eOBJECT_TYPE::SCHEDULABLE:									result = "SCHEDULABLE"sv; break;
	case	eOBJECT_TYPE::SCHEDULABLE_FUNCTION:							result = "SCHEDULABLE_FUNCTION"sv; break;
	case	eOBJECT_TYPE::SCHEDULABLE_FUNCTION_MEMBER:					result = "SCHEDULABLE_FUNCTION_MEMBER"sv; break;
	case	eOBJECT_TYPE::SCHEDULABLE_LIST:								result = "SCHEDULABLE_LIST"sv; break;
	case	eOBJECT_TYPE::SCHEDULABLE_EXCEPTION_OBSERVER:				result = "SCHEDULABLE_EXCEPTION_OBSERVER"sv; break;
	case	eOBJECT_TYPE::SCHEDULABLE_FACTORY_GARBAGE:					result = "SCHEDULABLE_FACTORY_GARBAGE"sv; break;

	case	eOBJECT_TYPE::UPDATABLE:									result = "UPDATABLE"sv; break;
	case	eOBJECT_TYPE::UPDATABLE_BATCH:								result = "UPDATABLE_BATCH"sv; break;

	case	eOBJECT_TYPE::FILE:											result = "FILE"sv; break;

	// 3) socket
	case	eOBJECT_TYPE::ACCEPTOR:										result = "ACCEPTOR"sv; break;
	case	eOBJECT_TYPE::CONNECTOR:									result = "CONNECTOR"sv; break;
	case	eOBJECT_TYPE::SOCKET:										result = "SOCKET"sv; break;

	// 4) time event
	case	eOBJECT_TYPE::TIME_EVENT_ENTITY:							result = "TIME_EVENT_ENTITY"sv; break;
	case	eOBJECT_TYPE::TIME_EVENT_ENTITY_ITERATION:					result = "TIME_EVENT_ENTITY_ITERATION"sv; break;
	case	eOBJECT_TYPE::TIME_EVENT_ENTITY_SCHEDULE:					result = "TIME_EVENT_ENTITY_SCHEDULE"sv; break;
	case	eOBJECT_TYPE::TIME_EVENT_ENTITY_BEGIN_END:					result = "TIME_EVENT_ENTITY_BEGIN_END"sv; break;
	case	eOBJECT_TYPE::TIME_EVENT_ENTITY_BEGIN_ITERATION_END:		result = "TIME_EVENT_ENTITY_BEGIN_ITERATION_END"sv; break;

	case	eOBJECT_TYPE::TIME_EVENT_EVENT:								result = "TIME_EVENT_EVENT"sv; break;
	case	eOBJECT_TYPE::TIME_EVENT_EVENT_ITERATION:					result = "TIME_EVENT_EVENT_ITERATION"sv; break;
	case	eOBJECT_TYPE::TIME_EVENT_EVENT_BEGIN_END:					result = "TIME_EVENT_EVENT_BEGIN_END"sv; break;
	case	eOBJECT_TYPE::TIME_EVENT_EVENT_BEGIN_ITERATION_END:			result = "TIME_EVENT_EVENT_BEGIN_ITERATION_END"sv; break;

	// 5) server system
	case	eOBJECT_TYPE::GROUP:										result = "GROUP"sv; break;
	case	eOBJECT_TYPE::GROUP_COUNT:									result = "GROUP_COUNT"sv; break;
	case	eOBJECT_TYPE::GROUP_LIST:									result = "GROUP_LIST"sv; break;
	case	eOBJECT_TYPE::GROUP_ARRAY:									result = "GROUP_ARRAY"sv; break;
	case	eOBJECT_TYPE::GROUP_MAP:									result = "GROUP_MAP"sv; break;

	// 6) account
	case	eOBJECT_TYPE::REPOSITORY:									result = "REPOSITORY"sv; break;
	case	eOBJECT_TYPE::REPOSITORY_DB:								result = "REPOSITORY_DB"sv; break;
	case	eOBJECT_TYPE::REPOSITORY_REDIS:								result = "REPOSITORY_REDIS"sv; break;
	case	eOBJECT_TYPE::REPOSITORY_HUB:								result = "REPOSITORY_HUB"sv; break;
	case	eOBJECT_TYPE::REPOSITORY_REMOTE:							result = "REPOSITORY_REMOTE"sv; break;
	case	eOBJECT_TYPE::REPOSITORY_ORM:								result = "REPOSITORY_ORM"sv; break;
	case	eOBJECT_TYPE::ACCESSABLE:									result = "ACCESSABLE"sv; break;
	case	eOBJECT_TYPE::ACCESSABLE_HOLDER:							result = "ACCESSABLE_HOLDER"sv; break;
	case	eOBJECT_TYPE::DEPOT:										result = "DEPOT"sv; break;
	case	eOBJECT_TYPE::DEPOT_REMOTE:									result = "DEPOT_REMOTE"sv; break;

	// 7) querier
	case	eOBJECT_TYPE::QUERIER:										result = "QUERIER"sv; break;
	case	eOBJECT_TYPE::QUERIER_DB_SQL:								result = "QUERIER_DB_SQL"sv; break;
	case	eOBJECT_TYPE::QUERIER_DB_CASSANDRA:							result = "QUERIER_DB_CASSANDRA"sv; break;
	case	eOBJECT_TYPE::QUERIER_DB_MONGODB:							result = "QUERIER_DB_MONGODB"sv; break;
	case	eOBJECT_TYPE::QUERIER_REDIS:								result = "QUERIER_REDIS"sv; break;
	case	eOBJECT_TYPE::QUERIER_HTTP:									result = "QUERIER_HTTP"sv; break;
	case	eOBJECT_TYPE::QUERIER_OAUTH:								result = "QUERIER_OAUTH"sv; break;
	case	eOBJECT_TYPE::QUERIER_KAKAO:								result = "QUERIER_KAKAO"sv; break;


	// 8) 3d - bounding 
	case	eOBJECT_TYPE::BOUNDING:										result = "BOUNDING"sv; break;
	case	eOBJECT_TYPE::BOUNDING_SPHERE:								result = "BOUNDING_SPHERE"sv; break;
	case	eOBJECT_TYPE::BOUNDING_ELLIPSOID:							result = "BOUNDING_ELLIPSOID"sv; break;
	case	eOBJECT_TYPE::BOUNDING_SAABB:								result = "BOUNDING_SAABB"sv; break;
	case	eOBJECT_TYPE::BOUNDING_AABB:								result = "BOUNDING_AABB"sv; break;
	case	eOBJECT_TYPE::BOUNDING_OBB:									result = "BOUNDING_OBB"sv; break;
	case	eOBJECT_TYPE::BOUNDING_CYLINDER:							result = "BOUNDING_CYLINDER"sv; break;
	case	eOBJECT_TYPE::BOUNDING_CONE:								result = "BOUNDING_CONE"sv; break;
	case	eOBJECT_TYPE::BOUNDING_CAPSULE:								result = "BOUNDING_CAPSULE"sv; break;
	case	eOBJECT_TYPE::BOUNDING_CONVEX:								result = "BOUNDING_CONVEX"sv; break;
	case	eOBJECT_TYPE::BOUNDING_LINE:								result = "BOUNDING_LINE"sv; break;
	case	eOBJECT_TYPE::BOUNDING_SEGMENT:								result = "BOUNDING_SEGMENT"sv; break;
	case	eOBJECT_TYPE::BOUNDING_CIRCLE:								result = "BOUNDING_CIRCLE"sv; break;
	case	eOBJECT_TYPE::BOUNDING_ELLIPSE:								result = "BOUNDING_ELLIPSE"sv; break;
	case	eOBJECT_TYPE::BOUNDING_TRIANGLE:							result = "BOUNDING_TRIANGLE"sv; break;
	case	eOBJECT_TYPE::BOUNDING_RECTANGLE:							result = "BOUNDING_RECTANGLE"sv; break;
	case	eOBJECT_TYPE::BOUNDING_PLANE:								result = "BOUNDING_PLANE"sv; break;
	case	eOBJECT_TYPE::BOUNDING_PLANES:								result = "BOUNDING_PLANES"sv; break;
	case	eOBJECT_TYPE::BOUNDING_TERRAIN:								result = "BOUNDING_TERRAIN"sv; break;
	case	eOBJECT_TYPE::BOUNDING_PORTAL:								result = "BOUNDING_PORTAL"sv; break;

	// 9) resource
	case	eOBJECT_TYPE::RESOURCE:										result = "RESOURCE"sv; break;
	case	eOBJECT_TYPE::RESOURCE_VERTEXATTRIB:						result = "RESOURCE_VERTEXATTRIB"sv; break;
	case	eOBJECT_TYPE::RESOURCE_BUFFER:								result = "RESOURCE_BUFFER"sv; break;
	case	eOBJECT_TYPE::RESOURCE_SHADER:								result = "RESOURCE_SHADER"sv; break;
	case	eOBJECT_TYPE::RESOURCE_PROGRAM:								result = "RESOURCE_PROGRAM"sv; break;
	case	eOBJECT_TYPE::RESOURCE_MESH_PRIMITIVE:						result = "RESOURCE_MESH_PRIMITIVE"sv; break;
	case	eOBJECT_TYPE::RESOURCE_MESH_INDEXEDPRIMITIVE:				result = "RESOURCE_MESH_INDEXEDPRIMITIVE"sv; break;
	case	eOBJECT_TYPE::RESOURCE_MESH_TRIPATCH:						result = "RESOURCE_MESH_TRIPATCH"sv; break;
	case	eOBJECT_TYPE::RESOURCE_MESH_RECTPATCH:						result = "RESOURCE_MESH_RECTPATCH"sv; break;
	case	eOBJECT_TYPE::RESOURCE_MESH_DISPLACEMENT:					result = "RESOURCE_MESH_DISPLACEMENT"sv; break;
	case	eOBJECT_TYPE::RESOURCE_MESH_TERRAIN:						result = "RESOURCE_MESH_TERRAIN"sv; break;
	case	eOBJECT_TYPE::RESOURCE_TEXTURE:								result = "RESOURCE_TEXTURE"sv; break;
	case	eOBJECT_TYPE::RESOURCE_SAMPLER:								result = "RESOURCE_SAMPLER"sv; break;
	case	eOBJECT_TYPE::RESOURCE_SURFACE:								result = "RESOURCE_SURFACE"sv; break;
	case	eOBJECT_TYPE::RESOURCE_VERTEXDECL:							result = "RESOURCE_VERTEXDECL"sv; break;
	case	eOBJECT_TYPE::RESOURCE_FONT:								result = "RESOURCE_FONT"sv; break;
	case	eOBJECT_TYPE::RESOURCE_BONEINFO:							result = "RESOURCE_BONEINFO"sv; break;
	case	eOBJECT_TYPE::RESOURCE_TIME_SET_ARRAY:						result = "RESOURCE_TIME_SET_ARRAY"sv; break;
	case	eOBJECT_TYPE::RESOURCE_SPRITE:								result = "RESOURCE_SPRITE"sv; break;
	case	eOBJECT_TYPE::RESOURCE_SPRITEBUNDLE:						result = "RESOURCE_SPRITEBUNDLE"sv; break;
	case	eOBJECT_TYPE::RESOURCE_KEYFRAME_TRANSFORM:					result = "RESOURCE_KEYFRAME_TRANSFORM"sv; break;
	case	eOBJECT_TYPE::RESOURCE_KEYFRAME_TRANSFORM_PRESET:			result = "RESOURCE_KEYFRAME_TRANSFORM_PRESET"sv; break;
	case	eOBJECT_TYPE::RESOURCE_KEYFRAME_VECTOR3:					result = "RESOURCE_KEYFRAME_VECTOR3"sv; break;
	case	eOBJECT_TYPE::RESOURCE_KEYFRAME_VECTOR4:					result = "RESOURCE_KEYFRAME_VECTOR4"sv; break;
	case	eOBJECT_TYPE::RESOURCE_KEYFRAME_LIGHT:						result = "RESOURCE_KEYFRAME_LIGHT"sv; break;
	case	eOBJECT_TYPE::RESOURCE_KEYFRAME_VIEW:						result = "RESOURCE_KEYFRAME_VIEW"sv; break;
	case	eOBJECT_TYPE::RESOURCE_TIMESET_ARRAY:						result = "RESOURCE_TIMESET_ARRAY"sv; break;
	case	eOBJECT_TYPE::RESOURCE_PLANELIST:							result = "RESOURCE_PLANELIST"sv; break;
	case	eOBJECT_TYPE::RESOURCE_SCENEINFO:							result = "RESOURCE_SCENEINFO"sv; break;

	// 10) modifier
	case	eOBJECT_TYPE::MODIFIER_LOCAL:								result = "MODIFIER_LOCAL"sv; break;
	case	eOBJECT_TYPE::MODIFIER_KEYFRAME_LIGHT:						result = "MODIFIER_KEYFRAME_LIGHT"sv; break;
	case	eOBJECT_TYPE::MODIFIER_KEYFRAME_TRANSFORM:					result = "MODIFIER_KEYFRAME_TRANSFORM"sv; break;
	case	eOBJECT_TYPE::MODIFIER_KEYFRAME_VIEW:						result = "MODIFIER_KEYFRAME_VIEW"sv; break;
	case	eOBJECT_TYPE::MODIFIER_CONSTRAINT_IK:						result = "MODIFIER_CONSTRAINT_IK"sv; break;

	// 11) controller
	case	eOBJECT_TYPE::CONTROL:										result = "CONTROL"sv; break;
	case	eOBJECT_TYPE::CONTROL_TIME:									result = "CONTROL_TIME"sv; break;
	case	eOBJECT_TYPE::CONTROL_TIME_SET:								result = "CONTROL_TIME_SET"sv; break;
	case	eOBJECT_TYPE::CONTROL_TIME_SET_ARRAY:						result = "CONTROL_TIME_SET_ARRAY"sv; break;

	// 12) applier
	case	eOBJECT_TYPE::APPLIER_CAMERA:								result = "APPLIER_CAMERA"sv; break;
	case	eOBJECT_TYPE::APPLIER_EFFECT:								result = "APPLIER_EFFECT"sv; break;
	case	eOBJECT_TYPE::APPLIER_LIGHT:								result = "APPLIER_LIGHT"sv; break;
	case	eOBJECT_TYPE::APPLIER_MATERIAL:								result = "APPLIER_MATERIAL"sv; break;
	case	eOBJECT_TYPE::APPLIER_TEXTURE:								result = "APPLIER_TEXTURE"sv; break;
	case	eOBJECT_TYPE::APPLIER_BONEINFO:								result = "APPLIER_BONEINFO"sv; break;
	case	eOBJECT_TYPE::APPLIER_RENDERTARGET:							result = "APPLIER_RENDERTARGET"sv; break;
	case	eOBJECT_TYPE::APPLIER_VERTEXSTREAM:							result = "APPLIER_VERTEXSTREAM"sv; break;
	case	eOBJECT_TYPE::APPLIER_STATEBLOCK:							result = "APPLIER_STATEBLOCK"sv; break;
	case	eOBJECT_TYPE::APPLIER_SCENEINFO:							result = "APPLIER_SCENEINFO"sv; break;

	// 13) renderer
	case	eOBJECT_TYPE::RENDERER_MESH:								result = "RENDERER_MESH"sv; break;
	case	eOBJECT_TYPE::RENDERER_WIRE:								result = "RENDERER_WIRE"sv; break;
	case	eOBJECT_TYPE::RENDERER_TEXT:								result = "RENDERER_TEXT"sv; break;
	case	eOBJECT_TYPE::RENDERER_BILLBOARD:							result = "RENDERER_BILLBOARD"sv; break;
	case	eOBJECT_TYPE::RENDERER_PARTICLE:							result = "RENDERER_PARTICLE"sv; break;
	case	eOBJECT_TYPE::RENDERER_RAY:									result = "RENDERER_RAY"sv; break;
	case	eOBJECT_TYPE::RENDERER_SKYBOX:								result = "RENDERER_SKYBOX"sv; break;
	case	eOBJECT_TYPE::RENDERER_VIEW:								result = "RENDERER_VIEW"sv; break;
	case	eOBJECT_TYPE::RENDERER_SCENE:								result = "RENDERER_SCENE"sv; break;
	case	eOBJECT_TYPE::RENDERER_SPRITE:								result = "RENDERER_SPRITE"sv; break;

	// 14) scene_node
	case	eOBJECT_TYPE::SCENENODE:									result = "SCENENODE"sv; break;
	case	eOBJECT_TYPE::SCENENODE_TM:									result = "SCENENODE_TM"sv; break;
	case	eOBJECT_TYPE::SCENENODE_BLENDING:							result = "SCENENODE_BLENDING"sv; break;
	case	eOBJECT_TYPE::SCENENODE_TREE:								result = "SCENENODE_TREE"sv; break;
	case	eOBJECT_TYPE::SCENENODE_TREE_TERRAIN:						result = "SCENENODE_TREE_TERRAIN"sv; break;
	case	eOBJECT_TYPE::SCENENODE_GRAPH:								result = "SCENENODE_GRAPH"sv; break;
	case	eOBJECT_TYPE::SCENENODE_PORTAL:								result = "SCENENODE_PORTAL"sv; break;
	case	eOBJECT_TYPE::SCENENODE_SPRITE:								result = "SCENENODE_SPRITE"sv; break;
	case	eOBJECT_TYPE::SCENENODE_BACKGROUND:							result = "SCENENODE_BACKGROUND"sv; break;
	case	eOBJECT_TYPE::SCENENODE_VIEW:								result = "SCENENODE_VIEW"sv; break;
	case	eOBJECT_TYPE::SCENENODE_LINK:								result = "SCENENODE_LINK"sv; break;

	// 15) entity
	case	eOBJECT_TYPE::ENTITY:										result = "ENTITY"sv; break;
	case	eOBJECT_TYPE::ENTITY_GENERAL:								result = "ENTITY_GENERAL"sv; break;
	case	eOBJECT_TYPE::ENTITY_ADDITIONAL:							result = "ENTITY_ADDITIONAL"sv; break;

	// 16) ui
	case	eOBJECT_TYPE::UI:											result = "UI"sv; break;
	case	eOBJECT_TYPE::UI_STATIC:									result = "UI_STATIC"sv; break;
	case	eOBJECT_TYPE::UI_EDIT:										result = "UI_EDIT"sv; break;
	case	eOBJECT_TYPE::UI_BUTTON:									result = "UI_BUTTON"sv; break;
	case	eOBJECT_TYPE::UI_LIST:										result = "UI_LIST"sv; break;
	case	eOBJECT_TYPE::UI_SCROLLBAR:									result = "UI_SCROLLBAR"sv; break;
	case	eOBJECT_TYPE::UI_SLIDER:									result = "UI_SLIDER"sv; break;
	case	eOBJECT_TYPE::UI_COMBO:										result = "UI_COMBO"sv; break;
	case	eOBJECT_TYPE::UI_FORM:										result = "UI_FORM"sv; break;
	case	eOBJECT_TYPE::UI_CHECKBOX:									result = "UI_CHECKBOX"sv; break;
	case	eOBJECT_TYPE::UI_LISTVIEW:									result = "UI_LISTVIEW"sv; break;
	case	eOBJECT_TYPE::UI_IMAGE:										result = "UI_IMAGE"sv; break;

	// 17) reference 
	case	eOBJECT_TYPE::REFERENCE:									result = "REFERENCE"sv; break;
	case	eOBJECT_TYPE::REFERENCE_SPHERE:								result = "REFERENCE_SPHERE"sv; break;
	case	eOBJECT_TYPE::REFERENCE_ELLIPSOID:							result = "REFERENCE_ELLIPSOID"sv; break;
	case	eOBJECT_TYPE::REFERENCE_AABB:								result = "REFERENCE_AABB"sv; break;
	case	eOBJECT_TYPE::REFERENCE_OBB:								result = "REFERENCE_OBB"sv; break;
	case	eOBJECT_TYPE::REFERENCE_CYLINDER:							result = "REFERENCE_CYLINDER"sv; break;
	case	eOBJECT_TYPE::REFERENCE_CONE:								result = "REFERENCE_CONE"sv; break;
	case	eOBJECT_TYPE::REFERENCE_CAPSULE:							result = "REFERENCE_CAPSULE"sv; break;
	case	eOBJECT_TYPE::REFERENCE_CONVEX:								result = "REFERENCE_CONVEX"sv; break;
	case	eOBJECT_TYPE::REFERENCE_LINE:								result = "REFERENCE_LINE"sv; break;
	case	eOBJECT_TYPE::REFERENCE_SEGMENT:							result = "REFERENCE_SEGMENT"sv; break;
	case	eOBJECT_TYPE::REFERENCE_CIRCLE:								result = "REFERENCE_CIRCLE"sv; break;
	case	eOBJECT_TYPE::REFERENCE_ELLIPSE:							result = "REFERENCE_ELLIPSE"sv; break;
	case	eOBJECT_TYPE::REFERENCE_TRIANGLE:							result = "REFERENCE_TRIANGLE"sv; break;
	case	eOBJECT_TYPE::REFERENCE_RECTANGLE:							result = "REFERENCE_RECTANGLE"sv; break;
	case	eOBJECT_TYPE::REFERENCE_PLANE:								result = "REFERENCE_PLANE"sv; break;
	case	eOBJECT_TYPE::REFERENCE_PLANES:								result = "REFERENCE_PLANES"sv; break;
	case	eOBJECT_TYPE::REFERENCE_TERRAIN:							result = "REFERENCE_TERRAIN"sv; break;
	case	eOBJECT_TYPE::REFERENCE_PORTAL:								result = "REFERENCE_PORTAL"sv; break;
	case	eOBJECT_TYPE::REFERENCE_POINT:								result = "REFERENCE_POINT"sv; break;
	case	eOBJECT_TYPE::REFERENCE_TRANSFORMMATRIX:					result = "REFERENCE_TRANSFORMMATRIX"sv; break;

	// default) 
	default:															result = " - ";
	}

	// return)
	return	result;
}