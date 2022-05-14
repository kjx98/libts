#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <chrono>
#include <queue>
#ifdef	BOOSTVER
#include <boost/chrono.hpp>
#endif
#include <iostream>
#include <benchmark/benchmark.h>
//#define	_NTEST
#include "ts3/timestamp.hpp"
#include "ts3/julian.hpp"

using namespace std::chrono;
using namespace ts3;

time_t	tStart;
time_t	timeCost;
uint64_t timeSt;
timespec	tpStart;
struct tm	localTM;
steady_clock::time_point tSt;
#ifdef	BOOSTVER
boost::chrono::steady_clock::time_point btSt;
#endif
char	srcB[2048];
char	dstB[2048];
#define	NLOOPS	1000

static void test_chrono(benchmark::State &state)
{
	steady_clock::time_point t1;
	for (auto _ : state) {
		t1 = steady_clock::now();
	}
}
BENCHMARK(test_chrono);

static void test_chronoMs(benchmark::State &state)
{
	steady_clock::time_point t1;
	milliseconds	dur;
	for (auto _ : state) {
		t1 = steady_clock::now();
		dur = std::chrono::duration_cast<milliseconds>(t1-tSt);
	}
}
BENCHMARK(test_chronoMs);

#ifdef	BOOSTVER
static void test_bchrono(benchmark::State &state)
{
	boost::chrono::steady_clock::time_point t1;
	for (auto _ : state) {
		t1 = boost::chrono::steady_clock::now();
	}
}
BENCHMARK(test_bchrono);

static void test_bchronoMs(benchmark::State &state)
{
	boost::chrono::steady_clock::time_point t1;
	boost::chrono::milliseconds	dur;
	for (auto _ : state) {
		t1 = boost::chrono::steady_clock::now();
		dur = boost::chrono::duration_cast<boost::chrono::milliseconds>(t1-btSt);
	}
}
BENCHMARK(test_bchronoMs);
#endif

#ifdef	__x86_64__
static void test_rdtscp(benchmark::State &state)
{
	uint64_t	v	[[maybe_unused]];
	for (auto _ : state) {
		v = ts3::rdtscp();
	}
}
BENCHMARK(test_rdtscp);
#endif

static void test_memcpy(benchmark::State &state)
{
	for (auto _ : state) {
		memcpy(dstB, srcB, 1500);
	}
}
BENCHMARK(test_memcpy);

static void test_queue(benchmark::State &state)
{
	std::queue<int>	que;
	for (auto _ : state) {
		int	i;
		//for (i=0;i<NLOOPS;i++) que.emplace(i);
		for (i=0;i<NLOOPS;i++) que.push(i);
		for (i=0;i<NLOOPS;i++) que.pop();
	}
}
BENCHMARK(test_queue);

static void test_timestamp(benchmark::State &state)
{
	timestamp	ts;
	for (auto _ : state) {
		timeSt = ts.now();
	}
}
BENCHMARK(test_timestamp);

static void test_timestampUs(benchmark::State &state)
{
	timestamp	ts;
	for (auto _ : state) {
		ts.nowUs();
	}
}
BENCHMARK(test_timestampUs);

static void test_timestampMs(benchmark::State &state)
{
	timestamp	ts;
	for (auto _ : state) {
		ts.nowMs();
	}
}
BENCHMARK(test_timestampMs);

static void test_timestampMsN(benchmark::State &state)
{
	timestamp	ts;
	for (auto _ : state) {
		ts.nowMsN();
	}
}
BENCHMARK(test_timestampMsN);

static void test_timestampSimClock(benchmark::State &state)
{
	sysclock   simClk(simClock);
    JulianDay  jd(2018,5,5);
    timestamp  ts("CST", false, simClk);
    simClk.setTime(jd.to_time_t());
	for (auto _ : state) {
		ts.nowMs();
	}
}
BENCHMARK(test_timestampSimClock);

static void test_gmtime(benchmark::State &state)
{
	auto tt = time(0);
	struct tm tmss;
	for (auto _ : state) {
		gmtime_r(&tt, &tmss);
	}
}
BENCHMARK(test_gmtime);

static void test_ts3gmtime(benchmark::State &state)
{
	auto tt = time(0);
	struct tm tmss;
	for (auto _ : state) {
		ts3::gmtime(tt, &tmss);
	}
}
BENCHMARK(test_ts3gmtime);

static void test_localtime(benchmark::State &state)
{
	auto tt = time(nullptr);
	struct tm tmss;
	for (auto _ : state) {
		localtime_r(&tt, &tmss);
	}
}
BENCHMARK(test_localtime);

static void test_klocaltime(benchmark::State &state)
{
#ifdef	__linux__
	auto tt = time(nullptr);
#else
	// FIXME: Asia/Shanghai
	auto tt = time(nullptr)+28800;
#endif
	for (auto _ : state) {
		klocaltime(tt);
	}
}
BENCHMARK(test_klocaltime);

static void test_mktime(benchmark::State &state)
{
	for (auto _ : state) {
		mktime(&localTM);
	}
}
BENCHMARK(test_mktime);

static void test_mkgmtime(benchmark::State &state)
{
	for (auto _ : state) {
		ts3::mkgmtime(&localTM);
	}
}
BENCHMARK(test_mkgmtime);

static void test_julian(benchmark::State &state)
{
	for (auto _ : state) {
		ts3::JulianDay	jd(2018,10, 1);
	}
}
BENCHMARK(test_julian);

static void test_gettimeofday(benchmark::State &state)
{
	::timeval tv;
	time_t		uSec;
	for (auto _ : state) {
		gettimeofday(&tv, 0);
		//if (tv.tv_usec >= 1000000) { tv.tv_usec = 0; }
		timeCost = uSec = (tv.tv_sec-tStart) * 1000000 + tv.tv_usec;
	}
}
BENCHMARK(test_gettimeofday);

static void test_clocktime(benchmark::State &state)
{
	struct timespec	tp;
	for (auto _ : state) {
		clock_gettime(CLOCK_REALTIME, &tp);
	}
}
BENCHMARK(test_clocktime);

static void test_monoclocktime(benchmark::State &state)
{
	struct timespec	tp;
	for (auto _ : state) {
		clock_gettime(CLOCK_MONOTONIC, &tp);
	}
}
BENCHMARK(test_monoclocktime);

static void test_opertime(benchmark::State &state)
{
	struct timespec	tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	for (auto _ : state) {
		int64_t ns __attribute__ ((unused));
	   	ns = tp - tpStart;
	}
}
BENCHMARK(test_opertime);

#ifdef	_NTEST
static void test_operMstime(benchmark::State &state)
{
	struct timespec	tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	for (auto _ : state) {
		timespec_deltaMs(tpStart,tp);
	}
}
BENCHMARK(test_operMstime);

static void test_operUstime(benchmark::State &state)
{
	struct timespec	tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	for (auto _ : state) {
		timespec_deltaUs(tpStart,tp);
	}
}
BENCHMARK(test_operUstime);
#endif

int main(int argc, char ** argv) {
	tStart = time(0);
	DateTimeMs	nn;
	timespec	tp2;
	//tzset();
	localtime_r(&tStart, &localTM);
	tSt=steady_clock::now();
#ifdef	BOOSTVER
	btSt=boost::chrono::steady_clock::now();
#endif
	clock_gettime(CLOCK_MONOTONIC, &tpStart);
	std::cout << "Now: " << nn.String() << std::endl;
	benchmark::Initialize(&argc, argv);
	benchmark::RunSpecifiedBenchmarks();
	std::cout << "gettimeofday cost " << double(timeCost)*0.001 << " ms" << std::endl;
	std::cout << "timestamp cost " << double(timeSt)*0.001 << " us" << std::endl;
	clock_gettime(CLOCK_MONOTONIC, &tp2);
	auto costNs=tp2-tpStart;
#ifdef	_NTEST
	auto costMs=timespec_deltaMs(tpStart,tp2);
	std::cout << "total cost ns: " << costNs << ", ms: " << costMs <<std::endl;
#else
	std::cout << "total cost ns: " << costNs << std::endl;
#endif
}
