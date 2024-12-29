#ifndef __ASSEMBLY__
#include <stdint.h>
#endif

#define _STR(x) #x
#define STR(x)  _STR(x)

#define CAT_(a, b, c, d, e, f, g, i, j, k, l, m, n, o, p, ...)                           \
    a##b##c##d##e##f##g##i##j##k##l##m##n##o##p

// Concatenate arbitrarily many token (max 16)
#define CAT(...) CAT_(__VA_ARGS__, , , , , , , , , , , , , , , , , )

#ifdef __ASSEMBLY__
#define _INTEGER(num, suffix) num
#else
#define _INTEGER(num, suffix) CAT(num, suffix)
#endif

#define _U32(x) _INTEGER(x, U)
#define _U64(x) _INTEGER(x, ULL)

#ifndef __ASSEMBLY__
#define _ptr(val) ((void *) (unsigned long) (val))
#define _ul(val)  ((unsigned long) (val))
#define _u(val)   ((unsigned int) (val))
#define _int(val) ((int) (val))
#endif

#define __aligned(x) __attribute__((__aligned__(x)))
#define __packed     __attribute__((__packed__))
#define __section(s) __attribute__((__section__(s)))

#define __text __section(".text")
#define __data __section(".data")

#define __user_text __section(".text.user")
#define __user_data __section(".data.user")

#ifndef __ASSEMBLY__
// A one-bit mask at bit n of type t
#define BIT_T(t, n) ((t) 1 << (n))

// A one-bit mask at bit n
#define BIT(n) BIT_T(uint64_t, n)

// Align an address, rounding the address down
#define ALIGN_DOWN(addr, align) ((_ul(addr)) & ~((_ul(align)) - 1))

// Align an address, rounding the address up
#define ALIGN_UP(addr, align) (((_ul(addr)) + ((_ul(align)) - 1)) & ~((_ul(align)) - 1))
#endif
