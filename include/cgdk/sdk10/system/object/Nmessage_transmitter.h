//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                                  Common                                   *
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
/**

 @class		Nmessage_transmitter


*/
//-----------------------------------------------------------------------------
class CGDK::Nmessage_transmitter : virtual public Imessage_transmitter
{
protected:
	virtual result_code			process_transmit_message(sMESSAGE& _msg) override;

	virtual bool				process_attach(object_ptr<Imessageable>&& _pmessageable) override { return this->process_attach_messageable_back(std::move(_pmessageable));}
	virtual bool				process_detach(Imessageable* _pmessageable) noexcept override;
	virtual int					process_reset_message_transmitter() noexcept override;

			bool				process_attach_messageable_front(object_ptr<Imessageable>&& _pmessageable);
			bool				process_attach_messageable_back(object_ptr<Imessageable>&& _pmessageable);
private:
			bool				_process_attach_messageable(object_ptr<Imessageable>&& _pmessageable, bool _is_front);
private:
			// - messageable 목록 (바꾸지 않는 snapshot : attach/detach할 때 새로 만들어 통째로 바꾼다)
			//   transmit은 snapshot 하나만 복사해 쓴다. (messageable마다 참조 count를 올리고 내리지 않는다.
			//   여러 socket이 같은 messageable(예: web server)을 공유할 때 그 참조 count 경합이 없다)
			//   snapshot이 messageable의 참조를 가지고 있으므로 transmit 중에 detach되어도 messageable은 해제되지 않는다.
			using CONTAINER_MESSAGEABLE = static_vector<object_ptr<Imessageable>, MAX_MEDIATOR>;
			std::shared_mutex	m_lockable_array_messageable;
			std::shared_ptr<const CONTAINER_MESSAGEABLE> m_parray_messageable;
};
