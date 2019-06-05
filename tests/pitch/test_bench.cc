#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <chrono>
#include <queue>
#include <iostream>
#include <benchmark/benchmark.h>
#include "ts3/timestamp.hpp"
#include "ts3/julian.hpp"
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
	pitch::pitchSystemEvent	sysEvt((pitch::eventCode)'O', 1, 2, ut.count());
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

int main(int argc, char ** argv) {
	tStart = time(0);
	DateTimeMs	nn;
	char	buff[256];
	timespec	tp2;
	tzset();
	localtime_r(&tStart, &localTM);
	tSt=steady_clock::now();
	clock_gettime(CLOCK_MONOTONIC, &tpStart);
	nn.String(buff);
	std::cout << "Now: " << buff << std::endl;
	benchmark::Initialize(&argc, argv);
	benchmark::RunSpecifiedBenchmarks();
	std::cout << "gettimeofday cost " << double(timeCost)*0.001 << " ms" << std::endl;
	std::cout << "timestamp cost " << double(timeSt)*0.001 << " us" << std::endl;
	clock_gettime(CLOCK_MONOTONIC, &tp2);
	auto costNs=tp2-tpStart;
	std::cout << "total cost ns: " << costNs <<std::endl;
}
