//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Server Admin Host Agent                          *
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
// ----------------------------------------------------------------------------
//
// websocket::Iobservable
//
// ----------------------------------------------------------------------------
class client::web::Iobservable :
// inherited classes)
	public						observable::list_sender<socket>,
	public						Nnameable,
	public                      Nobject_startable,
	public						Nmessage_transmitter,
	public						net::io::messageable::Nbase
{
// constructor/destructor)
protected:
			Iobservable() {}
	explicit Iobservable(const std::string_view _name);
	virtual ~Iobservable();

// framework) 
private:
	virtual	void				on_initialize(context& _context) override;
	virtual	void				on_destroy() override;
	virtual	void				on_start(context& _context) override;
	virtual	void				on_stop() override;

// implementation) 
public:
			std::string_view	m_webpage_str_target;
			char				m_webpage_index = 0;
};
