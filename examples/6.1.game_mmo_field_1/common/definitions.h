//*****************************************************************************
//*                                                                           *
//*                            lokon server system                            *
//*                                                                           *
//*                                  common                                   *
//*                                                                           *
//*  powerd by CGCII                                                          *
//*                                                                           *
//*  start : 2022/02/3                                                        *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//  include
//-----------------------------------------------------------------------------
typedef uint64_t REQUEST_ID;

typedef int64_t	ACCOUNT_UID;
typedef int64_t	ACCOUNT_INDEX;

typedef int64_t	AREA_INDEX;

typedef int64_t	AREA_ENTITY_ID;

namespace eENTITY_EVENT
{
	static const size_t SIZE = 4;
	typedef uint32_t TYPE;

	const TYPE	NONE = 0;
	const TYPE	ENTER = 1;
	const TYPE	LEAVE = 2;
	const TYPE	MOVE_TO = 3;
	const TYPE	DAMAGE = 4;
	const TYPE	ATTACK = 5;
	const TYPE	ACTION = 6;
	const TYPE	SKILL = 7;
	const TYPE	DEATH = 8;
	const TYPE	GET = 9;
	const TYPE	USE = 10;
	const TYPE	STATE = 11;
}

struct sAREA_INFO
{
	AREA_INDEX	id;
	double		tick_now = 0.0;
};


struct position3
{
	double x;
	double y;
	double z;
};

struct rotation3
{
	double pitch;
	double yaw;
	double roll;
};

struct eAREA_ENTITY_TYPE
{
	typedef uint64_t TYPE;

	static const TYPE NONE		 = 0;
	static const TYPE CHARACTER  = 1;
	static const TYPE MOB		 = 2;
	static const TYPE OBJECT	 = 3;
};

struct sAREA_ENTITY_INFO
{
	AREA_ENTITY_ID		id;
	eAREA_ENTITY_TYPE::TYPE	type;
	ACCOUNT_INDEX		owner;
};

struct sAREA_ENTITY_EVENT
{
	double				tick;
	AREA_ENTITY_ID		id;
	eENTITY_EVENT::TYPE	event_type;
};

#if defined(UE_BUILD_DEBUG) || defined(UE_BUILD_DEVELOPMENT) || defined(UE_BUILD_TEST) || defined(UE_BUILD_SHIPPING)
struct sAREA_ENTITY_STATE
{
	double		tick = 0.0;

	FVector		translation;
	FRotator	rotator;
	FVector		velocity;
	FVector		acceleration;
};
#else
struct sAREA_ENTITY_STATE
{
	double		tick = 0.0;

	position3	translation;
	rotation3	rotator;
	position3	velocity;
	position3	acceleration;
};
#endif

struct sENTITY_TAG
{
	ENABLE_STRUCT_SERIALIZABLE;

	std::string name;
};

struct sACCOUNT_INFO
{
	ENABLE_STRUCT_SERIALIZABLE;

	ACCOUNT_INDEX	account_index;

	sENTITY_TAG		tag;
};
