#include "pch.h"

using namespace CGDK;


net::concurrent_send_queue::concurrent_send_queue()
{
	this->m_vector_send_queued.reserve(64);
	this->m_vector_send_sending.reserve(64);
}
net::concurrent_send_queue::~concurrent_send_queue() noexcept
{
}

void net::concurrent_send_queue::enqueue(const SENDING& _sending)
{
	SCOPED_LOCK(m_cs_vector_send_queued)
	{
		this->m_vector_send_queued.emplace_back(_sending);
	}
}

void net::concurrent_send_queue::enqueue(SENDING&& _sending)
{
	SCOPED_LOCK(this->m_cs_vector_send_queued)
	{
		this->m_vector_send_queued.emplace_back(std::move(_sending));
	}
}

void net::concurrent_send_queue::send() noexcept
{
	SCOPED_LOCK(this->m_cs_vector_send_sending)
	{
		// check) 
		CGASSERT_ERROR(this->m_vector_send_sending.empty());

		// 1) get queued sendable
		SCOPED_LOCK(this->m_cs_vector_send_queued)
		{
			// check)
			RETURN_IF(this->m_vector_send_queued.empty());

			// - swap
			this->m_vector_send_queued.swap(this->m_vector_send_sending);

			// check) 
			CGASSERT_ERROR(this->m_vector_send_sending.empty() == false);
			CGASSERT_ERROR(this->m_vector_send_queued.empty());
		}

		// 2) send~
		for (auto& iter : this->m_vector_send_sending)
		{
			iter.psender->send(iter.buffer);
		}

		// 3) clear
		this->m_vector_send_sending.clear();
	}
}

void net::local_send_queue::enqueue(SENDING&& _sending)
{
	this->m_vector_send_queued.emplace_back(std::move(_sending));
}

void net::local_send_queue::send() noexcept
{
	// check)
	RETURN_IF(this->m_vector_send_queued.empty());

	// 2) send~
	for (auto& iter : this->m_vector_send_queued)
	{
		iter.psender->send(iter.buffer);
	}

	// 3) clear
	this->m_vector_send_queued.clear();
}
