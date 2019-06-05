#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <benchmark/benchmark.h>
#include "moldudp.hpp"

using namespace moldUDP;
using namespace ts3;

#include "testData.h"

static void test_moldEncodeHead(benchmark::State &state)
{
	u8	buff[20];
	for (auto _ : state) {
		head0.encode(buff, sizeof(buff));
	}
}
BENCHMARK(test_moldEncodeHead);

static void test_moldDecodeHead(benchmark::State &state)
{
	moldudp64_header head;
	for (auto _ : state) {
		head.decode(headBytes, 20);
	}
}
BENCHMARK(test_moldDecodeHead);

static void test_moldMarshal(benchmark::State &state)
{
	u8	testBuff[256];
	size_t bufLen;
	message_t msgsTest[]={msgs[0], msgs[1], msgs[2]};
	for (auto _ : state) {
		bufLen = sizeof(testBuff);
		marshal(testBuff, bufLen, msgsTest, 3);
	}
}
BENCHMARK(test_moldMarshal);

static void test_moldUnmarshal(benchmark::State &state)
{
	message_t msgsTest[10];
	for (auto _ : state) {
		unmarshal(msgBuf0, msg3Len, msgsTest, 3);
	}
}
BENCHMARK(test_moldUnmarshal);

int main(int argc, char ** argv) {
	benchmark::Initialize(&argc, argv);
	benchmark::RunSpecifiedBenchmarks();
}
