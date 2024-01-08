#include "pch.h"

using namespace CGDK;


NUMERIC NUMERIC::_operate_additive(const NUMERIC& _lhs, const NUMERIC& _rhs) noexcept
{
	// check)
	CGASSERT_ERROR(_lhs.fraction < _FRACTION_MAX);
	CGASSERT_ERROR(_rhs.fraction < _FRACTION_MAX);

	// declare) 
	NUMERIC tempValue;

	// 1) value와 fraction을 먼저 더한다.
	tempValue.value		 = _lhs.value + _rhs.value;
	tempValue.fraction	 = _lhs.fraction + _rhs.fraction;

	// 2-1)  fraction의 합계가 -_FRACTION_MAX의 범위를 벗어나지 않으면 그냥 사용...
	if (tempValue.fraction >= _FRACTION_MAX)
	{
		tempValue.fraction -= _FRACTION_MAX;
		++tempValue.value;
	}
	// 2-2) _FRACTION_MAX의 범위를 벗어날 경우 정수부로 올림처리한다.
	else if (tempValue.fraction <= -_FRACTION_MAX)
	{
		tempValue.fraction += _FRACTION_MAX;
		--tempValue.value;
	}

	// 3) value와 fraction의 부호가 다를 경우를 처리한다.
	_operate_normalize(tempValue);

	// check)
	CGASSERT_ERROR(tempValue.fraction>-_FRACTION_MAX && tempValue.fraction<_FRACTION_MAX)

	// return) 결과물을 저장한다.
	return	tempValue;
}

NUMERIC NUMERIC::_operate_substract(const NUMERIC& _lhs, const NUMERIC& _rhs) noexcept
{
	// check) 
	CGASSERT_ERROR(_lhs.fraction < _FRACTION_MAX);
	CGASSERT_ERROR(_rhs.fraction < _FRACTION_MAX);

	// declare) 
	NUMERIC tempValue;

	// 1) 정수부를 먼저 계산한다.
	tempValue.value		 = _lhs.value - _rhs.value;
	tempValue.fraction	 = _lhs.fraction - _rhs.fraction;

	// 2-1)  fraction의 합계가 -_FRACTION_MAX의 범위를 벗어나지 않으면 그냥 사용...
	if (tempValue.fraction >= _FRACTION_MAX)
	{
		tempValue.fraction -= _FRACTION_MAX;
		++tempValue.value;
	}
	// 2-2) _FRACTION_MAX의 범위를 벗어날 경우 정수부로 올림처리한다.
	else if (tempValue.fraction <= -_FRACTION_MAX)
	{
		tempValue.fraction += _FRACTION_MAX;
		--tempValue.value;
	}

	// 3) value와 fraction의 부호가 다를 경우를 처리한다.
	_operate_normalize(tempValue);

	// check)
	CGASSERT_ERROR(tempValue.fraction > -_FRACTION_MAX && tempValue.fraction < _FRACTION_MAX)

	// return)
	return	tempValue;
}

NUMERIC NUMERIC::_operate_multiply(const NUMERIC& _lhs, const NUMERIC& _rhs) noexcept
{
	// check)
	CGASSERT_ERROR(_lhs.fraction < _FRACTION_MAX);
	CGASSERT_ERROR(_rhs.fraction < _FRACTION_MAX);

	// declare) 
	NUMERIC tempValue;

	// 1) 정수부를 먼저 계산한다.
	tempValue.value		 = _lhs.value * _rhs.value;
	tempValue.fraction	 = (_lhs.fraction *  _rhs.fraction)/_FRACTION_MAX;

	auto	tempMid		 = _lhs.value * _rhs.fraction + _lhs.fraction * _rhs.value;
	tempValue.value		+= tempMid / _FRACTION_MAX;
	tempValue.fraction	+= tempMid % _FRACTION_MAX;


	// 2) fraction이 범위를 초과했을 경우 올림 처리를 한다.
	if (tempValue.fraction >= _FRACTION_MAX || tempValue.fraction <= -_FRACTION_MAX)
	{
		tempValue.value		+= (tempValue.fraction / _FRACTION_MAX);
		tempValue.fraction	 = (tempValue.fraction % _FRACTION_MAX);
	}

	// check)
	CGASSERT_ERROR(tempValue.fraction > -_FRACTION_MAX && tempValue.fraction < _FRACTION_MAX)

	// return) 
	return	tempValue;
}

NUMERIC NUMERIC::_operate_divide(const NUMERIC& _lhs, const NUMERIC& _rhs)
{
	// check)
	CGASSERT_ERROR(_lhs.fraction < _FRACTION_MAX);
	CGASSERT_ERROR(_rhs.fraction < _FRACTION_MAX);

	// declare)
	NUMERIC	tempValue;

	if (_rhs.fraction == 0)
	{
		tempValue.value		 = _lhs.value / _rhs.value;
		tempValue.fraction	 = ((_lhs.value - (tempValue.value * _rhs.value)) * _FRACTION_MAX + _lhs.fraction) / _rhs.value;
	}
	else if(_rhs.value == 0)
	{
		auto	a			 = _lhs.value / _rhs.fraction;
		auto	a_1			 = _lhs.value - (a * _rhs.fraction);

		// check) 
		CGASSERT_ERROR(a_1 <= (INT64_MAX / _FRACTION_MAX));

		auto	b			 = (a_1 * _FRACTION_MAX +_lhs.fraction) / _rhs.fraction;
		auto	c			 = b / _FRACTION_MAX;

		tempValue.value		 = a * _FRACTION_MAX + c;
		tempValue.fraction	 = b - (c * _FRACTION_MAX);
	}
	else
	{
		// - 몫을 먼저 구한다.
		int64_t	temp_div1	 = _lhs.value/(_rhs.value+1);

		// - 나머지의 value파트를 구한다.
		auto	temp		 = _lhs - (_rhs * NUMERIC(temp_div1, 0));

		// - 중간처리...
		if(temp.value >= _FRACTION_MAX)
		{
			// - Scale로 _FRACTION_MAX보다 높은 자리만 몫을 먼저 구한다.
			int64_t	temp_div2	 = (temp.value / _FRACTION_MAX)/(_rhs.value+1) * _FRACTION_MAX;

			// - 나머지의 value파트를 구한다.
			temp			-= (_rhs * NUMERIC(temp_div2, 0));

			// - 몫은 더해 줌.
			temp_div1		+= temp_div2;
		}

		// check) 
		CGASSERT_ERROR(temp.value <= (INT64_MAX / _FRACTION_MAX));

		// - 너머지를 추가한다.
		auto	a			 = temp.value * _FRACTION_MAX + temp.fraction;
		auto	b			 = a / _FRACTION_MAX;
		auto	c			 = a - b * _FRACTION_MAX;

		// - 최종값을 계산한다.
		tempValue.value		 = b + temp_div1;
		tempValue.fraction	 = c;
	}

	// check)
	CGASSERT_ERROR(tempValue.fraction > -_FRACTION_MAX && tempValue.fraction < _FRACTION_MAX)

	// return) 
	return	tempValue;
}

NUMERIC NUMERIC::_operate_modurate(const NUMERIC& _lhs, const NUMERIC& _rhs) noexcept
{
	// check)
	CGASSERT_ERROR(_lhs.fraction < _FRACTION_MAX);
	CGASSERT_ERROR(_rhs.fraction < _FRACTION_MAX);

	// declare)
	NUMERIC	tempValue;

	if (_rhs.fraction == 0)
	{
		tempValue.value		 = _lhs.value % _rhs.value;
		tempValue.fraction	 = _lhs.fraction;
	}
	else
	{
		tempValue	 = _lhs - (_lhs / _rhs) * _rhs;
	}

	// check)
	CGASSERT_ERROR(tempValue.fraction > -_FRACTION_MAX && tempValue.fraction < _FRACTION_MAX)

	// return) 
	return	tempValue;
}

NUMERIC NUMERIC::_operate_additive(const NUMERIC& _lhs, int64_t _rhs) noexcept
{
	// check)
	CGASSERT_ERROR(_lhs.fraction > -_FRACTION_MAX && _lhs.fraction < _FRACTION_MAX);

	// declare)
	NUMERIC	tempValue;

	// 1) 더하기...
	tempValue.value		 = _lhs.value + _rhs;
	tempValue.fraction	 = _lhs.fraction;

	// 2) Normalize 
	_operate_normalize(tempValue);

	// check)
	CGASSERT_ERROR(tempValue.fraction > -_FRACTION_MAX && tempValue.fraction < _FRACTION_MAX)

	// return) 
	return	tempValue;
}

NUMERIC NUMERIC::_operate_substract(const NUMERIC& _lhs, int64_t _rhs) noexcept
{
	// check)
	CGASSERT_ERROR(_lhs.fraction > -_FRACTION_MAX && _lhs.fraction < _FRACTION_MAX);

	// declare)
	NUMERIC	tempValue;

	// 1) 더하기...
	tempValue.value		 = _lhs.value - _rhs;
	tempValue.fraction	 = _lhs.fraction;

	// 2) Normalize 
	_operate_normalize(tempValue);

	// check)
	CGASSERT_ERROR(tempValue.fraction > -_FRACTION_MAX && tempValue.fraction < _FRACTION_MAX)

	// return) 
	return	tempValue;
}

NUMERIC NUMERIC::_operate_multiply(const NUMERIC& _lhs, int64_t _rhs) noexcept
{
	// check)
	CGASSERT_ERROR(_lhs.fraction > -_FRACTION_MAX && _lhs.fraction < _FRACTION_MAX);

	// declare)
	NUMERIC	tempValue;

	// 1) 더하기...
	tempValue.value		 = _lhs.value * _rhs;
	tempValue.fraction	 = _lhs.fraction * _rhs;

	// 2) fraction이 범위를 초과했을 경우 올림 처리를 한다.
	if (tempValue.fraction >= _FRACTION_MAX || tempValue.fraction <= -_FRACTION_MAX)
	{
		tempValue.value		+= (tempValue.fraction / _FRACTION_MAX);
		tempValue.fraction	 = (tempValue.fraction % _FRACTION_MAX);
	}

	// check)
	CGASSERT_ERROR(tempValue.fraction > -_FRACTION_MAX && tempValue.fraction < _FRACTION_MAX)

	// return) 
	return	tempValue;
}

NUMERIC NUMERIC::_operate_divide(const NUMERIC& _lhs, int64_t _rhs) noexcept
{
	// check)
	CGASSERT_ERROR(_lhs.fraction > -_FRACTION_MAX && _lhs.fraction < _FRACTION_MAX);

	// declare)
	NUMERIC	tempValue;

	tempValue.value		 = _lhs.value % _rhs;
	tempValue.fraction	 = _lhs.fraction;

	// check)
	CGASSERT_ERROR(tempValue.fraction > -_FRACTION_MAX && tempValue.fraction < _FRACTION_MAX)

	// return) 
	return	tempValue;
}

NUMERIC NUMERIC::_operate_modurate([[maybe_unused]] const NUMERIC& _lhs, [[maybe_unused]]  int64_t _rhs) noexcept
{
	// check)
	CGASSERT_ERROR(_lhs.fraction > -_FRACTION_MAX && _lhs.fraction < _FRACTION_MAX);

	// check) 
	CGASSERT_ERROR(false);

	// return) 
	return	NUMERIC();
}

void NUMERIC::_operate_normalize(NUMERIC& _InOut) noexcept
{
	// - value와 fraction의 부호가 다르면...
	if (_InOut.fraction != 0 && (_InOut.value^_InOut.fraction) < 0)
	{
		if (_InOut.value > 0)
		{
			--_InOut.value;
			_InOut.fraction += _FRACTION_MAX;
		}
		else
		{
			++_InOut.value;
			_InOut.fraction -= _FRACTION_MAX;
		}
	}

	// check)
	CGASSERT_ERROR(_InOut.fraction==0 || (_InOut.value^_InOut.fraction) >= 0)
}

char* NUMERIC::to_string(char* _Out, std::size_t _sizeBuffer) const
{
	// 1) integer part를 출력한다.
	int pos = __sprintf(_Out, _sizeBuffer, "%I64d", value);

	// 2) fraction part가 있을 경우에만 출력한다.
	if (fraction != 0)
	{
		// check) fraction part가 _FRACTION_MAX 보다 
		CGASSERT_ERROR(fraction < _FRACTION_MAX);

		// check) 
		THROW_IF(fraction >= _FRACTION_MAX, throwable::failure(eRESULT::OVERFLOW_ERROR));

		int64_t	temp_fraction = (fraction > 0) ? fraction : -fraction;
		int64_t	divid = _FRACTION_MAX / 10;

		_Out[pos++] = '.';
		while (temp_fraction != 0)
		{
			auto tx = temp_fraction / divid;
			_Out[pos++] = static_cast<char>('0' + tx);
			temp_fraction -= divid * tx;
			divid /= 10;
		}
		_Out[pos++] = 0;
	}

	// return)
	return	_Out;
}

wchar_t* NUMERIC::to_string(wchar_t* _Out, std::size_t _sizeBuffer) const
{
	// 1) integer part를 출력한다.
	int pos = __sprintf(_Out, _sizeBuffer, L"%I64d", value);

	// 2) fraction part가 있을 경우에만 출력한다.
	if (fraction != 0)
	{
		// check) fraction part가 _FRACTION_MAX 보다 
		CGASSERT_ERROR(fraction<_FRACTION_MAX);

		// check) 
		THROW_IF(fraction >= _FRACTION_MAX, throwable::failure(eRESULT::OVERFLOW_ERROR));

		int64_t	temp_fraction = (fraction > 0) ? fraction : -fraction;
		int64_t	divid = _FRACTION_MAX / 10;

		_Out[pos++] = '.';
		while (temp_fraction != 0)
		{
			auto tx = temp_fraction / divid;
			_Out[pos++] = static_cast<wchar_t>(L'0' + tx);
			temp_fraction -= divid * tx;
			divid /= 10;
		}
		_Out[pos++] = 0;
	}

	// return)
	return	_Out;
}

void NUMERIC::from_string(const char* _string)
{
	// check)
	CGASSERT_ERROR(_string != nullptr);

	// check)
	THROW_IF(_string == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) Parsing할 string
	auto* ch = _string;

	// check) 바로 0이면 리턴한다.
	RETURN_IF((*ch) == 0);

	// declare) 
	bool	bNeg		 = false;
	int64_t	tempValue	 = 0;
	int64_t	tempFraction = 0;

	if((*ch)=='-')
	{
		bNeg = true;
		++ch;
	}
	else if ((*ch) == '+')
	{
		++ch;
	}

	// 2) 정수부를 추출한다.
	while((*ch)!= 0 && (*ch)!='.')
	{
		tempValue *= 10;
		tempValue += (*ch)-'0';
		++ch;
	}

	// 3) 소수부를 추출한다.
	if((*ch)!=0)
	{
		// - 소수점 제거...
		++ch;

		// - 최대 Scale만큼만 읽어들이기 위해...
		int	i = _FRACTION_SCALE;

		// - 소수점을 읽어들인다.
		while((*ch)!=0 && i!=0)
		{
			tempFraction *= 10;
			tempFraction += (*ch)-'0';
			++ch;
			--i;
		}

		// - 읽지못한 나머지 자리만큼 올려준다.
		if(tempFraction!=0)
		{
			while(i!=0)
			{
				tempFraction *=10;
				--i;
			}
		}
	}

	// 4) 최종 저장
	if (bNeg==false)
	{
		value		 = tempValue;
		fraction	 = tempFraction;
	}
	else
	{
		value		 = -tempValue;
		fraction	 = -tempFraction;
	}
}
void NUMERIC::from_string(const wchar_t* _string)
{
	// check)
	CGASSERT_ERROR(_string != nullptr);

	// check)
	THROW_IF(_string == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) Parsing할 string
	auto* ch = _string;

	// check) 바로 0이면 리턴한다.
	RETURN_IF((*ch) == 0);

	// declare) 
	bool	bNeg		 = false;
	int64_t	tempValue	 = 0;
	int64_t	tempFraction = 0;

	if((*ch)==L'-')
	{
		bNeg = true;
		++ch;
	}
	else if ((*ch) == L'+')
	{
		++ch;
	}

	// 2) 정수부를 추출한다.
	while((*ch)!= 0 && (*ch)!=L'.')
	{
		tempValue *= 10;
		tempValue += (*ch)-L'0';
		++ch;
	}

	// 3) 소수부를 추출한다.
	if((*ch)!= 0)
	{
		// - 소수점 제거...
		++ch;

		// - 최대 Scale만큼만 읽어들이기 위해...
		int	i = _FRACTION_SCALE;

		// - 소수점을 읽어들인다.
		while((*ch)!= 0 && i!=0)
		{
			tempFraction *= 10;
			tempFraction += (*ch)-L'0';
			++ch;
			--i;
		}

		// - 읽지못한 나머지 자리만큼 올려준다.
		if(tempFraction!=0)
		{
			while(i!=0)
			{
				tempFraction *=10;
				--i;
			}
		}
	}

	// 4) 최종 저장
	if (bNeg==false)
	{
		value		 = tempValue;
		fraction	 = tempFraction;
	}
	else
	{
		value		 = -tempValue;
		fraction	 = -tempFraction;
	}
}

void NUMERIC::from_string(const std::string& _string)
{
	from_string(_string.c_str());
}

void NUMERIC::from_string(const std::wstring& _string)
{
	from_string(_string.c_str());
}


float NUMERIC::get_change_ratio(const NUMERIC& _now, const NUMERIC& _origin) noexcept
{
	// declare) 
	float	ratio;

	if(_origin.empty()==false)
	{
		if(_origin<=_now)
		{
			ratio	 = ((float)_now - (float)_origin) / ((float)_origin) * 100.0f;
		}
		else
		{
			ratio	 = ((float)_origin - (float)_now) / ((float)_origin) * (-100.0f);
		}
	}
	else
	{
		ratio	 = 0.0f;
	}

	// return)
	return	ratio;
}
