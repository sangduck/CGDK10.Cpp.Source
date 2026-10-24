#pragma once

#include <sys/types.h>
//-----------------------------------------------------------------------------
// blowfish context algorithm
//-----------------------------------------------------------------------------
#define BLOWFISH_SUBKEY_COUNT	16	// number of subkeys

struct blowfish_context
{
	uint32_t S[4][256];						// S-boxes
	uint32_t P[BLOWFISH_SUBKEY_COUNT + 2];	// subkeys
};

void blowfish_encipher(blowfish_context*, uint32_t*, uint32_t*);
void blowfish_init_state(blowfish_context*);
void blowfish_expand_0state(blowfish_context*, const uint8_t*, size_t);
void blowfish_expand_state(blowfish_context*, const uint8_t*, size_t, const uint8_t*, size_t);

void blowfish_key(blowfish_context*, const uint8_t*, size_t);
void blowfish_enc(blowfish_context*, uint32_t*, size_t);

uint32_t blowfish_stream_to_word(const uint8_t*, size_t, uint16_t*);


//-----------------------------------------------------------------------------
// bcrypt functions
//-----------------------------------------------------------------------------

#define BCRYPT_VERSION		'2'
#define BCRYPT_MAXSALT		16	// Precomputation is just so nice
#define BCRYPT_BLOCKS		6	// Ciphertext blocks
#define BCRYPT_MINROUNDS	16	// we have log2(rounds) in salt

#define _SALT_LEN			32	// max length

// bcrypt functions
void bcrypt_gensalt(char*, char, int, uint8_t*);
void node_bcrypt(char*, const std::string_view, size_t, const std::string_view);
void encode_salt(char*, uint8_t*, char, size_t, uint8_t);
