//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Utinity.Crypto Classes                           *
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
// CGDK::crypt::rsa
//
//
//-----------------------------------------------------------------------------
namespace crypt
{
	// definitions)
	struct aes_key
	{
		buffer_view	key;
		buffer_view iv;
	};

	namespace aes
	{
		// 1. functions (AES-256-GCM, key: 32 bytes)
		//    - _tag가 있으면 16 byte tag를 _tag에 쓰고 _tag로 검증한다.
		//    - _tag가 없는 함수는 tag를 암호문 뒤에 붙이고(+16 byte) 복호화할 때 검증한다.
		//    - 검증에 실패하면(변조) exception을 던진다.
		buffer_view					encrypt(buffer_view _out, buffer_view _tag, aes_key _key, const_buffer_view _source, const_buffer_view _source_add);
		inline	buffer_view			encrypt(buffer_view _out, aes_key _key, const_buffer_view _source, const_buffer_view _source_add = const_buffer_view()) { return encrypt(_out, buffer_view(), _key, _source, _source_add); }
		buffer_view					decrypt(buffer_view _out, buffer_view _tag, aes_key _key, const_buffer_view _source, const_buffer_view _source_add = const_buffer_view());
		inline	buffer_view			decrypt(buffer_view _out, aes_key _key, const_buffer_view _source, const_buffer_view _source_add = const_buffer_view()) { return decrypt(_out, buffer_view(), _key, _source, _source_add); }

		// 2. cryptor (AES-256-GCM)
		//    - initialize(key)로 32 byte key를 설정한다. (make_cryptor)
		//    - 암호화할 때마다 무작위 IV(12 byte)를 만든다. (같은 key로 IV를 재사용하지 않는다.)
		//    - 형식: [IV 12 byte][암호문][tag 16 byte] (복호화할 때 tag로 변조를 검증한다.)
		//    - key는 초기화 후 바뀌지 않으므로 여러 thread에서 동시에 사용할 수 있다.
		class cryptor : virtual public Icryptor
		{
		public:
			static constexpr std::size_t KEY_SIZE = 32;
			static constexpr std::size_t IV_SIZE = 12;
			static constexpr std::size_t TAG_SIZE = 16;

			virtual	~cryptor() noexcept;

			using Icryptor::initialize;
			virtual void				initialize(buffer_view _key) override;
			[[nodiscard]] virtual shared_buffer	encrypt(const_buffer_view _source) override;
			[[nodiscard]] virtual shared_buffer	decrypt(const_buffer_view _source) override;

		private:
			std::array<uint8_t, KEY_SIZE> m_key{};
			bool						m_is_initialized{ false };
		};

		[[nodiscard]] object_ptr<Icryptor>	make_cryptor(const_buffer_view _key);
		// - 무작위 32 byte key를 만든다.
		[[nodiscard]] shared_buffer		generate_key();
	}
}

}
