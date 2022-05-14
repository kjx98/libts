#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <bitset>
#include <chrono>
#include <queue>
#include <iostream>
//#define	_NTEST
#include "ts3/types.h"
#include "ts3/timestamp.hpp"
#include "ts3/julian.hpp"
#include "ts3/serialization.hpp"
#include "ts3/priceType.hpp"
#include "ts3/message.hpp"
#include <gtest/gtest.h>

using namespace std;
//using namespace ts3;
const char *fmtTime="%y-%m-%d %H:%M:%S";

TEST(testTS3, TestTypes)
{
	ts3::int48_t	v;
	uint32_t	loword;
	ASSERT_EQ(sizeof(v), (size_t)6);
	int64_t	v1=0x123456789012, v2=0x912345678901;
	ts3::int48_t vv1(v1), vv2(v2);
	EXPECT_EQ(v1, vv1.int64());
	EXPECT_NE(v2, vv2.int64());
	memcpy(&loword, &vv1, sizeof(loword));
	EXPECT_EQ(loword, vv1.int64()&0xffffffff);
	ts3::int48_t	v3(0x912345678901);
	std::cerr << "v3 value: " << std::hex << v3.int64() << std::endl;
	EXPECT_TRUE(v3.int64() < 0);
	std::bitset<512>	b1("1111010000");
	std::cerr << "Sizeof bitset<512> " << std::dec << sizeof(b1) << std::endl;
	EXPECT_TRUE(b1.test(4));
	std::cerr << "Sizeof unsigned " << sizeof(unsigned int) << std::endl;
	std::cerr << "Sizeof long " << sizeof(long) << std::endl;
}

TEST(testTS3, TestTimeval)
{
#ifdef	__LP64__
 	const time_t	tt=time(nullptr);
#else
	const time64_t	tt=time(nullptr);
#endif
	ts3::TimeVal	tv(&tt);
	ASSERT_EQ(sizeof(tv), (size_t)8);
	ASSERT_EQ(tv.seconds(), tt);
	ASSERT_EQ(tv.nanoSeconds(), (uint32_t)0);
	tv = ts3::TimeVal(tt<<32 | 123);
	EXPECT_EQ(tv.nanoSeconds(), (uint32_t)123);
	EXPECT_EQ(tv.unix(), tt);
	ts3::TimeVal	tvS, tvE;
	int	nsec;
#ifdef	_NTEST
	tvS.now();
	int	nIntrs=0;
	for(int i=0;i<10000;++i) if (tvS.usleep(100)<0) ++nIntrs;
	tvE.now();
	nsec = tvE.sub(tvS) / 10000000;
	ASSERT_TRUE(nsec != 100);
	std::cerr << "TimeVal.usleep(100) cost " << std::dec << nsec
			<< " microseconds with " << nIntrs << " Interrupts\n";
#endif
	tvS.now();
	for(int i=0;i<10000;++i) ts3::usleep(180);
	tvE.now();
	nsec = tvE.sub(tvS) / 10000000;
	ASSERT_TRUE(nsec >= 180);
	//ASSERT_TRUE(nsec - 180 < 10);
	std::cerr << "ts3::usleep(180) cost " << nsec << " microseconds\n";
	tvS.now();
	for(int i=0;i<10000;++i)
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	tvE.now();
	nsec = tvE.sub(tvS) / 10000000;
	std::cerr << "std::this_thread::sleep_for(100us) cost " << nsec
			<< " us\n";
	auto timeO = time(nullptr) + 3;
	ts3::sleep_to(timeO);
	tvE.now();
	ASSERT_EQ(timeO, tvE.seconds());
	std::cerr << "ts3::sleep_to diff ns " << tvE.nanoSeconds() << std::endl;
}

TEST(testTS3, TestLocalTime)
{
	auto st = time(nullptr);
	EXPECT_FALSE(ts3::__ts3_ts_inited);
	ts3::LocalTime	tt(st);
	auto tmp = tt.ltime();
	auto tmp1 = ts3::klocaltime(st, nullptr);
	ASSERT_EQ(st, tt.time());
	EXPECT_TRUE(ts3::__ts3_ts_inited);
	ASSERT_TRUE(tmp != nullptr);
	ASSERT_TRUE(tmp1 != nullptr);
	ASSERT_EQ(tmp->tm_hour, tmp1->tm_hour);
	ASSERT_EQ(tmp->tm_min, tmp1->tm_min);
	ASSERT_EQ(tmp->tm_sec, tmp1->tm_sec);
	auto ss = tt.SString();
	std::cerr << "Current: " << ss << std::endl; 
	int	nextHour = tmp->tm_hour + 1;
	int	lastHour = tmp->tm_hour - 1;
	if (nextHour > 23) nextHour = 0;
	if (lastHour < 0) lastHour += 24;
	tt.next_hm(nextHour);
	tmp = tt.ltime();
	ASSERT_EQ(tmp->tm_hour, nextHour);
	ASSERT_EQ(tmp->tm_min, 0);
	ASSERT_EQ(tmp->tm_sec, 0);
	ss = tt.SString();
	std::cerr << "Next Hour Time: " << ss << std::endl; 
	int	y, m, d;
	std::tie(y,m,d) = tt.ymd();
	std::cerr << y << "-" << m << "-" << d << ", day of week: "
			<< tmp->tm_wday << std::endl;
	tt.next_hm(lastHour);
	tmp = tt.ltime();
	ASSERT_EQ(tmp->tm_hour, lastHour);
	ASSERT_EQ(tmp->tm_min, 0);
	ASSERT_EQ(tmp->tm_sec, 0);
	ss = tt.SString();
	std::cerr << "Last Hour Time: " << ss << std::endl; 
}

#ifdef	__x86_64__
TEST(testTS3, TestTscClock)
{
#ifndef	NO_RDTSCP
	ASSERT_TRUE(ts3::check_rdtscp());
#endif
	auto st = time(nullptr);
	auto sTick = ts3::rdtscp();
	auto usp = ts3::tsc_clock::Instance().us_pertick();
	auto eTick = ts3::rdtscp();
	auto et = time(nullptr);
	EXPECT_TRUE( et >= st+1 );
	ts3::tsc_clock&	tscc(ts3::tsc_clock::Instance());
	auto jitter = tscc.jitter();
	std::cerr << "Overhead: " << tscc.overhead() << " us_pertick: "
			<< usp << " ticks_per_us: " << 1 / usp
			<< " jitter: " << jitter << std::endl;
	auto el = tscc.elapse(sTick, eTick);
	EXPECT_TRUE(el >= 1.0e6);
#ifdef	__linux__
	EXPECT_TRUE(el < 1.001e6);
#else
	EXPECT_TRUE(el < 1.07e6);
#endif
	std::cerr << "tsc_clock init cost " << tscc.elapse(sTick, eTick)
			<< "us\n";
}
#endif

TEST(testTS3, TestDatetime)
{
	struct tm	tmp;
	time_t	tn=time(nullptr);
#ifdef	__linux__
	cerr << "tz off: " << std::dec << timezone << std::endl;
#endif
	ASSERT_TRUE(localtime_r(&tn, &tmp) != nullptr);
	char	ss[128];
	ASSERT_TRUE(strftime(ss, sizeof(ss)-1,fmtTime, &tmp) > 0);
	std::cerr << "Current time: " << ss << std::endl;
	struct tm	tmpN;
	memset(&tmpN, 0, sizeof(tmpN));
	ASSERT_TRUE(strptime(ss, fmtTime, &tmpN) != nullptr);
	ASSERT_EQ(tmp.tm_sec, tmpN.tm_sec);
	ASSERT_EQ(tmp.tm_min, tmpN.tm_min);
	ASSERT_EQ(tmp.tm_hour, tmpN.tm_hour);
	ASSERT_EQ(tmp.tm_mday, tmpN.tm_mday);
	ASSERT_EQ(tmp.tm_mon, tmpN.tm_mon);
	ASSERT_EQ(tmp.tm_year, tmpN.tm_year);
#ifdef	__linux__
	//ts3::DateTime<ts3::duration::ms> ts1(tn-timezone, 123);
	ts3::DateTime<ts3::duration::ms> ts1(tn, 123);
#else
	// FIXME: currently for Asia/Shanghai TZ
	ts3::DateTime<ts3::duration::ms> ts1(tn, 123);
	//ASSERT_TRUE(localtime_r(&tn, &tmp) != nullptr);
	ASSERT_TRUE(gmtime_r(&tn, &tmp) != nullptr);
	tn -= 28800;
#endif
	//DateTime<...> String via GMT
	auto tsSS = ts1.String();
	ASSERT_TRUE(tsSS != "");
	memcpy(ss, tsSS.data(), tsSS.size());
	const char *sp=strptime(ss, fmtTime, &tmpN);
	ASSERT_TRUE(sp != nullptr);
	EXPECT_EQ(tmp, tmpN);
	EXPECT_EQ(tn, mktime(&tmpN));
	EXPECT_EQ(".123", string(sp, 4));
#ifdef	__linux__
	EXPECT_EQ(tn-timezone, ts3::mkgmtime(&tmpN));
#else
	tn+= 28800;
	EXPECT_EQ(tn, ts3::mkgmtime(&tmpN));
#endif
	ts3::DateTimeMs	dtMs(ts1.count());
	auto ttpp=dtMs.tmPtr();
	EXPECT_EQ(tmp, *ttpp);
}

TEST(testTS3, TestTimeStamp)
{
	//ts3::sysclock	clk;
	//clk.setTime(time(nullptr));
	//ts3::timestamp	ts("CST", false, clk);
	ts3::timestamp	ts("CST", false);
	cerr << "Sys clock resolution: " << ts.resolution() << "ns" << endl;
	auto bt = ts.baseTime();
#if	defined(__LP64__) || !defined(__linux__)
	struct tm *tmp=localtime(&bt);
#else
	struct tm *tmp=localtime64(&bt);
#endif
	ASSERT_TRUE(tmp->tm_hour == 0 && tmp->tm_min == 0 && tmp->tm_sec == 0);
	auto msTS=ts.nowMs();
	time_t tt=msTS/1000 + bt;
	cerr << "ms timestamp: " << msTS << " baseTime: " << ts.baseTime() << endl;
	ts3::DateTime<ts3::duration::ms> ts2(ts.baseTime(), msTS);
	cerr << "cur ms: " << ts2.count() << endl;
	EXPECT_EQ(ts2.to_time_t(), tt);
#ifdef	USE_TIME_POINT
	auto tt1=ts.timeMs(msTS);
	auto ntt =std::chrono::system_clock::to_time_t(tt1);
	EXPECT_EQ(ntt, tt);
#endif
	struct timespec sp, sp1;
	ASSERT_EQ(clock_gettime(CLOCK_REALTIME, &sp), 0);
	sp1 = sp;
	ASSERT_EQ(sp, sp1);
}

TEST(testTS3, TestJulian)
{
	ts3::JulianDay	jd(2018, 10, 1);
	int	year, month, mday;
	ASSERT_TRUE(jd.getYMD(year, month, mday));
	EXPECT_EQ(year, 2018);
	EXPECT_EQ(month, 10);
	EXPECT_EQ(mday, 1);
	uint32_t testDate=20181001;
	ts3::JulianDay	jd1(testDate);
	EXPECT_EQ(jd1.Uint32(), testDate);
	EXPECT_EQ(jd1, jd);
	auto tt = time(nullptr);
	struct tm *tmp=gmtime(&tt);
	ts3::JulianDay jd2(tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday);
	char	bb[128];
	sprintf(bb, "%04d-%02d-%02d", tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday);
	// JulianDay count() % 7 plus 1 is weekday
	std::cerr << "Current date: " << bb << " JulianDay: " << jd2.count()
			<< std::endl << " weekDay: " << tmp->tm_wday << " julianWD :"
			<< jd2.count()%7 << std::endl;
	ts3::JulianDay	jd3(1970,1,1);
	std::cerr << "1970-01-01 epoch julianDay: " << jd3.count() << endl;
	ASSERT_EQ(jd3.count(), ts3::julian_Epoch);
	struct tm tmGMT, tmTS;
	gmtime_r(&tt, &tmGMT);
	ts3::gmtime(tt, &tmTS);
	EXPECT_EQ(tmGMT, tmTS);
	auto ttnn = jd2.to_time_t();
	tmTS.tm_hour = tmTS.tm_min = tmTS.tm_sec = 0;
#if	defined(__LP64__) || !defined(__linux__)
	gmtime_r(&ttnn, &tmGMT);
#else
	gmtime64_r(&ttnn, &tmGMT);
#endif
	EXPECT_EQ(tmGMT, tmTS);
}

TEST(testTS3, TestClock)
{
	ts3::sysclock	simClk(ts3::simClock);
	ts3::JulianDay	jd(2018,5,5);
	ts3::timestamp	ts("CST", false, simClk);
	simClk.setTime(jd.to_time_t());
	auto msTS=ts.nowMs();
	ts3::DateTime<ts3::duration::ms> ts2(ts.baseTime(), msTS);
	auto tt = ts2.to_time_t();
#if	defined(__LP64__) || !defined(__linux__)
	struct tm	*tmp=gmtime(&tt);
#else
	struct tm	*tmp=gmtime64(&tt);
#endif
	EXPECT_EQ(tmp->tm_year+1900, 2018);
	EXPECT_EQ(tmp->tm_mon+1, 5);
	EXPECT_EQ(tmp->tm_mday, 5);
	cerr << "ts<ms>: " << ts2.String() << endl;
	ts3::usleep(100);
	ts3::DateTime<ts3::duration::us> ts22(ts.baseTime(), ts.nowUs());
	cerr << "ts<ms> after sleep 100us: " << ts22.String() << endl;
	tt += 3600*5;
	simClk.setTime(tt);
	usleep(100);
	ts3::DateTime<ts3::duration::us> ts23(ts.baseTime(), ts.nowUs());
	cerr << "ts<ms> 5:00 after sleep 100us: " << ts23.String() << endl;
}

TEST(testTS3, TestSubhour)
{
	ts3::timestamp	ts;
	auto msTS = ts.nowUs();
	ts3::DateTime<ts3::duration::us> ts2(ts.baseTime(), msTS);
	cerr << "ts<us>: " << ts2.String() << endl;
	ts3::subHour	st(ts2.count());
	ASSERT_EQ(sizeof(st), (size_t)4);
	struct	tm tmGMT;
	ts2.tmPtr(&tmGMT);
	int	mm,sec;
	uint32_t	mS;
	std::tie(mm,sec,mS) = st.Time();
	EXPECT_EQ(mm, tmGMT.tm_min);
	EXPECT_EQ(sec, tmGMT.tm_sec);
	cerr << "Min: " << mm << " sec: " << sec << " us: " << mS << endl;
	const uint32_t utt=ts2.count() % ts3::HourUs;
	ts3::subHour	st1(utt);
	std::tie(mm,sec,mS) = st1.Time();
	EXPECT_EQ(mm, tmGMT.tm_min);
	EXPECT_EQ(sec, tmGMT.tm_sec);
}


TEST(testTS3, TestSerial)
{
	struct	pkgBuf {
		int16_t		a;
		int32_t		b;
		char		cc[8];
	} __attribute__((packed));
   	pkgBuf pbuf={1,2,"test"};
	struct	upkgBuf {
		uint16_t	a;
		uint32_t	b;
		char		cc[8];
		std::string	ss;
	};
	upkgBuf ubuf={1,2,"test", "is ok"};
	EXPECT_NE(sizeof(pbuf), sizeof(ubuf));
	char	bb[32];
	memcpy(pbuf.cc, ubuf.cc, sizeof(ubuf.cc));
	ts3::Serialization	serialB(bb, sizeof(bb));
#ifdef	_NTEST
	ASSERT_TRUE(serialB.encode(ubuf.a));
	ASSERT_TRUE(serialB.encode(ubuf.b));
	ASSERT_TRUE(serialB.encodeBytes((u8 *)ubuf.cc, sizeof(ubuf.cc)));
	size_t bLen=serialB.size();
	EXPECT_EQ(sizeof(pbuf), bLen);
	EXPECT_TRUE(memcmp(bb, &pbuf, sizeof(pbuf)) == 0);
	ASSERT_TRUE(serialB.encode(ubuf.ss));
	bLen=serialB.size();
	EXPECT_EQ(sizeof(pbuf)+ubuf.ss.size()+1, bLen);
	ts3::Serialization  serialC(bb, bLen);
	uint16_t	a;
	uint32_t	b;
	char		cc[8];
	std::string	ss;
	ASSERT_TRUE(serialC.decode(a));
	ASSERT_TRUE(serialC.decode(b));
	ASSERT_TRUE(serialC.decodeBytes((u8 *)cc, sizeof(cc)));
	ASSERT_TRUE(serialC.decode(ss));
	EXPECT_EQ(ubuf.a, a);
	EXPECT_EQ(ubuf.b, b);
	ASSERT_TRUE(memcmp(ubuf.cc, cc, sizeof(cc)) == 0);
	EXPECT_EQ(ubuf.ss, ss);
	ts3::Serialization  serialC1(bb, bLen);
	ASSERT_TRUE(serialC1.decode(a));
	ASSERT_TRUE(serialC1.decode(b));
	EXPECT_EQ(ubuf.a, a);
	EXPECT_EQ(ubuf.b, b);
#else
	uint16_t	a;
	uint32_t	b;
	char		cc[8];
	std::string	ss;
	size_t		bLen;
#endif
	memset(bb, 0, sizeof(bb));
	ts3::Serialization	serialB1(bb, sizeof(bb));
	ASSERT_TRUE(serialB1.encode(ubuf.a, ubuf.b));
	ASSERT_TRUE(serialB1.encodeBytes((const u8*)ubuf.cc, sizeof(ubuf.cc)));
	ASSERT_TRUE(serialB1.encode(ubuf.ss));
	bLen=serialB1.size();
	//EXPECT_EQ(sizeof(pbuf), bLen);
	EXPECT_EQ(sizeof(pbuf)+6, bLen);
	EXPECT_TRUE(memcmp(bb, &pbuf, sizeof(pbuf)) == 0);
	ts3::Serialization  serialC2(bb, bLen);
	auto res=serialC2.decode(a, b);
	ASSERT_TRUE(res);
	EXPECT_EQ(ubuf.a, a);
	EXPECT_EQ(ubuf.b, b);
	ASSERT_TRUE(serialC2.decodeBytes((u8 *)cc, sizeof(cc)));
	ASSERT_TRUE(memcmp(ubuf.cc, cc, sizeof(cc)) == 0);
	ASSERT_TRUE(serialC2.decode(ss));
	EXPECT_EQ(ubuf.ss, ss);
}

TEST(testTS3, TestPriceType)
{
	ASSERT_EQ(ts3::toDouble(12345, 2), 123.45);
	ASSERT_EQ(ts3::fromDouble<int>(1234.56, 2), 123456);
	ASSERT_EQ(ts3::fromDouble<int32_t>(1234.53, 2), (int32_t)123453);
	ASSERT_EQ(ts3::fromDouble<int64_t>(123412345678.53, 2), (int64_t)12341234567853);
}

TEST(testTS3, TestPString)
{
	const char	*ss="test";
	ts3::pstring<20>	ps(ss);
	EXPECT_EQ(sizeof(ps), (size_t)20);
	EXPECT_EQ(ps.String(), std::string(ss));
	EXPECT_EQ(ps, std::string(ss));
	EXPECT_EQ(std::string(ss), ps);
	EXPECT_EQ(ps.String(), ss);
}

TEST(testTS3, TestMessage)
{
	ts3::CLmessage	clm;
	EXPECT_EQ(sizeof(clm), (size_t)64);
}

#ifdef	__x86_64__
TEST(testTS3, TestCPUid)
{
	auto res = ts3::x86_cpuid();
	EXPECT_TRUE(res != 0);
	std::cerr << "CPUID " << std::hex << res << std::dec << std::endl;

}
#endif

int main(int argc,char *argv[])
{
	//tzset();
    testing::InitGoogleTest(&argc, argv);//将命令行参数传递给gtest
    return RUN_ALL_TESTS();   //RUN_ALL_TESTS()运行所有测试案例
}
