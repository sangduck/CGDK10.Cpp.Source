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
/*!
* @class Ireceivable_stream
*
* @ingroup net::io
*
* @brief stream용 저수준 I/O를 통해 데이터수신 처리를 정의하는 클래스
*
*/
//-----------------------------------------------------------------------------
class net::io::Ireceivable_stream :
	virtual public				Ireferenceable
{
public:
	// @brief reset할 때 호출하는 함수.(기본은 빈 것...) @return 0 성공 @return !0 실패
	virtual	void				process_reset_receivable() PURE;
	// @brief receive를 prepare하는 함수이다.(이 함수 이후에 receive가 가능해진다.) @return 0 성공 @return !0 실패
	//    (만약 처리 중 잘못되는 경우 throw를 할 수도 있다.)
	virtual	bool				prepare_receive() PURE;
	// @brief receive를 받았을 때 호출된다. @return 0 성공 @return !0 실패 @return 0 성공 @return !0 실패
	virtual	intptr_t			process_complete_receivable(intptr_t _result, std::size_t _transferred) PURE;
#if defined(_SYS_EPOLL_H)
	// @brief io_uring completion : this receivable receives with the RECV op of the io_context (receivable::Nstream). false : readiness only
	virtual	bool				is_io_completion_supported() const noexcept { return false; }
	// @brief io_uring completion : posts the first RECV op (Isocket::process_start_io). false : failed (the caller closes the socket)
	virtual	bool				process_start_receive() noexcept { return false; }
#endif
};


}