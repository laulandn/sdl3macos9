/*
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_test.h>

#ifdef __MWERKS__
extern void SDL_TriggerBreakpoint();
#define INT64_C(a) (long long)a
#define UINT64_C(a) (unsigned long long)a
#endif

/*
 * Watcom C flags these as Warning 201: "Unreachable code" if you just
 *  compare them directly, so we push it through a function to keep the
 *  compiler quiet.  --ryan.
 */
static int
badsize(size_t sizeoftype, size_t hardcodetype)
{
    return sizeoftype != hardcodetype;
}

SDL_COMPILE_TIME_ASSERT(SDL_MAX_SINT8, SDL_MAX_SINT8 == 127);
SDL_COMPILE_TIME_ASSERT(SDL_MIN_SINT8, SDL_MIN_SINT8 == -128);
SDL_COMPILE_TIME_ASSERT(SDL_MAX_UINT8, SDL_MAX_UINT8 == 255);
SDL_COMPILE_TIME_ASSERT(SDL_MIN_UINT8, SDL_MIN_UINT8 == 0);

SDL_COMPILE_TIME_ASSERT(SDL_MAX_SINT16, SDL_MAX_SINT16 == 32767);
SDL_COMPILE_TIME_ASSERT(SDL_MIN_SINT16, SDL_MIN_SINT16 == -32768);
SDL_COMPILE_TIME_ASSERT(SDL_MAX_UINT16, SDL_MAX_UINT16 == 65535);
SDL_COMPILE_TIME_ASSERT(SDL_MIN_UINT16, SDL_MIN_UINT16 == 0);

SDL_COMPILE_TIME_ASSERT(SDL_MAX_SINT32, SDL_MAX_SINT32 == 2147483647);
SDL_COMPILE_TIME_ASSERT(SDL_MIN_SINT32, SDL_MIN_SINT32 == ~0x7fffffff); /* Instead of -2147483648, which is treated as unsigned by some compilers */
SDL_COMPILE_TIME_ASSERT(SDL_MAX_UINT32, SDL_MAX_UINT32 == 4294967295u);
SDL_COMPILE_TIME_ASSERT(SDL_MIN_UINT32, SDL_MIN_UINT32 == 0);

SDL_COMPILE_TIME_ASSERT(SDL_MAX_SINT64, SDL_MAX_SINT64 == INT64_C(9223372036854775807));
SDL_COMPILE_TIME_ASSERT(SDL_MIN_SINT64, SDL_MIN_SINT64 == ~INT64_C(0x7fffffffffffffff)); /* Instead of -9223372036854775808, which is treated as unsigned by compilers */
SDL_COMPILE_TIME_ASSERT(SDL_MAX_UINT64, SDL_MAX_UINT64 == UINT64_C(18446744073709551615));
SDL_COMPILE_TIME_ASSERT(SDL_MIN_UINT64, SDL_MIN_UINT64 == 0);

static int TestTypes(bool verbose)
{
    int error = 0;

    if (badsize(sizeof(bool), 1)) {
        if (verbose) {
            SDL_Log("sizeof(bool) != 1, instead = %u", (unsigned int)sizeof(bool));
        }
        ++error;
    }
    if (badsize(sizeof(Uint8), 1)) {
        if (verbose) {
            SDL_Log("sizeof(Uint8) != 1, instead = %u", (unsigned int)sizeof(Uint8));
        }
        ++error;
    }
    if (badsize(sizeof(Uint16), 2)) {
        if (verbose) {
            SDL_Log("sizeof(Uint16) != 2, instead = %u", (unsigned int)sizeof(Uint16));
        }
        ++error;
    }
    if (badsize(sizeof(Uint32), 4)) {
        if (verbose) {
            SDL_Log("sizeof(Uint32) != 4, instead = %u", (unsigned int)sizeof(Uint32));
        }
        ++error;
    }
    if (badsize(sizeof(Uint64), 8)) {
        if (verbose) {
            SDL_Log("sizeof(Uint64) != 8, instead = %u", (unsigned int)sizeof(Uint64));
        }
        ++error;
    }
    if (verbose && !error) {
        SDL_Log("All data types are the expected size.");
    }

    return error ? 1 : 0;
}

static int TestEndian(bool verbose)
{
    int error = 0;
    Uint16 value = 0x1234;
    int real_byteorder;
    int real_floatwordorder = 0;
    Uint16 value16 = 0xCDAB;
    Uint16 swapped16 = 0xABCD;
    Uint32 value32 = 0xEFBEADDE;
    Uint32 swapped32 = 0xDEADBEEF;
    Uint64 value64, swapped64;

    union
    {
        double d;
        Uint32 ui32[2];
    } value_double;

    value64 = 0xEFBEADDE;
    value64 <<= 32;
    value64 |= 0xCDAB3412;
    swapped64 = 0x1234ABCD;
    swapped64 <<= 32;
    swapped64 |= 0xDEADBEEF;
    value_double.d = 3.141593;

    if (verbose) {
        SDL_Log("Detected a %s endian machine.",
                (SDL_BYTEORDER == SDL_LIL_ENDIAN) ? "little" : "big");
    }
    if ((*((char *)&value) >> 4) == 0x1) {
        real_byteorder = SDL_BIG_ENDIAN;
    } else {
        real_byteorder = SDL_LIL_ENDIAN;
    }
    if (real_byteorder != SDL_BYTEORDER) {
        if (verbose) {
            SDL_Log("Actually a %s endian machine!",
                    (real_byteorder == SDL_LIL_ENDIAN) ? "little" : "big");
        }
        ++error;
    }
    if (verbose) {
        SDL_Log("Detected a %s endian float word order machine.",
                (SDL_FLOATWORDORDER == SDL_LIL_ENDIAN) ? "little" : "big");
    }
    if (value_double.ui32[0] == 0x82c2bd7f && value_double.ui32[1] == 0x400921fb) {
        real_floatwordorder = SDL_LIL_ENDIAN;
    } else if (value_double.ui32[0] == 0x400921fb && value_double.ui32[1] == 0x82c2bd7f) {
        real_floatwordorder = SDL_BIG_ENDIAN;
    }
    if (real_floatwordorder != SDL_FLOATWORDORDER) {
        if (verbose) {
            SDL_Log("Actually a %s endian float word order machine!",
                    (real_floatwordorder == SDL_LIL_ENDIAN) ? "little" : (real_floatwordorder == SDL_BIG_ENDIAN) ? "big"
                                                                                                                 : "unknown");
        }
        ++error;
    }
    if (verbose) {
        SDL_Log("Value 16 = 0x%X, swapped = 0x%X", value16,
                SDL_Swap16(value16));
    }
    if (SDL_Swap16(value16) != swapped16) {
        if (verbose) {
            SDL_Log("16 bit value swapped incorrectly!");
        }
        ++error;
    }
    if (verbose) {
        SDL_Log("Value 32 = 0x%" SDL_PRIX32 ", swapped = 0x%" SDL_PRIX32,
                value32,
                SDL_Swap32(value32));
    }
    if (SDL_Swap32(value32) != swapped32) {
        if (verbose) {
            SDL_Log("32 bit value swapped incorrectly!");
        }
        ++error;
    }
    if (verbose) {
        SDL_Log("Value 64 = 0x%" SDL_PRIX64 ", swapped = 0x%" SDL_PRIX64, value64,
                SDL_Swap64(value64));
    }
    if (SDL_Swap64(value64) != swapped64) {
        if (verbose) {
            SDL_Log("64 bit value swapped incorrectly!");
        }
        ++error;
    }
    return error ? 1 : 0;
}

static int TST_allmul(void *a, void *b, int arg, void *result, void *expected)
{
    (*(unsigned long long *)result) = ((*(unsigned long long *)a) * (*(unsigned long long *)b));
    return (*(unsigned long long *)result) == (*(unsigned long long *)expected);
}

static int TST_alldiv(void *a, void *b, int arg, void *result, void *expected)
{
    (*(long long *)result) = ((*(long long *)a) / (*(long long *)b));
    return (*(long long *)result) == (*(long long *)expected);
}

static int TST_allrem(void *a, void *b, int arg, void *result, void *expected)
{
    (*(long long *)result) = ((*(long long *)a) % (*(long long *)b));
    return (*(long long *)result) == (*(long long *)expected);
}

static int TST_ualldiv(void *a, void *b, int arg, void *result, void *expected)
{
    (*(unsigned long long *)result) = ((*(unsigned long long *)a) / (*(unsigned long long *)b));
    return (*(unsigned long long *)result) == (*(unsigned long long *)expected);
}

static int TST_uallrem(void *a, void *b, int arg, void *result, void *expected)
{
    (*(unsigned long long *)result) = ((*(unsigned long long *)a) % (*(unsigned long long *)b));
    return (*(unsigned long long *)result) == (*(unsigned long long *)expected);
}

static int TST_allshl(void *a, void *b, int arg, void *result, void *expected)
{
    (*(long long *)result) = (*(long long *)a) << arg;
    return (*(long long *)result) == (*(long long *)expected);
}

static int TST_aullshl(void *a, void *b, int arg, void *result, void *expected)
{
    (*(unsigned long long *)result) = (*(unsigned long long *)a) << arg;
    return (*(unsigned long long *)result) == (*(unsigned long long *)expected);
}

static int TST_allshr(void *a, void *b, int arg, void *result, void *expected)
{
    (*(long long *)result) = (*(long long *)a) >> arg;
    return (*(long long *)result) == (*(long long *)expected);
}

static int TST_aullshr(void *a, void *b, int arg, void *result, void *expected)
{
    (*(unsigned long long *)result) = (*(unsigned long long *)a) >> arg;
    return (*(unsigned long long *)result) == (*(unsigned long long *)expected);
}

typedef int (*LL_Intrinsic)(void *a, void *b, int arg, void *result, void *expected);

typedef struct
{
    const char *operation;
    LL_Intrinsic routine;
    unsigned long long a, b;
    int arg;
    unsigned long long expected_result;
} LL_Test;

static LL_Test LL_Tests[] = {
    /* UNDEFINED {"_allshl",   &TST_allshl,   0xFFFFFFFFFFFFFFFFllu,                  0llu, 65, 0x0000000000000000ll}, */
    { "_allshl", &TST_allshl, 0xFFFFFFFFFFFFFFFFllu, 0llu, 1, 0xFFFFFFFFFFFFFFFEllu },
    { "_allshl", &TST_allshl, 0xFFFFFFFFFFFFFFFFllu, 0llu, 32, 0xFFFFFFFF00000000llu },
    { "_allshl", &TST_allshl, 0xFFFFFFFFFFFFFFFFllu, 0llu, 33, 0xFFFFFFFE00000000llu },
    { "_allshl", &TST_allshl, 0xFFFFFFFFFFFFFFFFllu, 0llu, 0, 0xFFFFFFFFFFFFFFFFllu },

    { "_allshr", &TST_allshr, 0xAAAAAAAA55555555llu, 0llu, 63, 0xFFFFFFFFFFFFFFFFllu },
    /* UNDEFINED {"_allshr",   &TST_allshr,   0xFFFFFFFFFFFFFFFFllu,                  0llu, 65, 0xFFFFFFFFFFFFFFFFll}, */
    { "_allshr", &TST_allshr, 0xFFFFFFFFFFFFFFFFllu, 0llu, 1, 0xFFFFFFFFFFFFFFFFllu },
    { "_allshr", &TST_allshr, 0xFFFFFFFFFFFFFFFFllu, 0llu, 32, 0xFFFFFFFFFFFFFFFFllu },
    { "_allshr", &TST_allshr, 0xFFFFFFFFFFFFFFFFllu, 0llu, 33, 0xFFFFFFFFFFFFFFFFllu },
    { "_allshr", &TST_allshr, 0xFFFFFFFFFFFFFFFFllu, 0llu, 0, 0xFFFFFFFFFFFFFFFFllu },
    /* UNDEFINED {"_allshr",   &TST_allshr,   0x5F5F5F5F5F5F5F5Fllu,                  0llu, 65, 0x0000000000000000ll}, */
    { "_allshr", &TST_allshr, 0x5F5F5F5F5F5F5F5Fllu, 0llu, 1, 0x2FAFAFAFAFAFAFAFllu },
    { "_allshr", &TST_allshr, 0x5F5F5F5F5F5F5F5Fllu, 0llu, 32, 0x000000005F5F5F5Fllu },
    { "_allshr", &TST_allshr, 0x5F5F5F5F5F5F5F5Fllu, 0llu, 33, 0x000000002FAFAFAFllu },

    /* UNDEFINED {"_aullshl",  &TST_aullshl,  0xFFFFFFFFFFFFFFFFllu,                  0llu, 65, 0x0000000000000000ll}, */
    { "_aullshl", &TST_aullshl, 0xFFFFFFFFFFFFFFFFllu, 0llu, 1, 0xFFFFFFFFFFFFFFFEllu },
    { "_aullshl", &TST_aullshl, 0xFFFFFFFFFFFFFFFFllu, 0llu, 32, 0xFFFFFFFF00000000llu },
    { "_aullshl", &TST_aullshl, 0xFFFFFFFFFFFFFFFFllu, 0llu, 33, 0xFFFFFFFE00000000llu },
    { "_aullshl", &TST_aullshl, 0xFFFFFFFFFFFFFFFFllu, 0llu, 0, 0xFFFFFFFFFFFFFFFFllu },

    /* UNDEFINED {"_aullshr",  &TST_aullshr,  0xFFFFFFFFFFFFFFFFllu,                  0llu, 65, 0x0000000000000000ll}, */
    { "_aullshr", &TST_aullshr, 0xFFFFFFFFFFFFFFFFllu, 0llu, 1, 0x7FFFFFFFFFFFFFFFllu },
    { "_aullshr", &TST_aullshr, 0xFFFFFFFFFFFFFFFFllu, 0llu, 32, 0x00000000FFFFFFFFllu },
    { "_aullshr", &TST_aullshr, 0xFFFFFFFFFFFFFFFFllu, 0llu, 33, 0x000000007FFFFFFFllu },
    { "_aullshr", &TST_aullshr, 0xFFFFFFFFFFFFFFFFllu, 0llu, 0, 0xFFFFFFFFFFFFFFFFllu },

    { "_allmul", &TST_allmul, 0xFFFFFFFFFFFFFFFFllu, 0x0000000000000000llu, 0, 0x0000000000000000llu },
    { "_allmul", &TST_allmul, 0x0000000000000000llu, 0xFFFFFFFFFFFFFFFFllu, 0, 0x0000000000000000llu },
    { "_allmul", &TST_allmul, 0x000000000FFFFFFFllu, 0x0000000000000001llu, 0, 0x000000000FFFFFFFllu },
    { "_allmul", &TST_allmul, 0x0000000000000001llu, 0x000000000FFFFFFFllu, 0, 0x000000000FFFFFFFllu },
    { "_allmul", &TST_allmul, 0x000000000FFFFFFFllu, 0x0000000000000010llu, 0, 0x00000000FFFFFFF0llu },
    { "_allmul", &TST_allmul, 0x0000000000000010llu, 0x000000000FFFFFFFllu, 0, 0x00000000FFFFFFF0llu },
    { "_allmul", &TST_allmul, 0x000000000FFFFFFFllu, 0x0000000000000100llu, 0, 0x0000000FFFFFFF00llu },
    { "_allmul", &TST_allmul, 0x0000000000000100llu, 0x000000000FFFFFFFllu, 0, 0x0000000FFFFFFF00llu },
    { "_allmul", &TST_allmul, 0x000000000FFFFFFFllu, 0x0000000010000000llu, 0, 0x00FFFFFFF0000000llu },
    { "_allmul", &TST_allmul, 0x0000000010000000llu, 0x000000000FFFFFFFllu, 0, 0x00FFFFFFF0000000llu },
    { "_allmul", &TST_allmul, 0x000000000FFFFFFFllu, 0x0000000080000000llu, 0, 0x07FFFFFF80000000llu },
    { "_allmul", &TST_allmul, 0x0000000080000000llu, 0x000000000FFFFFFFllu, 0, 0x07FFFFFF80000000llu },
    { "_allmul", &TST_allmul, 0xFFFFFFFFFFFFFFFEllu, 0x0000000080000000llu, 0, 0xFFFFFFFF00000000llu },
    { "_allmul", &TST_allmul, 0x0000000080000000llu, 0xFFFFFFFFFFFFFFFEllu, 0, 0xFFFFFFFF00000000llu },
    { "_allmul", &TST_allmul, 0xFFFFFFFFFFFFFFFEllu, 0x0000000080000008llu, 0, 0xFFFFFFFEFFFFFFF0llu },
    { "_allmul", &TST_allmul, 0x0000000080000008llu, 0xFFFFFFFFFFFFFFFEllu, 0, 0xFFFFFFFEFFFFFFF0llu },
    { "_allmul", &TST_allmul, 0x00000000FFFFFFFFllu, 0x00000000FFFFFFFFllu, 0, 0xFFFFFFFE00000001llu },

    { "_alldiv", &TST_alldiv, 0x0000000000000000llu, 0x0000000000000001llu, 0, 0x0000000000000000llu },
    { "_alldiv", &TST_alldiv, 0x0000000000000000llu, 0xFFFFFFFFFFFFFFFFllu, 0, 0x0000000000000000llu },
    { "_alldiv", &TST_alldiv, 0x0000000000000001llu, 0xFFFFFFFFFFFFFFFFllu, 0, 0xFFFFFFFFFFFFFFFFllu },
    { "_alldiv", &TST_alldiv, 0xFFFFFFFFFFFFFFFFllu, 0x0000000000000001llu, 0, 0xFFFFFFFFFFFFFFFFllu },
    { "_alldiv", &TST_alldiv, 0x0000000000000001llu, 0xFFFFFFFFFFFFFFFFllu, 0, 0xFFFFFFFFFFFFFFFFllu },
    { "_alldiv", &TST_alldiv, 0x0000000000000001llu, 0x0000000000000001llu, 0, 0x0000000000000001llu },
    { "_alldiv", &TST_alldiv, 0xFFFFFFFFFFFFFFFFllu, 0xFFFFFFFFFFFFFFFFllu, 0, 0x0000000000000001llu },
    { "_alldiv", &TST_alldiv, 0x000000000FFFFFFFllu, 0x0000000000000001llu, 0, 0x000000000FFFFFFFllu },
    { "_alldiv", &TST_alldiv, 0x0000000FFFFFFFFFllu, 0x0000000000000010llu, 0, 0x00000000FFFFFFFFllu },
    { "_alldiv", &TST_alldiv, 0x0000000000000100llu, 0x000000000FFFFFFFllu, 0, 0x0000000000000000llu },
    { "_alldiv", &TST_alldiv, 0x00FFFFFFF0000000llu, 0x0000000010000000llu, 0, 0x000000000FFFFFFFllu },
    { "_alldiv", &TST_alldiv, 0x07FFFFFF80000000llu, 0x0000000080000000llu, 0, 0x000000000FFFFFFFllu },
    { "_alldiv", &TST_alldiv, 0xFFFFFFFFFFFFFFFEllu, 0x0000000080000000llu, 0, 0x0000000000000000llu },
    { "_alldiv", &TST_alldiv, 0xFFFFFFFEFFFFFFF0llu, 0xFFFFFFFFFFFFFFFEllu, 0, 0x0000000080000008llu },
    { "_alldiv", &TST_alldiv, 0x7FFFFFFEFFFFFFF0llu, 0xFFFFFFFFFFFFFFFEllu, 0, 0xC000000080000008llu },
    { "_alldiv", &TST_alldiv, 0x7FFFFFFEFFFFFFF0llu, 0x0000FFFFFFFFFFFEllu, 0, 0x0000000000007FFFllu },
    { "_alldiv", &TST_alldiv, 0x7FFFFFFEFFFFFFF0llu, 0x7FFFFFFEFFFFFFF0llu, 0, 0x0000000000000001llu },

    { "_allrem", &TST_allrem, 0x0000000000000000llu, 0x0000000000000001llu, 0, 0x0000000000000000llu },
    { "_allrem", &TST_allrem, 0x0000000000000000llu, 0xFFFFFFFFFFFFFFFFllu, 0, 0x0000000000000000llu },
    { "_allrem", &TST_allrem, 0x0000000000000001llu, 0xFFFFFFFFFFFFFFFFllu, 0, 0x0000000000000000llu },
    { "_allrem", &TST_allrem, 0xFFFFFFFFFFFFFFFFllu, 0x0000000000000001llu, 0, 0x0000000000000000llu },
    { "_allrem", &TST_allrem, 0x0000000000000001llu, 0xFFFFFFFFFFFFFFFFllu, 0, 0x0000000000000000llu },
    { "_allrem", &TST_allrem, 0x0000000000000001llu, 0x0000000000000001llu, 0, 0x0000000000000000llu },
    { "_allrem", &TST_allrem, 0xFFFFFFFFFFFFFFFFllu, 0xFFFFFFFFFFFFFFFFllu, 0, 0x0000000000000000llu },
    { "_allrem", &TST_allrem, 0x000000000FFFFFFFllu, 0x0000000000000001llu, 0, 0x0000000000000000llu },
    { "_allrem", &TST_allrem, 0x0000000FFFFFFFFFllu, 0x0000000000000010llu, 0, 0x000000000000000Fllu },
    { "_allrem", &TST_allrem, 0x0000000000000100llu, 0x000000000FFFFFFFllu, 0, 0x0000000000000100llu },
    { "_allrem", &TST_allrem, 0x00FFFFFFF0000000llu, 0x0000000010000000llu, 0, 0x0000000000000000llu },
    { "_allrem", &TST_allrem, 0x07FFFFFF80000000llu, 0x0000000080000000llu, 0, 0x0000000000000000llu },
    { "_allrem", &TST_allrem, 0xFFFFFFFFFFFFFFFEllu, 0x0000000080000000llu, 0, 0xFFFFFFFFFFFFFFFEllu },
    { "_allrem", &TST_allrem, 0xFFFFFFFEFFFFFFF0llu, 0xFFFFFFFFFFFFFFFEllu, 0, 0x0000000000000000llu },
    { "_allrem", &TST_allrem, 0x7FFFFFFEFFFFFFF0llu, 0xFFFFFFFFFFFFFFFEllu, 0, 0x0000000000000000llu },
    { "_allrem", &TST_allrem, 0x7FFFFFFEFFFFFFF0llu, 0x0000FFFFFFFFFFFEllu, 0, 0x0000FFFF0000FFEEllu },
    { "_allrem", &TST_allrem, 0x7FFFFFFEFFFFFFF0llu, 0x7FFFFFFEFFFFFFF0llu, 0, 0x0000000000000000llu },

    { "_ualldiv", &TST_ualldiv, 0x0000000000000000llu, 0x0000000000000001llu, 0, 0x0000000000000000llu },
    { "_ualldiv", &TST_ualldiv, 0x0000000000000000llu, 0xFFFFFFFFFFFFFFFFllu, 0, 0x0000000000000000llu },
    { "_ualldiv", &TST_ualldiv, 0x0000000000000001llu, 0xFFFFFFFFFFFFFFFFllu, 0, 0x0000000000000000llu },
    { "_ualldiv", &TST_ualldiv, 0xFFFFFFFFFFFFFFFFllu, 0x0000000000000001llu, 0, 0xFFFFFFFFFFFFFFFFllu },
    { "_ualldiv", &TST_ualldiv, 0x0000000000000001llu, 0xFFFFFFFFFFFFFFFFllu, 0, 0x0000000000000000llu },
    { "_ualldiv", &TST_ualldiv, 0x0000000000000001llu, 0x0000000000000001llu, 0, 0x0000000000000001llu },
    { "_ualldiv", &TST_ualldiv, 0xFFFFFFFFFFFFFFFFllu, 0xFFFFFFFFFFFFFFFFllu, 0, 0x0000000000000001llu },
    { "_ualldiv", &TST_ualldiv, 0x000000000FFFFFFFllu, 0x0000000000000001llu, 0, 0x000000000FFFFFFFllu },
    { "_ualldiv", &TST_ualldiv, 0x0000000FFFFFFFFFllu, 0x0000000000000010llu, 0, 0x00000000FFFFFFFFllu },
    { "_ualldiv", &TST_ualldiv, 0x0000000000000100llu, 0x000000000FFFFFFFllu, 0, 0x0000000000000000llu },
    { "_ualldiv", &TST_ualldiv, 0x00FFFFFFF0000000llu, 0x0000000010000000llu, 0, 0x000000000FFFFFFFllu },
    { "_ualldiv", &TST_ualldiv, 0x07FFFFFF80000000llu, 0x0000000080000000llu, 0, 0x000000000FFFFFFFllu },
    { "_ualldiv", &TST_ualldiv, 0xFFFFFFFFFFFFFFFEllu, 0x0000000080000000llu, 0, 0x00000001FFFFFFFFllu },
    { "_ualldiv", &TST_ualldiv, 0xFFFFFFFEFFFFFFF0llu, 0xFFFFFFFFFFFFFFFEllu, 0, 0x0000000000000000llu },
    { "_ualldiv", &TST_ualldiv, 0x7FFFFFFEFFFFFFF0llu, 0xFFFFFFFFFFFFFFFEllu, 0, 0x0000000000000000llu },
    { "_ualldiv", &TST_ualldiv, 0x7FFFFFFEFFFFFFF0llu, 0x0000FFFFFFFFFFFEllu, 0, 0x0000000000007FFFllu },
    { "_ualldiv", &TST_ualldiv, 0x7FFFFFFEFFFFFFF0llu, 0x7FFFFFFEFFFFFFF0llu, 0, 0x0000000000000001llu },

    { "_uallrem", &TST_uallrem, 0x0000000000000000llu, 0x0000000000000001llu, 0, 0x0000000000000000llu },
    { "_uallrem", &TST_uallrem, 0x0000000000000000llu, 0xFFFFFFFFFFFFFFFFllu, 0, 0x0000000000000000llu },
    { "_uallrem", &TST_uallrem, 0x0000000000000001llu, 0xFFFFFFFFFFFFFFFFllu, 0, 0x0000000000000001llu },
    { "_uallrem", &TST_uallrem, 0xFFFFFFFFFFFFFFFFllu, 0x0000000000000001llu, 0, 0x0000000000000000llu },
    { "_uallrem", &TST_uallrem, 0x0000000000000001llu, 0xFFFFFFFFFFFFFFFFllu, 0, 0x0000000000000001llu },
    { "_uallrem", &TST_uallrem, 0x0000000000000001llu, 0x0000000000000001llu, 0, 0x0000000000000000llu },
    { "_uallrem", &TST_uallrem, 0xFFFFFFFFFFFFFFFFllu, 0xFFFFFFFFFFFFFFFFllu, 0, 0x0000000000000000llu },
    { "_uallrem", &TST_uallrem, 0x000000000FFFFFFFllu, 0x0000000000000001llu, 0, 0x0000000000000000llu },
    { "_uallrem", &TST_uallrem, 0x0000000FFFFFFFFFllu, 0x0000000000000010llu, 0, 0x000000000000000Fllu },
    { "_uallrem", &TST_uallrem, 0x0000000000000100llu, 0x000000000FFFFFFFllu, 0, 0x0000000000000100llu },
    { "_uallrem", &TST_uallrem, 0x00FFFFFFF0000000llu, 0x0000000010000000llu, 0, 0x0000000000000000llu },
    { "_uallrem", &TST_uallrem, 0x07FFFFFF80000000llu, 0x0000000080000000llu, 0, 0x0000000000000000llu },
    { "_uallrem", &TST_uallrem, 0xFFFFFFFFFFFFFFFEllu, 0x0000000080000000llu, 0, 0x000000007FFFFFFEllu },
    { "_uallrem", &TST_uallrem, 0xFFFFFFFEFFFFFFF0llu, 0xFFFFFFFFFFFFFFFEllu, 0, 0xFFFFFFFEFFFFFFF0llu },
    { "_uallrem", &TST_uallrem, 0x7FFFFFFEFFFFFFF0llu, 0xFFFFFFFFFFFFFFFEllu, 0, 0x7FFFFFFEFFFFFFF0llu },
    { "_uallrem", &TST_uallrem, 0x7FFFFFFEFFFFFFF0llu, 0x0000FFFFFFFFFFFEllu, 0, 0x0000FFFF0000FFEEllu },
    { "_uallrem", &TST_uallrem, 0x7FFFFFFEFFFFFFF0llu, 0x7FFFFFFEFFFFFFF0llu, 0, 0x0000000000000000llu },

    { NULL, NULL, 0, 0, 0, 0 }
};

static int Test64Bit(bool verbose)
{
    LL_Test *t;
    int failed = 0;

    for (t = LL_Tests; t->routine; t++) {
        unsigned long long result = 0;
        unsigned int *al = (unsigned int *)&t->a;
        unsigned int *bl = (unsigned int *)&t->b;
        unsigned int *el = (unsigned int *)&t->expected_result;
        unsigned int *rl = (unsigned int *)&result;

        if (!t->routine(&t->a, &t->b, t->arg, &result, &t->expected_result)) {
            if (verbose) {
                SDL_Log("%s(0x%08X%08X, 0x%08X%08X, %3d, produced: 0x%08X%08X, expected: 0x%08X%08X", t->operation, al[1], al[0], bl[1], bl[0],
                        t->arg, rl[1], rl[0], el[1], el[0]);
            }
            ++failed;
        }
    }
    if (verbose && (failed == 0)) {
        SDL_Log("All 64bit intrinsic tests passed");
    }
    return failed ? 1 : 0;
}

static int TestCPUInfo(bool verbose)
{
    if (verbose) {
        SDL_Log("Number of logical CPU cores: %d", SDL_GetNumLogicalCPUCores());
        SDL_Log("CPU cache line size: %d", SDL_GetCPUCacheLineSize());
        SDL_Log("AltiVec %s", SDL_HasAltiVec() ? "detected" : "not detected");
        SDL_Log("MMX %s", SDL_HasMMX() ? "detected" : "not detected");
        SDL_Log("SSE %s", SDL_HasSSE() ? "detected" : "not detected");
        SDL_Log("SSE2 %s", SDL_HasSSE2() ? "detected" : "not detected");
        SDL_Log("SSE3 %s", SDL_HasSSE3() ? "detected" : "not detected");
        SDL_Log("SSE4.1 %s", SDL_HasSSE41() ? "detected" : "not detected");
        SDL_Log("SSE4.2 %s", SDL_HasSSE42() ? "detected" : "not detected");
        SDL_Log("AVX %s", SDL_HasAVX() ? "detected" : "not detected");
        SDL_Log("AVX2 %s", SDL_HasAVX2() ? "detected" : "not detected");
        SDL_Log("AVX-512F %s", SDL_HasAVX512F() ? "detected" : "not detected");
        SDL_Log("ARM SIMD %s", SDL_HasARMSIMD() ? "detected" : "not detected");
        SDL_Log("NEON %s", SDL_HasNEON() ? "detected" : "not detected");
        SDL_Log("LSX %s", SDL_HasLSX() ? "detected" : "not detected");
        SDL_Log("LASX %s", SDL_HasLASX() ? "detected" : "not detected");
        SDL_Log("System RAM %d MB", SDL_GetSystemRAM());
    }
    return 0;
}

static int TestAssertions(bool verbose)
{
    SDL_assert(1);
    SDL_assert_release(1);
    SDL_assert_paranoid(1);
    SDL_assert(0 || 1);
    SDL_assert_release(0 || 1);
    SDL_assert_paranoid(0 || 1);

#if 0 /* enable this to test assertion failures. */
    SDL_assert_release(1 == 2);
    SDL_assert_release(5 < 4);
    SDL_assert_release(0 && "This is a test");
#endif

    {
        const SDL_AssertData *item = SDL_GetAssertionReport();
        while (item) {
            SDL_Log("'%s', %s (%s:%d), triggered %u times, always ignore: %s.",
                    item->condition, item->function, item->filename,
                    item->linenum, item->trigger_count,
                    item->always_ignore ? "yes" : "no");
            item = item->next;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int i;
    bool verbose = true;
    int status = 0;
    SDLTest_CommonState *state;

    /* Initialize test framework */
    state = SDLTest_CommonCreateState(argv, 0);
    if (!state) {
        return 1;
    }

    /* Parse commandline */
    for (i = 1; i < argc;) {
        int consumed;

        consumed = SDLTest_CommonArg(state, i);
        if (!consumed) {
            if (SDL_strcmp(argv[i], "-q") == 0) {
                verbose = false;
                consumed = 1;
            }
        }
        if (consumed <= 0) {
            static const char *options[] = { "[-q]", NULL };
            SDLTest_CommonLogUsage(state, argv[0], options);
            return 1;
        }

        i += consumed;
    }

    if (verbose) {
        SDL_Log("This system is running %s", SDL_GetPlatform());
    }

    status += TestTypes(verbose);
    status += TestEndian(verbose);
    status += Test64Bit(verbose);
    status += TestCPUInfo(verbose);
    status += TestAssertions(verbose);

    SDL_Quit();
    SDLTest_CommonDestroyState(state);

    return status;
}
