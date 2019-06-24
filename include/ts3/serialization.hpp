#pragma once
#ifndef	__TS3_SERIALIZATION__
#define	__TS3_SERIALIZATION__

#include <string.h>
#include <cassert>
#include <type_traits>
#include "ts3/types.h"

namespace ts3 {

const	int	defBufSize=64;
class Serialization {
public:
	Serialization() = default;
	Serialization(const Serialization &) = default;
	Serialization(const void *bufP, int bSize): bSize_(bSize),
		bufp_((u8 *)bufP) {}
	int	Size() { return off_; }
	void *Data() { return bufp_; }
	bool Error() { return err_; }
	template<size_t nSiz>bool encode(const pstring<nSiz> & ss) {
		if (err_) return !err_;
		if (ss.size() == 0) return encode1b(0);
		if (off_ + (int)ss.size() > bSize_) {
			err_ = true;
			return !err_;
		}
		encode1b(ss.size());
		memcpy(bufp_ + off_, ss.data(), ss.size());
		off_ += ss.size();
		return true;
	}
	template<typename T>bool encode(const T v) {
		static_assert(std::is_integral<T>::value, "Integral required.");
		if (err_) return !err_;
		const int	vLen=sizeof(T);
		if (off_ + vLen > bSize_) {
			err_ = true;
			return !err_;
		}
		memcpy(bufp_ + off_, &v, vLen);
		off_ += vLen;
		return true;
	}
	template<typename T, typename... Args>bool encode(const T v, const Args... args) {
		if (!encode(v)) return false;
		return encode(args...);
	}
	bool encode1b(const u8 v) {
		if (err_) return !err_;	
		if (off_ >= bSize_) {
			err_ = true;
			return !err_;
		}
		bufp_[off_++] = v;
		return true;
	}
	bool encode(const std::string& ss) {
		if (err_) return !err_;
		if (ss.size() == 0) return encode1b(0);
		if (off_ + (int)ss.size() > bSize_) {
			err_ = true;
			return !err_;
		}
		encode1b(ss.size());
		memcpy(bufp_ + off_, ss.data(), ss.size());
		off_ += ss.size();
		return true;
	}
	bool encodeBytes(const u8 *buf, const int bLen) {
		if (err_) return !err_;
		assert(bLen>0);
		if (off_ + bLen > bSize_) {
			err_ = true;
			return !err_;
		}
		memcpy(bufp_+off_, buf, bLen); \
		off_ += bLen;
		return true;
	}
	template<size_t nSiz>bool decode(pstring<nSiz> & ss) {
		if (err_) return !err_;
		if (off_ >= bSize_) {
			err_ = true;
			return !err_;
		}
		size_t sLen = bufp_[off_++];
		if (sLen == 0) {
			ss = pstring<nSiz>();
			return true;
		}
		if (sLen > nSiz || off_ + (int)sLen > bSize_) {
			err_ = true;
			return !err_;
		}
		ss = pstring<nSiz>((const char *)(bufp_+off_), sLen);
		off_ += sLen;
		return true;
	}
	template<typename T>bool decode(T& v) {
		static_assert(std::is_integral<T>::value, "Integral required.");
		if (err_) return !err_;
		const int	vLen=sizeof(T);
		if (off_ + vLen > bSize_) {
			err_ = true;
			return !err_;
		}
		memcpy(&v, bufp_ + off_, vLen);
		off_ += vLen;
		return true;
	}
	template<typename T, typename... Args>bool decode(T& v, Args &... args) {
		decode(v);
		if (err_) return !err_;	
		decode(args...);
		return !err_;
	}
	bool decode1b(u8& v) {
		if (err_) return !err_;	
		if (off_ >= bSize_) {
			err_ = true;
			return !err_;
		}
		v = bufp_[off_++];
		return true;
	}
	bool decode(std::string& ss) {
		if (err_) return !err_;
		if (off_ >= bSize_) {
			err_ = true;
			return !err_;
		}
		size_t sLen = bufp_[off_++];
		if (sLen == 0) {
			ss = std::string();
			return true;
		}
		if (off_ + (int)sLen > bSize_) {
			err_ = true;
			return !err_;
		}
		ss = std::string((const char *)(bufp_+off_), sLen);
		off_ += sLen;
		return true;
	}
	bool decodeBytes(u8 *buf, const int bLen) {
		if (err_) return !err_;
		assert(bLen>0);
		if (off_ + bLen > bSize_) {
			err_ = true;
			return !err_;
		}
		memcpy(buf, bufp_+off_, bLen); \
		off_ += bLen;
		return true;
	}
private:
	int		bSize_ = defBufSize;
	int		off_ = 0;
	bool	err_ = false;
	u8		*bufp_ = sBuff;
	u8		sBuff[defBufSize];
};

}
#endif	//	__TS3_SERIALIZATION__
