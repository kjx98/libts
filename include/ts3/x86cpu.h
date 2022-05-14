#pragma once
#ifndef	__TS3_X86CPU_H__
#define	__TS3_X86CPU_H__

#ifdef	__x86_64__
#include <cpuid.h>
#include <x86intrin.h>
#include "cdefs.h"

namespace	ts3 {

// cpuid 8000-0001(EXTENDED_FEATURES), edx bit 27, CPU_FEATURE_RDTSCP
// cpuid 8000-0001(EXTENDED_FEATURES), edx bit 26, CPU_FEATURE_PDPE1Gb
forceinline unsigned int x86_ext_feature81() {
#ifndef	ommit
	unsigned int eax, ebx, ecx, edx = 0;
	__cpuid(0x80000001, eax, ebx, ecx, edx);
#else
	unsigned int edx = 0;
	asm volatile("cpuid\n"
			: "=d" (edx)
			: "a" (0x80000001)
			: "%rbx", "%rcx");
#endif
	return edx;
}

forceinline bool check_rdtscp() noexcept {
	return x86_ext_feature81() & (1 << 27);	
}

forceinline bool check_pdpe1gb() noexcept {
	return x86_ext_feature81() & (1 << 26);	
}

forceinline uint64_t x86_cpuid() noexcept {
	unsigned int edx = 0, eax = 0;
	asm volatile("cpuid\n"
			: "=d" (edx), "=a" (eax)
			: "a" (0x01)
			: "%rbx", "%rcx");
	uint64_t res=edx;
	res <<= 32;
	res |= (uint64_t)eax;
	return __builtin_bswap64(res);
}

}

#endif	// __x86_64__
#endif	// __TS3_X86CPU_H__
