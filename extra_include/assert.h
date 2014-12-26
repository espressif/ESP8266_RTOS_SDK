/*
	assert.h
*/

#ifdef __cplusplus
extern "C" {
#endif

#include "_ansi.h"

#undef assert

#ifdef NDEBUG           /* required by ANSI standard */
#define assert(p)  	((void)0)
#else

#ifdef __STDC__
#define assert(e)       ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))
#else   /* PCC */
#define assert(e)       ((e) ? (void)0 : __assert(__FILE__, __LINE__, "e"))
#endif

#endif /* NDEBUG */

void _EXFUN(__assert,(const char *, int, const char *));

#ifdef __cplusplus
}
#endif
