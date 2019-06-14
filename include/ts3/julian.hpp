#pragma once
#ifndef	__TS3_JULIAN__
#define	__TS3_JULIAN__

#include <stdint.h>
#include <time.h>
#include <string>
#include <chrono>
#include <tuple>

namespace ts3 {

#define	TS3_JULIAN_ADJUSTMENT	1721425
#define	TS3_JULIAN_EPOCH		2440588

constexpr int32_t	julianEpoch = TS3_JULIAN_EPOCH;

class JulianDay {
public:
	bool operator==(const JulianDay &j) const {
		return jDN_ == j.jDN_;
	}
	//JulianDay() = default;
	JulianDay(const JulianDay &) = default;
	JulianDay(const int v=0) : jDN_(v) {}
	JulianDay(int y, int m, int d) {
		jDN_ = newJDN(y, m, d);
	}
	explicit JulianDay(const uint32_t days) {
		int year = days / 10000;
		int mon = (days % 10000) / 100;
		int mday = days % 100;
		jDN_ = newJDN(year, mon, mday);
	}
	uint32_t Uint32() {
		int	y,m,d;
		std::tie(y,m,d) = date(jDN_);
		if (y >= 0) {
			return y * 10000 + m*100 + d;
		}
		return 0;
	}
	int32_t	count() { return jDN_; }
	time_t to_time_t() {
		time_t	res=jDN_ - julianEpoch;
		res *= 3600*24;
		return res;
	}
	std::string String8() {
		return std::to_string(jDN_);
	}
	std::string String() {
		int	y,m,d;
		char	buff[64];
		std::tie(y,m,d) = date(jDN_);
		if (y >= 0) {
			sprintf(buff, "%04d-%02d-%02d", y, m, d);
		} else {
			y = -y;
			sprintf(buff, "BC %04d-%02d-%02d", y, m, d);
		}
		return std::string(buff, strlen(buff));
	}
	bool getYMD(int &y, int &m, int &d) {
		std::tie(y, m , d) = date(jDN_);
		return true;
	}
private:
	static	std::tuple<int,int,int> date(int j) {
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
	static	int	newJDN(int year, int month, int day) {
		if (month < 1 || month > 12 || day < 1 || day > 31) return 0;
		int res = (1461 * (year + 4800 + (month-14)/12)) / 4;
		res += (367 * (month - 2 - 12*((month-14)/12))) / 12;
		res -= (3 * ((year + 4900 + (month-14)/12) / 100)) / 4;
		res += day - 32075;
		return res;
	};
	int32_t	jDN_ = 0;
};


struct tm*	gmtime(const time_t &timeV, struct tm *result) {
	if (result == nullptr) return result;
	int	days=timeV/(3600*24);
	int	hms=timeV % (3600*24);
	JulianDay jd(days+julianEpoch);
	int	y,m,d;
	if (!jd.getYMD(y,m,d)) return nullptr;
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
#endif	// __TS3__TIMESTAMP__
