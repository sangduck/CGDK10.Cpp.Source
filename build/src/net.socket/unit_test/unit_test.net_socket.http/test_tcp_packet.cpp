#include "pch.h"

//-----------------------------------------------------------------------------
// tcp message framing (net::io::packetable::Nstream : [uint32 size(header included)][body])
//
//  - the data is fed in pieces like the receivable does (unprocessed bytes are kept and the next piece is appended)
//  - split at every byte, several messages in one receive, the required size, invalid size
//-----------------------------------------------------------------------------
namespace CGDK
{

class test_tcp_socket : public net::socket::tcp<>
{
public:
	// - the framing stops when the socket handle is invalid (closed). a dummy handle is set while testing.
	//   (process_packet does not use the handle. it is reset before the destruction : nothing is closed)
			test_tcp_socket() { this->native_handle(static_cast<CGSOCKET>(0x7fff0001)); }
	virtual ~test_tcp_socket() { this->native_handle(INVALID_SOCKET); }

			std::vector<std::string> messages;			// - body of each message (header excluded)
			std::size_t			last_required{ 0 };

			void				feed(std::string_view _data)
	{
		auto buf = alloc_shared_buffer(this->m_pending.size() + _data.size() + 16);
		if (this->m_pending.size() != 0)
			buf.append(this->m_pending.size(), this->m_pending.data());
		buf.append(_data.size(), _data.data());

		this->last_required = this->process_packet(buf, net::sockaddr());

		// - unprocessed bytes
		this->m_pending = (buf.size() != 0) ? shared_buffer(buf) : shared_buffer();
	}
			void				feed_split(std::string_view _data, std::size_t _size)
	{
		for (std::size_t pos = 0; pos < _data.size(); pos += _size)
			this->feed(_data.substr(pos, _size));
	}
	[[nodiscard]] std::size_t	pending_size() const noexcept { return this->m_pending.size(); }

protected:
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		this->messages.emplace_back(reinterpret_cast<const char*>(_msg.buf_message.data()) + sizeof(uint32_t), _msg.buf_message.size() - sizeof(uint32_t));
		return eRESULT::DONE;
	}

			shared_buffer		m_pending;
};

// - [uint32 size][body]
static std::string make_message(std::string_view _body)
{
	const uint32_t size = static_cast<uint32_t>(sizeof(uint32_t) + _body.size());
	std::string message(reinterpret_cast<const char*>(&size), sizeof(size));
	message.append(_body);
	return message;
}

TEST(tcp_packet, single_message)
{
	auto psocket = make_own<test_tcp_socket>();
	psocket->feed(make_message("hello"));

	ASSERT_EQ(psocket->messages.size(), 1u);
	EXPECT_EQ(psocket->messages[0], "hello");
	EXPECT_EQ(psocket->pending_size(), 0u);
}

TEST(tcp_packet, split_at_every_byte)
{
	const std::string data = make_message("first message") + make_message("") + make_message(std::string(300, 'x')) + make_message("last");

	for (std::size_t size = 1; size <= data.size(); ++size)
	{
		auto psocket = make_own<test_tcp_socket>();
		psocket->feed_split(data, size);

		ASSERT_EQ(psocket->messages.size(), 4u) << "piece size " << size;
		EXPECT_EQ(psocket->messages[0], "first message") << "piece size " << size;
		EXPECT_EQ(psocket->messages[1], "") << "piece size " << size;
		EXPECT_EQ(psocket->messages[2], std::string(300, 'x')) << "piece size " << size;
		EXPECT_EQ(psocket->messages[3], "last") << "piece size " << size;
		EXPECT_EQ(psocket->pending_size(), 0u) << "piece size " << size;
	}
}

TEST(tcp_packet, many_messages_in_one_receive)
{
	std::string data;
	for (int i = 0; i < 1000; ++i)
		data += make_message(std::to_string(i));

	auto psocket = make_own<test_tcp_socket>();
	psocket->feed(data);

	ASSERT_EQ(psocket->messages.size(), 1000u);
	EXPECT_EQ(psocket->messages[0], "0");
	EXPECT_EQ(psocket->messages[999], "999");
}

TEST(tcp_packet, required_size)
{
	// - only the header of a 10000 byte message : the required size is the message size (the receive buffer grows to it)
	const std::string message = make_message(std::string(10000 - sizeof(uint32_t), 'y'));

	auto psocket = make_own<test_tcp_socket>();
	psocket->feed(message.substr(0, 4));
	EXPECT_TRUE(psocket->messages.empty());
	EXPECT_EQ(psocket->last_required, 10000u);

	psocket->feed(message.substr(4, 5000));
	EXPECT_TRUE(psocket->messages.empty());
	EXPECT_EQ(psocket->last_required, 10000u);

	psocket->feed(message.substr(5004));
	ASSERT_EQ(psocket->messages.size(), 1u);
	EXPECT_EQ(psocket->messages[0].size(), 10000u - sizeof(uint32_t));
}

TEST(tcp_packet, invalid_size)
{
	// - size smaller than the header (0, 3) : the connection is closed (exception)
	for (uint32_t size : { 0u, 3u })
	{
		auto psocket = make_own<test_tcp_socket>();
		std::string data(reinterpret_cast<const char*>(&size), sizeof(size));
		data += "abcd";
		EXPECT_ANY_THROW(psocket->feed(data)) << "size " << size;
	}
}

}
