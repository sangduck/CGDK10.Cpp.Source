#include "pch.h"

namespace CGDK
{

void edc::crc::generate_crc_table(uint32_t _key)
{
	// Assert) _key는 0이면 안된다.
	CGASSERT_ERROR(_key != 0);

 	// 1) Table을 계산한다.
	for (int i=0; i<256; ++i)
	{
		uint32_t tempCRC = i;
		for (int j=8; j>0; j--) 
		{
			if (tempCRC & 1) 
			{
				tempCRC = (tempCRC >> 1) ^ _key;	// 1이면 XOR 연산으로 뺀다.
			}
			else 
			{
				tempCRC >>= 1;						// 0일 경우 그냥 쉬프트만 한다.
			}
		}
		m_table_CRC.at(i) = tempCRC;
	}

	// 2) Key값을 설정한다.
	m_key = _key;
}

uint32_t edc::crc::get_crc(const uint8_t* _pdata, size_t _data_length) const
{
	// check) parameter 검사.
	CGASSERT_ERROR(_pdata != nullptr);
	CGASSERT_ERROR(_data_length != 0);

	// check) _pdata이 nullptr일 경우 Return한다.
	RETURN_IF(_pdata == nullptr, 0);

	// declare) 
    uint32_t tempCRC = 0;

	// 1) crc를 계산한다.
	for(size_t i=0 ; i<_data_length; ++i)
	{
		tempCRC = (tempCRC>>8) ^ m_table_CRC.at((tempCRC^*(_pdata+i)) & 0xff);
	}

	// return) 
 	return tempCRC;
}


}