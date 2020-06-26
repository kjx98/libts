#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
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
}

TEST(testTS3, TestTimeval)
{
	const time_t	tt=time(nullptr);
	ts3::timeval	tv(&tt);
	ASSERT_EQ(sizeof(tv), (size_t)8);
	ASSERT_EQ(tv.seconds(), tt);
	ASSERT_EQ(tv.nanoSeconds(), (uint32_t)0);
	tv = ts3::timeval(tt<<32 | 123);
	EXPECT_EQ(tv.nanoSeconds(), (uint32_t)123);
	EXPECT_EQ(tv.unix(), tt);
	ts3::timeval	tvS, tvE;
	tvS.now();
	int	nIntrs=0;
	for(int i=0;i<10000;++i) if (tvS.usleep(100)<0) ++nIntrs;
	tvE.now();
	int nsec = tvE.sub(tvS) / 10000000;
	ASSERT_TRUE(nsec != 100);
	std::cerr << "timeval.usleep(100) cost " << std::dec << nsec
			<< " useconds with " << nIntrs << " Interrupts\n";
	tvS.now();
	for(int i=0;i<10000;++i) ts3::usleep(150);
	tvE.now();
	nsec = tvE.sub(tvS) / 10000000;
	ASSERT_TRUE(nsec == 150 || nsec == 151);
	std::cerr << "ts3::usleep(150) cost " << nsec << " useconds\n";
	auto timeO = time(nullptr) + 3;
	ts3::usleep_to(timeO);
	tvE.now();
	ASSERT_EQ(timeO, tvE.seconds());
	std::cerr << "ts3::usleep_to nsec " << tvE.nanoSeconds() << std::endl;
}

TEST(testTS3, TestDatetime)
{
	struct tm	tmp;
	time_t	tn=time(nullptr);
	tzset();
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
	ASSERT_TRUE(ts1.String(ss) != nullptr);
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
	struct tm *tmp=localtime(&bt);
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
	gmtime_r(&ttnn, &tmGMT);
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
	struct tm	*tmp=gmtime(&tt);
	EXPECT_EQ(tmp->tm_year+1900, 2018);
	EXPECT_EQ(tmp->tm_mon+1, 5);
	EXPECT_EQ(tmp->tm_mday, 5);
	char	ss[128];
	cerr << "ts<ms>: " << ts2.String(ss) << endl;
	ts3::usleep(100);
	ts3::DateTime<ts3::duration::us> ts22(ts.baseTime(), ts.nowUs());
	cerr << "ts<ms> after sleep 100us: " << ts22.String(ss) << endl;
	tt += 3600*5;
	simClk.setTime(tt);
	usleep(100);
	ts3::DateTime<ts3::duration::us> ts23(ts.baseTime(), ts.nowUs());
	cerr << "ts<ms> 5:00 after sleep 100us: " << ts23.String(ss) << endl;
}

TEST(testTS3, TestSubhour)
{
	ts3::timestamp	ts;
	auto msTS = ts.nowUs();
	ts3::DateTime<ts3::duration::us> ts2(ts.baseTime(), msTS);
	char	ss[128];
	cerr << "ts<us>: " << ts2.String(ss) << endl;
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
	ASSERT_TRUE(serialB.encode2b(ubuf.a));
	ASSERT_TRUE(serialB.encode4b(ubuf.b));
	ASSERT_TRUE(serialB.encodeBytes((u8 *)ubuf.cc, sizeof(ubuf.cc)));
	ASSERT_TRUE(serialB.encode(ubuf.ss));
	size_t bLen=serialB.Size();
	EXPECT_EQ(sizeof(pbuf), bLen);
	EXPECT_TRUE(memcmp(bb, &pbuf, sizeof(pbuf)) == 0);
	ts3::Serialization  serialC(bb, bLen);
	uint16_t	a;
	uint32_t	b;
	char		cc[8];
	std::string	ss;
	ASSERT_TRUE(serialC.decode2b(a));
	ASSERT_TRUE(serialC.decode4b(b));
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
	bLen=serialB1.Size();
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

int main(int argc,char *argv[])
{
    testing::InitGoogleTest(&argc, argv);//将命令行参数传递给gtest
    return RUN_ALL_TESTS();   //RUN_ALL_TESTS()运行所有测试案例
}
