/* math.h -- Definitions for the math floating point package.  */

#ifndef  _MATH_H_
#define  _MATH_H_

#include <sys/reent.h>
#include <machine/ieeefp.h>
#include "_ansi.h"

_BEGIN_STD_C

#ifndef HUGE_VAL

/* Define HUGE_VAL as infinity, unless HUGE_VAL is already defined
   (which might have been done by something like math-68881.h).  */

union __dmath
{
  __ULong i[2];
  double d;
};

/* Declare this as an array without bounds so that no matter what small data
   support a port and/or library has, this reference will be via the general
   method for accessing globals. */
extern __IMPORT const union __dmath __infinity[];

#if defined(__GNUC__) &&	\
  ( (__GNUC__ >= 4) || \
    ( (__GNUC__ >= 3) && defined(__GNUC_MINOR__) && (__GNUC_MINOR__ >= 3) ) )

 /* gcc >= 3.3 implicitly defines builtins for HUGE_VALx values.  */

# ifndef HUGE_VAL
#  define HUGE_VAL (__builtin_huge_val())
# endif

# ifndef HUGE_VALF
#  define HUGE_VALF (__builtin_huge_valf())
# endif

# ifndef HUGE_VALL
#  define HUGE_VALL (__builtin_huge_vall())
# endif

# ifndef INFINITY
#  define INFINITY (__builtin_inff())
# endif

# ifndef NAN
#  define NAN (__builtin_nanf(""))
# endif

#else /* !gcc >= 3.3  */

 /*      No builtins.  Use fixed defines instead.  (All 3 HUGE plus the INFINITY
  * and NAN macros are required to be constant expressions.  Using a variable--
  * even a static const--does not meet this requirement, as it cannot be
  * evaluated at translation time.)
  *      The infinities are done using numbers that are far in excess of
  * something that would be expected to be encountered in a floating-point
  * implementation.  (A more certain way uses values from float.h, but that is
  * avoided because system includes are not supposed to include each other.)
  *      This method might produce warnings from some compilers.  (It does in
  * newer GCCs, but not for ones that would hit this #else.)  If this happens,
  * please report details to the Newlib mailing list.  */

 #ifndef HUGE_VAL
  #define HUGE_VAL (1.0e999999999)
 #endif

 #ifndef HUGE_VALF
  #define HUGE_VALF (1.0e999999999F)
 #endif

 #if !defined(HUGE_VALL)  &&  defined(_HAVE_LONG_DOUBLE)
  #define HUGE_VALL (1.0e999999999L)
 #endif

 #if !defined(INFINITY)
  #define INFINITY (HUGE_VALF)
 #endif

 #if !defined(NAN)
  #if defined(__GNUC__)  &&  defined(__cplusplus)
    /* Exception:  older g++ versions warn about the divide by 0 used in the
     * normal case (even though older gccs do not).  This trick suppresses the
     * warning, but causes errors for plain gcc, so is only used in the one
     * special case.  */
    static const union { __ULong __i[1]; float __d; } __Nanf = {0x7FC00000};
    #define NAN (__Nanf.__d)
  #else
    #define NAN (0.0F/0.0F)
  #endif
 #endif

#endif /* !gcc >= 3.3  */

#endif /* ! defined (HUGE_VAL) */

/* Reentrant ANSI C functions.  */

#ifndef __math_68881
extern double atan _PARAMS((double));
extern double cos _PARAMS((double));
extern double sin _PARAMS((double));
extern double tan _PARAMS((double));
extern double tanh _PARAMS((double));
extern double frexp _PARAMS((double, int *));
extern double modf _PARAMS((double, double *));
extern double ceil _PARAMS((double));
extern double fabs _PARAMS((double));
extern double floor _PARAMS((double));
#endif /* ! defined (__math_68881) */

/* Non reentrant ANSI C functions.  */

#ifndef _REENT_ONLY
#ifndef __math_6881
extern double acos _PARAMS((double));
extern double asin _PARAMS((double));
extern double atan2 _PARAMS((double, double));
extern double cosh _PARAMS((double));
extern double sinh _PARAMS((double));
extern double exp _PARAMS((double));
extern double ldexp _PARAMS((double, int));
extern double log _PARAMS((double));
extern double log10 _PARAMS((double));
extern double pow _PARAMS((double, double));
extern double sqrt _PARAMS((double));
extern double fmod _PARAMS((double, double));
#endif /* ! defined (__math_68881) */
#endif /* ! defined (_REENT_ONLY) */

#ifndef __STRICT_ANSI__

/* ISO C99 types and macros. */

#ifndef FLT_EVAL_METHOD
#define FLT_EVAL_METHOD 0
typedef float float_t;
typedef double double_t;
#endif /* FLT_EVAL_METHOD */

#define FP_NAN         0
#define FP_INFINITE    1
#define FP_ZERO        2
#define FP_SUBNORMAL   3
#define FP_NORMAL      4

extern int __fpclassifyf (float x);
extern int __fpclassifyd (double x);
extern int __signbitf (float x);
extern int __signbitd (double x);

#define fpclassify(x) \
          (__extension__ ({__typeof__(x) __x = (x); \
                           (sizeof (__x) == sizeof (float))  ? __fpclassifyf(__x) : __fpclassifyd(__x);}))

#define isfinite(x) \
          (__extension__ ({__typeof__(x) __x = (x); \
                           fpclassify(__x) != FP_INFINITE && fpclassify(__x) != FP_NAN;}))
#define isnormal(x) \
          (__extension__ ({__typeof__(x) __x = (x); \
                           fpclassify(__x) == FP_NORMAL;}))
#define signbit(x) \
          (__extension__ ({__typeof__(x) __x = (x); \
                           (sizeof(__x) == sizeof(float)) ? __signbitf(__x) : __signbitd(__x);}))

#define isgreater(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           !isunordered(__x,__y) && (__x > __y);}))
#define isgreaterequal(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           !isunordered(__x,__y) && (__x >= __y);}))
#define isless(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           !isunordered(__x,__y) && (__x < __y);}))
#define islessequal(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           !isunordered(__x,__y) && (__x <= __y);}))
#define islessgreater(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           !isunordered(__x,__y) && (__x < __y || __x > __y);}))

#define isunordered(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           fpclassify(__x) == FP_NAN || fpclassify(__y) == FP_NAN;}))

/* Non ANSI double precision functions.  */

extern double infinity _PARAMS((void));
extern double nan _PARAMS((const char *));
extern int isnan _PARAMS((double));
extern int isinf _PARAMS((double));
extern int finite _PARAMS((double));
extern double copysign _PARAMS((double, double));
extern int ilogb _PARAMS((double));

extern double asinh _PARAMS((double));
extern double cbrt _PARAMS((double));
extern double nextafter _PARAMS((double, double));
extern double rint _PARAMS((double));
extern double scalbn _PARAMS((double, int));

extern double exp2 _PARAMS((double));
extern double scalbln _PARAMS((double, long int));
extern double tgamma _PARAMS((double));
extern double nearbyint _PARAMS((double));
extern long int lrint _PARAMS((double));
extern double round _PARAMS((double));
extern long int lround _PARAMS((double));
extern double trunc _PARAMS((double));
extern double remquo _PARAMS((double, double, int *));
extern double copysign _PARAMS((double, double));
extern double fdim _PARAMS((double, double));
extern double fmax _PARAMS((double, double));
extern double fmin _PARAMS((double, double));
extern double fma _PARAMS((double, double, double));
extern void sincos _PARAMS((double, double *, double *));

#ifndef __math_68881
extern double log1p _PARAMS((double));
extern double expm1 _PARAMS((double));
#endif /* ! defined (__math_68881) */

#ifndef _REENT_ONLY
extern double acosh _PARAMS((double));
extern double atanh _PARAMS((double));
extern double remainder _PARAMS((double, double));
extern double gamma _PARAMS((double));
extern double gamma_r _PARAMS((double, int *));
extern double lgamma _PARAMS((double));
extern double lgamma_r _PARAMS((double, int *));
extern double erf _PARAMS((double));
extern double erfc _PARAMS((double));
extern double y0 _PARAMS((double));
extern double y1 _PARAMS((double));
extern double yn _PARAMS((int, double));
extern double j0 _PARAMS((double));
extern double j1 _PARAMS((double));
extern double jn _PARAMS((int, double));
#define log2(x) (log (x) / M_LOG2_E)

#ifndef __math_68881
extern double hypot _PARAMS((double, double));
#endif

extern double cabs();
extern double drem _PARAMS((double, double));

#endif /* ! defined (_REENT_ONLY) */

#endif /* ! defined (__STRICT_ANSI__) */

#if !defined(__STRICT_ANSI__) || defined(__cplusplus)

/* Single precision versions of ANSI functions.  */

extern float atanf _PARAMS((float));
extern float cosf _PARAMS((float));
extern float sinf _PARAMS((float));
extern float tanf _PARAMS((float));
extern float tanhf _PARAMS((float));
extern float frexpf _PARAMS((float, int *));
extern float modff _PARAMS((float, float *));
extern float ceilf _PARAMS((float));
extern float fabsf _PARAMS((float));
extern float floorf _PARAMS((float));

#ifndef _REENT_ONLY
extern float acosf _PARAMS((float));
extern float asinf _PARAMS((float));
extern float atan2f _PARAMS((float, float));
extern float coshf _PARAMS((float));
extern float sinhf _PARAMS((float));
extern float expf _PARAMS((float));
extern float ldexpf _PARAMS((float, int));
extern float logf _PARAMS((float));
extern float log10f _PARAMS((float));
extern float powf _PARAMS((float, float));
extern float sqrtf _PARAMS((float));
extern float fmodf _PARAMS((float, float));
#endif /* ! defined (_REENT_ONLY) */

#endif /* !defined(__STRICT_ANSI__) || defined(__cplusplus) */

#ifndef __STRICT_ANSI__

/* Other single precision functions.  */

extern float exp2f _PARAMS((float));
extern float scalblnf _PARAMS((float, long int));
extern float tgammaf _PARAMS((float));
extern float nearbyintf _PARAMS((float));
extern long int lrintf _PARAMS((float));
extern float roundf _PARAMS((float));
extern long int lroundf _PARAMS((float));
extern float truncf _PARAMS((float));
extern float remquof _PARAMS((float, float, int *));
extern float copysignf _PARAMS((float, float));
extern float fdimf _PARAMS((float, float));
extern float fmaxf _PARAMS((float, float));
extern float fminf _PARAMS((float, float));
extern float fmaf _PARAMS((float, float, float));

extern float infinityf _PARAMS((void));
extern float nanf _PARAMS((const char *));
extern int isnanf _PARAMS((float));
extern int isinff _PARAMS((float));
extern int finitef _PARAMS((float));
extern float copysignf _PARAMS((float, float));
extern int ilogbf _PARAMS((float));

extern float asinhf _PARAMS((float));
extern float cbrtf _PARAMS((float));
extern float nextafterf _PARAMS((float, float));
extern float rintf _PARAMS((float));
extern float scalbnf _PARAMS((float, int));
extern float log1pf _PARAMS((float));
extern float expm1f _PARAMS((float));
extern void sincosf _PARAMS((float, float *, float *));

#ifndef _REENT_ONLY
extern float acoshf _PARAMS((float));
extern float atanhf _PARAMS((float));
extern float remainderf _PARAMS((float, float));
extern float gammaf _PARAMS((float));
extern float gammaf_r _PARAMS((float, int *));
extern float lgammaf _PARAMS((float));
extern float lgammaf_r _PARAMS((float, int *));
extern float erff _PARAMS((float));
extern float erfcf _PARAMS((float));
extern float y0f _PARAMS((float));
extern float y1f _PARAMS((float));
extern float ynf _PARAMS((int, float));
extern float j0f _PARAMS((float));
extern float j1f _PARAMS((float));
extern float jnf _PARAMS((int, float));
#define log2f(x) (logf (x) / (float) M_LOG2_E)
extern float hypotf _PARAMS((float, float));

extern float cabsf();
extern float dremf _PARAMS((float, float));

#endif /* ! defined (_REENT_ONLY) */

/* The gamma functions use a global variable, signgam.  */
#ifndef _REENT_ONLY
#define signgam (*__signgam())
extern int *__signgam _PARAMS((void));
#endif /* ! defined (_REENT_ONLY) */

#define __signgam_r(ptr) _REENT_SIGNGAM(ptr)

/* The exception structure passed to the matherr routine.  */

#ifdef __cplusplus
struct __exception 
#else
struct exception 
#endif
{
  int type;
  char *name;
  double arg1;
  double arg2;
  double retval;
  int err;
};

#ifdef __cplusplus
extern int matherr _PARAMS((struct __exception *e));
#else
extern int matherr _PARAMS((struct exception *e));
#endif

/* Values for the type field of struct exception.  */

#define DOMAIN 1
#define SING 2
#define OVERFLOW 3
#define UNDERFLOW 4
#define TLOSS 5
#define PLOSS 6

/* Useful constants.  */

#define MAXFLOAT	3.40282347e+38F

#define M_E		2.7182818284590452354
#define M_LOG2E		1.4426950408889634074
#define M_LOG10E	0.43429448190325182765
#define M_LN2		0.69314718055994530942
#define M_LN10		2.30258509299404568402
#define M_PI		3.14159265358979323846
#define M_TWOPI         (M_PI * 2.0)
#define M_PI_2		1.57079632679489661923
#define M_PI_4		0.78539816339744830962
#define M_3PI_4		2.3561944901923448370E0
#define M_SQRTPI        1.77245385090551602792981
#define M_1_PI		0.31830988618379067154
#define M_2_PI		0.63661977236758134308
#define M_2_SQRTPI	1.12837916709551257390
#define M_SQRT2		1.41421356237309504880
#define M_SQRT1_2	0.70710678118654752440
#define M_LN2LO         1.9082149292705877000E-10
#define M_LN2HI         6.9314718036912381649E-1
#define M_SQRT3   	1.73205080756887719000
#define M_IVLN10        0.43429448190325182765 /* 1 / log(10) */
#define M_LOG2_E        0.693147180559945309417
#define M_INVLN2        1.4426950408889633870E0  /* 1 / log(2) */

/* Global control over fdlibm error handling.  */

enum __fdlibm_version
{
  __fdlibm_ieee = -1,
  __fdlibm_svid,
  __fdlibm_xopen,
  __fdlibm_posix
};

#define _LIB_VERSION_TYPE enum __fdlibm_version
#define _LIB_VERSION __fdlib_version

extern __IMPORT _CONST _LIB_VERSION_TYPE _LIB_VERSION;

#define _IEEE_  __fdlibm_ieee
#define _SVID_  __fdlibm_svid
#define _XOPEN_ __fdlibm_xopen
#define _POSIX_ __fdlibm_posix

#endif /* ! defined (__STRICT_ANSI__) */

_END_STD_C

#ifdef __FAST_MATH__
#include <machine/fastmath.h>
#endif

#endif /* _MATH_H_ */
