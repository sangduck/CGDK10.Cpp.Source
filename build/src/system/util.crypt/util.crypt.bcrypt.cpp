#include "pch.h"
#include "bcrypt/node_blf.h"


namespace CGDK
{

const char* error = ":";
void strcpy_error(char* _destination)
{
	*(uint16_t*)_destination = *(uint16_t*)error;
}
 
void encode_base64(uint8_t* _buffer, uint8_t* data, size_t _data_len)
{
	static const uint8_t base64_code[] = "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

	uint8_t* bp = _buffer;
	uint8_t* p = data;
	uint8_t c1, c2;
	while (p < data + _data_len)
	{
		c1 = *p++;
		*bp++ = base64_code[(c1 >> 2)];
		c1 = (c1 & 0x03) << 4;
		if (p >= data + _data_len)
		{
			*bp++ = base64_code[c1];
			break;
		}
		c2 = *p++;
		c1 |= (c2 >> 4) & 0x0f;
		*bp++ = base64_code[c1];
		c1 = (c2 & 0x0f) << 2;
		if (p >= data + _data_len)
		{
			*bp++ = base64_code[c1];
			break;
		}
		c2 = *p++;
		c1 |= (c2 >> 6) & 0x03;
		*bp++ = base64_code[c1];
		*bp++ = base64_code[c2 & 0x3f];
	}
	*bp = '\0';
}

void decode_base64(uint8_t* _buffer, size_t _buffer_len, const uint8_t* _data)
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

	#define CHAR64(c)  ( (c) > 127 ? 255 : index_64[(c)])

	uint8_t* bp = _buffer;
	const uint8_t* p = _data;
	uint8_t c1, c2, c3, c4;
	while (bp < _buffer + _buffer_len)
	{
		c1 = CHAR64(*p);
		c2 = CHAR64(*(p + 1));

		// Invalid data
		if (c1 == 255 || c2 == 255)
			break;

		*bp++ = (c1 << 2) | ((c2 & 0x30) >> 4);

		if (bp >= _buffer + _buffer_len)
			break;

		c3 = CHAR64(*(p + 2));

		if (c3 == 255)
			break;

		*bp++ = ((c2 & 0x0f) << 4) | ((c3 & 0x3c) >> 2);

		if (bp >= _buffer + _buffer_len)
			break;

		c4 = CHAR64(*(p + 3));

		if (c4 == 255)
			break;

		*bp++ = ((c3 & 0x03) << 6) | c4;

		p += 4;
	}

	#undef CHAR64
}

void encode_salt(char* _out_salt, uint8_t* _csalt, char _minor, size_t _csalt_len, int _rounds)
{
	_out_salt[0] = '$';
	_out_salt[1] = BCRYPT_VERSION;
	_out_salt[2] = _minor;
	_out_salt[3] = '$';

    // Max rounds are 31
#ifdef _WIN32
	_snprintf_s(_out_salt + 4, 4, 4, "%2.2u$", ((uint32_t)_rounds) & 0x001F);
#else
	snprintf(_out_salt + 4, 4, "%2.2u$", ((uint32_t)_rounds) & 0x001F);
#endif

	encode_base64((uint8_t*)_out_salt + 7, _csalt, _csalt_len);
}

void bcrypt_gensalt(char* _out_salt, char _minor, int _rounds, uint8_t* _seed) 
{
	if (_rounds < 4)
		_rounds = 4;
	else if (_rounds > 31)
		_rounds = 31;

	encode_salt(_out_salt, _seed, _minor, BCRYPT_MAXSALT, _rounds);
}

void node_bcrypt(char* _out_encrypted, std::string_view _key, std::string_view _salt)
{
	// - 
	const char* salt = _salt.data();

	// 1) discard "$" identifier
	salt++;

	// 2) check version - '2'
	if (*salt > BCRYPT_VERSION)
	{
		// how do i handle errors ? return ':'
		strcpy_error(_out_encrypted);
		return;
	}

	// 3) check for minor versions
	uint8_t minor;
	if (salt[1] != '$')
	{
		// - minor version
		 switch (salt[1])
		 {
		 case 'a': /* 'ab' should not yield the same as 'abab' */
		 case 'b': /* cap input length at 72 bytes */
				 minor = salt[1];
				 salt++;
				 break;
		 default:
				 strcpy_error(_out_encrypted);
				 return;
		 }
	}
	else
	{
		minor = 0;
	}

	// 4) discard version + "$" identifier
	salt += 2;

	if (salt[2] != '$')
	{
		// out of sync with passwd entry
		strcpy_error(_out_encrypted);
		return;
	}

	// 6) computer power doesn't increase linear, 2^x should be fine
	int n = atoi(salt);
	if (n > 31 || n < 0)
	{
		strcpy_error(_out_encrypted);
		return;
	}

	// 7) rounds
	uint32_t rounds;
	uint8_t logr = (uint8_t)n;
	if ((rounds = (uint32_t) 1 << logr) < BCRYPT_MINROUNDS)
	{
		strcpy_error(_out_encrypted);
		return;
	}

	// 8) discard num rounds + "$" identifier
	salt += 3;

	if (strlen(salt) * 3 / 4 < BCRYPT_MAXSALT)
	{
		strcpy_error(_out_encrypted);
		return;
	}

	// we dont want the base64 salt but the raw data
	uint8_t csalt[BCRYPT_MAXSALT];
	decode_base64(csalt, BCRYPT_MAXSALT, (const uint8_t*)salt);
	uint8_t salt_len = BCRYPT_MAXSALT;

	// declare)
	const uint8_t* key_data = reinterpret_cast<const uint8_t*>(_key.data());
	auto key_len = _key.size();

	if (minor <= 'a')
	{
		key_len = key_len + (minor >= 'a' ? 1 : 0);
	}
	else
	{
		// cap key_len at the actual maximum supported length here to avoid integer wraparound
		if (key_len > 72)
			key_len =72;

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

	{
		uint32_t i = 0;
		_out_encrypted[i++] = '$';
		_out_encrypted[i++] = BCRYPT_VERSION;
		if (minor)
			_out_encrypted[i++] = minor;
		_out_encrypted[i++] = '$';

	#ifdef _WIN32
		_snprintf_s(_out_encrypted + i, 4, 4, "%2.2u$", logr & 0x001F);
	#else
		snprintf(_out_encrypted + i, 4, "%2.2u$", logr & 0x001F);
	#endif
		encode_base64((uint8_t*)_out_encrypted + i + 3, csalt, BCRYPT_MAXSALT);
		encode_base64((uint8_t*)_out_encrypted + strlen(_out_encrypted), ciphertext, 4 * BCRYPT_BLOCKS - 1);
		memset(&state, 0, sizeof(state));
		memset(ciphertext, 0, sizeof(ciphertext));
		memset(csalt, 0, sizeof(csalt));
		memset(cdata, 0, sizeof(cdata));
	}
}

std::string crypt::bcrypt::hash_password(std::string_view _password, std::string_view _salt)
{
	std::string hashed_password(61, '\0');
	node_bcrypt(&hashed_password[0], _password, _salt);
	hashed_password.resize(60);
	return hashed_password;
}

void arc4random_buf(void* buf, size_t nbytes)
{
	for (size_t n = 0; n < nbytes; ++n)
	{
		((char*)(buf))[n] = rand() % 256;
	}
}

std::string crypt::bcrypt::generate_hash(std::string_view _password, int _rounds, char _minor)
{
	// 1) 
	unsigned char seed[17]{};
	arc4random_buf(seed, 16);

	// declare) 
	char salt[_SALT_LEN];

	bcrypt_gensalt(salt, _minor, _rounds, seed);

	std::string hashed_password(61, '\0');
	node_bcrypt(&hashed_password[0], _password, salt);
	hashed_password.resize(60);
	return hashed_password;
}

bool crypt::bcrypt::check_password(std::string_view _password, std::string_view _hashed_password)
{
	std::string got(61, '\0');
	node_bcrypt(&got[0], _password, _hashed_password);
	got.resize(60);
	return _hashed_password == got;
}


}