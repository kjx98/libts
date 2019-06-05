#include "moldudp.hpp"

using namespace moldUDP;
using namespace ts3;

moldudp64_header head0={"2019041801", 1, 2};
u8	headBytes[20]="2019041801";
u8	msgBuf0[256];
size_t msg0Len=256;
size_t msg1Len=10;
size_t msg2Len=220;
size_t msg3Len = 222;
message_t msgs[3];

void initTest() {
	memset(headBytes+10, 0, 10);
	headBytes[17] = 1;
	headBytes[19] = 2;
	memset(msgBuf0, 0, sizeof(msgBuf0));
	msgBuf0[1] = 8;
	msgBuf0[11] = 208;
	msgBuf0[223] = 64;
	msgs[0].unmarshal(&msgBuf0[2], 8);
    msgs[1].unmarshal(&msgBuf0[12], 208);
    msgs[2].unmarshal(nullptr, 0);
}
