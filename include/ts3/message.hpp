#pragma once
#ifndef	__TS3_MESSAGE_HPP__
#define	__TS3_MESSAGE_HPP__

#include <cassert>
#include "types.h"

namespace ts3 {
class message_t {
public:
	bool operator==(const message_t &msg) const noexcept {
		if (bValid_ != msg.bValid_) return false;
		if ( ts3_unlikely(!bValid_) ) return true;
		if (length_ != msg.length_) return false;
		if (ts3_unlikely(length_ == 0)) return true;
		return memcmp(dataPtr_, msg.dataPtr_, length_) == 0;
	}
	// return true, if successfully marshal
	bool marshal(u8 *buff, size_t& buflen) noexcept {
		if ( ts3_unlikely(!bValid_) ) return false;
		if (ts3_unlikely(buflen < sizeof(length_)+length_)) return false;
		buflen = length_;
		u16	bLen = htole16(length_);
		memcpy(buff, &bLen, sizeof(u16));
		memcpy(buff+sizeof(u16), dataPtr_, length_);
		return true;
	}
	bool unmarshal(void *buff, size_t buflen) noexcept {
		length_ = buflen;
		dataPtr_ = (buflen>0)?buff:nullptr;
		bValid_ = true;
		return true;
	}
	bool isNil() const {
		return !bValid_;
	};
	const size_t size() const noexcept { return length_; }
	char * data() const noexcept { return (char *)dataPtr_; }
	message_t(const void *buff, size_t buflen) noexcept {
		assert(buflen >= 0);
		length_ = buflen;
		dataPtr_ = buff;
		bValid_ = true;
	};
	message_t() = default;
	message_t(const message_t &msg) = default;
private:
	u16		length_ = 0;	// le16
	bool	bValid_ = false;
	const void *dataPtr_ = nullptr;
};


class alignas(64) CLmessage {
public:
	bool operator==(const CLmessage &msg) const noexcept {
		if (length_ != msg.length_) return false;
		if (ts3_unlikely(length_ == 0)) return true;
		return memcmp(buf_, msg.buf_, length_) == 0;
	}
	explicit operator bool () {
		return length_ != 0;
	}
	bool isNil() const {
		return length_ == 0;
	};
	const size_t size() const noexcept { return length_; }
	char * data() const noexcept { return (char *)buf_; }
	void clear() noexcept { length_ = 0; }
	void SetSize(int v) noexcept {
		if (v > 0 && v <= (int)sizeof(buf_)) length_ = v;
	};
	size_t cap() const { return sizeof(buf_); }
	CLmessage() : length_(0) {
		static_assert(sizeof(*this) == 64, "sizeof CLmessage MUST be 64");
	}
	CLmessage(const CLmessage &msg) noexcept : length_(msg.length_) {
		static_assert(sizeof(msg) == 64, "sizeof CLmessage MUST be 64");
		if (ts3_unlikely(length_ > sizeof(buf_))) length_ = sizeof(buf_);
		if (ts3_likely(length_ > 0)) memcpy(buf_, msg.buf_, length_);
	}
	CLmessage(const void *data, const size_t ll) noexcept : length_(ll) {
		if (ts3_unlikely(length_ > sizeof(buf_))) length_ = sizeof(buf_);
		if (ts3_likely(length_ > 0)) memcpy(buf_, data, length_);
	}
	// return true, if successfully marshal
	bool marshal(u8 *buff, size_t& buflen) noexcept {
		if ( ts3_unlikely(length_ == 0) ) return false;
		if (ts3_unlikely(buflen < sizeof(length_)+length_)) return false;
#ifdef	BOOST_LITTLE_ENDIAN11
		memcpy(buff, &length_, length_+sizeof(length_));
#else
		u16	bLen = htole16(length_);
		memcpy(buff, &bLen, sizeof(u16));
		memcpy(buff+sizeof(u16), buf_, length_);
#endif
		return true;
	}
	bool unmarshal(void *buff, size_t buflen) noexcept {
		length_ = buflen;
		if ( ts3_unlikely(length_ >= sizeof(buf_)) ) {
			length_ = 0;
			return false;
		}
		if ( ts3_likely(length_>0) ) memcpy(buf_, buff, length_);
		return true;
	}
private:
	u16		length_ = 0;
	u8		buf_[62];
};

}
#endif	//	__TS3_MESSAGE_HPP__
