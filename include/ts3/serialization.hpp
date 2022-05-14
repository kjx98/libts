#pragma once
#ifndef	__TS3_SERIALIZATION_HPP__
#define	__TS3_SERIALIZATION_HPP__

#include <type_traits>
#include "types.h"

namespace ts3 {

class Serialization {
public:
	Serialization() = default;
	Serialization(const Serialization &sr) : bSize_(sr.bSize_),
		off_(sr.off_), err_(sr.err_), bufp_(sr.bufp_) {}
	Serialization(const void *bufP, int bSize): bSize_(bSize),
		bufp_((u8 *)bufP) {}
	size_t	size() const { return off_; }
	void *data() const { return bufp_; }
	bool error() const { return err_; }
	template<size_t nSiz>bool encode(const pstring<nSiz> & ss) noexcept {
		if (ts3_unlikely(err_)) return !err_;
		if (ss.size() == 0) return encode1b(0);
		if (ts3_unlikely(off_ + (int)ss.size() > bSize_)) {
			err_ = true;
			return !err_;
		}
		encode1b(ss.size());
		memcpy(bufp_ + off_, ss.data(), ss.size());
		off_ += ss.size();
		return true;
	}
	template<typename T>bool encode(const T v) noexcept {
		static_assert(std::is_integral<T>::value, "Integral required.");
		if (ts3_unlikely(err_)) return !err_;
		const int	vLen=sizeof(T);
		if (ts3_unlikely(off_ + vLen > bSize_)) {
			err_ = true;
			return !err_;
		}
		memcpy(bufp_ + off_, &v, vLen);
		off_ += vLen;
		return true;
	}
	template<typename T, typename... Args>
	bool encode(const T v, const Args... args) noexcept
	{
		if (ts3_unlikely(!encode(v))) return false;
		return encode(args...);
	}
	bool encode1b(const u8 v) noexcept {
		if (ts3_unlikely(err_)) return !err_;
		if (ts3_unlikely(off_ >= bSize_)) {
			err_ = true;
			return !err_;
		}
		bufp_[off_++] = v;
		return true;
	}
	bool encode(const std::string& ss) noexcept {
		if (ts3_unlikely(err_)) return !err_;
		if (ss.size() == 0) return encode1b(0);
		if (ts3_unlikely(off_ + (int)ss.size() > bSize_)) {
			err_ = true;
			return !err_;
		}
		encode1b(ss.size());
		memcpy(bufp_ + off_, ss.data(), ss.size());
		off_ += ss.size();
		return true;
	}
	bool encodeBytes(const u8 *buf, const int bLen) noexcept {
		if (ts3_unlikely(err_)) return !err_;
		assert(bLen>0);
		if (ts3_unlikely(off_ + bLen > bSize_)) {
			err_ = true;
			return !err_;
		}
		memcpy(bufp_+off_, buf, bLen);
		off_ += bLen;
		return true;
	}
	template<size_t nSiz>bool decode(pstring<nSiz> & ss) noexcept {
		if (ts3_unlikely(err_)) return !err_;
		if (ts3_unlikely(off_ >= bSize_)) {
			err_ = true;
			return !err_;
		}
		size_t sLen = bufp_[off_++];
		if (sLen == 0) {
			ss = pstring<nSiz>();
			return true;
		}
		if (ts3_unlikely(sLen > nSiz || off_ + (int)sLen > bSize_)) {
			err_ = true;
			return !err_;
		}
		ss = pstring<nSiz>((const char *)(bufp_+off_), sLen);
		off_ += sLen;
		return true;
	}
	template<typename T>bool decode(T& v) noexcept {
		static_assert(std::is_integral<T>::value, "Integral required.");
		if (ts3_unlikely(err_)) return !err_;
		const int	vLen=sizeof(T);
		if (ts3_unlikely(off_ + vLen > bSize_)) {
			err_ = true;
			return !err_;
		}
		memcpy(&v, bufp_ + off_, vLen);
		off_ += vLen;
		return true;
	}
	template<typename T, typename... Args>bool
	decode(T& v, Args &... args) noexcept
	{
		decode(v);
		if (ts3_unlikely(err_)) return !err_;
		decode(args...);
		return !err_;
	}
	bool decode1b(u8& v) noexcept {
		if (ts3_unlikely(err_)) return !err_;
		if (ts3_unlikely(off_ >= bSize_)) {
			err_ = true;
			return !err_;
		}
		v = bufp_[off_++];
		return true;
	}
	bool decode(std::string& ss) noexcept {
		if (ts3_unlikely(err_)) return !err_;
		if (ts3_unlikely(off_ >= bSize_)) {
			err_ = true;
			return !err_;
		}
		size_t sLen = bufp_[off_++];
		if (sLen == 0) {
			ss = std::string();
			return true;
		}
		if (ts3_unlikely(off_ + (int)sLen > bSize_)) {
			err_ = true;
			return !err_;
		}
		ss = std::string((const char *)(bufp_+off_), sLen);
		off_ += sLen;
		return true;
	}
	bool decodeBytes(u8 *buf, const int bLen) noexcept {
		if (ts3_unlikely(err_)) return !err_;
		assert(bLen>0);
		if (ts3_unlikely(off_ + bLen > bSize_)) {
			err_ = true;
			return !err_;
		}
		memcpy(buf, bufp_+off_, bLen);
		off_ += bLen;
		return true;
	}
private:
	int		bSize_ = 0;
	int		off_ = 0;
	bool	err_ = false;
	u8		*bufp_ = nullptr;
};

}
#endif	//	__TS3_SERIALIZATION_HPP_
