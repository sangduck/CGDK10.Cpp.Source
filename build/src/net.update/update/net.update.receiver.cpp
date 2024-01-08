#include "pch.h"


void CGDK::update::receiver::set_transmitter(Ireferenceable* _psource)
{
	// 1) get sender
	auto* psender = dynamic_cast<net::io::Isender*>(_psource);

	// cheeck)
	THROW_IF(psender == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'_psource is not' 'net::io::Isender' object"sv));

	// 2) get sender
	auto* pmessage_transmitter = dynamic_cast<Imessage_transmitter*>(_psource);

	// cheeck)
	THROW_IF(pmessage_transmitter == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'_psource is not' 'Imessage_transmitter' objject"sv));

	// 3) set and register
	pmessage_transmitter->register_messageable(this);

	// 4) set ...
	this->m_psender = psender;
	this->m_pmessage_transmitter = pmessage_transmitter;
}

void CGDK::update::receiver::reset_transmitter(Ireferenceable* _psource) noexcept
{
	// 1) get sender
	auto* psender = dynamic_cast<net::io::Isender*>(_psource);

	// cheeck)
	RETURN_IF(psender == nullptr,);

	// cheeck)
	RETURN_IF(this->m_psender != psender,);

	// 2) get sender
	auto* pmessage_transmitter = dynamic_cast<Imessage_transmitter*>(_psource);

	// cheeck)
	RETURN_IF(this->m_pmessage_transmitter != pmessage_transmitter,);

	// 3) 
	pmessage_transmitter->unregister_messageable(this);

	// 4) reset
	this->m_psender.reset();
	this->m_pmessage_transmitter.reset();
}
