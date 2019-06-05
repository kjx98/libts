#pragma once
#ifndef	__TS3_MESSAGE__
#define	__TS3_MESSAGE__

#include <string.h>
#include <assert.h>
#include "ts3/types.h"

namespace ts3 {
class message_t {
public:
	bool operator==(const message_t &msg) const {
		if (bValid_ != msg.bValid_) return false;
		if (! bValid_ ) return true;
		if (length_ != msg.length_) return false;
		if (length_ == 0) return true;
		return memcmp(dataPtr_, msg.dataPtr_, length_) == 0;
	}
	message_t& operator=(const message_t &msg) {
		if (this != &msg) {
			bValid_ = msg.bValid_;
			if (bValid_) {
				length_ = msg.length_;
				dataPtr_ = msg.dataPtr_;
			} else {
				length_ = 0;
				dataPtr_ = nullptr;
			}
		}
		return *this;
	}
	// return true, if successfully marshal
	bool marshal(u8 *buff, size_t& buflen) {
		if ( !bValid_ ) return false;
		if (buflen < sizeof(length_)+length_) return false;
		buflen = length_;
		auto lPtr = (u16 *)buff;
		*lPtr = length_;
		memcpy(buff+sizeof(length_), dataPtr_, length_);
		return true;
	}
	bool unmarshal(void *buff, size_t buflen) {
		length_ = buflen;
		dataPtr_ = (buflen>0)?buff:nullptr;
		bValid_ = true;
		return true;
	}
	bool isNil() {
		return !bValid_;
	};
	size_t size() { return length_; }
	const void * data() { return dataPtr_; }
	message_t(const void *buff, size_t buflen) {
		assert(buflen >= 0);
		length_ = buflen;
		dataPtr_ = buff;
		bValid_ = true;
	};
	message_t() {
		bValid_ = false;
		length_ = 0;
		dataPtr_ = nullptr;
	};
	message_t(const message_t &msg) {
		bValid_ = msg.bValid_;
		if (bValid_) {
			length_ = msg.length_;
			dataPtr_ = msg.dataPtr_;
		} else {
			length_ = 0;
			dataPtr_ = nullptr;
		}
	}
private:
	u16		length_;	// le16
	bool	bValid_;
	const void *dataPtr_;
};

}
#endif	//	__TS3_MESSAGE__
