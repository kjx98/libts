#include "crayUDP.hpp"

using namespace crayUDP;
using namespace ts3;

crayUDP_header head0(20190418,1,2);
u8	headBytes[16];
u8	msgBuf0[256];
size_t msg0Len=256;
size_t msg1Len=10;
size_t msg2Len=220;
size_t msg3Len = 222;
message_t msgs[3];

void initTest() {
	memcpy(headBytes, &head0, sizeof(head0));
	memset(msgBuf0, 0, sizeof(msgBuf0));
	msgBuf0[0] = 8;
	msgBuf0[10] = 208;
	msgBuf0[222] = 64;
	msgs[0].unmarshal(&msgBuf0[2], 8);
	msgs[1].unmarshal(&msgBuf0[12], 208);
	msgs[2].unmarshal(nullptr, 0);
}
