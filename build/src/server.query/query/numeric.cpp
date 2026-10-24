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

//-----------------------------------------------------------------------------
// unsigned 128 bit integer : for the exact division of NUMERIC ((lhs x 10^8) / rhs)
//-----------------------------------------------------------------------------
namespace
{
	struct _uint128
	{
		uint64_t hi = 0;
		uint64_t lo = 0;
	};

	// - 64 bit x 64 bit => 128 bit
	_uint128 _mul_64(uint64_t _a, uint64_t _b) noexcept
	{
		const uint64_t a_lo = _a & 0xffffffffu;
		const uint64_t a_hi = _a >> 32;
		const uint64_t b_lo = _b & 0xffffffffu;
		const uint64_t b_hi = _b >> 32;

		const uint64_t p0 = a_lo * b_lo;
		const uint64_t p1 = a_lo * b_hi;
		const uint64_t p2 = a_hi * b_lo;
		const uint64_t p3 = a_hi * b_hi;
		const uint64_t mid = (p0 >> 32) + (p1 & 0xffffffffu) + (p2 & 0xffffffffu);

		return _uint128{ p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32), (mid << 32) | (p0 & 0xffffffffu) };
	}

	// - 128 bit x 64 bit (the result must be less than 2^128)
	_uint128 _mul(const _uint128& _a, uint64_t _b) noexcept
	{
		auto result = _mul_64(_a.lo, _b);
		result.hi += _a.hi * _b;
		return result;
	}

	_uint128 _add(const _uint128& _a, uint64_t _b) noexcept
	{
		_uint128 result{ _a.hi, _a.lo + _b };
		if (result.lo < _a.lo)
			++result.hi;
		return result;
	}

	_uint128 _sub(const _uint128& _a, const _uint128& _b) noexcept
	{
		return _uint128{ _a.hi - _b.hi - ((_a.lo < _b.lo) ? 1u : 0u), _a.lo - _b.lo };
	}

	bool _is_less(const _uint128& _a, const _uint128& _b) noexcept
	{
		return (_a.hi < _b.hi) || (_a.hi == _b.hi && _a.lo < _b.lo);
	}

	// - _n / _d (bit by bit long division : _d must be less than 2^127)
	_uint128 _div(const _uint128& _n, const _uint128& _d) noexcept
	{
		_uint128 quotient;
		_uint128 remainder;

		for (int i = 127; i >= 0; --i)
		{
			// - remainder = (remainder << 1) | (bit i of _n)
			const uint64_t bit = (i >= 64) ? ((_n.hi >> (i - 64)) & 1u) : ((_n.lo >> i) & 1u);
			remainder.hi = (remainder.hi << 1) | (remainder.lo >> 63);
			remainder.lo = (remainder.lo << 1) | bit;

			// - subtract
			if (_is_less(remainder, _d) == false)
			{
				remainder = _sub(remainder, _d);

				if (i >= 64)
					quotient.hi |= (uint64_t(1) << (i - 64));
				else
					quotient.lo |= (uint64_t(1) << i);
			}
		}

		return quotient;
	}

	// - |value x 10^8 + fraction| and its sign
	_uint128 _get_scaled_magnitude(const NUMERIC& _numeric, bool& _is_negative) noexcept
	{
		_is_negative = (_numeric.value < 0) || (_numeric.value == 0 && _numeric.fraction < 0);

		const uint64_t magnitude_value = (_numeric.value < 0) ? (0 - static_cast<uint64_t>(_numeric.value)) : static_cast<uint64_t>(_numeric.value);
		const uint64_t magnitude_fraction = (_numeric.fraction < 0) ? (0 - static_cast<uint64_t>(_numeric.fraction)) : static_cast<uint64_t>(_numeric.fraction);

		auto scaled = _mul_64(magnitude_value, static_cast<uint64_t>(_FRACTION_MAX));

		// - fraction with the same sign is added (with the other sign : subtracted)
		if (_numeric.value == 0 || _numeric.fraction == 0 || ((_numeric.value < 0) == (_numeric.fraction < 0)))
			return _add(scaled, magnitude_fraction);
		else
			return _sub(scaled, _uint128{ 0, magnitude_fraction });
	}
}

NUMERIC NUMERIC::_operate_divide(const NUMERIC& _lhs, const NUMERIC& _rhs)
{
	// check)
	CGASSERT_ERROR(_lhs.fraction > -_FRACTION_MAX && _lhs.fraction < _FRACTION_MAX);
	CGASSERT_ERROR(_rhs.fraction > -_FRACTION_MAX && _rhs.fraction < _FRACTION_MAX);

	// check) divide by zero
	THROW_IF(_rhs.value == 0 && _rhs.fraction == 0, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) magnitudes scaled by 10^8
	bool is_negative_lhs = false;
	bool is_negative_rhs = false;
	const auto scaled_lhs = _get_scaled_magnitude(_lhs, is_negative_lhs);
	const auto scaled_rhs = _get_scaled_magnitude(_rhs, is_negative_rhs);

	// 2) quotient scaled by 10^8 : (lhs x 10^8) / rhs (truncated toward zero)
	const auto quotient = _div(_mul(scaled_lhs, static_cast<uint64_t>(_FRACTION_MAX)), scaled_rhs);

	// 3) split into the value part and the fraction part
	const auto quotient_value = _div(quotient, _uint128{ 0, static_cast<uint64_t>(_FRACTION_MAX) });
	const auto quotient_fraction = _sub(quotient, _mul(quotient_value, static_cast<uint64_t>(_FRACTION_MAX)));

	// check) overflow
	CGASSERT_ERROR(quotient_value.hi == 0 && quotient_value.lo <= static_cast<uint64_t>(INT64_MAX));

	// declare)
	NUMERIC	tempValue(static_cast<int64_t>(quotient_value.lo), static_cast<int64_t>(quotient_fraction.lo));

	// 4) sign
	if (is_negative_lhs != is_negative_rhs)
	{
		tempValue.value		 = -tempValue.value;
		tempValue.fraction	 = -tempValue.fraction;
	}

	// check)
	CGASSERT_ERROR(tempValue.fraction > -_FRACTION_MAX && tempValue.fraction < _FRACTION_MAX)

	// return)
	return	tempValue;
}

NUMERIC NUMERIC::_operate_modulo(const NUMERIC& _lhs, const NUMERIC& _rhs) noexcept
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
		// - lhs - (integer part of lhs / rhs) x rhs
		tempValue	 = _lhs - NUMERIC((_lhs / _rhs).value, 0) * _rhs;
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

	// check)
	CGASSERT_ERROR(_rhs != 0);

	// 1) integer part / fraction part (the remainder of the integer part is carried to the fraction)
	tempValue.value		 = _lhs.value / _rhs;
	tempValue.fraction	 = ((_lhs.value - tempValue.value * _rhs) * _FRACTION_MAX + _lhs.fraction) / _rhs;

	// check)
	CGASSERT_ERROR(tempValue.fraction > -_FRACTION_MAX && tempValue.fraction < _FRACTION_MAX)

	// return) 
	return	tempValue;
}

NUMERIC NUMERIC::_operate_modulo([[maybe_unused]] const NUMERIC& _lhs, [[maybe_unused]]  int64_t _rhs) noexcept
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
	//    (value is 0 and fraction is negative : "-0" - the sign is in the fraction part only)
	int pos = (value == 0 && fraction < 0) ? std::snprintf(_Out, _sizeBuffer, "-0") : std::snprintf(_Out, _sizeBuffer, "%lld", static_cast<long long>(value));

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
	//    (value is 0 and fraction is negative : L"-0" - the sign is in the fraction part only)
	int pos = (value == 0 && fraction < 0) ? std::swprintf(_Out, _sizeBuffer, L"-0") : std::swprintf(_Out, _sizeBuffer, L"%lld", static_cast<long long>(value));

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

	// check) 바로 0이면 리턴한다.
	RETURN_IF((*_string) == 0);

	// declare) 
	bool	bNeg		 = false;
	int64_t	tempValue	 = 0;
	int64_t	tempFraction = 0;

	if((*_string)=='-')
	{
		bNeg = true;
		++_string;
	}
	else if ((*_string) == '+')
	{
		++_string;
	}

	// 2) 정수부를 추출한다.
	while((*_string)!= 0 && (*_string)!='.')
	{
		tempValue *= 10;
		tempValue += (*_string)-'0';
		++_string;
	}

	// 3) 소수부를 추출한다.
	if((*_string)!=0)
	{
		// - 소수점 제거...
		++_string;

		// - 최대 Scale만큼만 읽어들이기 위해...
		int	i = _FRACTION_SCALE;

		// - 소수점을 읽어들인다.
		while((*_string)!=0 && i!=0)
		{
			tempFraction *= 10;
			tempFraction += (*_string)-'0';
			++_string;
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

	// check) 바로 0이면 리턴한다.
	RETURN_IF((*_string) == 0);

	// declare) 
	bool	bNeg		 = false;
	int64_t	tempValue	 = 0;
	int64_t	tempFraction = 0;

	if((*_string)==L'-')
	{
		bNeg = true;
		++_string;
	}
	else if ((*_string) == L'+')
	{
		++_string;
	}

	// 2) 정수부를 추출한다.
	while((*_string)!= 0 && (*_string)!=L'.')
	{
		tempValue *= 10;
		tempValue += (*_string)-L'0';
		++_string;
	}

	// 3) 소수부를 추출한다.
	if((*_string)!= 0)
	{
		// - 소수점 제거...
		++_string;

		// - 최대 Scale만큼만 읽어들이기 위해...
		int	i = _FRACTION_SCALE;

		// - 소수점을 읽어들인다.
		while((*_string)!= 0 && i!=0)
		{
			tempFraction *= 10;
			tempFraction += (*_string)-L'0';
			++_string;
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
