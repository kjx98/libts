#pragma once
#ifndef	__TS3_TYPES__
#define	__TS3_TYPES__

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

class int48_t {
public:
	int48_t(): loword_(0), hiword_(0) {}
	// hiword is 16 bits, no need & 0xffff
	int48_t(int64_t v): loword_(v & 0xffffffff), hiword_(v >>32) {}
	int48_t& operator=(const int48_t &v) {
		if (this != &v) {
			loword_ = v.loword_;
			hiword_ = v.hiword_;
		}
		return *this;
	}
	int64_t int64() const {
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

#undef bitwise

#endif	// __TS3_TYPES__
