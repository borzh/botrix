//----------------------------------------------------------------------------------------------------------------
// Util defines for debugging.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_DEFINES_H__
#define __GOOD_DEFINES_H__


#ifndef abstract
#	define abstract
#endif


#ifndef NULL
#	define NULL                         0
#endif


#ifdef LINUX
    typedef char Char;
#endif


#ifndef TIME_INFINITE
#	define TIME_INFINITE                0xFFFFFFFF
#endif


#ifndef MAX_UINT32
/// Max of unsigned 32bit int.
#	define MAX_UINT32                   0xFFFFFFFF
#endif


#ifndef MAX_INT32
/// Max of signed 32bit int.
#	define MAX_INT32                    0x7FFFFFFF
#endif


#ifndef MAX_UINT16
/// Max of unsigned 16bit short.
#	define MAX_UINT16                   0xFFFF
#endif


#ifndef MAX_INT16
/// Max of signed 16bit short.
#	define MAX_INT16                    0x7FFF
#endif


#ifndef MAX2
/// Max of two numbers.
#	define MAX2(a, b)                   ((a)>=(b)?(a):(b))
#endif


#ifndef MIN2
/// Min of two numbers.
#	define MIN2(a, b)                   ((a)<=(b)?(a):(b))
#endif


#ifndef SQR
/// Square of a number.
#	define SQR(x)                       ((x)*(x))
#endif


/// Get first byte of a number.
#define GET_1ST_BYTE(x)                 ((x) & 0xFF)

/// Get second byte of a number.
#define GET_2ND_BYTE(x)                 ( ((x) >> 8) & 0xFF )

/// Get third byte of a number.
#define GET_3RD_BYTE(x)                 ( ((x) >> 16) & 0xFF )

/// Get fourth byte of a number.
#define GET_4TH_BYTE(x)                 ( ((x) >> 24) & 0xFF )



/// Make first byte of a number.
#define MAKE_1ST_BYTE(x)                ((x) & 0xFF)

/// Make second byte of a number.
#define MAKE_2ND_BYTE(x)                ( ((x) & 0xFF) << 8 )

/// Make third byte of a number.
#define MAKE_3RD_BYTE(x)                ( ((x) & 0xFF) << 16 )

/// Make fourth byte of a number.
#define MAKE_4TH_BYTE(x)                ( ((x) & 0xFF) << 24 )



/// Set first byte of a number.
#define SET_1ST_BYTE(b, number)         number = (number & 0xFFFFFF00)  |  ( (b)&0xFF )

/// Set second byte of a number.
#define SET_2ND_BYTE(b, number)         number = (number & 0xFFFF00FF)  |  ( ((b)&0xFF) << 8 )

/// Set third byte of a number.
#define SET_3RD_BYTE(b, number)         number = (number & 0xFF00FFFF)  |  ( ((b)&0xFF) << 16 )

/// Set fourth byte of a number.
#define SET_4TH_BYTE(b, number)         number = (number & 0x00FFFFFF)  |  ( (b) << 24 )



/// Get first 16bits word of a number.
#define GET_1ST_WORD(x)                 ((x) & 0xFFFF)

/// Get second 16bits word of a number.
#define GET_2ND_WORD(x)                 ( ((x) >> 16) & 0xFFFF )


/// Set first 16bits word of a number.
#define SET_1ST_WORD(word, number)      number = ( (number & 0xFFFF0000) | ((word) & 0xFFFF) )

/// Set second 16bits word of a number.
#define SET_2ND_WORD(word, number)      number = ( (number & 0xFFFF)     | ((word) << 16) )



/// Enum to bit flag.
#define ENUM_TO_FLAG(x)                 ( 1<<(x) )


/// Set flag in flags.
#define FLAG_SET(flag, flags)           flags |= (flag)

/// Clear flag in flags.
#define FLAG_CLEAR(flag, flags)         flags &= ~(flag)

/// To know if some flag is cleared.
#define FLAG_CLEARED(flag, flags)       ( ((flags) & (flag)) == 0 )

/// Return true if some flag is set.
#define FLAG_SOME_SET(flag, flags)      ( ((flag) & (flags)) || ((flag) == 0) )

/// Return true if all flags are set.
#define FLAG_ALL_SET(flag, flags)       ( ( ((flag) & (flags)) == (flag) ) || ((flag) == 0) )


/// Get needed size of bit array in bytes.
#define BIT_ARRAY_SIZE(size)            ( ((size)>>3) + (((size)&7) ? 1 : 0) )

/// Set bit in bit's array.
#define BIT_ARRAY_SET(index, arr)       arr[(index)>>3] |= 1 << ((index)&7)

/// Clear bit in bit's array.
#define BIT_ARRAY_CLEAR(index, arr)     arr[(index)>>3] &= ~( 1 << ((index)&7) )

/// Return true if bit in bit's array is set.
#define BIT_ARRAY_IS_SET(index, arr)    ( arr[(index)>>3] & (1 << ((index)&7)) )


/// Debug break, stopping execution.
#ifndef BreakDebugger
#	ifdef _WIN32
#		define AsmBreak()          __asm { int 3 }
#	else
#		define AsmBreak()          __asm __volatile( "pause" );
#	endif
#endif


/// Debug asserts. Will produce debug break, allowing debugging if exp is false.
#if defined(DEBUG) || defined(_DEBUG)

#	ifndef DebugPrint
#		include <stdio.h>
#		define DebugPrint printf
#	endif

#	ifndef DebugAssert
#		define DebugAssert(exp)\
            if ( !(exp) )\
            {\
                DebugPrint("Assert failed: (" #exp ") at %s(), file %s, line %d\n", __FUNCTION__, __FILE__, __LINE__);\
                AsmBreak();\
            }
#	endif

#else // if defined(DEBUG) || defined(_DEBUG)

#	ifndef DebugPrint
#		define DebugPrint(...)
#	endif

#	ifndef DebugAssert
#		ifdef BETA_VERSION
#			define DebugAssert(exp)\
                if (!(exp))\
                {\
                    DebugPrint("Assert failed: (" #exp ") at %s(), file %s, line %d\n", __FUNCTION__, __FILE__, __LINE__);\
                    exit(1);\
                }
#		else
#			define DebugAssert(...)
#		endif
#	endif

#endif


#endif // __GOOD_DEFINES_H__
