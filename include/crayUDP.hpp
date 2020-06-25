#pragma once

#include "ts3/cdefs.h"
#include "ts3/message.hpp"

// for header, memcpy slower than 3 integer assign
using namespace ts3;

namespace crayUDP
{
/*
 * CrayUDP
 */

typedef	u32	session_t;

class crayUDP_header {
public:
	bool inline verifySize(size_t bufLen) noexcept {
		return bufLen >= sizeof(crayUDP_header);
	}
	crayUDP_header *decodeInline(void *buff, size_t bufLen) noexcept {
		if (ts3_unlikely(bufLen < sizeof(crayUDP_header))) return nullptr;
		crayUDP_header *tp=(crayUDP_header *)buff;
#ifndef	BOOST_LITTLE_ENDIAN
		tp->session_ = le32toh(tp->session_);
		tp->crc16_ = le16toh(tp->crc16_);
		tp->msgCount_ = le16toh(tp->msgCount_);
		tp->seqNo_ = le64toh(tp->seqNo_);
#endif
		return tp;
	}
	bool decode(void *buff, size_t bufLen) noexcept {
		if (ts3_unlikely(bufLen < sizeof(crayUDP_header))) return false;
#ifdef	BOOST_LITTLE_ENDIAN11
		memcpy(this, buff, sizeof(*this));
#else
		crayUDP_header *tp=(crayUDP_header *)buff;
		session_  = le32toh(tp->session_);
		crc16_ = le16toh(tp->crc16_);
		msgCount_ = le16toh(tp->msgCount_);
		seqNo_ = le64toh(tp->seqNo_);
#endif
		return true;
	}
	bool encodeInline() noexcept {
#ifndef	BOOST_LITTLE_ENDIAN11
		session_  = htole32(session_);
		crc16_ = htole16(crc16_);
		msgCount_ = htole16(msgCount_);
		seqNo_ = htole64(seqNo_);
#endif
		return true;
	}
	bool encode(void *buff, size_t bufLen) noexcept {
		if (ts3_unlikely(bufLen < sizeof(crayUDP_header))) return false;
#ifdef	BOOST_LITTLE_ENDIAN11
		memcpy(buff, this, sizeof(*this));
#else
		crayUDP_header *tp=(crayUDP_header *)buff;
		tp->session_ = htole32(session_);
		tp->seqNo_ = htole64(seqNo_);
		tp->crc16_ = htole16(crc16_);
		tp->msgCount_ = htole16(msgCount_);
#endif
		return true;
	}
	inline bool operator==(const crayUDP_header &head) const {
		if (seqNo_ != head.seqNo_) return false;
		if (msgCount_ != head.msgCount_) return false;
		return session_ == head.session_;
	}
	crayUDP_header() noexcept {
		memset(this, 0, sizeof(*this));
	}
	crayUDP_header(const session_t sess, const u64 seqNo, const u16 nMsg):
		session_(sess),
		crc16_(0),
		msgCount_(nMsg),
		seqNo_(seqNo)
	{
	}
	inline session_t session() const { return session_; }
	inline u16 msgCnt() const { return msgCount_; }
	u16	checkSum() const { return crc16_; }
	bool setCheckSum(const u16 newCheckSum) noexcept {
		if (crc16_ != 0) return false;
		crc16_ = newCheckSum;
		return true;
	}
	inline u64 seqNo () const { return seqNo_; }
private:
	session_t	session_;	// le32
	u16			crc16_;		// reserved
	u16			msgCount_;	// le16
	u64			seqNo_;		// le64
} __attribute__((packed));


// same struct as crayUDP_header
using crayUDP_request = crayUDP_header;

template<typename MsgType> forceinline
size_t marshal(u8 *buff, const size_t bufLen, MsgType& msg) noexcept 
{
	auto ret = 0;
	auto mLen = msg.size();
	if (ts3_unlikely(sizeof(u16)+mLen > bufLen)) return ret;
	u16 mm = htole16(mLen);
	memcpy(buff, &mm, sizeof(mm));
	ret += sizeof(u16);
	if (ts3_likely(mLen > 0)) {
		memcpy(buff+ret, msg.data(), mLen);
		ret += mLen;
	}
	return ret;
}

template<typename MsgType> forceinline
int marshal(u8 *buff, size_t& bufLen, MsgType msgs[], int nMsgs) noexcept 
{
	auto n = bufLen;
	int	i;
	bufLen = 0;
	for (i=0;i<nMsgs;++i) {
		auto mLen = msgs[i].size();
		if (ts3_unlikely(bufLen+sizeof(u16)+mLen > n)) break;
		u16 mm = htole16(mLen);
		memcpy(buff+bufLen, &mm, sizeof(mm));
		bufLen += sizeof(u16);
		if (ts3_likely(mLen > 0)) {
			memcpy(buff+bufLen, msgs[i].data(), mLen);
			bufLen += mLen;
		}
	}
	return i;
}

template<typename MsgType> forceinline
int unmarshal(u8 *buff, size_t bufLen, MsgType msgs[], int nMsgs) noexcept 
{
	if (ts3_unlikely(bufLen == 0)) return 0;
	int i=0;
	size_t off;
	for(off=0;off<bufLen;) {
		if (ts3_unlikely(off + sizeof(u16) > bufLen)) return -1;
		u16 length;
		// it's better use tmp, since RISC u16 must 16bits aligned
		memcpy((void *)&length, buff+off, sizeof(length));
		length = le16toh(length);
		off += sizeof(u16);
		if (ts3_unlikely(off + length > bufLen)) return -1;
		if (! msgs[i].unmarshal(buff+off, length) ) return -1;
		++i;
		off += length;
		if (i == nMsgs) break;
	}
	if (ts3_unlikely(off != bufLen)) return -1;
	return i;
}

}
