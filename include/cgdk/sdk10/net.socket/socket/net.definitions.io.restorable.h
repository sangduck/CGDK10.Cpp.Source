//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          network socket classes                           *
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

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// 1. [sequence_id_t]
//
//
//-----------------------------------------------------------------------------
using sequence_id_t = uint64_t;


//-----------------------------------------------------------------------------
//
// 2. [restore_server_id_t]
//
//
//-----------------------------------------------------------------------------
struct restore_server_id_t
{
public:
	uint64_t	id_key;
	uint64_t	id_certify;

public:
	restore_server_id_t() noexcept : id_key(0), id_certify(0) {}

	bool				empty() const noexcept	{ return id_key == 0 || id_certify == 0; }
	bool				exist() const noexcept	{ return id_key != 0 && id_certify != 0; }
	void				reset() noexcept		{ id_key=0; id_certify=0;}

	restore_server_id_t&	operator = (const restore_server_id_t& _rhs) noexcept { id_key = _rhs.id_key; id_certify = _rhs.id_certify; return *this; }

	bool				operator ==(const restore_server_id_t& _rhs) const noexcept { return id_key == _rhs.id_key && id_certify == _rhs.id_certify; }
	bool				operator !=(const restore_server_id_t& _rhs) const noexcept { return id_key != _rhs.id_key || id_certify != _rhs.id_certify; }
	bool				operator > (const restore_server_id_t& _rhs) const noexcept { return id_key >  _rhs.id_key; }
	bool				operator >=(const restore_server_id_t& _rhs) const noexcept { return id_key >= _rhs.id_key; }
	bool				operator < (const restore_server_id_t& _rhs) const noexcept { return id_key <  _rhs.id_key; }
	bool				operator <=(const restore_server_id_t& _rhs) const noexcept { return id_key <= _rhs.id_key; }
};


//-----------------------------------------------------------------------------
//
// 3. [restore_client_io_t]
//
//
//-----------------------------------------------------------------------------
struct restore_client_io_t
{
public:
	uint64_t	id_key;
	uint64_t	id_certify;

public:
	restore_client_io_t() noexcept : id_key(0), id_certify(0) {}

	bool				empty() const noexcept	{ return id_key == 0 || id_certify == 0; }
	bool				exist() const noexcept	{ return id_key != 0 && id_certify != 0; }
	void				reset() noexcept		{ id_key=0; id_certify=0;}

	restore_client_io_t&	operator = (const restore_client_io_t& _rhs) noexcept { id_key = _rhs.id_key; id_certify = _rhs.id_certify; return *this; }

	bool				operator ==(const restore_client_io_t& _rhs) const noexcept { return id_key == _rhs.id_key && id_certify == _rhs.id_certify; }
	bool				operator !=(const restore_client_io_t& _rhs) const noexcept { return id_key != _rhs.id_key || id_certify != _rhs.id_certify; }
	bool				operator > (const restore_client_io_t& _rhs) const noexcept { return id_key >  _rhs.id_key; }
	bool				operator >=(const restore_client_io_t& _rhs) const noexcept { return id_key >= _rhs.id_key; }
	bool				operator < (const restore_client_io_t& _rhs) const noexcept { return id_key <  _rhs.id_key; }
	bool				operator <=(const restore_client_io_t& _rhs) const noexcept { return id_key <= _rhs.id_key; }
};


}

