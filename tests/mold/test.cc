#include <stdlib.h>
#include <string>
#include <gtest/gtest.h>
#include "moldudp.hpp"

using namespace moldUDP;
using namespace ts3;
#include "testData.h"

TEST(testMold, TestDecodeHead1)
{
	u8	hh[20];
	session_t	ses;
	session_t	ses1("2019041801");
	ASSERT_TRUE(ses == 0);
	ASSERT_FALSE(ses1 == 0);
	ASSERT_TRUE(sizeof(session_t) == 10);
	ASSERT_TRUE(sizeof(head0) == 20);
	memcpy(hh, headBytes, 20);
	moldudp64_header *hp=nullptr;
	hp = hp->decodeInline(hh, 20);
	EXPECT_NE(hp, nullptr);
	EXPECT_EQ(head0 ,*hp);
}

TEST(testMold, TestDecodeHead2)
{
	moldudp64_header head;
	ASSERT_TRUE(head.decode(headBytes, 20));
	ASSERT_TRUE(head0 == head);
}

TEST(testMold, TestEncodeHead1)
{
	u8	buff[20];
	ASSERT_TRUE(head0.encode(buff, sizeof(buff)));
	ASSERT_TRUE(memcmp(buff, headBytes, 20) == 0);
}

TEST(testMold, TestEncodeHead2)
{
	moldudp64_header head1= head0;
	ASSERT_TRUE(head1.encodeInline());
	ASSERT_TRUE(memcmp(&head1, headBytes, 20) == 0);
}

TEST(testMold, TestMarshal)
{
	struct args {
	u8	*buff;
	message_t msgs[10];
	int		nMsgs;
	};
	u8	testBuff[256];
	size_t bufLen;
	struct testR {
		struct args arg;
		size_t	wantBufLen;
		int		wantRet;
	} tests[]={
		{{msgBuf0, {msgs[0]}, 1}, msg1Len, 1},
		{{msgBuf0, {msgs[0], msgs[1]}, 2}, msg2Len, 2},
		{{msgBuf0, {msgs[0], msgs[1], msgs[2]}, 3}, msg3Len, 3},
	};
	int n=3;	// 3 tests
	for (int i=0;i<n;i++){
		bufLen = sizeof(testBuff);
		auto ret = marshal(testBuff, bufLen, tests[i].arg.msgs, tests[i].arg.nMsgs);
		EXPECT_EQ(tests[i].wantRet, ret);
		EXPECT_EQ(tests[i].wantBufLen, bufLen);
		EXPECT_TRUE(memcmp(tests[i].arg.buff, testBuff, tests[i].wantBufLen)==0);
	}
}

TEST(testMold, TestUnmarshal)
{
	struct args {
	u8	*buff;
	size_t bufLen;
	int	nMsgs;
	};
	struct testR {
		struct args arg;
		message_t msgs[10];
		int		wantRet;
	} tests[]={
		{{msgBuf0, msg0Len, 4}, {}, -1},
		{{msgBuf0, msg1Len, 1}, {msgs[0]}, 1},
		{{msgBuf0, msg2Len, 2}, {msgs[0], msgs[1]}, 2},
		{{msgBuf0, msg3Len, 3}, {msgs[0], msgs[1], msgs[2]}, 3},
	};
	message_t msgsRet[10];
	int n=4;	// 4 tests
	for (int i=0;i<n;i++){
		auto ret = unmarshal(tests[i].arg.buff, tests[i].arg.bufLen, msgsRet,
					tests[i].arg.nMsgs);
		EXPECT_EQ(tests[i].wantRet, ret);
		if (ret > 0) {
			for (int j=0;j<ret;j++) {
				EXPECT_TRUE(msgsRet[j] == tests[i].msgs[j]);
			}
		}
	}
}

int main(int argc, char *argv[])
{
	initTest();
    testing::InitGoogleTest(&argc, argv);//将命令行参数传递给gtest
    return RUN_ALL_TESTS();   //RUN_ALL_TESTS()运行所有测试案例
}
