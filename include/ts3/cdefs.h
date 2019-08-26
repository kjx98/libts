#pragma once
// \file ts3/cdefs.h
///
/// libts 的一些基本设置.
///
/// \author fonwinz@gmail.com

/// \defgroup AlNum		字母数字
/// \defgroup Thrs		Thread
/// \defgroup Misc		其他工具
/// \defgroup ts3_MACRO	宏定义

#ifndef	__TS3_CDEFS_H__
#define	__TS3_CDEFS_H__

#include <cassert>
// force MSVC using utf8 output
#ifdef	_MSC_VER
#pragma execution_character_set("UTF-8")
#endif

#ifdef	__GNUC__
# define ts3_unlikely(cond)	__builtin_expect ((cond), 0)
# define ts3_likely(cond)	__builtin_expect (!!(cond), 1)
#define forceinline __inline__ __attribute__((always_inline))
#else
# define ts3_unlikely(cond)	(cond)
# define ts3_likely(cond)	(cond)
#ifdef _MSC_VER
#define forceinline __forceinline
#else
#define forceinline
#endif
#endif


/// \ingroup ts3_MACRO
#define ts3_NON_COPYABLE(className)        \
	className(const className& r) = delete; \
	className& operator=(const className& r) = delete;

/// \ingroup ts3_MACRO
#define ts3_NON_MOVEABLE(className)         \
	className(const className&& r) = delete; \
	className& operator=(const className&& r) = delete;

/// \ingroup ts3_MACRO
/// 使用巨集方式設定「禁止複製及move」.
#define ts3_NON_COPY_NON_MOVE(className) \
	ts3_NON_COPYABLE(className)           \
	ts3_NON_MOVEABLE(className)

#endif	// __TS3_CDEFS_H__
