#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <chrono>
#include <queue>
#include <iostream>
#include <benchmark/benchmark.h>
#include "ts3/timestamp.hpp"
#include "ts3/julian.hpp"
#include "ts3/priceType.hpp"
#include "pitch.hpp"

using namespace std::chrono;
using namespace ts3;
//using namespace ts3::pitch;

time_t	tStart;
time_t	timeCost;
uint64_t timeSt;
timespec	tpStart;
struct tm	localTM;
steady_clock::time_point tSt;

static void test_timestampUs(benchmark::State &state)
{
	timestamp	ts;
	for (auto _ : state) {
		ts.nowUs();
	}
}
BENCHMARK(test_timestampUs);

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

static void test_marshalSysEvent(benchmark::State &state)
{
	struct timespec	sp;
	clock_gettime(CLOCK_MONOTONIC, &sp);
	ts3::DateTime<ts3::duration::us> ut(sp);
	pitch::pitchSystemEvent	sysEvt((pitch::eventCode)'O', 2, ut.count());
	char	bb[128];
	for (auto _ : state) {
		sysEvt.marshal(bb, sizeof(bb));
	}
}
BENCHMARK(test_marshalSysEvent);

static void test_marshalSymDir(benchmark::State &state)
{
	struct timespec	sp;
	clock_gettime(CLOCK_MONOTONIC, &sp);
	ts3::DateTime<ts3::duration::us> ut(sp);
	pitch::pitchSymbolDirectory	symDir('F', "cu1906", 1, 0, 2, 3, ut.count(),
					5,5, 10000, 100000);
	char	bb[128];
	for (auto _ : state) {
		symDir.marshal(bb, sizeof(bb));
	}
}
BENCHMARK(test_marshalSymDir);

static void test_priceToDouble(benchmark::State &state)
{
	int32_t	v = 123456789;
	for (auto _ : state) {
		benchmark::DoNotOptimize(ts3::toDouble(v, 3));
	}
}
BENCHMARK(test_priceToDouble);

static void test_priceFromDouble(benchmark::State &state)
{
	for (auto _ : state) {
		benchmark::DoNotOptimize(ts3::fromDouble<int32_t>(1234567.90, 2));
	}
}
BENCHMARK(test_priceFromDouble);

static void test_strtod(benchmark::State &state)
{
	for (auto _ : state) {
		benchmark::DoNotOptimize(strtod("123456.789", nullptr));
	}
}
BENCHMARK(test_strtod);

#ifdef	__GLIBC_USE
#if __GLIBC_USE(IEC_60559_BFP_EXT)
static void test_strfromd(benchmark::State &state)
{
	double	v=123456.789;
	char	buf[64];
	for (auto _ : state) {
		strfromd(buf, sizeof(buf)-1, "%.3f", v);
	}
}
BENCHMARK(test_strfromd);
#endif
#endif

static void test_strtol(benchmark::State &state)
{
	for (auto _ : state) {
		benchmark::DoNotOptimize(strtol("123456789", nullptr, 10));
	}
}
BENCHMARK(test_strtol);

static void test_strfroml(benchmark::State &state)
{
	long	v=123456789;
	for (auto _ : state) {
		benchmark::DoNotOptimize(std::to_string(v));
	}
}
BENCHMARK(test_strfroml);

int main(int argc, char ** argv) {
	tStart = time(0);
	DateTimeMs	nn;
	timespec	tp2;
	tzset();
	localtime_r(&tStart, &localTM);
	tSt=steady_clock::now();
	clock_gettime(CLOCK_MONOTONIC, &tpStart);
	std::cout << "Now: " << nn.String() << std::endl;
	benchmark::Initialize(&argc, argv);
	benchmark::RunSpecifiedBenchmarks();
	std::cout << "gettimeofday cost " << double(timeCost)*0.001 << " ms" << std::endl;
	std::cout << "timestamp cost " << double(timeSt)*0.001 << " us" << std::endl;
	clock_gettime(CLOCK_MONOTONIC, &tp2);
	auto costNs=tp2-tpStart;
	std::cout << "total cost ns: " << costNs <<std::endl;
}
