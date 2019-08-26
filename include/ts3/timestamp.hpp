#pragma once
#ifndef	__TS3_TIMESTAMP_HPP__
#define	__TS3_TIMESTAMP_HPP__

#include <time.h>
#include <chrono>
#include <thread>
#include <memory>
#include <tuple>
#include "types.h"

#define	TS3_TIME_MILLISECOND	1000
#define	TS3_TIME_MICROSECOND	1000000
#define	TS3_TIME_NANOSECOND		1000000000L

bool forceinline	operator==(const tm& left, const tm& right) {
	if (left.tm_year != right.tm_year || left.tm_mon != right.tm_mon ||
		left.tm_mday != right.tm_mday || left.tm_hour != right.tm_hour)
		return false;
	return (left.tm_min == right.tm_min && left.tm_sec == right.tm_sec);
}

bool forceinline	operator==(const timespec& left, const timespec& right) {
	return left.tv_sec == right.tv_sec && left.tv_nsec == right.tv_nsec;
}

forceinline timespec& operator+=(timespec& tt, const timespec &tv) {
	tt.tv_sec += tv.tv_sec;
	tt.tv_nsec += tv.tv_nsec;
	if (tt.tv_nsec >= TS3_TIME_NANOSECOND) {
		tt.tv_nsec -= TS3_TIME_NANOSECOND;
		tt.tv_sec++;
	}
	return tt;
}

forceinline timespec& operator-=(timespec& tt, const timespec &tv) {
	tt.tv_sec -= tv.tv_sec;
	tt.tv_nsec -= tv.tv_nsec;
	if (tt.tv_nsec < 0) {
		tt.tv_nsec += TS3_TIME_NANOSECOND;
		tt.tv_sec--;
	}
	return tt;
}


namespace ts3 {

forceinline struct tm* klocaltime(const time_t tval, struct tm *stm=nullptr) noexcept
{
	static	tm	tms;
	if (stm == nullptr) stm = &tms;
#ifdef	__linux__
	time_t tt = tval - timezone;
#else
	time_t tt = tval;
#endif
	return gmtime_r(&tt, stm);
}

forceinline time_t mkgmtime(const struct tm* stm) noexcept {
	static const int cumdays[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
	long     year = 1900 + stm->tm_year + stm->tm_mon / 12;
	time_t   result = (year - 1970) * 365 + cumdays[stm->tm_mon % 12];
	result += (year - 1968) / 4;
	result -= (year - 1900) / 100;
	result += (year - 1600) / 400;
	if ( (year % 4) == 0 &&  ((year % 100) != 0 || (year % 400) == 0)
	&&  (stm->tm_mon % 12) < 2 )
		--result;
	result += stm->tm_mday - 1;
	result *= 24;
	result += stm->tm_hour;
	result *= 60;
	result += stm->tm_min;
	result *= 60;
	result += stm->tm_sec;
	//if (stm->tm_isdst == 1)
	//   result -= 3600;
	return (result);
}

// CLOCK_MONOTONIC not compatible with chrono, even steady_clock
//#define	TS3_SYSCLOCK	CLOCK_MONOTONIC
#define	TS3_SYSCLOCK	CLOCK_REALTIME

namespace duration {
enum duration_t : int64_t {
	ms	= TS3_TIME_MILLISECOND,
	us = TS3_TIME_MICROSECOND,
	ns = TS3_TIME_NANOSECOND,
};

constexpr double msDiv = 1.0/ms;
constexpr double usDiv = 1.0/us;
constexpr double nsDiv = 1.0/ns;
}

constexpr uint32_t	HourUs=3600*(uint32_t)duration::us;

class	timeval {
public:
	explicit timeval(const time_t *t): sec(*t) {}
	timeval(int64_t tv): sec(tv>>32), nanosec(tv & 0x3ffffff) {}
	timeval() = default;
	timeval(const timeval &) = default;
	timeval(const timespec &tp) : sec(tp.tv_sec), nanosec(tp.tv_nsec) {
	}
	void Now() {
		timespec	tp;
		clock_gettime(TS3_SYSCLOCK, &tp);
		sec = tp.tv_sec;
		nanosec = tp.tv_nsec;
	}
	timeval& operator=(const timeval &tv) noexcept {
		if (ts3_likely(this != &tv)) {
			sec = tv.sec;
			nanosec = tv.nanosec;
		}
		return *this;
	}
	bool operator==(const timeval &tv) {
		return sec == tv.sec && nanosec == tv.nanosec;
	}
	bool operator<(const timeval &tv) {
		return ts3_unlikely(sec == tv.sec)? (nanosec < tv.nanosec)
					: (sec < tv.sec);
	}
	// return diff in seconds
	friend double operator-(const timeval& lhs, const timeval& rhs) noexcept
	{
		auto res = lhs.sec - rhs.sec;
		auto rem = lhs.nanosec - rhs.nanosec;
		return res + rem * duration::nsDiv;
	}
	int64_t Sub(const timeval& rhs) noexcept
	{
		int64_t	res = sec - rhs.sec;
		res *= duration::ns;
		res += nanosec - rhs.nanosec;
		return res;
	}
	//time_t	to_time_t() const { return sec; }
	time_t	unix() const { return sec; }
	time_t	seconds() const { return sec; }
	uint32_t nanoSeconds() const { return nanosec; }
private:
	int32_t		sec = 0;
	int32_t		nanosec = 0;
};


int64_t forceinline operator-(const timespec& left, const timespec& right) noexcept {
	int64_t	res=(left.tv_sec - right.tv_sec) * duration::ns;
	res += left.tv_nsec - right.tv_nsec;
	return res;
}


enum sysclock_t {
	realClock = 0,
	simClock,
};


class sysclock {
public:
	sysclock(sysclock_t clockTyp=realClock): clockType_(clockTyp) {
		if (clockType_ == simClock)  {
			timeAdj_ = std::make_shared<timespec>();
			timeAdj_->tv_sec = 0;
			timeAdj_->tv_nsec = 0;
		}
	}
	sysclock(const sysclock& sc): clockType_(sc.clockType_),
	   	timeAdj_(sc.timeAdj_)	{}
	void setTime(const time_t tt) noexcept {
		if (ts3_unlikely(clockType_ != simClock)) return; //	error
		struct timespec	sp_;
		clock_gettime(TS3_SYSCLOCK, &sp_);
		timeAdj_->tv_sec = tt;
		timeAdj_->tv_nsec = 0;
		*timeAdj_ -= sp_;
	}
	void setTime(const class timeval& tv) noexcept {
		if (ts3_unlikely(clockType_ != simClock)) return; //	error
		struct timespec	sp_;
		clock_gettime(TS3_SYSCLOCK, &sp_);
		timeAdj_->tv_sec = tv.seconds();
		timeAdj_->tv_nsec = tv.nanoSeconds();
		*timeAdj_ -= sp_;
	}
	void now(timespec &sp) noexcept {
		clock_gettime(TS3_SYSCLOCK, &sp);
		if (clockType_ == realClock) return;
		sp += *timeAdj_;
	}
	void now(class timeval& tv) noexcept {
		timespec	sp;
		now(sp);
		tv = timeval(sp);
	}
	time_t time() noexcept {
		timespec	sp;
		now(sp);
		return sp.tv_sec;
	}
private:
	sysclock_t	clockType_;
	std::shared_ptr<timespec>	timeAdj_;
};

#ifdef	_NTEST
inline int64_t timespec_delta(const timespec &before, const timespec &after)
{
	return duration::ns * (after.tv_sec - before.tv_sec) + (after.tv_nsec - before.tv_nsec);
}

inline int64_t timespec_deltaUs(const timespec &before, const timespec &after)
{
	return duration::us * (after.tv_sec - before.tv_sec) + (after.tv_nsec - before.tv_nsec)/1000;
}

inline int64_t timespec_deltaMs(const timespec &before, const timespec &after)
{
	return (after - before)/duration::us;
}
#endif

// "busy sleep" while suggesting that other threads run
// for a small amount of time
void forceinline	usleep(int64_t us) noexcept
{
	timespec	tSt, tp;
	clock_gettime(TS3_SYSCLOCK, &tSt);
    auto intV = us *1000;
    do {
        std::this_thread::yield();
        //sched_yield(); // same effect as prev line
		clock_gettime(TS3_SYSCLOCK, &tp);
    } while (ts3_likely(tp - tSt < intV));
}

class subHour {
public:
	subHour() = default;
	subHour(const subHour& ) = default;
	explicit subHour(const uint32_t uTime): tv_(uTime) {
		if (ts3_unlikely(tv_ >= HourUs)) tv_ -= HourUs;
	}
	subHour(int64_t uT): tv_(uT % HourUs) {}
	subHour(uint8_t min, uint8_t sec, uint32_t uS=0) {
		tv_ = uS % duration::us;
		uint32_t tt = (min*60 + sec) % 3600;
		tv_ += tt * duration::us;
	}
	std::tuple<uint8_t,uint8_t,uint32_t> Time() noexcept {
		uint8_t min,sec;
		uint32_t res, ms;
		ms = tv_ / duration::us;
		res = tv_ % duration::us;
		min = ms / 60;
		sec = ms % 60;
		return std::make_tuple(min,sec,res);
	}
private:
	uint32_t	tv_ = 0;
};

class timestamp
{
public:
	timestamp() noexcept: tz_("GMT") {
		struct timespec tp;
		clock_getres(TS3_SYSCLOCK, &tp);
		_res = tp.tv_nsec;
		sysClock_.now(tp);
		_baseTime =tp.tv_sec; }
	timestamp(time_t baseT) noexcept : _baseTime(baseT), tz_("GMT") {
		struct timespec tp;
		clock_getres(TS3_SYSCLOCK, &tp);
		_res = tp.tv_nsec;
	}
	timestamp(const char *tz, bool isGMT=true, const sysclock clk=sysclock()) :
			tz_(tz), sysClock_(clk)
	{
		struct timespec tp;
		clock_getres(TS3_SYSCLOCK, &tp);
		_res = tp.tv_nsec;
		sysClock_.now(tp);
		struct	tm *tmp;
		if (isGMT) tmp=gmtime(&tp.tv_sec); else tmp = localtime(&tp.tv_sec);
		time_t	offt=tmp->tm_hour*3600 + tmp->tm_min*60+ tmp->tm_sec;
		_baseTime = tp.tv_sec - offt;
	}
	timestamp & operator=(const timestamp &ts) {
		if (ts3_likely(this != &ts)) {
			_res = ts._res;
			_baseTime = ts._baseTime;
			tz_ = ts.tz_;
			sysClock_ = ts.sysClock_;
		}
		return *this;
	}
	int64_t nowUs() noexcept {
		struct timespec tp;
		sysClock_.now(tp);
		return duration::us * (tp.tv_sec - _baseTime) + (tp.tv_nsec)/1000;
	}
	int64_t now() noexcept {
		struct timespec tp;
		//clock_gettime(TS3_SYSCLOCK, &tp);
		sysClock_.now(tp);
		return duration::ns * (tp.tv_sec - _baseTime) + tp.tv_nsec;
	};
	// compact milliseconds timestamp
	int64_t nowMs() noexcept {
		struct timespec tp;
		sysClock_.now(tp);
		return 1000 * (tp.tv_sec - _baseTime) + tp.tv_nsec/(int)duration::us;
	};
	uint32_t nowMsN() noexcept {
		struct timespec tp;
		sysClock_.now(tp);
		uint32_t res =  (tp.tv_sec - _baseTime) << 15;
		res |= (tp.tv_nsec >> 15) & 0x7fff;
		return res;
	};
	int32_t	resolution() { return _res; }
	time_t	baseTime() { return _baseTime; }
	time_t	to_time(int64_t nsVal) {
		return _baseTime + (nsVal/duration::ns);
	}
	time_t	to_timeMs(int64_t msVal) {
		return _baseTime + (msVal/duration::ms);
	}
#ifdef	USE_TIME_POINT
	std::chrono::system_clock::time_point time(int64_t nsVal) noexcept {
		std::chrono::nanoseconds dur(_baseTime*duration::ns + nsVal);
		return std::chrono::system_clock::time_point(dur);
	}
	std::chrono::system_clock::time_point timeMs(int64_t msVal) noexcept {
		std::chrono::milliseconds dur(_baseTime*1000 + msVal);
		return std::chrono::system_clock::time_point(dur);
	}
#endif
private:
	time_t	_baseTime;
	int32_t	_res;
	const char *tz_;
	class sysclock sysClock_;
};

template <duration::duration_t dur>
class DateTime {
	constexpr static int64_t dur_div = duration::ns/dur;
public:
	DateTime() = default;
	DateTime(const DateTime &) = default;
	DateTime(int64_t tt): time_(tt) {}
	explicit DateTime(const struct timespec &tp) {
		time_ = tp.tv_sec * dur;
		time_ += tp.tv_nsec / dur_div;
	}
	explicit DateTime(const timeval &tv) {
		time_ = tv.seconds() * dur;
		time_ += tv.nanoSeconds() / dur_div;
	}
	explicit DateTime(time_t baseTime, int64_t off) {
		time_ = baseTime * dur;
		time_ += off;
	}
	bool operator==(const DateTime &dt) {
		return time_ == dt.time_;
	}
	explicit operator bool () {
		return time_ != 0;
	}
	friend bool operator==(const DateTime &dt, std::nullptr_t) {
		return dt.time_ == 0;
	}
	int64_t count() { return time_; }
	time_t	to_time_t() { return time_/dur; }
	struct tm *tmPtr(struct tm *bufp=nullptr) noexcept {
		time_t	sec_ = time_/dur;
#ifdef	__linux__
		sec_ -= timezone;
#endif
		if (bufp == nullptr) return gmtime(&sec_);
		return gmtime_r(&sec_, bufp);
	};
	char *String(char *bufp) noexcept {
		if (ts3_unlikely(bufp == nullptr)) return nullptr;
		if (ts3_unlikely(time_ == 0)) { *bufp = 0; return bufp; }
		auto tmp = tmPtr();
		int	msec_ = time_ % dur;
		// strftime without milliseconds
		//auto res = strftime(bufp, 16, "%y-%m-%d %H:%M:%S", tmp);
		switch (dur) {
		case duration::ms :
	    sprintf(bufp, "%02d-%02d-%02d %02d:%02d:%02d.%03d", tmp->tm_year%100,
			tmp->tm_mon+1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min,
			tmp->tm_sec, msec_);
			break;
		case duration::us :
	    sprintf(bufp, "%02d-%02d-%02d %02d:%02d:%02d.%06d", tmp->tm_year%100,
			tmp->tm_mon+1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min,
			tmp->tm_sec, msec_);
			break;
		case duration::ns :
	    sprintf(bufp, "%02d-%02d-%02d %02d:%02d:%02d.%06d", tmp->tm_year%100,
			tmp->tm_mon+1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min,
			tmp->tm_sec, msec_);
			break;
		}
		return bufp;
	}
private:
	int64_t	time_ = 0;
};

class DateTimeMs {
public:
	DateTimeMs(const uint64_t tt) {
		sec_ = (tt / duration::ms);
		msec_ = (tt % duration::ms);
	};
	DateTimeMs(const int64_t	tt) {
		sec_ = (tt / duration::ms);
		msec_ = (tt % duration::ms);
	};
	DateTimeMs(const struct timespec &tp) {
		sec_ = tp.tv_sec;
		msec_ = tp.tv_nsec / (duration::ns/duration::ms);
	}
	DateTimeMs() {
		struct timespec tp;
		clock_gettime(TS3_SYSCLOCK, &tp);
		sec_ = tp.tv_sec;
		msec_ = (tp.tv_nsec) / (duration::ns/duration::ms);
	};
	struct tm *tmPtr(struct tm *bufp=nullptr) noexcept {
#ifdef	__linux__
		time_t	secs = sec_ - timezone;
		if (ts3_unlikely(bufp == nullptr)) return gmtime(&secs);
		return gmtime_r(&secs, bufp);
#else
		if (ts3_unlikely(bufp == nullptr)) return gmtime(&sec_);
		return gmtime_r(&sec_, bufp);
#endif
	};
	char *String(char *bufp) noexcept {
		if (ts3_unlikely(bufp == nullptr)) return nullptr;
		auto tmp = tmPtr();
	    sprintf(bufp, "%02d-%02d-%02d %02d:%02d:%02d.%03d", tmp->tm_year%100,
			tmp->tm_mon+1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min,
			tmp->tm_sec, msec_);
		return bufp;
	}
	int	ms() { return msec_; }
private:
	time_t	sec_;
	int		msec_;
};

}
#endif	// __TS3_TIMESTAMP_HPP__
