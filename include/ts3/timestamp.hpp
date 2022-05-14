#pragma once
#ifndef	__TS3_TIMESTAMP_HPP__
#define	__TS3_TIMESTAMP_HPP__

#include <time.h>
#if	!defined(__LP64__) && defined(__linux__)
#include <time64.h>
#endif
#include <chrono>
#include <thread>
#include <memory>
#include <tuple>
#include <string>
#include "types.h"
#ifdef	__x86_64__
#include "x86cpu.h"
#endif

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

bool forceinline	operator<(const timespec& left, const timespec& right) {
	return left.tv_sec < right.tv_sec || (left.tv_sec == right.tv_sec &&
			left.tv_nsec < right.tv_nsec);
}

forceinline timespec& operator+=(timespec& tt, const timespec& tv) {
	tt.tv_sec += tv.tv_sec;
	tt.tv_nsec += tv.tv_nsec;
	if (tt.tv_nsec >= TS3_TIME_NANOSECOND) {
		tt.tv_nsec -= TS3_TIME_NANOSECOND;
		tt.tv_sec++;
	}
	return tt;
}

forceinline timespec& operator-=(timespec& tt, const timespec& tv) {
	tt.tv_sec -= tv.tv_sec;
	tt.tv_nsec -= tv.tv_nsec;
	if (tt.tv_nsec < 0) {
		tt.tv_nsec += TS3_TIME_NANOSECOND;
		tt.tv_sec--;
	}
	return tt;
}

forceinline timespec& operator+=(timespec& tt, const int64_t av) {
	auto secV = av / TS3_TIME_NANOSECOND;
	auto secM = av % TS3_TIME_NANOSECOND;
	tt.tv_sec += secV;
	tt.tv_nsec += secM;
	if (tt.tv_nsec >= TS3_TIME_NANOSECOND) {
		tt.tv_nsec -= TS3_TIME_NANOSECOND;
		tt.tv_sec++;
	} else if (tt.tv_nsec < 0) {
		tt.tv_nsec += TS3_TIME_NANOSECOND;
		tt.tv_sec--;
	}
	return tt;
}


namespace ts3 {

#if	!defined(__LP64__) && defined(__linux__)
using	utime_t = time64_t;
#else
using	utime_t = time_t;
#endif

// get CPU tick count
#ifdef	__x86_64__
forceinline	uint64_t rdtscp() {
#ifndef	NO_RDTSCP
#ifdef	__clang__
	uint64_t lo, hi;
	uint32_t aux;
	asm volatile("rdtscp\n" : "=a"(lo), "=d"(hi), "=c"(aux) : :);
	return (hi << 32) + lo;
#else
	unsigned int aux;
	return __rdtscp(&aux);
#endif
#else	// NO_RDTSCP
	uint64_t tsc;
	asm volatile("mfence;rdtsc;"		// serializing read of tsc
				"shl $32,%%rdx; "  // shift higher 32 bits stored in rdx up
				"or %%rdx,%%rax"   // and or onto rax
				: "=a"(tsc)        // output to tsc variable
				:
				: "%rcx", "%rdx"); // rcx and rdx are clobbered
	return tsc;
#endif
}
#endif	// __x86_64__

// timezone must initialized by tzset() on Linux
#if	__cplusplus >= 201703L
inline bool	__ts3_ts_inited = false;
#endif
forceinline struct tm*
klocaltime(const utime_t tval, struct tm *stm=nullptr) noexcept
{
	static	tm	tms;
#if	__cplusplus >= 201703L
	if (!__ts3_ts_inited) {
		tzset();
		__ts3_ts_inited = true;
	}
#endif
	if (stm == nullptr) stm = &tms;
#ifdef	__linux__
	utime_t tt = tval - timezone;
#else
	utime_t tt = tval;
#endif
#if	!defined(__LP64__) && defined(__linux__)
	return gmtime64_r(&tt, stm);
#else
	return gmtime_r(&tt, stm);
#endif
}

forceinline utime_t mkgmtime(const struct tm* stm) noexcept {
	static const int cumdays[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243,
									273, 304, 334 };
	long	year = 1900 + stm->tm_year + stm->tm_mon / 12;
	utime_t	result = (year - 1970) * 365 + cumdays[stm->tm_mon % 12];
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
#define	TS3_STEADY_CLOCK	CLOCK_MONOTONIC
#define	TS3_SYSCLOCK		CLOCK_REALTIME

namespace duration {
enum duration_t : int64_t {
	ms	= TS3_TIME_MILLISECOND,
	us = TS3_TIME_MICROSECOND,
	ns = TS3_TIME_NANOSECOND,
};

constexpr double msDiv = 1.0/ms;
constexpr double usDiv = 1.0/us;
constexpr double nsDiv = 1.0/ns;
}	// end namespace duration

constexpr uint32_t	HourUs=3600*(uint32_t)duration::us;
constexpr int64_t	SysJitt=100000;	// 100us
using	sec_t = std::chrono::seconds;
using	msec_t = std::chrono::milliseconds;
using	usec_t = std::chrono::microseconds;

class	TimeVal {
public:
	explicit TimeVal(const utime_t *t): _sec(*t), _nanosec(0) {}
	TimeVal(const int64_t tv): _sec(tv>>32), _nanosec(tv & 0x3ffffff) {}
	TimeVal(const int sec, const int nsec): _sec(sec), _nanosec(nsec) {}
	TimeVal() = default;
	TimeVal(const TimeVal &) = default;
	TimeVal(const timespec &tp) : _sec(tp.tv_sec), _nanosec(tp.tv_nsec) { }
	void now() {
		timespec	tp;
		clock_gettime(TS3_SYSCLOCK, &tp);
		_sec = tp.tv_sec;
		_nanosec = tp.tv_nsec;
	}
	bool operator==(const TimeVal &tv) {
		return _sec == tv._sec && _nanosec == tv._nanosec;
	}
	bool operator<(const TimeVal &tv) {
		return ts3_unlikely(_sec == tv._sec)? (_nanosec < tv._nanosec)
					: (_sec < tv._sec);
	}
	// return diff in seconds
	friend double operator-(const TimeVal& lhs, const TimeVal& rhs) noexcept
	{
		auto res = lhs._sec - rhs._sec;
		auto rem = lhs._nanosec - rhs._nanosec;
		return res + rem * duration::nsDiv;
	}
	int64_t sub(const TimeVal& rhs) noexcept
	{
		int64_t	res = _sec - rhs._sec;
		res *= duration::ns;
		res += _nanosec - rhs._nanosec;
		return res;
	}
#if	__cplusplus >= 201703L
	utime_t	unix() const { return _sec; }
#endif
	utime_t	seconds() const { return _sec; }
	uint32_t nanoSeconds() const { return _nanosec; }
#ifdef	_NTEST
	int	usleep(int64_t usec) noexcept {
		struct	timespec tp;
		if (usec > 999999) tp.tv_sec = usec / 1000000; else tp.tv_sec = 0;
		if (tp.tv_sec > 60) {
			//kt_warn(0, "usleep too long %d seconds", (int)tp.tv_sec);
			tp.tv_sec = 60;
		}
		tp.tv_nsec = (usec % 1000000)*1000; // nano second
		return nanosleep(&tp, nullptr);
	}
#endif
private:
	uint32_t	_sec = 0;
	int32_t		_nanosec = 0;
};


int64_t forceinline
operator-(const timespec& left, const timespec& right) noexcept {
	int64_t	res=(left.tv_sec - right.tv_sec) * duration::ns;
	res += left.tv_nsec - right.tv_nsec;
	return res;
}

class LocalTime {
public:
	LocalTime(const utime_t ts=0): sec_(ts),nsec_(0) {
		if (sec_ == 0) {
			struct timespec	sp_;
			clock_gettime(TS3_SYSCLOCK, &sp_);
			sec_ = sp_.tv_sec;
			nsec_ = sp_.tv_nsec;
		}
		klocaltime(sec_, &tm_);
	}
	LocalTime(const LocalTime&) = default;
	LocalTime(const timespec & tp): sec_(tp.tv_sec), nsec_(tp.tv_nsec) {
		klocaltime(sec_, &tm_);
	}
	LocalTime(timespec && tp): sec_(tp.tv_sec), nsec_(tp.tv_nsec) {
		klocaltime(sec_, &tm_);
	}
	std::string SString() noexcept {
		char	buff[32];
		strftime(buff, 24, "%y-%m-%d %H:%M:%S", &tm_);
		return std::string(buff, strlen(buff));
	}
	utime_t		time() noexcept { return sec_; }
	constexpr struct tm*	ltime() noexcept { return &tm_; }
	std::tuple<int,int,int> ymd() noexcept {
		return std::make_tuple(tm_.tm_year+1900,tm_.tm_mon+1,tm_.tm_mday);
	}
	int32_t nanoSeconds() noexcept { return nsec_; }
	// assume sec == 0 mostly
	int	time_next_hm(const int hour, const int min=0, const int sec=0) noexcept
	{
		int	minutes = (hour - tm_.tm_hour)*60 + (min-tm_.tm_min);
		if (minutes <= 0) minutes += 24*60;
		int ss = (sec-tm_.tm_sec) + minutes*60;
		return ss;
	}
	// calculate microseconds next to hour/minute/second
	int64_t us_next_hm(const int hour, const int min=0, const int sec=0)
	{
		int64_t		us = time_next_hm(hour, min, sec)*(int64_t)1000000;
		us -= nsec_/1000;
		return us;
	}
	void next_hm(const int hour, const int min=0, const int sec=0) noexcept
	{
		auto ss = time_next_hm(hour, min, sec);
		// reset nsec_ while hms changed
		if (ss != 0) {
			nsec_ = 0;
			sec_ += ss;
			klocaltime(sec_, &tm_);
		}
	}
private:
	utime_t	sec_;
	int32_t	nsec_;
	struct tm	tm_;
};

#ifdef	__x86_64__
class tsc_clock {
public:
	//tsc_clock(const tsc_clock &) = default;
	TS3_DISABLE_COPY_MOVE(tsc_clock)
	static tsc_clock&	Instance() noexcept {
		static	tsc_clock	tsc_clock_;
#ifndef	NO_RDTSCP
		if (!check_rdtscp()) {
			perror("No RDTSCP support");
			abort();	// no RDTSCP
		}
#endif
		if (tsc_clock_.start_ == 0) {
			tsc_clock_.start_ = rdtscp();
			tsc_clock_.calibration();
		}
		return tsc_clock_;
	}
	const double us_pertick() noexcept { return us_pertick_; }
	const uint64_t overhead() noexcept { return overhead_; }
	const int64_t jitter() noexcept {
		if (jitter_ == 0) calibration_sleep();
		return jitter_;
	}
	double elapse(const uint64_t startT, const uint64_t endT) noexcept {
		if (endT <= startT + overhead_)
			return (endT - startT) * us_pertick_;
		return (endT - startT - overhead_) * us_pertick_;
	}
private:
	tsc_clock() : us_pertick_(1000.0), start_(0), overhead_(0), jitter_(0) {
	}
	void calibration() {
		overhead_ = 1e9;
		for (int i = 0; i<10; ++i) {
			auto start = rdtscp();
			auto stop = rdtscp();
			auto diff = stop - start;
			if (overhead_ > diff) overhead_ = diff;
		}
		{
			timespec	tp1, tp2;
			clock_gettime(TS3_STEADY_CLOCK, &tp1);
			auto start = rdtscp();
			std::this_thread::sleep_for(sec_t(1));
			auto stop = rdtscp();
			clock_gettime(TS3_STEADY_CLOCK, &tp2);
			int64_t time_span = tp2 - tp1;
			us_pertick_ = time_span / ((stop - start - overhead_) * 1000.0);
		}
	}
	void calibration_sleep() {
		timespec	tp1, tp2;
		clock_gettime(TS3_STEADY_CLOCK, &tp1);
		for(int i=0;i<10000;++i)
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		clock_gettime(TS3_STEADY_CLOCK, &tp2);
		jitter_ = (tp2 - tp1) / 10000000;
		jitter_ -= 100;
	}
	double		us_pertick_;
	uint64_t	start_;
	uint64_t	overhead_;
	int64_t		jitter_;
};
#endif

enum sysclock_t {
	realClock = 0,
	simClock,
};


class sysclock {
public:
	sysclock(sysclock_t clockTyp=realClock): clockType_(clockTyp),
		clockid_(TS3_SYSCLOCK)
	{
		if (clockType_ == simClock)  {
			timeAdj_ = std::make_shared<timespec>();
			timeAdj_->tv_sec = 0;
			timeAdj_->tv_nsec = 0;
			clockid_ = TS3_STEADY_CLOCK;
		}
	}
	sysclock(const sysclock& sc): clockType_(sc.clockType_),
		clockid_(sc.clockid_),
		timeAdj_(sc.timeAdj_)	{}
	void setTime(const utime_t tt) noexcept {
		if (ts3_unlikely(clockType_ != simClock)) return; //	error
		struct timespec	sp_, spA;
		clock_gettime(clockid_, &sp_);
		spA.tv_sec = tt;
		spA.tv_nsec = 0;
		spA -= sp_;
		// no rollback time
		if (*timeAdj_ < spA) *timeAdj_ = spA;
	}
	void setTime(const TimeVal& tv) noexcept {
		if (ts3_unlikely(clockType_ != simClock)) return; //	error
		struct timespec	sp_, spA;
		clock_gettime(clockid_, &sp_);
		spA.tv_sec = tv.seconds();
		spA.tv_nsec = tv.nanoSeconds();
		spA -= sp_;
		// no rollback time
		if (*timeAdj_ < spA) *timeAdj_ = spA;
	}
	void now(timespec &sp) noexcept {
		clock_gettime(clockid_, &sp);
		if (clockType_ == realClock) return;
		sp += *timeAdj_;
	}
	void now(TimeVal& tv) noexcept {
		timespec	sp;
		now(sp);
		tv = TimeVal(sp);
	}
	utime_t time() noexcept {
		timespec	sp;
		now(sp);
		return sp.tv_sec;
	}
private:
	sysclock_t	clockType_;
	clockid_t	clockid_;
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
void forceinline	nsleep(int64_t nsec) noexcept
{
#ifdef	ommit
	timespec	tSt, tp;
	clock_gettime(TS3_SYSCLOCK, &tSt);
    do {
        std::this_thread::yield();
        //sched_yield(); // same effect as prev line
		clock_gettime(TS3_SYSCLOCK, &tp);
    } while (ts3_likely(tp - tSt < nsec));
#else
	timespec	tpe, tp;
	clock_gettime(TS3_SYSCLOCK, &tpe);
	tp.tv_sec = tpe.tv_sec;
	tp.tv_nsec = tpe.tv_nsec;
	tpe += nsec;
	if (nsec > SysJitt) {
		nsec -= SysJitt;
		tp += nsec;
		while (clock_nanosleep(TS3_SYSCLOCK,TIMER_ABSTIME,&tp, nullptr) == EINTR);
	}
    while (ts3_likely(tp < tpe)) {
        std::this_thread::yield();
        //sched_yield(); // same effect as prev line
		clock_gettime(TS3_SYSCLOCK, &tp);
    }
#endif
}

void forceinline	usleep(int64_t us) noexcept
{
	nsleep(us * 1000);
}

// "busy sleep" while suggesting that other threads run
// for a small amount of time
void forceinline	sleep_to(utime_t timeo) noexcept
{
	timespec	tpe, tp;
	clock_gettime(TS3_SYSCLOCK, &tp);
	if (tp.tv_sec >= timeo) return;
	tpe.tv_sec = timeo;
	tpe.tv_nsec = 0;
	auto nsleepv = tpe - tp;
	nsleep(nsleepv);
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
	// return minutes/seconds/microseconds within hour
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
	timestamp(utime_t baseT) noexcept : _baseTime(baseT), tz_("GMT") {
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
#if	!defined(__LP64__) && defined(__linux__)
		utime_t		tv_sec_ = tp.tv_sec;
		if (isGMT) tmp=gmtime64(&tv_sec_); else tmp = localtime64(&tv_sec_);
#else
		if (isGMT) tmp=gmtime(&tp.tv_sec); else tmp = localtime(&tp.tv_sec);
#endif
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
	utime_t	baseTime() { return _baseTime; }
	utime_t	to_time(int64_t nsVal) {
		return _baseTime + (nsVal/duration::ns);
	}
	utime_t	to_timeMs(int64_t msVal) {
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
	utime_t	_baseTime;
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
	explicit DateTime(const TimeVal &tv) {
		time_ = tv.seconds() * dur;
		time_ += tv.nanoSeconds() / dur_div;
	}
	explicit DateTime(utime_t baseTime, int64_t off) {
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
	utime_t	to_time_t() { return time_/dur; }
	struct tm *tmPtr(struct tm *bufp=nullptr) noexcept {
		utime_t	sec_ = time_/dur;
#ifdef	__linux__
		sec_ -= timezone;
#endif
#if	!defined(__LP64__) && defined(__linux__)
		if (bufp == nullptr) return gmtime64(&sec_);
		return gmtime64_r(&sec_, bufp);
#else
		if (bufp == nullptr) return gmtime(&sec_);
		return gmtime_r(&sec_, bufp);
#endif
	};
	// return short format timestamp
	std::string SString() noexcept {
		if (ts3_unlikely(time_ == 0)) { std::string(""); }
		char	buff[32];
		auto tmp = tmPtr();
		int	msec_ = time_ % dur;
		// strftime without milliseconds
		//auto res = strftime(bufp, 16, "%y-%m-%d %H:%M:%S", tmp);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
		switch (dur) {
		case duration::ms :
		snprintf(buff, 20, "%02d:%02d:%02d.%03d", tmp->tm_hour, tmp->tm_min,
						tmp->tm_sec, msec_);
			break;
		case duration::us :
		snprintf(buff, 20, "%02d:%02d:%02d.%06d", tmp->tm_hour, tmp->tm_min,
						tmp->tm_sec, msec_);
			break;
		case duration::ns :
	    snprintf(buff, 24, "%02d:%02d:%02d.%09d", tmp->tm_hour, tmp->tm_min,
						tmp->tm_sec, msec_);
			break;
		}
#pragma GCC diagnostic pop
		return std::string(buff, strlen(buff));
	}
	std::string String() noexcept {
		if (ts3_unlikely(time_ == 0)) { std::string(""); }
		char	buff[32];
		auto tmp = tmPtr();
		int	msec_ = time_ % dur;
		// strftime without milliseconds
		//auto res = strftime(bufp, 16, "%y-%m-%d %H:%M:%S", tmp);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
		switch (dur) {
		case duration::ms :
		snprintf(buff, 24, "%02d-%02d-%02d %02d:%02d:%02d.%03d",
			tmp->tm_year%100, tmp->tm_mon+1, tmp->tm_mday, tmp->tm_hour,
			tmp->tm_min, tmp->tm_sec, msec_);
			break;
		case duration::us :
		snprintf(buff, 30, "%02d-%02d-%02d %02d:%02d:%02d.%06d",
			tmp->tm_year%100, tmp->tm_mon+1, tmp->tm_mday, tmp->tm_hour,
			tmp->tm_min, tmp->tm_sec, msec_);
			break;
		case duration::ns :
	    snprintf(buff, 30, "%02d-%02d-%02d %02d:%02d:%02d.%09d",
			tmp->tm_year%100, tmp->tm_mon+1, tmp->tm_mday, tmp->tm_hour,
			tmp->tm_min, tmp->tm_sec, msec_);
			break;
		}
		return std::string(buff, strlen(buff));
#pragma GCC diagnostic pop
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
		utime_t	secs = sec_ - timezone;
#ifdef	__LP64__
		if (ts3_unlikely(bufp == nullptr)) return gmtime(&secs);
		return gmtime_r(&secs, bufp);
#else
		if (ts3_unlikely(bufp == nullptr)) return gmtime64(&secs);
		return gmtime64_r(&secs, bufp);
#endif
#else
		if (ts3_unlikely(bufp == nullptr)) return gmtime(&sec_);
		return gmtime_r(&sec_, bufp);
#endif
	};
	std::string String() noexcept {
		char	buff[32];
		auto tmp = tmPtr();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
	    snprintf(buff, 30, "%02d-%02d-%02d %02d:%02d:%02d.%03d",
			tmp->tm_year%100, tmp->tm_mon+1, tmp->tm_mday, tmp->tm_hour,
			tmp->tm_min, tmp->tm_sec, msec_);
#pragma GCC diagnostic pop
		return std::string(buff, strlen(buff));
	}
	int	ms() { return msec_; }
private:
	utime_t	sec_;
	int		msec_;
};

}
#endif	// __TS3_TIMESTAMP_HPP__
