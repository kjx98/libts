#pragma once
#ifndef	__TS3_PRICETYPE__
#define	__TS3_PRICETYPE__

#include <stdlib.h>
#include "ts3/types.h"

namespace ts3
{

namespace fixed {
const double dMulti[]={0.01,0.1,1.0,10.0,100.0,1000.0,10000.0,100000.0,1000000.0};
const double dDiv[]={100.0,10.0,1.0,0.1,0.01,0.001,0.0001,0.00001,0.000001};
const int	digitMax=6,digitMin=-2;

static inline double digitMulti(int ndig) {
	if (ndig < digitMin || ndig > digitMax) return 1.0;
	return dMulti[ndig+2];
}

static inline double digitDiv(int ndig) {
	if (ndig < digitMin || ndig > digitMax) return 1.0;
	return dDiv[ndig+2];
}
}

template<typename T>static inline double toDouble(const T v, const int ndig) {
	return (double)v * fixed::digitDiv(ndig);
}

template<typename T>static inline T fromDouble(const double v, const int ndig) {
	return (T)(v * fixed::digitMulti(ndig));
}


} // ts3
#endif	// __TS3_PRICETYPE__
