#pragma once

namespace CGDK
{

enum class SLENGTH_TYPE
{
	NOT_DEFINED	 = 0,
	UINT8		 = 1,
	INT8		 = 2,
	UINT16		 = 3,
	INT16		 = 4,
	UINT32		 = 5,
	INT32		 = 6,
	UINT64		 = 7,
	INT64		 = 8,
	VARIABLE	 = 9,
	FIXED		 = 10,
};

struct sMESSAGE_NODE
{
	SLENGTH_TYPE	type = SLENGTH_TYPE::NOT_DEFINED;
	int				offset = 0;
	uint64_t		value = 0;

	uint64_t		size() const
	{
		switch (this->type)
		{
		case	SLENGTH_TYPE::UINT8:
		case	SLENGTH_TYPE::INT8:
				return sizeof(uint8_t);

		case	SLENGTH_TYPE::UINT16:
		case	SLENGTH_TYPE::INT16:
				return sizeof(uint16_t);

		case	SLENGTH_TYPE::UINT32:
		case	SLENGTH_TYPE::INT32:
				return sizeof(uint32_t);

		case	SLENGTH_TYPE::UINT64:
		case	SLENGTH_TYPE::INT64:
				return sizeof(uint64_t);

		case	SLENGTH_TYPE::VARIABLE:
				return sizeof(uint64_t);

		case	SLENGTH_TYPE::FIXED:
				return this->value;
		default:
				throw throwable::failure(eRESULT::INVALID_ARGUMENT);
		}
	}
	uint64_t		get(const buffer& _buf) const
	{
		// declare) 
		uint64_t result = 0;

		switch (this->type)
		{
		case	SLENGTH_TYPE::UINT8:
		case	SLENGTH_TYPE::INT8:
				result = _buf.front<uint8_t>(this->offset);
				break;

		case	SLENGTH_TYPE::UINT16:
		case	SLENGTH_TYPE::INT16:
				result = _buf.front<uint16_t>(this->offset);
				break;

		case	SLENGTH_TYPE::UINT32:
		case	SLENGTH_TYPE::INT32:
				result = _buf.front<uint32_t>(this->offset);
				break;

		case	SLENGTH_TYPE::UINT64:
		case	SLENGTH_TYPE::INT64:
				result = _buf.front<uint64_t>(this->offset);
				break;

		case	SLENGTH_TYPE::VARIABLE:
				result = _buf.front<uint64_t>(this->offset);
				break;

		case	SLENGTH_TYPE::FIXED:
				result = this->value;
				break;

		case	SLENGTH_TYPE::NOT_DEFINED:
				throw throwable::failure(eRESULT::INVALID_ARGUMENT);
		}

		// return) 
		return static_cast<uint64_t>(result);
	}
	void			set(buffer_view& _buf, uint64_t _value) const
	{
		switch (this->type)
		{
		case	SLENGTH_TYPE::UINT8:
		case	SLENGTH_TYPE::INT8:
				_buf.front<uint8_t>(this->offset) = static_cast<uint8_t>(_value);
				break;

		case	SLENGTH_TYPE::UINT16:
		case	SLENGTH_TYPE::INT16:
				_buf.front<uint16_t>(this->offset) = static_cast<uint16_t>(_value);
				break;

		case	SLENGTH_TYPE::UINT32:
		case	SLENGTH_TYPE::INT32:
				_buf.front<uint32_t>(this->offset) = static_cast<uint32_t>(_value);
				break;

		case	SLENGTH_TYPE::UINT64:
		case	SLENGTH_TYPE::INT64:
				_buf.front<uint64_t>(this->offset) = static_cast<uint64_t>(_value);
				break;

		case	SLENGTH_TYPE::VARIABLE:
				CGASSERT_ERROR(false);
				break;

		case	SLENGTH_TYPE::FIXED:
				_buf.front<uint64_t>(this->offset) = this->value;
				break;

		case	SLENGTH_TYPE::NOT_DEFINED:
				throw throwable::failure(eRESULT::INVALID_ARGUMENT);
		}
	}
};

struct sMESSAGE_HEADER
{
	virtual ~sMESSAGE_HEADER() noexcept {}

	uint64_t		header_size = 4;
	uint64_t		header_weight = 0;
	sMESSAGE_NODE	header_length { SLENGTH_TYPE::UINT16, 2, 0 };
	std::string		header_lengh_type = "header+body"s;
	std::vector<sMESSAGE_NODE> vector_headers;

	uint64_t		get_required_size(const buffer& _buf)
	{
		return this->header_length.get(_buf) + this->header_weight;
	}
	void			set_message_size(buffer_view& _buf, uint64_t _length)
	{
		// check)
		THROW_IF(_length < this->header_weight, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_SHORT_LENGTH));

		this->header_length.set(_buf, _length - this->header_weight);
	}

	sMESSAGE_NODE	read_setting_header_node(context& _context)
	{
		sMESSAGE_NODE node_info;

		// header.offset
		auto context_offset = _context.find("offset"sv);
		if (context_offset.is_valid())
		{
			node_info.offset = context_offset;
		}

		// header.type
		auto context_type = _context.find("type"sv);
		if (context_type.is_valid())
		{
			std::string data_type = context_type;

			if (data_type == "int8"sv
				|| data_type == "int8_t"sv
				|| data_type == "byte"sv)
			{
				node_info.type = SLENGTH_TYPE::INT8;
			}
			else if (data_type == "uint8"sv
				|| data_type == "uint8_t"sv
				|| data_type == "char"sv)
			{
				node_info.type = SLENGTH_TYPE::UINT8;
			}
			else if (data_type == "int16"sv
				|| data_type == "int16_t"sv
				|| data_type == "short"sv)
			{
				node_info.type = SLENGTH_TYPE::INT16;
			}
			else if (data_type == "uint16"sv
				|| data_type == "uint16_t"sv
				|| data_type == "ushort"sv)
			{
				node_info.type = SLENGTH_TYPE::UINT16;
			}
			else if (data_type == "int32"sv
				|| data_type == "int32_t"sv
				|| data_type == "int"sv)
			{
				node_info.type = SLENGTH_TYPE::INT32;
			}
			else if (data_type == "uint32"sv
				|| data_type == "uint32_t"sv
				|| data_type == "uint"sv)
			{
				node_info.type = SLENGTH_TYPE::UINT32;
			}
			else if (data_type == "int64"sv
				|| data_type == "int64_t"sv
				|| data_type == "long"sv)
			{
				node_info.type = SLENGTH_TYPE::INT64;
			}
			else if (data_type == "uint64"sv
				|| data_type == "uint64_t"sv
				|| data_type == "ulong"sv)
			{
				node_info.type = SLENGTH_TYPE::UINT64;
			}
			else if (data_type == "variable"sv)
			{
				node_info.type = SLENGTH_TYPE::VARIABLE;
			}
		}

		auto context_value = _context.find("value"sv);
		if (context_value.is_valid())
		{
			//auto context_size = _context.find("message_size"sv);
			//if (context_size.is_valid())
			//{
			//	std::string size_type = context_size;
			//	if (size_type == "body"sv
			//		|| size_type == "body_only"sv
			//		|| size_type == "body only"sv)
			//	{
			//		this->length_weight = this->header_size;
			//	}
			//	else if (size_type == "header_include"sv
			//		|| size_type == "header include"sv
			//		|| size_type == "header_body"sv
			//		|| size_type == "header body"sv)
			//	{
			//		this->length_weight = 0;
			//	}
			//	else if (size_type == "constant"sv)
			//	{
			//		this->length_weight = 0;
			//	}
			//}
		}

		return node_info;
	}
	void			read_setting(context& _context)
	{
		// - header
		{
			auto context_header = _context.find("header"sv);
			if (context_header.is_valid())
			{
				uint64_t		temp_header_size = 0;
				uint64_t		temp_header_weight = 0;
				sMESSAGE_NODE	temp_header_length{ SLENGTH_TYPE::UINT16, 2, 0 };
				std::string		temp_header_lengh_type = "header+body"s;
				std::vector<sMESSAGE_NODE> temp_vector_headers;

				bool is_header_weighted = false;
				for (auto& iter : context_header)
				{
					// check) 
					CONTINUE_IF(iter.second.is_invalid());

					auto read_message_node = read_setting_header_node(iter.second);

					if (iter.first == "length"sv)
					{
						temp_header_length = read_message_node;

						auto context_value = iter.second.find("value"sv);
						if (context_value.is_valid())
						{
							temp_header_lengh_type = (std::string)context_value;
							if (temp_header_lengh_type == "body_only")
							{
								is_header_weighted = true;
							}
						}
					}
					else
					{
						temp_vector_headers.push_back(read_message_node);
					}

					auto temp = static_cast<uint64_t>(read_message_node.offset + read_message_node.size());
					if (temp > temp_header_size)
					{
						temp_header_size = temp;
					}
				}

				this->header_size = temp_header_size;
				if (is_header_weighted)
				{
					temp_header_weight = temp_header_size;
				}

				// - set
				this->header_size = temp_header_size;
				this->header_weight = temp_header_weight;
				this->header_length = temp_header_length;
				this->header_lengh_type = std::move(temp_header_lengh_type);
				this->vector_headers = std::move(temp_vector_headers);
			}
		}
	}
};

class packetable_stream_variable :
// inherited classes)
	virtual public				net::io::Isocket,
	virtual public				net::io::Ipacketable,
	virtual public				Imessageable,
	virtual public				net::io::statistics::Nsocket
{
// definitions)
public:
			using TMESSAGE_HEAD = int32_t;

	static sMESSAGE_HEADER		message_header_type;

// implementation)
protected:
	virtual	void				process_pre_message(sMESSAGE_NETWORK& /*_msg*/) {}
	virtual	std::size_t			process_packet(shared_buffer& _buffer, const net::sockaddr& _address_peer) override
	{
		// check) _buffer.data_가 0이어서는 안된다.
		THROW_IF(_buffer.data() == nullptr, throwable::failure(eRESULT::INVALID_MESSAGE_BUFFER_IS_NULL), LOG_DEBUG << "(err ) _buffer.data_ is nullptr " __CGFUNCTION__ ""sv);

		// check) _buffer->size_가 0이어서는 안된다.
		CGASSERT_ERROR(_buffer.size() != 0)

		// declare) 
		int count_message = 0; // 처리한 패킷 수의 처리르 위해...

		// declare) 임시 sMESSAGE_NETWORK (전달용)
		sMESSAGE_NETWORK msg;

		// 1) shared_buffer의 Address와 length를 써넣는다.
		shared_buffer message = _buffer ^ message_header_type.header_size;

		// loop) remained_size가 GetSizeOfPacketHead()보다 작으면 끝냄.
		std::size_t remained_size = _buffer.size();

		try
		{
			while(remained_size >= message_header_type.header_size)
			{
				// 2) message 길이를 구한다.
				auto message_size = message_header_type.get_required_size(message);

				// 3) set size;
				((buffer_view&)message).set_size(message_size);

				// check) 만약 tempNetBuffer.size_이 TMESSAGE_HEAD보다 작으면 잘못된 것이므로 Exception을 던진다.
				//        (주의: message크기가 0Byte일 경우 Exception을 던지지 않으면 무한루프에 빠져버릴수 있다.)
				THROW_IF(message_size < message_header_type.header_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_SHORT_LENGTH), LOG_DEBUG << "(err ) message size is too short. message size: "sv << message_size << " " __CGFUNCTION__ ""sv);

				// check) MessageHead크기 보다 작으면 빠져나간다.
				BREAK_IF(message_size > remained_size);

				// 3) 전달용 shared_buffer에 address와 length를 써넣는다.
				msg.buf_message = message;
				msg.address = _address_peer;

				// debug)
			#if !defined(NDEBUG)
				if (this->is_socket_debug_logging_enabled())
					net::io::_debug_log_socket_on_message(this->native_handle(), message);
			#endif

				// 4) process message
				{
					// - 훅함수 process_pre_message()를 호출한다.
					this->process_pre_message(msg);

					// - message를 Imessageable로 전달하여 처리한다.
					this->process_message(msg);

					// - History에 Queueing한다.
					this->push_message_to_history(message);
				}

				// 7) message수를 증가시킨다.
				++count_message;

				// 8) message Data를 이동시킨다.
				((buffer_view&)message).add_data(message_size);
				remained_size -= message_size;

				// check) 접속이 종료되었으면 바로 끝낸다.
				if (this->is_invalid_native_handle())
				{
					// - 주의) remained_size를 0으로 해주는 이유는 ReminedSize와 RequiredSize를 비교하여
					//         처리하는 부분이 있으므로 오류로 인한 처리의 중단에서는 Remained를 0으로 설정해 주어야 한다.
					//         즉 어차피 소켓이 비정상 종료되는 것이므로 모든 데이터들이 모두 처리된 것으로 설정할 필요가 있다!
					remained_size = 0;

					// break) 
					break;
				}
			}
		}
		catch(...)
		{
			// statistiscs) 이때까지 처리된 Packet의 통계처리를 한다.
			this->statistics_on_receive_message(count_message);

			// reraise) 
			throw;
		}

		// statistics) receive message수를 세린다.
		this->statistics_on_receive_message(count_message);

		// 9) 남은 message를 설정한다.
		_buffer = buffer_view(message.data(), remained_size);

		// return) 다음 message의 크기를 return한다.
		return static_cast<std::size_t>(message.size());
	}
	virtual	void				process_reset_packetable() noexcept override { this->reset_message_history();}
};


}