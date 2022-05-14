#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <chrono>
#include <queue>
#include <iostream>
#include <benchmark/benchmark.h>
#include "ts3/message.hpp"
#include "ringBuf.hpp"

using namespace std::chrono;
using namespace ts3;

time_t	tStart;
time_t	timeCost;
uint64_t timeSt;
timespec	tpStart;
char	srcB[2048];
char	dstB[2048];

static void test_chrono(benchmark::State &state)
{
	system_clock::time_point t1;
	for (auto _ : state) {
		t1 = system_clock::now();
	}
}
BENCHMARK(test_chrono);

int main(int argc, char ** argv) {
	tStart = time(0);
	DateTimeMs	nn;
	timespec	tp2;
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
