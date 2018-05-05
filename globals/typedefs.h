
#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <stddef.h>
/**
 * Basic definitions and simple functions to be used everywhere..
 */

#ifndef _STR
#define _STR(m_x) #m_x
#define _MKSTR(m_x) _STR(m_x)
#endif

/*
#include "version.h"

#define VERSION_MKSTRING _MKSTR(VERSION_MAJOR)"."_MKSTR(VERSION_MINOR)"."_MKSTR(VERSION_REVISION)"-"_MKSTR(VERSION_STATUS)
#define VERSION_FULL_NAME _MKSTR(VERSION_NAME)" v"VERSION_MKSTRING
*/

#if defined(__GNUC__) && (__GNUC__ >= 4 )
#    define _FORCE_INLINE_ __attribute__((always_inline)) inline
#    define _FORCE_ALIGN_ __attribute__((aligned(16)))

#elif defined(_MSC_VER)
#	define _FORCE_INLINE_ __forceinline
#error no idea how to align in MSVC, find out
#else
#    define _FORCE_INLINE_ inline
#    define _FORCE_ALIGN_
#endif


//custom, gcc-safe offsetof, because gcc complains a lot.
template<class T>
T *_nullptr() { T*t=NULL; return t; }

#define OFFSET_OF(st, m) \
((size_t) ( (char *)&(_nullptr<st>()->m) - (char *)0 ))
/**
 * Some platforms (devices) not define NULL
 */

/**
 * Windows defines a lot of badly stuff we'll never ever use. undefine it.
 */

#ifdef _WIN32
#	undef min // override standard definition
#	undef max // override standard definition
#	undef ERROR // override (really stupid) wingdi.h standard definition
#	undef DELETE // override (another really stupid) winnt.h standard definition
#	undef MessageBox // override winuser.h standard definition
#	undef MIN // override standard definition
#	undef MAX // override standard definition
#	undef CLAMP // override standard definition
#	undef Error
#	undef OK
#endif

#include "error_macros.h"
#include "error_list.h"

/**
 * Types defined for portability.
 * libSDL uses the same convention, so if libSDL is in use, we just use SDL ones.
 */
 
#ifdef _MSC_VER

/* Microsoft Visual C doesn't support the C98, C++0x standard types, so redefine them */

typedef signed __int8		int8_t;
typedef unsigned __int8		uint8_t;
typedef signed __int16		int16_t;
typedef unsigned __int16	uint16_t;
typedef signed __int32		int32_t;
typedef unsigned __int32	uint32_t;
typedef signed __int64		int64_t;
typedef unsigned __int64	uint64_t;

#else
 
#ifdef NO_STDINT_H
typedef unsigned char   uint8_t;
typedef signed char     int8_t;
typedef unsigned short  uint16_t;
typedef signed short    int16_t;
typedef unsigned int    uint32_t;
typedef signed int      int32_t;
typedef long long	int64_t;
typedef unsigned long long int64_t;
#else
#include <stdint.h>
#endif

#endif
/** Generic ABS function */

#ifndef ABS
#define ABS(m_v) ((m_v<0)?(-(m_v)):(m_v))
#endif

#ifndef MIN
#define MIN(m_a,m_b) (((m_a)<(m_b))?(m_a):(m_b))
#endif

#ifndef MAX
#define MAX(m_a,m_b) (((m_a)>(m_b))?(m_a):(m_b))
#endif

#ifndef CLAMP
#define CLAMP(m_a,m_min,m_max) (((m_a)<(m_min))?(m_min):(((m_a)>(m_max))?m_max:m_a))
#endif

/** Generic swap template */
#ifndef SWAP

#define SWAP(m_x,m_y) __swap_tmpl(m_x,m_y)
template<class T>
inline void __swap_tmpl(T &x, T &y ) {

	T aux=x;
	x=y;
	y=aux;
}

#endif //swap

#define HEX2CHR( m_hex ) ( (m_hex>='0' && m_hex<='9')?(m_hex-'0'):\
	((m_hex>='A' && m_hex<='F')?(10+m_hex-'A'):\
	((m_hex>='a' && m_hex<='f')?(10+m_hex-'a'):0)))




/** Function to find the nearest (bigger) power of 2 to an integer */

static inline unsigned int nearest_power_of_2(unsigned int p_number) {

	for (int i=30;i>=0;i--) {

		if (p_number&(1<<i))
			return ((p_number==(unsigned int)(1<<i)) ? p_number : (1<<(i+1)));
	}

	return 0;
}

/** Function to find the nearest (bigger) power of 2 to an integer */

static inline unsigned int nearest_shift(unsigned int p_number) {

	for (int i=30;i>=0;i--) {

		if (p_number&(1<<i))
			return i+1;
	}

	return 0;
}

/** get a shift value from a power of 2 */
static inline int get_shift_from_power_of_2( unsigned int p_pixel ) {
	// return a GL_TEXTURE_SIZE_ENUM


	for (unsigned int i=0;i<32;i++) {

		if (p_pixel==(unsigned int)(1<<i))
			return i;
	}

	return -1;
}

/** Swap 32 bits value for endianness */
static inline uint32_t BSWAP32(uint32_t x) {
	return((x<<24)|((x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x>>24));
}

/** When compiling with RTTI, we can add an "extra"
 * layer of safeness in many operations, so dynamic_cast
 * is used besides casting by enum.
 */

template<class T>
struct Comparator {

	inline bool operator()(const T& p_a, const T& p_b) const { return (p_a<p_b); }

};


#define __STRX(m_index) #m_index
#define __STR(m_index) __STRX(m_index)


#endif  /* typedefs.h */

