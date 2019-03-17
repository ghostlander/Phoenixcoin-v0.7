#ifndef DATATYPES_H
#define DATATYPES_H

#if defined(_MSC_VER) && (_MSC_VER < 1800)
#include "msc_inttypes.h"
#else
#include <inttypes.h>
#endif /* _MSC_VER */

/* Our abbreviations for the ISO C99 integer data types */
typedef unsigned char           uchar;   /* uint8_t */
typedef unsigned short          ushort;  /* uint16_t */
typedef unsigned int            uint;    /* uint32_t */
typedef long long int           int64;   /* int64_t */
typedef unsigned long long int  uint64;  /* uint64_t */

#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif

#ifndef PRI64d
#if defined(_MSC_VER) || defined(__MSVCRT__)
#define PRI64d  "I64d"
#define PRI64u  "I64u"
#define PRI64x  "I64x"
#else
#define PRI64d  "lld"
#define PRI64u  "llu"
#define PRI64x  "llx"
#endif /* _MSC_VER */
#endif /* PRI64d */

/* Format characters for (s)size_t and ptrdiff_t */
#if defined(_MSC_VER) || defined(__MSVCRT__)
/* (s)size_t and ptrdiff_t have the same size specifier in MSVC:
 * http://msdn.microsoft.com/en-us/library/tcxf1dw6%28v=vs.100%29.aspx */
#define PRIszx    "Ix"
#define PRIszu    "Iu"
#define PRIszd    "Id"
#define PRIpdx    "Ix"
#define PRIpdu    "Iu"
#define PRIpdd    "Id"
#else /* C99 standard */
#define PRIszx    "zx"
#define PRIszu    "zu"
#define PRIszd    "zd"
#define PRIpdx    "tx"
#define PRIpdu    "tu"
#define PRIpdd    "td"
#endif /* _MSC_VER */

#endif /* DATATYPES_H */
