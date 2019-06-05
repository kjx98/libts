#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <benchmark/benchmark.h>
#include "crayUDP.hpp"

using namespace crayUDP;
using namespace ts3;

#include "testData.h"

static void test_crayEncodeHead(benchmark::State &state)
{
	u8	buff[20];
	for (auto _ : state) {
		head0.encode(buff, sizeof(buff));
	}
}
BENCHMARK(test_crayEncodeHead);

static void test_crayDecodeHead(benchmark::State &state)
{
	crayUDP_header head;
	for (auto _ : state) {
		head.decode(headBytes, sizeof(head));
	}
}
BENCHMARK(test_crayDecodeHead);

static void test_crayMarshal(benchmark::State &state)
{
	u8	testBuff[256];
	size_t bufLen;
	message_t msgsTest[]={msgs[0], msgs[1], msgs[2]};
	for (auto _ : state) {
		bufLen = sizeof(testBuff);
		marshal(testBuff, bufLen, msgsTest, 3);
	}
}
BENCHMARK(test_crayMarshal);

static void test_crayUnmarshal(benchmark::State &state)
{
	message_t msgsTest[10];
	for (auto _ : state) {
		unmarshal(msgBuf0, msg3Len, msgsTest, 3);
	}
}
BENCHMARK(test_crayUnmarshal);

int main(int argc, char ** argv) {
	benchmark::Initialize(&argc, argv);
	benchmark::RunSpecifiedBenchmarks();
}
