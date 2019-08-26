#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <chrono>
#include <queue>
#include <iostream>
#include "ts3/types.h"
#include "ts3/timestamp.hpp"
#include "ts3/julian.hpp"
#include "ts3/serialization.hpp"
#include "proto/pitch_message.h"
#include "pitch.hpp"
#include <gtest/gtest.h>

using namespace std;
//using namespace ts3;
using namespace ts3::pitch;
const char *fmtTime="%y-%m-%d %H:%M:%S";


TEST(testTS3, TestSystemEvent)
{
	struct timespec	sp;
	clock_gettime(CLOCK_MONOTONIC, &sp);
	ts3::DateTime<ts3::duration::us> ut(sp);
	ts3::pitch::pitchSystemEvent	sysEvt((eventCode)'O', 2, ut.count());
	char	bb[128];
	EXPECT_NE(sizeof(sysEvt), sizeof(struct pitch_msg_system_event));
	cerr << "sizeof pitch_msg_system_event: " << sizeof(pitch_msg_system_event) << std::endl;
	int enLen = sysEvt.marshal(bb, sizeof(bb));
	ASSERT_TRUE(enLen>0);
	EXPECT_EQ(enLen, sizeof(struct pitch_msg_system_event));
	EXPECT_EQ(bb[0], MSG_SYSTEM_EVENT);
	pitchSystemEvent nSys;
	EXPECT_TRUE(nSys.unmarshal(bb+1, enLen-1));
	EXPECT_EQ(nSys, sysEvt);
}

TEST(testTS3, TestSymDir)
{
	struct timespec	sp;
	clock_gettime(CLOCK_MONOTONIC, &sp);
	ts3::DateTime<ts3::duration::us> ut(sp);
	pitchSymbolDirectory	symDir('F', "cu1906", 1, 0, 2, 3, ut.count(),
					5,5, 10000, 100000);
	char	bb[128];
	// just eq, usual not
	EXPECT_NE(sizeof(symDir), sizeof(struct pitch_msg_symbol_directory));
	cerr << "sizeof pitch_msg_symbol_directory: " << sizeof(pitch_msg_symbol_directory) << std::endl;
	int enLen = symDir.marshal(bb, sizeof(bb));
	ASSERT_TRUE(enLen>0);
	EXPECT_EQ(enLen, sizeof(struct pitch_msg_symbol_directory));
	EXPECT_EQ(bb[0], MSG_SYMBOL_DIRECTORY);
	pitchSymbolDirectory nSym;
	EXPECT_TRUE(nSym.unmarshal(bb+1, enLen-1));
	EXPECT_EQ(nSym, symDir);
}


TEST(testTS3, TestSymTradeAct)
{
	struct timespec	sp;
	clock_gettime(CLOCK_MONOTONIC, &sp);
	ts3::DateTime<ts3::duration::us> ut(sp);
	symbolTradingAction	symTrade((tradingState)'P', 0, 1, 2, ut.count());
	char	bb[128];
	// just eq, usual not
	EXPECT_NE(sizeof(symTrade), sizeof(struct pitch_msg_symbol_trading_action));
	int enLen = symTrade.marshal(bb, sizeof(bb));
	ASSERT_TRUE(enLen>0);
	EXPECT_EQ(enLen, sizeof(struct pitch_msg_symbol_trading_action));
	EXPECT_EQ(bb[0], MSG_SYMBOL_TRADING_ACTION);
	symbolTradingAction nSym;
	EXPECT_TRUE(nSym.unmarshal(bb+1, enLen-1));
	EXPECT_EQ(nSym, symTrade);
}


TEST(testTS3, TestAddOrder)
{
	struct timespec	sp;
	clock_gettime(CLOCK_MONOTONIC, &sp);
	ts3::DateTime<ts3::duration::us> ut(sp);
	addOrder	symTrade((buySellIndicator)'B', 1, 2, ut.count(), 123, 11, 100);
	char	bb[128];
	// just eq, usual not
	EXPECT_NE(sizeof(symTrade), sizeof(struct pitch_msg_add_order));
	int enLen = symTrade.marshal(bb, sizeof(bb));
	ASSERT_TRUE(enLen>0);
	EXPECT_EQ(enLen, sizeof(struct pitch_msg_add_order));
	EXPECT_EQ(bb[0], MSG_ADD_ORDER);
	addOrder nSym;
	EXPECT_TRUE(nSym.unmarshal(bb+1, enLen-1));
	EXPECT_EQ(nSym, symTrade);
}


TEST(testTS3, TestExeOrder)
{
	struct timespec	sp;
	clock_gettime(CLOCK_MONOTONIC, &sp);
	ts3::DateTime<ts3::duration::us> ut(sp);
	orderExecuted	symTrade('p', 1, 2, ut.count(), 190531001, 11, 2019053001);
	char	bb[128];
	// just eq, usual not
	EXPECT_NE(sizeof(symTrade), sizeof(struct pitch_msg_order_executed));
	int enLen = symTrade.marshal(bb, sizeof(bb));
	ASSERT_TRUE(enLen>0);
	EXPECT_EQ(enLen, sizeof(struct pitch_msg_order_executed));
	EXPECT_EQ(bb[0], MSG_ORDER_EXECUTED);
	orderExecuted nSym;
	EXPECT_TRUE(nSym.unmarshal(bb+1, enLen-1));
	EXPECT_EQ(nSym, symTrade);
}


TEST(testTS3, TestExeOrderPrice)
{
	struct timespec	sp;
	clock_gettime(CLOCK_MONOTONIC, &sp);
	ts3::DateTime<ts3::duration::us> ut(sp);
	orderEexecutedWithPrice	symTrade('p', 1, 2, ut.count(), 190531001, 11, 2019053001, 200);
	char	bb[128];
	// just eq, usual not
	EXPECT_NE(sizeof(symTrade), sizeof(struct pitch_msg_order_executed_with_price));
	int enLen = symTrade.marshal(bb, sizeof(bb));
	ASSERT_TRUE(enLen>0);
	EXPECT_EQ(enLen, sizeof(struct pitch_msg_order_executed_with_price));
	EXPECT_EQ(bb[0], MSG_ORDER_EXECUTED_WITH_PRICE);
	orderEexecutedWithPrice nSym;
	EXPECT_TRUE(nSym.unmarshal(bb+1, enLen-1));
	EXPECT_EQ(nSym, symTrade);
}


TEST(testTS3, TestOrderCancel)
{
	struct timespec	sp;
	clock_gettime(CLOCK_MONOTONIC, &sp);
	ts3::DateTime<ts3::duration::us> ut(sp);
	orderCancel	symTrade((const cancelCode)'U', 1, 2, ut.count(), 190531001, 11);
	char	bb[128];
	// just eq, usual not
	EXPECT_NE(sizeof(symTrade), sizeof(struct pitch_msg_order_cancel));
	int enLen = symTrade.marshal(bb, sizeof(bb));
	ASSERT_TRUE(enLen>0);
	EXPECT_EQ(enLen, sizeof(struct pitch_msg_order_cancel));
	EXPECT_EQ(bb[0], MSG_ORDER_CANCEL);
	orderCancel nSym;
	EXPECT_TRUE(nSym.unmarshal(bb+1, enLen-1));
	EXPECT_EQ(nSym, symTrade);
}


TEST(testTS3, TestOrderDel)
{
	struct timespec	sp;
	clock_gettime(CLOCK_MONOTONIC, &sp);
	ts3::DateTime<ts3::duration::us> ut(sp);
	orderDelete	symTrade((const cancelCode)'U', 1, 2, ut.count(), 190531001);
	char	bb[128];
	// just eq, usual not
	EXPECT_NE(sizeof(symTrade), sizeof(struct pitch_msg_order_delete));
	int enLen = symTrade.marshal(bb, sizeof(bb));
	ASSERT_TRUE(enLen>0);
	EXPECT_EQ(enLen, sizeof(struct pitch_msg_order_delete));
	EXPECT_EQ(bb[0], MSG_ORDER_DELETE);
	orderDelete nSym;
	EXPECT_TRUE(nSym.unmarshal(bb+1, enLen-1));
	EXPECT_EQ(nSym, symTrade);
}


TEST(testTS3, TestOrderReplace)
{
	struct timespec	sp;
	clock_gettime(CLOCK_MONOTONIC, &sp);
	ts3::DateTime<ts3::duration::us> ut(sp);
	orderReplace	symTrade(1, 2, ut.count(), 123, 190531001, 11, 100);
	char	bb[128];
	// just eq, usual not
	EXPECT_NE(sizeof(symTrade), sizeof(struct pitch_msg_order_replace));
	int enLen = symTrade.marshal(bb, sizeof(bb));
	ASSERT_TRUE(enLen>0);
	EXPECT_EQ(enLen, sizeof(struct pitch_msg_order_replace));
	EXPECT_EQ(bb[0], MSG_ORDER_REPLACE);
	orderReplace nSym;
	EXPECT_TRUE(nSym.unmarshal(bb+1, enLen-1));
	EXPECT_EQ(nSym, symTrade);
	auto msgP = ts3::pitch::unmarshal(bb, enLen);
	ASSERT_NE(msgP, nullptr);
	EXPECT_EQ(msgP->MessageType(), MSG_ORDER_REPLACE);
	auto tp=std::static_pointer_cast<orderReplace>(msgP);
	EXPECT_EQ(nSym, *tp);
}


TEST(testTS3, TestTrade)
{
	struct timespec	sp;
	clock_gettime(CLOCK_MONOTONIC, &sp);
	ts3::DateTime<ts3::duration::us> ut(sp);
	msgTrade	symTrade((buySellIndicator)'B', 1, 2, ut.count(), 190531001, 11, 100, 20190531001);
	char	bb[128];
	// just eq, usual not
	EXPECT_NE(sizeof(symTrade), sizeof(struct pitch_msg_trade));
	int enLen = symTrade.marshal(bb, sizeof(bb));
	ASSERT_TRUE(enLen>0);
	EXPECT_EQ(enLen, sizeof(struct pitch_msg_trade));
	EXPECT_EQ(bb[0], MSG_TRADE);
	msgTrade nSym;
	EXPECT_TRUE(nSym.unmarshal(bb+1, enLen-1));
	EXPECT_EQ(nSym, symTrade);
	auto msgP = ts3::pitch::unmarshal(bb, enLen);
	ASSERT_NE(msgP, nullptr);
	EXPECT_EQ(msgP->MessageType(), MSG_TRADE);
	auto tp=std::static_pointer_cast<msgTrade>(msgP);
	EXPECT_EQ(nSym, *tp);
}


TEST(testTS3, TestCrossTrade)
{
	struct timespec	sp;
	clock_gettime(CLOCK_MONOTONIC, &sp);
	ts3::DateTime<ts3::duration::us> ut(sp);
	crossTrade	symTrade((u8)'X', 1, 2, ut.count(), 11, 50100, 50000, 100, 20190531001);
	char	bb[128];
	// just eq, usual not
	EXPECT_NE(sizeof(symTrade), sizeof(struct pitch_msg_cross_trade));
	int enLen = symTrade.marshal(bb, sizeof(bb));
	ASSERT_TRUE(enLen>0);
	EXPECT_EQ(enLen, sizeof(struct pitch_msg_cross_trade));
	EXPECT_EQ(bb[0], MSG_CROSS_TRADE);
	crossTrade nSym;
	EXPECT_TRUE(nSym.unmarshal(bb+1, enLen-1));
	EXPECT_EQ(nSym, symTrade);
	auto msgP = ts3::pitch::unmarshal(bb, enLen);
	ASSERT_NE(msgP, nullptr);
	EXPECT_EQ(msgP->MessageType(), MSG_CROSS_TRADE);
	auto tp=std::static_pointer_cast<crossTrade>(msgP);
	EXPECT_EQ(nSym, *tp);
}

int main(int argc,char *argv[])
{
    testing::InitGoogleTest(&argc, argv);//将命令行参数传递给gtest
    return RUN_ALL_TESTS();   //RUN_ALL_TESTS()运行所有测试案例
}
