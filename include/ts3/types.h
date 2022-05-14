#pragma once
#ifndef	__TS3_TYPES_H__
#define	__TS3_TYPES_H__

#ifdef	__linux__
#include <endian.h>
#if	__BYTE_ORDER == __LITTLE_ENDIAN
#define	BOOST_LITTLE_ENDIAN
#endif
#else	//	__linux__
#include <boost/version.hpp>
#if	BOOST_VERSION > 106500
#include <boost/predef/other/endian.h>
#if	defined(BOOST_ENDIAN_LITTLE_BYTE) && !defined(BOOST_LITTLE_ENDIAN)
#define	BOOST_LITTLE_ENDIAN
#endif
#else	// BOOST_VERSION
#include <boost/detail/endian.hpp>
#endif
#endif	//	__linux__
#include <stdint.h>
#include <cstring>
#include <string>
#include "cdefs.h"


#ifdef __CHECKER__
#define bitwise __attribute__((bitwise))
#else
#define bitwise
#endif
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint16_t bitwise le16;
typedef uint32_t bitwise le32;
typedef uint64_t bitwise le64;

typedef uint16_t bitwise be16;
typedef uint32_t bitwise be32;
typedef uint64_t bitwise be64;
#undef bitwise


namespace	ts3 {
class int48_t {
public:
	int48_t(): loword_(0), hiword_(0) {}
	// hiword is 16 bits, no need & 0xffff
	int48_t(int64_t v): loword_(v & 0xffffffff), hiword_(v >>32) {}
	int48_t(const int48_t &) = default;
	int64_t int64() const
	{
		int64_t v= ((int64_t)hiword_) << 32;
		v |= loword_;
		return v;
	}
	friend bool operator==(const int64_t &v64, const int48_t &v48) {
		return v64 == v48.int64();
	}
	friend bool operator==(const int48_t &v48, const int64_t &v64) {
		return v48.int64() == v64;
	}
	bool operator==(const int48_t &v) {
		return (loword_ == v.loword_ && hiword_ == v.hiword_);
	}
private:
	uint32_t	loword_;
	int16_t		hiword_;
} __attribute__((packed));


uint32_t forceinline __hash(const u8* data, size_t len)
{
	uint32_t	hash(2166136261);
	for (size_t i=0;i<len;++i) {
		hash = (16777619 * hash) ^ data[i];
	}
	return hash;
}

template<size_t nSize>
class pstring {
public:
	pstring() {
		static_assert(nSize > 0 && nSize < 256, "size must between 0 and 255");
		sBuf_[0] = 0;
	}
	pstring(const pstring &) = default;
	pstring(const char *ss, const size_t sLen=0) noexcept
	{
		static_assert(nSize > 0 && nSize < 256, "size must between 0 and 255");
		size_t ll = sLen;
		if (ll == 0) ll = strlen(ss);
		if (ts3_unlikely(ll > nSize-1)) ll = nSize-1;
		sBuf_[0] = ll;
		memcpy(sBuf_+1, ss, ll);
	}
	pstring(const std::string &ss) noexcept
	{
		static_assert(nSize > 0 && nSize < 256, "size must between 0 and 255");
		auto ll = ss.size();
		if (ts3_unlikely(ll > nSize-1)) ll = nSize-1;
		sBuf_[0] = ll;
		memcpy(sBuf_+1, ss.data(), ll);
	}
	struct hash
	{
		size_t operator()(pstring const &s) const noexcept {
			return ts3::__hash(s.data(), s.size());
		}
	};
	std::string String() const noexcept {
		return std::string(data(), size());
	}
	char *data() const noexcept { return (char *)&sBuf_[1]; }
	size_t size() const noexcept { return sBuf_[0]; }
	size_t length() const noexcept { return sBuf_[0]; }
	friend bool operator==(const pstring &lhs, const pstring& rhs) noexcept
	{
		return memcmp(lhs.data(), rhs.data(), nSize) == 0;
	}
	friend bool operator==(const pstring &lhs, const std::string& rhs) noexcept
	{
		if (ts3_unlikely(lhs.size() != rhs.size())) return false;
		return memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
	}
	friend bool operator==(const std::string &lhs, const pstring& rhs) noexcept
	{
		if (ts3_unlikely(lhs.size() != rhs.size())) return false;
		return memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
	}
	friend bool operator<(const pstring& lhs, const pstring& rhs) noexcept
	{
		return memcmp(lhs.data(), rhs.data(), nSize) < 0;
	}
private:
	uint8_t	sBuf_[nSize];
};

}

#endif	// __TS3_TYPES_H__
