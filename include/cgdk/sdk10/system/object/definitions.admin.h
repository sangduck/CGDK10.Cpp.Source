//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                           Server.Admin Classes                            *
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
//-----------------------------------------------------------------------------
//
// Definitions for buffer
//
//-----------------------------------------------------------------------------
namespace CGDK
{
namespace server_system
{
	//-----------------------------------------------------------------------------
	/**

	@class		iid_t (instance id)
	@brief
	@todo
	@ref		sSETTING_INFO, sSERVICE_INSTANCE_INFO
	@details	\n

	*/
	//-----------------------------------------------------------------------------
	// 1) instance id (매번 객체화될 때마다 할당되는 id이다. (0:비객체화, >0:객체화))
	typedef uint64_t iid_t;

	// 2) 
	constexpr iid_t IID_NONE{ 0 };
	constexpr iid_t IID_AUTO{ static_cast<iid_t>(-1)};
	constexpr int32_t RANK_NONE{ 0 };
	constexpr int32_t RANK_AUTO{ -1 };


	//-----------------------------------------------------------------------------
	/**

	@class		sRANK
	@brief		설정 정보의 기본 내용
	@todo
	@ref		sHOST_SETTING_INFO, sSERVICE_INSTANCE_INFO, SERVICE_SETTING
	@details	\n

	*/
	//-----------------------------------------------------------------------------
	// 1) revision
	struct sREVISION
	{
		ENABLE_STRUCT_SERIALIZABLE

		std::string			url;						// - URL
		int64_t				revision{ 0 };				// - version
		chrono::time::utc::time_point time;				// - time
	};

	// 2) rank(역할)
	struct sRANK
	{
		// 1) group/field
		int32_t				group{ RANK_NONE };
		int32_t				field{ RANK_NONE };

		// 2) role/Index
		int32_t				role{ RANK_NONE };
		int32_t				index{ RANK_AUTO };

		// function)
		bool				empty() const noexcept { return this->group == RANK_NONE && this->field == RANK_NONE && this->role == RANK_NONE && this->index <= RANK_NONE; }
		bool				exist() const noexcept { return !this->empty(); }
		void				clear() noexcept;

		std::string			to_string() const;
	};
	inline bool operator == (const sRANK& _lhs, const sRANK& _rhs) noexcept { return _lhs.group == _rhs.group && _lhs.field == _rhs.field && _lhs.role == _rhs.role && _lhs.index == _rhs.index; }
	inline bool operator != (const sRANK& _lhs, const sRANK& _rhs) noexcept { return !(_lhs == _rhs); }
	inline bool operator >  (const sRANK& _lhs, const sRANK& _rhs) noexcept { return (_lhs.group > _rhs.group) ? true : (_lhs.group < _rhs.group) ? false : (_lhs.field > _rhs.field) ? true : (_lhs.field < _rhs.field) ? false : (_lhs.role > _rhs.role) ? true : (_lhs.role < _rhs.role) ? false : (_lhs.index > _rhs.index); }
	inline bool operator <= (const sRANK& _lhs, const sRANK& _rhs) noexcept { return !(_lhs > _rhs); }
	inline bool operator <  (const sRANK& _lhs, const sRANK& _rhs) noexcept { return  (_rhs > _lhs); }
	inline bool operator >= (const sRANK& _lhs, const sRANK& _rhs) noexcept { return !(_lhs > _rhs); }
}

std::string to_string(server_system::iid_t _iid);

}

