#pragma once
#ifndef	__TS3_PRICETYPE_HPP__
#define	__TS3_PRICETYPE_HPP__

#include <stdlib.h>
#include "cdefs.h"

namespace ts3
{

namespace fixed {
constexpr double dMulti[]={0.01,0.1,1.0,10.0,100.0,1000.0,10000.0,
		100000.0,1000000.0};
constexpr double dDiv[]={100.0,10.0,1.0,0.1,0.01,0.001,0.0001,
		0.00001,0.000001};
constexpr int	digitMax=6,digitMin=-2;

forceinline double digitMulti(int ndig) noexcept {
	if (ts3_unlikely(ndig < digitMin || ndig > digitMax)) return 1.0;
	return dMulti[ndig+2];
}

forceinline double digitDiv(int ndig) noexcept {
	if (ts3_unlikely(ndig < digitMin || ndig > digitMax)) return 1.0;
	return dDiv[ndig+2];
}
}	// end namespace fixed

template<typename T> forceinline double toDouble(const T v, const int ndig) {
	return (double)v * fixed::digitDiv(ndig);
}

template<typename T> forceinline T fromDouble(const double v, const int ndig) {
	return (T)(v * fixed::digitMulti(ndig) + 0.5);
}


} // ts3
#endif	// __TS3_PRICETYPE_HPP__
