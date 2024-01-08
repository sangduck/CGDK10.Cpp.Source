#include "pch.h"

//#if defined(_WIN32)
namespace CGDK
{

#define	SIZE_OF_MD5_FILEBUFFER	65536
#define MD5_ROTATE_LEFT(x,n)	(((x) << (n)) | ((x) >> (32 - (n)))) 


edc::md5::md5() noexcept :
	m_buffer{},
	m_is_process(false),
	m_state(),
	m_count{0,0}
{
}

edc::md5::~md5() noexcept
{
}

void edc::md5::push_md5(const void* _buffer, uint32_t _size)
{ 
	// check) 
	RETURN_IF(m_is_process != true);

	// check) 
	RETURN_IF(_size ==0);


	//-----------------------------------------------------------------
	// 1. 전체 길이를 구해준다.(Bit단위로...)
	//-----------------------------------------------------------------
	// declare) 
	uint32_t	s_index; 
	uint32_t	s_part_size; 

	// 1) 남은 길이를 구한다.(512Bit인지확인)
	s_index		 = (m_count[0] >> 3) & 0x3f; 

	// 2) Bit로 크기를 구하기 위해 8을 곱한다.
	// - Bit로 변환하기 위해 8을 곱해서 길이를 더한다.
	m_count[0]	+= (_size << 3);

	// - 자리올림처리...(Overflow났을 경우 1을 더해줌, 값이 더 작아졌다는 것은 Overflow를 의미함.)
	if(m_count[0] < (_size << 3))
	{
		m_count[1]++; 
	}

	// - 상위 3Bit를 더함.
	m_count[1]	+= _size >> 29;

	// 3) 무자란 길이를 구한다.
	s_part_size	 = 64 - s_index; 

	// check) 512Bit단위로만 동작하게 함...(512가 안될경우 값만 복사하고 끝낸다.)
	if(_size < s_part_size)
	{
		// 값을 복사한다.
		memcpy(m_buffer+s_index, _buffer, _size);
		return;
	}


	//-----------------------------------------------------------------
	// 2. 이미 존재하던 데이터어 붙혀진 데이터 MD5처리
	//-----------------------------------------------------------------
	// - m_buffer에 이전에 512Bit가 되지 않아서 처리되지 않고 그냥 복사
	//   만 해놓았던 데이터에 이어 붙혀 512Bit를 만든후 MD5로 계산한다.

	// 1) m_buffer의 뒷부분에 데이터를 붙힌다.
	memcpy(m_buffer + s_index, _buffer, s_part_size);

	// 2) 붙힌 Data의 MD5계산을 한다.
	transform_md5(reinterpret_cast<void*>(&m_state), reinterpret_cast<void*>(m_buffer));


	//-----------------------------------------------------------------
	// 3. 가운데부분 MD5처리.
	//-----------------------------------------------------------------
	// - 이어붙힌 이후로 MD5는 512Bit단위로 MD5를 구하면 된다.

	// declare)
	uint32_t i;

	// 1) 그 이후부터 16 uint32_t식 Block으로 MD5값을 구한다.
	for(i = s_part_size;(i + static_cast<uint32_t>(63)) < _size; i += 64)
	{ 
		transform_md5(reinterpret_cast<void*>(&m_state), reinterpret_cast<const void*>(reinterpret_cast<const uint8_t*>(_buffer) + i));
	} 

	//-----------------------------------------------------------------
	// 4. 남은 Byte처리
	//-----------------------------------------------------------------
	// - 512Bit가 되지 않은 나머지는 다음을 위해 복사해둔다.

	// 1) 남은 Byte수를 구한다.
	uint32_t	remainSize	 = _size - i;

	// 1) 512Bit의 배수가 되지 않고 남은 값들은 복사해 놓는다.
	memcpy(m_buffer, reinterpret_cast<const void*>(reinterpret_cast<const uint8_t*>(_buffer) + i), remainSize);
} 

bool edc::md5::push_md5(FILE_HANDLE _handle_file, uint32_t _Start, uint32_t _End)
{
	// check) 
	RETURN_IF(m_is_process!=true, false);

	// check) File이 제대로 열였는가?
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE, false);

	// check) File이 제대로 열였는가?
	RETURN_IF(_Start == _End, false);

	// 2) 임시 File shared_buffer를 만든다.
#if defined(_WIN32)
	uint8_t* shared_buffer = reinterpret_cast<uint8_t*>(MALLOC(SIZE_OF_MD5_FILEBUFFER CGNEW_DEBUG_INFO_COMMA CGNEW_DEBUG_INFO_PARAMETERS_MALLOC));
#else
	uint8_t* shared_buffer = reinterpret_cast<uint8_t*>(MALLOC(SIZE_OF_MD5_FILEBUFFER));
#endif

	// check) 메모리가 제대로 할당되었는지 확인한다.
	RETURN_IF(shared_buffer ==nullptr, false);

	// 3) MD5 값 만들기
	for(uint32_t position= _Start; position<_End; position+=SIZE_OF_MD5_FILEBUFFER)
	{ 
		// - file seek
		api::file::seek(_handle_file, position, eFILE_ORIGIN::BEGIN);

		// - 읽을 크기
		auto dwToRead = _End-position;

		SATURATE_MAX(dwToRead, SIZE_OF_MD5_FILEBUFFER);

		// - File에서 읽어들인다.
		auto dwReaded = api::file::read(_handle_file, dwToRead, shared_buffer);

		// Assert) 읽어야 하는 Byte와 읽은 Byte수가 같은가?
		CGASSERT_ERROR(dwToRead==dwReaded);

		// check) 읽어들인 값이 0이면 끝낸다.
		BREAK_IF(dwReaded==0);

		// - MD5값을 계산한다.
		push_md5(shared_buffer, static_cast<uint32_t>(dwReaded));
	}

	// 5) 임시 File shared_buffer를 free한다.
	SAFE_FREE(shared_buffer);

	// return) 성공
	return true;
}

void MD5_F(uint32_t& a, const uint32_t b, const uint32_t c, const uint32_t d, const uint32_t x, const uint32_t t, const int s)
{
	a += ((b & c) | ((~b) & d)) + x + t;
	a  = MD5_ROTATE_LEFT(a,s);
	a += b;
}
void MD5_G(uint32_t& a, const uint32_t b, const uint32_t c, const uint32_t d, const uint32_t x, const uint32_t t, const int s)
{
	a += ((b & d) | (c & (~d))) + x + t;
	a  = MD5_ROTATE_LEFT(a,s);
	a += b;
}

void MD5_H(uint32_t& a, const uint32_t b, const uint32_t c, const uint32_t d, const uint32_t x, const uint32_t t, const int s)
{
	a += (b ^ c ^ d) + x + t;
	a  = MD5_ROTATE_LEFT(a,s);
	a += b;
}

void MD5_I(uint32_t& a, const uint32_t b, const uint32_t c, const uint32_t d, const uint32_t x, const uint32_t t, const int s)
{
	a += (c ^ (b | (~d))) + x + t;
	a  = MD5_ROTATE_LEFT(a,s);
	a += b;
}

// 16개의 uint32_t Block의 MD5값을 구한다.
void edc::md5::transform_md5(void* _buffer, const void* _psource)
{ 
	//-----------------------------------------------------------------
	// 1. A,B,C,D값을 읽어온다.
	//-----------------------------------------------------------------
	// 1) 값을 읽어옴.
	uint32_t a = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(_buffer) + sizeof(uint32_t) * 0);
	uint32_t b = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(_buffer) + sizeof(uint32_t) * 1); 
	uint32_t c = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(_buffer) + sizeof(uint32_t) * 2); 
	uint32_t d = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(_buffer) + sizeof(uint32_t) * 3); 


	//-----------------------------------------------------------------
	// 2. decode한다.
	//-----------------------------------------------------------------
	// declare) 
	uint32_t	s_from32[16]; 

	// 2) decode한다.
	memcpy(s_from32, _psource, sizeof(uint32_t)*16);

	// 4) 0~15 변환
	//    - TEMP = A+F(B,C,D)+X[i]+T[t]	(i=t)
	//    - B=Sn(TEMP2)+B; (Sn은 7,12,17,22 순서대로)
	#define MD5_s11		7 
	#define MD5_s12		12 
	#define MD5_s13		17 
	#define MD5_s14		22 

	MD5_F(a, b, c, d, s_from32[ 0], 0xd76aa478, MD5_s11);
	MD5_F(d, a, b, c, s_from32[ 1], 0xe8c7b756, MD5_s12);
	MD5_F(c, d, a, b, s_from32[ 2], 0x242070db, MD5_s13);
	MD5_F(b, c, d, a, s_from32[ 3], 0xc1bdceee, MD5_s14);
	MD5_F(a, b, c, d, s_from32[ 4], 0xf57c0faf, MD5_s11);
	MD5_F(d, a, b, c, s_from32[ 5], 0x4787c62a, MD5_s12);
	MD5_F(c, d, a, b, s_from32[ 6], 0xa8304613, MD5_s13);
	MD5_F(b, c, d, a, s_from32[ 7], 0xfd469501, MD5_s14);
	MD5_F(a, b, c, d, s_from32[ 8], 0x698098d8, MD5_s11);
	MD5_F(d, a, b, c, s_from32[ 9], 0x8b44f7af, MD5_s12);
	MD5_F(b, c, d, a, s_from32[11], 0x895cd7be, MD5_s14);
	MD5_F(a, b, c, d, s_from32[12], 0x6b901122, MD5_s11);
	MD5_F(d, a, b, c, s_from32[13], 0xfd987193, MD5_s12);
	MD5_F(c, d, a, b, s_from32[14], 0xa679438e, MD5_s13);
	MD5_F(b, c, d, a, s_from32[15], 0x49b40821, MD5_s14);

	// 5) 16~31 변환
	//    - TEMP = A+G(B,C,D)+X[i]+T[t]	(i=(1+5t) mod 16)
	//    - TEMP2 = C;
	//    - A=D;
	//    - C=B;
	//    - B=Sn(TEMP2)+B; (Sn은 5,9,14,20 순서대로)
	//    - D=C;
	#define MD5_s21		5
	#define MD5_s22		9
	#define MD5_s23		14 
	#define MD5_s24		20 

	MD5_G(a, b, c, d, s_from32[ 1], MD5_s21, 0xf61e2562); /* 17 */ 
	MD5_G(d, a, b, c, s_from32[ 6], MD5_s22, 0xc040b340); /* 18 */ 
	MD5_G(c, d, a, b, s_from32[11], MD5_s23, 0x265e5a51); /* 19 */ 
	MD5_G(b, c, d, a, s_from32[ 0], MD5_s24, 0xe9b6c7aa); /* 20 */ 
	MD5_G(a, b, c, d, s_from32[ 5], MD5_s21, 0xd62f105d); /* 21 */ 
	MD5_G(d, a, b, c, s_from32[10], MD5_s22, 0x02441453); /* 22 */ 
	MD5_G(c, d, a, b, s_from32[15], MD5_s23, 0xd8a1e681); /* 23 */ 
	MD5_G(b, c, d, a, s_from32[ 4], MD5_s24, 0xe7d3fbc8); /* 24 */ 
	MD5_G(a, b, c, d, s_from32[ 9], MD5_s21, 0x21e1cde6); /* 25 */ 
	MD5_G(d, a, b, c, s_from32[14], MD5_s22, 0xc33707d6); /* 26 */ 
	MD5_G(c, d, a, b, s_from32[ 3], MD5_s23, 0xf4d50d87); /* 27 */ 
	MD5_G(b, c, d, a, s_from32[ 8], MD5_s24, 0x455a14ed); /* 28 */ 
	MD5_G(a, b, c, d, s_from32[13], MD5_s21, 0xa9e3e905); /* 29 */ 
	MD5_G(d, a, b, c, s_from32[ 2], MD5_s22, 0xfcefa3f8); /* 30 */ 
	MD5_G(c, d, a, b, s_from32[ 7], MD5_s23, 0x676f02d9); /* 31 */ 
	MD5_G(b, c, d, a, s_from32[12], MD5_s24, 0x8d2a4c8a); /* 32 */ 

	// 6) 32~47 변환
	//    - TEMP = A+H(B,C,D)+X[i]+T[t]	(i=(5+3t) mod 16)
	//    - TEMP2 = C;
	//    - A=D;
	//    - C=B;
	//    - B=Sn(TEMP2)+B; (Sn은 4,11,16,23 순서대로)
	//    - D=C;
	#define MD5_s31		4 
	#define MD5_s32		11 
	#define MD5_s33		16 
	#define MD5_s34		23 

	MD5_H(a, b, c, d, s_from32[ 5], MD5_s31, 0xfffa3942); /* 33 */ 
	MD5_H(d, a, b, c, s_from32[ 8], MD5_s32, 0x8771f681); /* 34 */ 
	MD5_H(c, d, a, b, s_from32[11], MD5_s33, 0x6d9d6122); /* 35 */ 
	MD5_H(b, c, d, a, s_from32[14], MD5_s34, 0xfde5380c); /* 36 */ 
	MD5_H(a, b, c, d, s_from32[ 1], MD5_s31, 0xa4beea44); /* 37 */ 
	MD5_H(d, a, b, c, s_from32[ 4], MD5_s32, 0x4bdecfa9); /* 38 */ 
	MD5_H(c, d, a, b, s_from32[ 7], MD5_s33, 0xf6bb4b60); /* 39 */ 
	MD5_H(b, c, d, a, s_from32[10], MD5_s34, 0xbebfbc70); /* 40 */ 
	MD5_H(a, b, c, d, s_from32[13], MD5_s31, 0x289b7ec6); /* 41 */ 
	MD5_H(d, a, b, c, s_from32[ 0], MD5_s32, 0xeaa127fa); /* 42 */ 
	MD5_H(c, d, a, b, s_from32[ 3], MD5_s33, 0xd4ef3085); /* 43 */ 
	MD5_H(b, c, d, a, s_from32[ 6], MD5_s34, 0x04881d05); /* 44 */ 
	MD5_H(a, b, c, d, s_from32[ 9], MD5_s31, 0xd9d4d039); /* 45 */ 
	MD5_H(d, a, b, c, s_from32[12], MD5_s32, 0xe6db99e5); /* 46 */ 
	MD5_H(c, d, a, b, s_from32[15], MD5_s33, 0x1fa27cf8); /* 47 */ 
	MD5_H(b, c, d, a, s_from32[ 2], MD5_s34, 0xc4ac5665); /* 48 */ 

	// 6) 32~47 변환
	//    - TEMP = A+I(B,C,D)+X[i]+T[t]	(i=(7t) mod 16)
	//    - TEMP2 = C;
	//    - A=D;
	//    - C=B;
	//    - B=Sn(TEMP2)+B; (Sn은 6,10,15,21 순서대로)
	//    - D=C;
	#define MD5_s41		6 
	#define MD5_s42		10
	#define MD5_s43		15 
	#define MD5_s44		21 

	MD5_I(a, b, c, d, s_from32[ 0], MD5_s41, 0xf4292244); /* 49 */ 
	MD5_I(d, a, b, c, s_from32[ 7], MD5_s42, 0x432aff97); /* 50 */ 
	MD5_I(c, d, a, b, s_from32[14], MD5_s43, 0xab9423a7); /* 51 */ 
	MD5_I(b, c, d, a, s_from32[ 5], MD5_s44, 0xfc93a039); /* 52 */ 
	MD5_I(a, b, c, d, s_from32[12], MD5_s41, 0x655b59c3); /* 53 */ 
	MD5_I(d, a, b, c, s_from32[ 3], MD5_s42, 0x8f0ccc92); /* 54 */ 
	MD5_I(c, d, a, b, s_from32[10], MD5_s43, 0xffeff47d); /* 55 */ 
	MD5_I(b, c, d, a, s_from32[ 1], MD5_s44, 0x85845dd1); /* 56 */ 
	MD5_I(a, b, c, d, s_from32[ 8], MD5_s41, 0x6fa87e4f); /* 57 */ 
	MD5_I(d, a, b, c, s_from32[15], MD5_s42, 0xfe2ce6e0); /* 58 */ 
	MD5_I(c, d, a, b, s_from32[ 6], MD5_s43, 0xa3014314); /* 59 */ 
	MD5_I(b, c, d, a, s_from32[13], MD5_s44, 0x4e0811a1); /* 60 */ 
	MD5_I(a, b, c, d, s_from32[ 4], MD5_s41, 0xf7537e82); /* 61 */ 
	MD5_I(d, a, b, c, s_from32[11], MD5_s42, 0xbd3af235); /* 62 */ 
	MD5_I(c, d, a, b, s_from32[ 2], MD5_s43, 0x2ad7d2bb); /* 63 */ 
	MD5_I(b, c, d, a, s_from32[ 9], MD5_s44, 0xeb86d391); /* 64 */ 


	//-----------------------------------------------------------------
	// 3. A,B,C,D값을 써넣는다.
	//-----------------------------------------------------------------
	*reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(_buffer) + sizeof(uint32_t) * 0)	+= a; 
	*reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(_buffer) + sizeof(uint32_t) * 1)	+= b; 
	*reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(_buffer) + sizeof(uint32_t) * 2)	+= c; 
	*reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(_buffer) + sizeof(uint32_t) * 3)	+= d; 
} 

bool edc::md5::initialize()
{ 
	// 1) MD5 초기화를 한다.
	m_state.data[0]	 = 0x67452301; 
	m_state.data[1]	 = 0xefcdab89; 
	m_state.data[2]	 = 0x98badcfe; 
	m_state.data[3]	 = 0x10325476; 

	m_count[0]			 = 0; 
	m_count[1]			 = 0; 

	m_is_process		 = true;

	// return) 
	return true;
}

void edc::md5::padding()
{ 
	// check) 
	RETURN_IF(m_is_process!=true);

	// 1) padding 초기화
	uint8_t s_padding[64]; 
	__zero_memory(reinterpret_cast<void*>(&s_padding), sizeof(s_padding));
	s_padding[0] = 0x80;

	// 2) 
	uint32_t s_index = (m_count[0] >> 3) & 0x3f;		// 64Byte의 나머지 구함.
	uint32_t s_padding_size = (s_index < 56) ? (56 - s_index) : (120 - s_index);
	push_md5(reinterpret_cast<void*>(s_padding), s_padding_size); 

	// 3) 
	uint8_t s_bits[8]; 
	memcpy(s_bits, m_count, sizeof(s_bits));
	push_md5(reinterpret_cast<void*>(s_bits), sizeof(s_bits)); 

	// 3) 
	m_is_process = false;
}

bool edc::md5::calculate_md5(const std::string_view _filename)
{ 
	// 1) MD5를 초기화한다.
	initialize();

	// 2) File open한다.
	auto hFile = api::file::open(_filename, eFILE_OPERATION::READ);

	// check) hFile이 nullptr이면 끝낸다.
	RETURN_IF(hFile == INVALID_FILE_HANDLE, false);

	// 3) 임시 File shared_buffer를 만든다.
#if defined(_WIN32)
	auto shared_buffer = reinterpret_cast<uint8_t*>(MALLOC(SIZE_OF_MD5_FILEBUFFER CGNEW_DEBUG_INFO_COMMA CGNEW_DEBUG_INFO_PARAMETERS_MALLOC));
#else
	auto shared_buffer = reinterpret_cast<uint8_t*>(MALLOC(SIZE_OF_MD5_FILEBUFFER));
#endif

	// check) 메모리가 제대로 할당되었는지 확인한다.
	RETURN_IF(shared_buffer ==nullptr, false);

	// 4) MD5 값 만들기
	for(;;)
	{ 
		// - File에서 읽어들인다.
		auto s_read_bytes = api::file::read(hFile, SIZE_OF_MD5_FILEBUFFER, shared_buffer);

		// check) 읽어들인 값이 0이면 끝낸다.
		BREAK_IF(s_read_bytes==0);

		// - MD5값을 계산한다.
		push_md5(shared_buffer, static_cast<uint32_t>(s_read_bytes));
	}

	// 5) File을 닫는다.
	api::file::close(hFile);

	// 6) 임시 File shared_buffer를 free한다.
	SAFE_FREE(shared_buffer);

	// 7) padding
	padding();

	// return) 성공
	return true;
} 

bool edc::md5::calculate_md5(const std::wstring_view _filename)
{
	char	str_buffer[MAX_STRING_LENGTH]{};

	return	calculate_md5(encode_string(str_buffer, _filename.data()));
}

bool edc::md5::calculate_md5(const std::u16string_view _filename)
{
	char	str_buffer[MAX_STRING_LENGTH]{};

	return	calculate_md5(encode_string(str_buffer, _filename.data()));
}

bool edc::md5::calculate_md5(const std::u32string_view /*_filename*/)
{
	// Error) Not supported
	CGASSERT_ERROR(false);

	return	false;
}


}
//#endif
