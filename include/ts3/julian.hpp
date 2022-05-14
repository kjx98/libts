#pragma once
#ifndef	__TS3_JULIAN_HPP__
#define	__TS3_JULIAN_HPP__

#include <time.h>
#if	!defined(__LP64__) && defined(__linux__)
#include <time64.h>
#endif
#include <chrono>
#include <tuple>
#include <string>
#include "cdefs.h"

namespace ts3 {

//#define	TS3_JULIAN_ADJUSTMENT	1721425
//#define	TS3_JULIAN_EPOCH		2440588

constexpr int32_t	julian_Epoch = 2440588;
#if	!defined(__LP64__) && defined(__linux__)
using	utime_t = time64_t;
#else
using	utime_t = time_t;
#endif

class JulianDay {
public:
	bool operator==(const JulianDay &j) const { return jDN_ == j.jDN_; }
	bool operator<(const JulianDay &j) const { return jDN_ < j.jDN_; }
	JulianDay(const JulianDay &) = default;
	JulianDay(const int v=0) : jDN_(v) {}
	JulianDay(const int y, const int m, const int d) noexcept {
		jDN_ = newJDN(y, m, d);
	}
	explicit JulianDay(const uint32_t days) noexcept {
		int year = days / 10000;
		int mon = (days % 10000) / 100;
		int mday = days % 100;
		jDN_ = newJDN(year, mon, mday);
	}
	uint32_t Uint32() noexcept {
		auto [y,m,d] = date(jDN_);
		if (ts3_likely(y >= 0)) {
			return y * 10000 + m*100 + d;
		}
		return 0;
	}
	int32_t	count() { return jDN_; }
	utime_t to_time_t() {
		utime_t	res=jDN_ - julian_Epoch;
		res *= 3600*24;
		return res;
	}
	std::string String8() noexcept { return std::to_string(jDN_); }
	std::string String() noexcept {
		char	buff[64];
		auto [y,m,d] = date(jDN_);
		if (ts3_likely(y >= 0)) {
			snprintf(buff, sizeof(buff)-1, "%04d-%02d-%02d", y, m, d);
		} else {
			y = -y;
			snprintf(buff, sizeof(buff)-1, "BC %04d-%02d-%02d", y, m, d);
		}
		buff[sizeof(buff)-1] = 0;
		return std::string(buff, strlen(buff));
	}
	bool getYMD(int &y, int &m, int &d) noexcept {
		std::tie(y, m , d) = date(jDN_);
		return true;
	}
private:
	static	std::tuple<int,int,int> date(int j) noexcept {
		int y, m, d;
		int f = j + 1401 + (((4*j+274277)/146097)*3)/4 - 38;
		int e = 4*f + 3;
		int g = (e % 1461) / 4;
		int h = 5*g + 2;
		d = (h%153)/5 + 1;
		m = (h/153+2)%12 + 1;
		y = e/1461 - 4716 + (12+2-m)/12;
		return std::make_tuple(y,m,d);
	}
	static	int	newJDN(int year, int month, int day) noexcept {
		if (month < 1 || month > 12 || day < 1 || day > 31) return 0;
		int res = (1461 * (year + 4800 + (month-14)/12)) / 4;
		res += (367 * (month - 2 - 12*((month-14)/12))) / 12;
		res -= (3 * ((year + 4900 + (month-14)/12) / 100)) / 4;
		res += day - 32075;
		return res;
	};
	int32_t	jDN_ = 0;
};


forceinline struct tm*	gmtime(const utime_t timeV, struct tm *result) noexcept
{
	if (result == nullptr) return result;
	int	days=timeV/(3600*24);
	int	hms=timeV % (3600*24);
	JulianDay jd(days+julian_Epoch);
	int	y,m,d;
	if (ts3_unlikely(jd.getYMD(y,m,d) == 0)) return nullptr;
	result->tm_year = y-1900;
	result->tm_mon = m-1;
	result->tm_mday = d;
	result->tm_hour = hms/3600;
	hms %= 3600;
	result->tm_min = hms/60;
	result->tm_sec = hms%60;
	return result;
}

}
#endif	// __TS3_JULIAN_HPP__
