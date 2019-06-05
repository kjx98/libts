#pragma once

#include "ts3/types.h"
#include <string.h>
#include "ts3/message.hpp"

using namespace ts3;

namespace moldUDP
{
/*
 * MoldUDP64
 */

class session_t {
	char	id[10];
public:
	inline bool operator==(const session_t &rh) const {
		return memcmp(id, rh.id, sizeof(id))==0;
	};
	inline bool operator==(const int rh) const {
		if (rh != 0) return false;
		auto p64=reinterpret_cast<const u64 *>(&id[0]);
		if (*p64 != 0) return false;
		auto p16=reinterpret_cast<const u16 *>(&id[8]);
		return *p16 == 0;
	};
	inline bool operator!=(const session_t &rh) const {
		return memcmp(id, rh.id, sizeof(id))!=0;
	};
	session_t() {
		memset(this, 0, sizeof(*this));
	};
	session_t( const session_t &rh) {
		memcpy(id, rh.id, sizeof(id));
	};
	session_t( const char *rh) {
		memcpy(id, rh, sizeof(id));
	}
	inline session_t & operator=(const session_t rh) {
		if (this != &rh) {
			memcpy(id, rh.id, sizeof(id));
		}
		return *this;
	}
}  __attribute__((packed));

struct moldudp64_header {
	bool inline verifySize(size_t bufLen) noexcept {
		return bufLen >= sizeof(moldudp64_header);
	}
	moldudp64_header *decodeInline(void *buff, size_t bufLen) noexcept {
		if (bufLen < sizeof(moldudp64_header)) return nullptr;
		moldudp64_header *tp=(moldudp64_header *)buff;
		tp->SequenceNumber = be64toh(tp->SequenceNumber);
		tp->MessageCount = be16toh(tp->MessageCount);
		return tp;
	}
	bool decode(void *buff, size_t bufLen) noexcept {
		if (bufLen < sizeof(moldudp64_header)) return false;
		moldudp64_header *tp=(moldudp64_header *)buff;
		Session =  tp->Session;
		SequenceNumber = be64toh(tp->SequenceNumber);
		MessageCount = be16toh(tp->MessageCount);
		return true;
	}
	bool encodeInline() noexcept {
		this->SequenceNumber = htobe64(this->SequenceNumber);
		this->MessageCount = htobe16(this->MessageCount);
		return true;
	}
	bool encode(void *buff, size_t bufLen) noexcept {
		if (bufLen < sizeof(moldudp64_header)) return false;
		moldudp64_header *tp=(moldudp64_header *)buff;
		tp->Session = Session;
		tp->SequenceNumber = htobe64(SequenceNumber);
		tp->MessageCount = htobe16(MessageCount);
		return true;
	};
	inline bool operator==(const moldudp64_header &head) const {
		if (SequenceNumber != head.SequenceNumber) return false;
		if (MessageCount != head.MessageCount) return false;
		return Session == head.Session;
	};
	moldudp64_header(const session_t sess, const u64 seqNo=1, const u16 nMsg=0):
			Session(sess),
			SequenceNumber(seqNo),
			MessageCount(nMsg)
	{
	};
	moldudp64_header() {}
	session_t	Session;
	u64			SequenceNumber;	// be64
	u16			MessageCount;	// be16
} __attribute__((packed));


// same struct as moldudp64_header
using moldudp64_request = struct moldudp64_header;

static inline int marshal(u8 *buff, size_t& bufLen, message_t *msgs, int nMsgs)
{
	auto n = bufLen;
	int	i;
	bufLen = 0;
	for (i=0;i<nMsgs;i++) {
		auto mLen = msgs[i].size();
		if (bufLen+sizeof(u16)+mLen > n) break;
		*(u16 *)(buff+bufLen) = htobe16(mLen);
		bufLen += sizeof(u16);
		if (mLen > 0) {
			memcpy(buff+bufLen, msgs[i].data(), mLen);
			bufLen += mLen;
		}
	}
	return i;
}

static inline int unmarshal(u8 *buff, size_t bufLen, message_t *msgs, int nMsgs)
{
	if (bufLen == 0) return 0;
	int i=0;
	size_t off;
	for(off=0;off<bufLen;) {
		if (off + sizeof(u16) > bufLen) return -1;
		u16 length = be16toh(*(u16 *)(buff+off));
		off += sizeof(u16);
		if (off + length > bufLen) return -1;
		msgs[i++].unmarshal(buff+off, length);
		off += length;
		if (i == nMsgs) break;
	}
	if (off != bufLen) return -1;
	return i;
}
}
