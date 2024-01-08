#include "pch.h"


uint32_t CGDK::edc::get_checksum8(uint8_t* _pdata, uint32_t _data_length)
{
	// 1) 계산한다.
	uint32_t tempResult = 0;
	uint8_t* tempEnd = _pdata+_data_length;

	for(; _pdata<tempEnd; _pdata++)
	{
		tempResult+= *_pdata;
	}

	// return)
	return tempResult;
}

uint32_t CGDK::edc::get_checksum16(uint16_t* _pdata, uint32_t _data_length)
{
	// check) 2의 배수인지 검사한다.
	CGASSERT((_data_length & 0x03) != 0, 0);

	// 1) 계산한다.
	uint32_t tempResult = 0;
	uint16_t* tempEnd = _pdata+(_data_length>>1);

	for(; _pdata<tempEnd; _pdata++)
	{
		tempResult += *_pdata;
	}

	// return)
	return tempResult;
}

uint32_t CGDK::edc::get_checksum32(uint32_t* _pdata, uint32_t _data_length)
{
	// check) 4의 배수인지 검사한다.
	CGASSERT((_data_length & 0x03) != 0, 0);

	// 1) 계산한다.
	uint32_t tempResult = 0;
	uint32_t* tempEnd = _pdata+(_data_length>>2);

	for(; _pdata<tempEnd; _pdata++)
	{
		tempResult += *_pdata;
	}

	// return)
	return tempResult;
}
