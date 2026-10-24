#include "pch.h"
#include "bcrypt/node_blf.h"
#include <openssl/rand.h>
#include <openssl/crypto.h>


namespace CGDK
{

// internal helpers (not exported)
namespace
{

constexpr size_t BCRYPT_HASH_LEN = 60;	// "$2b$10$" (7) + salt (22) + hash (31)

void encode_base64(uint8_t* _buffer, const uint8_t* data, size_t _data_len)
{
	static const uint8_t base64_code[] = "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

	const uint8_t* p = data;
	uint8_t c1, c2;
	while (p < data + _data_len)
	{
		c1 = *p++;
		*_buffer++ = base64_code[(c1 >> 2)];
		c1 = (c1 & 0x03) << 4;
		if (p >= data + _data_len)
		{
			*_buffer++ = base64_code[c1];
			break;
		}
		c2 = *p++;
		c1 |= (c2 >> 4) & 0x0f;
		*_buffer++ = base64_code[c1];
		c1 = (c2 & 0x0f) << 2;
		if (p >= data + _data_len)
		{
			*_buffer++ = base64_code[c1];
			break;
		}
		c2 = *p++;
		c1 |= (c2 >> 6) & 0x03;
		*_buffer++ = base64_code[c1];
		*_buffer++ = base64_code[c2 & 0x3f];
	}
	*_buffer = '\0';
}

bool decode_base64(uint8_t* _buffer, size_t _buffer_len, const uint8_t* _data)
{
	static const uint8_t index_64[128] = {
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 0, 1, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63, 255, 255,
		255, 255, 255, 255, 255, 2, 3, 4, 5, 6,
		7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
		255, 255, 255, 255, 255, 255, 28, 29, 30,
		31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
		51, 52, 53, 255, 255, 255, 255, 255
	};

	auto char64 = [](uint8_t c) -> uint8_t { return (c > 127) ? 255 : index_64[c]; };

	uint8_t* bp = _buffer;
	uint8_t c1, c2, c3, c4;
	while (bp < _buffer + _buffer_len)
	{
		c1 = char64(*_data);
		if (c1 == 255)
			return false;
		c2 = char64(*(_data + 1));
		if (c2 == 255)
			return false;

		*bp++ = static_cast<uint8_t>((c1 << 2) | ((c2 & 0x30) >> 4));

		if (bp >= _buffer + _buffer_len)
			break;

		c3 = char64(*(_data + 2));
		if (c3 == 255)
			return false;

		*bp++ = static_cast<uint8_t>(((c2 & 0x0f) << 4) | ((c3 & 0x3c) >> 2));

		if (bp >= _buffer + _buffer_len)
			break;

		c4 = char64(*(_data + 3));
		if (c4 == 255)
			return false;

		*bp++ = static_cast<uint8_t>(((c3 & 0x03) << 6) | c4);

		_data += 4;
	}

	return true;
}

bool is_supported_minor(char _minor) noexcept
{
	// 'y' (php/crypt_blowfish) is identical to 'b'
	return _minor == 'a' || _minor == 'b' || _minor == 'y';
}

void encode_salt(char* _out_salt, const uint8_t* _csalt, char _minor, size_t _csalt_len, int _rounds)
{
	_out_salt[0] = '$';
	_out_salt[1] = BCRYPT_VERSION;
	_out_salt[2] = _minor;
	_out_salt[3] = '$';

	// Max rounds are 31
#ifdef _WIN32
	_snprintf_s(_out_salt + 4, 5, 4, "%2.2u$", ((uint32_t)_rounds) & 0x001F);
#else
	snprintf(_out_salt + 4, 5, "%2.2u$", ((uint32_t)_rounds) & 0x001F);
#endif

	encode_base64((uint8_t*)_out_salt + 7, _csalt, _csalt_len);
}

// returns the hashed string ("$2b$..", 60 chars) or empty string on invalid setting(salt)
std::string node_bcrypt(std::string_view _key, std::string_view _setting)
{
	// - '_key'/'_setting' are string_view (may not be NUL terminated) - copy them.
	//   (the key must be followed by NUL. minor 'a'/'b' hash the terminating NUL too.)
	std::string key_buffer(_key);
	const std::string setting(_setting.substr(0, std::min<size_t>(_setting.size(), 64)));

	// - wipe the key copy on every return path
	struct cleanse_on_exit
	{
		std::string& buffer;
		~cleanse_on_exit() { if (!buffer.empty()) OPENSSL_cleanse(buffer.data(), buffer.size()); }
	} cleanse_key{ key_buffer };

	const char* salt = setting.c_str();

	// 1) "$" identifier
	if (salt[0] != '$')
		return {};
	++salt;

	// 2) check version - '2'
	if (salt[0] != BCRYPT_VERSION)
		return {};

	// 3) check for minor versions
	char minor;
	if (salt[1] != '$')
	{
		if (!is_supported_minor(salt[1]))
			return {};

		minor = salt[1];
		++salt;
	}
	else
	{
		minor = 0;
	}

	// 4) discard version + "$" identifier
	if (salt[1] != '$')
		return {};
	salt += 2;

	// 5) rounds - exactly 2 digits + "$"
	if (salt[0] < '0' || salt[0] > '9' || salt[1] < '0' || salt[1] > '9' || salt[2] != '$')
		return {};

	const int n = (salt[0] - '0') * 10 + (salt[1] - '0');
	if (n > 31)
		return {};

	// 6) rounds
	const uint8_t logr = static_cast<uint8_t>(n);
	const uint32_t rounds = static_cast<uint32_t>(1) << logr;
	if (rounds < BCRYPT_MINROUNDS)
		return {};

	// 7) discard num rounds + "$" identifier
	salt += 3;

	// check) need 22 base64 chars for 16 bytes salt
	if (strlen(salt) < 22)
		return {};

	// we dont want the base64 salt but the raw data
	uint8_t csalt[BCRYPT_MAXSALT];
	if (!decode_base64(csalt, BCRYPT_MAXSALT, reinterpret_cast<const uint8_t*>(salt)))
		return {};
	const uint8_t salt_len = BCRYPT_MAXSALT;

	// declare)
	const uint8_t* key_data = reinterpret_cast<const uint8_t*>(key_buffer.c_str());
	size_t key_len = key_buffer.size();

	if (minor == 0 || minor == 'a')
	{
		// - 'a' includes the NUL ('ab' should not yield the same as 'abab')
		key_len = key_len + ((minor == 'a') ? 1 : 0);

		// - original implementation stores key_len in uint8_t (wraparound)
		key_len = static_cast<uint8_t>(key_len);
	}
	else
	{
		// cap key_len at the actual maximum supported length here to avoid integer wraparound
		if (key_len > 72)
			key_len = 72;

		key_len++; // include the NUL
	}

	// setting up S-Boxes and Subkeys
	blowfish_context state;
	blowfish_init_state(&state);
	blowfish_expand_state(&state, csalt, salt_len, key_data, key_len);

	for (uint32_t k = 0; k < rounds; ++k)
	{
		blowfish_expand_0state(&state, key_data, key_len);
		blowfish_expand_0state(&state, csalt, salt_len);
	}

	// this can be precomputed later
	uint8_t ciphertext[4 * BCRYPT_BLOCKS + 1] = "OrpheanBeholderScryDoubt";

	uint32_t cdata[BCRYPT_BLOCKS];
	uint16_t j = 0;
	for (uint32_t i = 0; i < BCRYPT_BLOCKS; ++i)
	{
		cdata[i] = blowfish_stream_to_word(ciphertext, 4 * BCRYPT_BLOCKS, &j);
	}

	// now do the encryption
	for (uint32_t k = 0; k < 64; ++k)
	{
		blowfish_enc(&state, cdata, BCRYPT_BLOCKS / 2);
	}

	for (uint32_t i = 0; i < BCRYPT_BLOCKS; ++i)
	{
		ciphertext[4 * i + 3] = cdata[i] & 0xff;
		cdata[i] = cdata[i] >> 8;
		ciphertext[4 * i + 2] = cdata[i] & 0xff;
		cdata[i] = cdata[i] >> 8;
		ciphertext[4 * i + 1] = cdata[i] & 0xff;
		cdata[i] = cdata[i] >> 8;
		ciphertext[4 * i + 0] = cdata[i] & 0xff;
	}

	char encrypted[BCRYPT_HASH_LEN + 8]{};
	{
		uint32_t i = 0;
		encrypted[i++] = '$';
		encrypted[i++] = BCRYPT_VERSION;
		if (minor)
			encrypted[i++] = minor;
		encrypted[i++] = '$';

	#ifdef _WIN32
		_snprintf_s(encrypted + i, 5, 4, "%2.2u$", logr & 0x001F);
	#else
		snprintf(encrypted + i, 5, "%2.2u$", logr & 0x001F);
	#endif
		encode_base64((uint8_t*)encrypted + i + 3, csalt, BCRYPT_MAXSALT);
		encode_base64((uint8_t*)encrypted + strlen(encrypted), ciphertext, 4 * BCRYPT_BLOCKS - 1);
	}

	std::string result(encrypted);

	OPENSSL_cleanse(&state, sizeof(state));
	OPENSSL_cleanse(ciphertext, sizeof(ciphertext));
	OPENSSL_cleanse(csalt, sizeof(csalt));
	OPENSSL_cleanse(cdata, sizeof(cdata));
	OPENSSL_cleanse(encrypted, sizeof(encrypted));

	return result;
}

}

std::string crypt::bcrypt::hash_password(std::string_view _password, std::string_view _salt)
{
	auto hashed_password = node_bcrypt(_password, _salt);

	// check)
	THROW_IF(hashed_password.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("bcrypt: invalid salt")));

	return hashed_password;
}

std::string crypt::bcrypt::generate_hash(std::string_view _password, int _rounds, char _minor)
{
	// check)
	THROW_IF(!is_supported_minor(_minor), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("bcrypt: unsupported minor version")));

	// - rounds (log2) 4 ~ 31
	if (_rounds < 4)
		_rounds = 4;
	else if (_rounds > 31)
		_rounds = 31;

	// 1) random salt (CSPRNG)
	uint8_t seed[BCRYPT_MAXSALT]{};
	THROW_IF(RAND_bytes(seed, static_cast<int>(sizeof(seed))) != 1, throwable::failure(eRESULT::FAIL, 0, std::string_view("bcrypt: RAND_bytes failed")));

	// 2) encode salt
	char salt[_SALT_LEN]{};
	encode_salt(salt, seed, _minor, BCRYPT_MAXSALT, _rounds);
	OPENSSL_cleanse(seed, sizeof(seed));

	// 3) hash
	return hash_password(_password, salt);
}

bool crypt::bcrypt::check_password(std::string_view _password, std::string_view _hashed_password)
{
	// check) "$2a$"/"$2b$"/"$2y$" = 60, "$2$" = 59
	if (_hashed_password.size() != BCRYPT_HASH_LEN && _hashed_password.size() != BCRYPT_HASH_LEN - 1)
		return false;

	// 1) hash with same setting
	const auto got = node_bcrypt(_password, _hashed_password);

	// check)
	if (got.size() != _hashed_password.size())
		return false;

	// 2) constant time compare
	return CRYPTO_memcmp(got.data(), _hashed_password.data(), got.size()) == 0;
}


}
