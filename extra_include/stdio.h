/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)stdio.h	5.3 (Berkeley) 3/15/86
 */

/*
 * NB: to fit things in six character monocase externals, the
 * stdio code uses the prefix `__s' for stdio objects, typically
 * followed by a three-character attempt at a mnemonic.
 */

#ifndef _STDIO_H_
#define	_STDIO_H_

#include "_ansi.h"

#define	_FSTDIO			/* ``function stdio'' */

#define __need_size_t
#include <stddef.h>

#define __need___va_list
#include <stdarg.h>

/*
 * <sys/reent.h> defines __FILE, _fpos_t.
 * They must be defined there because struct _reent needs them (and we don't
 * want reent.h to include this file.
 */

#include <sys/reent.h>
#include <sys/types.h>

_BEGIN_STD_C

typedef __FILE FILE;

#ifdef __CYGWIN__
#ifdef __CYGWIN_USE_BIG_TYPES__
typedef _fpos64_t fpos_t;
#else
typedef _fpos_t fpos_t;
#endif
#else
typedef _fpos_t fpos_t;
#ifdef __LARGE64_FILES
typedef _fpos64_t fpos64_t;
#endif
#endif /* !__CYGWIN__ */

#include <sys/stdio.h>

#define	__SLBF	0x0001		/* line buffered */
#define	__SNBF	0x0002		/* unbuffered */
#define	__SRD	0x0004		/* OK to read */
#define	__SWR	0x0008		/* OK to write */
	/* RD and WR are never simultaneously asserted */
#define	__SRW	0x0010		/* open for reading & writing */
#define	__SEOF	0x0020		/* found EOF */
#define	__SERR	0x0040		/* found error */
#define	__SMBF	0x0080		/* _buf is from malloc */
#define	__SAPP	0x0100		/* fdopen()ed in append mode - so must  write to end */
#define	__SSTR	0x0200		/* this is an sprintf/snprintf string */
#define	__SOPT	0x0400		/* do fseek() optimisation */
#define	__SNPT	0x0800		/* do not do fseek() optimisation */
#define	__SOFF	0x1000		/* set iff _offset is in fact correct */
#define	__SMOD	0x2000		/* true => fgetline modified _p text */
#if defined(__CYGWIN__)
#  define __SCLE  0x4000        /* convert line endings CR/LF <-> NL */
#endif
#define	__SL64	0x8000		/* is 64-bit offset large file */

/*
 * The following three definitions are for ANSI C, which took them
 * from System V, which stupidly took internal interface macros and
 * made them official arguments to setvbuf(), without renaming them.
 * Hence, these ugly _IOxxx names are *supposed* to appear in user code.
 *
 * Although these happen to match their counterparts above, the
 * implementation does not rely on that (so these could be renumbered).
 */
#define	_IOFBF	0		/* setvbuf should set fully buffered */
#define	_IOLBF	1		/* setvbuf should set line buffered */
#define	_IONBF	2		/* setvbuf should set unbuffered */

#ifndef NULL
#define	NULL	0
#endif

#define	EOF	(-1)

#ifdef __BUFSIZ__
#define	BUFSIZ		__BUFSIZ__
#else
#define	BUFSIZ		1024
#endif

#ifdef __FOPEN_MAX__
#define FOPEN_MAX	__FOPEN_MAX__
#else
#define	FOPEN_MAX	20
#endif

#ifdef __FILENAME_MAX__
#define FILENAME_MAX    __FILENAME_MAX__
#else
#define	FILENAME_MAX	1024
#endif

#ifdef __L_tmpnam__
#define L_tmpnam	__L_tmpnam__
#else
#define	L_tmpnam	FILENAME_MAX
#endif

#ifndef __STRICT_ANSI__
#define P_tmpdir        "/tmp"
#endif

#ifndef SEEK_SET
#define	SEEK_SET	0	/* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define	SEEK_CUR	1	/* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define	SEEK_END	2	/* set file offset to EOF plus offset */
#endif

#define	TMP_MAX		26

#ifndef _REENT_ONLY
#define	stdin	(_REENT->_stdin)
#define	stdout	(_REENT->_stdout)
#define	stderr	(_REENT->_stderr)
#else /* _REENT_ONLY */
#define	stdin	(_impure_ptr->_stdin)
#define	stdout	(_impure_ptr->_stdout)
#define	stderr	(_impure_ptr->_stderr)
#endif /* _REENT_ONLY */

#define _stdin_r(x)	((x)->_stdin)
#define _stdout_r(x)	((x)->_stdout)
#define _stderr_r(x)	((x)->_stderr)

/*
 * Functions defined in ANSI C standard.
 */

#ifdef __GNUC__
#define __VALIST __gnuc_va_list
#else
#define __VALIST char*
#endif

FILE *	_EXFUN(tmpfile, (void));
char *	_EXFUN(tmpnam, (char *));
int	_EXFUN(fclose, (FILE *));
int	_EXFUN(fflush, (FILE *));
FILE *	_EXFUN(freopen, (const char *, const char *, FILE *));
void	_EXFUN(setbuf, (FILE *, char *));
int	_EXFUN(setvbuf, (FILE *, char *, int, size_t));
int	_EXFUN(fprintf, (FILE *, const char *, ...));
int	_EXFUN(fscanf, (FILE *, const char *, ...));
int	_EXFUN(printf, (const char *, ...));
int	_EXFUN(scanf, (const char *, ...));
int	_EXFUN(sscanf, (const char *, const char *, ...));
int	_EXFUN(vfprintf, (FILE *, const char *, __VALIST));
int	_EXFUN(vprintf, (const char *, __VALIST));
int	_EXFUN(vsprintf, (char *, const char *, __VALIST));
int	_EXFUN(fgetc, (FILE *));
char *  _EXFUN(fgets, (char *, int, FILE *));
int	_EXFUN(fputc, (int, FILE *));
int	_EXFUN(fputs, (const char *, FILE *));
int	_EXFUN(getc, (FILE *));
int	_EXFUN(getchar, (void));
char *  _EXFUN(gets, (char *));
int	_EXFUN(putc, (int, FILE *));
int	_EXFUN(putchar, (int));
int	_EXFUN(puts, (const char *));
int	_EXFUN(ungetc, (int, FILE *));
size_t	_EXFUN(fread, (_PTR, size_t _size, size_t _n, FILE *));
size_t	_EXFUN(fwrite, (const _PTR , size_t _size, size_t _n, FILE *));
#ifdef _COMPILING_NEWLIB
int	_EXFUN(fgetpos, (FILE *, _fpos_t *));
#else
int	_EXFUN(fgetpos, (FILE *, fpos_t *));
#endif
int	_EXFUN(fseek, (FILE *, long, int));
#ifdef _COMPILING_NEWLIB
int	_EXFUN(fsetpos, (FILE *, const _fpos_t *));
#else
int	_EXFUN(fsetpos, (FILE *, const fpos_t *));
#endif
long	_EXFUN(ftell, ( FILE *));
void	_EXFUN(rewind, (FILE *));
void	_EXFUN(clearerr, (FILE *));
int	_EXFUN(feof, (FILE *));
int	_EXFUN(ferror, (FILE *));
void    _EXFUN(perror, (const char *));
#ifndef _REENT_ONLY
FILE *	_EXFUN(fopen, (const char *_name, const char *_type));
int	_EXFUN(sprintf, (char *, const char *, ...));
int	_EXFUN(remove, (const char *));
int	_EXFUN(rename, (const char *, const char *));
#endif
#ifndef __STRICT_ANSI__
#ifdef _COMPILING_NEWLIB
int	_EXFUN(fseeko, (FILE *, _off_t, int));
_off_t	_EXFUN(ftello, ( FILE *));
#else
int	_EXFUN(fseeko, (FILE *, off_t, int));
off_t	_EXFUN(ftello, ( FILE *));
#endif
#ifndef _REENT_ONLY
int	_EXFUN(asiprintf, (char **, const char *, ...));
int	_EXFUN(asprintf, (char **, const char *, ...));
int	_EXFUN(fcloseall, (_VOID));
int	_EXFUN(fiprintf, (FILE *, const char *, ...));
int	_EXFUN(iprintf, (const char *, ...));
int	_EXFUN(siprintf, (char *, const char *, ...));
int	_EXFUN(snprintf, (char *, size_t, const char *, ...));
int	_EXFUN(sniprintf, (char *, size_t, const char *, ...));
char *	_EXFUN(tempnam, (const char *, const char *));
int	_EXFUN(vasiprintf, (char **, const char *, __VALIST));
int	_EXFUN(vasprintf, (char **, const char *, __VALIST));
int	_EXFUN(vsniprintf, (char *, size_t, const char *, __VALIST));
int	_EXFUN(vsnprintf, (char *, size_t, const char *, __VALIST));
int	_EXFUN(vfiprintf, (FILE *, const char *, __VALIST));
int	_EXFUN(vfiscanf, (FILE *, const char *, __VALIST));
int	_EXFUN(vfscanf, (FILE *, const char *, __VALIST));
int	_EXFUN(viscanf, (const char *, __VALIST));
int	_EXFUN(vscanf, (const char *, __VALIST));
int	_EXFUN(vsiscanf, (const char *, const char *, __VALIST));
int	_EXFUN(vsscanf, (const char *, const char *, __VALIST));
#endif
#endif

/*
 * Routines in POSIX 1003.1.
 */

#ifndef __STRICT_ANSI__
#ifndef _REENT_ONLY
FILE *	_EXFUN(fdopen, (int, const char *));
#endif
int	_EXFUN(fileno, (FILE *));
int	_EXFUN(getw, (FILE *));
int	_EXFUN(pclose, (FILE *));
FILE *  _EXFUN(popen, (const char *, const char *));
int	_EXFUN(putw, (int, FILE *));
void    _EXFUN(setbuffer, (FILE *, char *, int));
int	_EXFUN(setlinebuf, (FILE *));
int	_EXFUN(getc_unlocked, (FILE *));
int	_EXFUN(getchar_unlocked, (void));
void	_EXFUN(flockfile, (FILE *));
int	_EXFUN(ftrylockfile, (FILE *));
void	_EXFUN(funlockfile, (FILE *));
int	_EXFUN(putc_unlocked, (int, FILE *));
int	_EXFUN(putchar_unlocked, (int));
#endif

/*
 * Recursive versions of the above.
 */

int	_EXFUN(_asiprintf_r, (struct _reent *, char **, const char *, ...));
int	_EXFUN(_asprintf_r, (struct _reent *, char **, const char *, ...));
int	_EXFUN(_fcloseall_r, (struct _reent *));
FILE *	_EXFUN(_fdopen_r, (struct _reent *, int, const char *));
FILE *	_EXFUN(_fopen_r, (struct _reent *, const char *, const char *));
int	_EXFUN(_fclose_r, (struct _reent *, FILE *));
int	_EXFUN(_fiscanf_r, (struct _reent *, FILE *, const char *, ...));
int	_EXFUN(_fscanf_r, (struct _reent *, FILE *, const char *, ...));
int	_EXFUN(_fseek_r, (struct _reent *, FILE *, long, int));
long	_EXFUN(_ftell_r, (struct _reent *, FILE *));
int	_EXFUN(_getchar_r, (struct _reent *));
char *	_EXFUN(_gets_r, (struct _reent *, char *));
int	_EXFUN(_iprintf_r, (struct _reent *, const char *, ...));
int	_EXFUN(_iscanf_r, (struct _reent *, const char *, ...));
int	_EXFUN(_mkstemp_r, (struct _reent *, char *));
char *	_EXFUN(_mktemp_r, (struct _reent *, char *));
void	_EXFUN(_perror_r, (struct _reent *, const char *));
int	_EXFUN(_printf_r, (struct _reent *, const char *, ...));
int	_EXFUN(_putchar_r, (struct _reent *, int));
int	_EXFUN(_puts_r, (struct _reent *, const char *));
int	_EXFUN(_remove_r, (struct _reent *, const char *));
int	_EXFUN(_rename_r, (struct _reent *,
			   const char *_old, const char *_new));
int	_EXFUN(_scanf_r, (struct _reent *, const char *, ...));
int	_EXFUN(_siprintf_r, (struct _reent *, char *, const char *, ...));
int	_EXFUN(_siscanf_r, (struct _reent *, const char *, const char *, ...));
int	_EXFUN(_sniprintf_r, (struct _reent *, char *, size_t, const char *, ...));
int	_EXFUN(_snprintf_r, (struct _reent *, char *, size_t, const char *, ...));
int	_EXFUN(_sprintf_r, (struct _reent *, char *, const char *, ...));
int	_EXFUN(_sscanf_r, (struct _reent *, const char *, const char *, ...));
char *	_EXFUN(_tempnam_r, (struct _reent *, const char *, const char *));
FILE *	_EXFUN(_tmpfile_r, (struct _reent *));
char *	_EXFUN(_tmpnam_r, (struct _reent *, char *));
int	_EXFUN(_ungetc_r, (struct _reent *, int, FILE *));
int	_EXFUN(_vasiprintf_r, (struct _reent *, char **, const char *, __VALIST));
int	_EXFUN(_vasprintf_r, (struct _reent *, char **, const char *, __VALIST));
int	_EXFUN(_vfiprintf_r, (struct _reent *, FILE *, const char *, __VALIST));
int	_EXFUN(_vfprintf_r, (struct _reent *, FILE *, const char *, __VALIST));
int	_EXFUN(_viprintf_r, (struct _reent *, const char *, __VALIST));
int	_EXFUN(_vprintf_r, (struct _reent *, const char *, __VALIST));
int	_EXFUN(_vsiprintf_r, (struct _reent *, char *, const char *, __VALIST));
int	_EXFUN(_vsprintf_r, (struct _reent *, char *, const char *, __VALIST));
int	_EXFUN(_vsniprintf_r, (struct _reent *, char *, size_t, const char *, __VALIST));
int	_EXFUN(_vsnprintf_r, (struct _reent *, char *, size_t, const char *, __VALIST));
int	_EXFUN(_vfiscanf_r, (struct _reent *, FILE *, const char *, __VALIST));
int	_EXFUN(_vfscanf_r, (struct _reent *, FILE *, const char *, __VALIST));
int	_EXFUN(_viscanf_r, (struct _reent *, const char *, __VALIST));
int	_EXFUN(_vscanf_r, (struct _reent *, const char *, __VALIST));
int	_EXFUN(_vsscanf_r, (struct _reent *, const char *, const char *, __VALIST));
int	_EXFUN(_vsiscanf_r, (struct _reent *, const char *, const char *, __VALIST));

ssize_t _EXFUN(__getdelim, (char **, size_t *, int, FILE *));
ssize_t _EXFUN(__getline, (char **, size_t *, FILE *));

#ifdef __LARGE64_FILES
#if !defined(__CYGWIN__) || defined(_COMPILING_NEWLIB)
FILE *	_EXFUN(fdopen64, (int, const char *));
FILE *  _EXFUN(fopen64, (const char *, const char *));
_off64_t _EXFUN(ftello64, (FILE *));
_off64_t _EXFUN(fseeko64, (FILE *, _off64_t, int));
int     _EXFUN(fgetpos64, (FILE *, _fpos64_t *));
int     _EXFUN(fsetpos64, (FILE *, const _fpos64_t *));
FILE *  _EXFUN(tmpfile64, (void));

FILE *	_EXFUN(_fdopen64_r, (struct _reent *, int, const char *));
FILE *  _EXFUN(_fopen64_r, (struct _reent *,const char *, const char *));
_off64_t _EXFUN(_ftello64_r, (struct _reent *, FILE *));
_off64_t _EXFUN(_fseeko64_r, (struct _reent *, FILE *, _off64_t, int));
int     _EXFUN(_fgetpos64_r, (struct _reent *, FILE *, _fpos64_t *));
int     _EXFUN(_fsetpos64_r, (struct _reent *, FILE *, const _fpos64_t *));
FILE *  _EXFUN(_tmpfile64_r, (struct _reent *));
#endif /* !__CYGWIN__ */
#endif /* __LARGE64_FILES */
 
/*
 * Routines internal to the implementation.
 */

int	_EXFUN(__srget, (FILE *));
int	_EXFUN(__swbuf, (int, FILE *));

/*
 * Stdio function-access interface.
 */

#ifndef __STRICT_ANSI__
FILE	*_EXFUN(funopen,(const _PTR _cookie,
		int (*readfn)(_PTR _cookie, char *_buf, int _n),
		int (*writefn)(_PTR _cookie, const char *_buf, int _n),
		fpos_t (*seekfn)(_PTR _cookie, fpos_t _off, int _whence),
		int (*closefn)(_PTR _cookie)));

#define	fropen(cookie, fn) funopen(cookie, fn, (int (*)())0, (fpos_t (*)())0, (int (*)())0)
#define	fwopen(cookie, fn) funopen(cookie, (int (*)())0, fn, (fpos_t (*)())0, (int (*)())0)
#endif

/*
 * The __sfoo macros are here so that we can 
 * define function versions in the C library.
 */
#define       __sgetc_raw(p) (--(p)->_r < 0 ? __srget(p) : (int)(*(p)->_p++))

#ifdef __SCLE
static __inline__ int __sgetc(FILE *__p)
  {
    int __c = __sgetc_raw(__p);
    if ((__p->_flags & __SCLE) && (__c == '\r'))
      {
      int __c2 = __sgetc_raw(__p);
      if (__c2 == '\n')
        __c = __c2;
      else
        ungetc(__c2, __p);
      }
    return __c;
  }
#else
#define __sgetc(p) __sgetc_raw(p)
#endif

#ifdef _never /* __GNUC__ */
/* If this inline is actually used, then systems using coff debugging
   info get hopelessly confused.  21sept93 rich@cygnus.com.  */
static __inline int __sputc(int _c, FILE *_p) {
	if (--_p->_w >= 0 || (_p->_w >= _p->_lbfsize && (char)_c != '\n'))
		return (*_p->_p++ = _c);
	else
		return (__swbuf(_c, _p));
}
#else
/*
 * This has been tuned to generate reasonable code on the vax using pcc
 */
#define       __sputc_raw(c, p) \
	(--(p)->_w < 0 ? \
		(p)->_w >= (p)->_lbfsize ? \
			(*(p)->_p = (c)), *(p)->_p != '\n' ? \
				(int)*(p)->_p++ : \
				__swbuf('\n', p) : \
			__swbuf((int)(c), p) : \
		(*(p)->_p = (c), (int)*(p)->_p++))
#ifdef __SCLE
#define __sputc(c, p) \
        ((((p)->_flags & __SCLE) && ((c) == '\n')) \
          ? __sputc_raw('\r', (p)) : 0 , \
        __sputc_raw((c), (p)))
#else
#define __sputc(c, p) __sputc_raw(c, p)
#endif
#endif

#define	__sfeof(p)	(((p)->_flags & __SEOF) != 0)
#define	__sferror(p)	(((p)->_flags & __SERR) != 0)
#define	__sclearerr(p)	((void)((p)->_flags &= ~(__SERR|__SEOF)))
#define	__sfileno(p)	((p)->_file)

#define	feof(p)		__sfeof(p)
#define	ferror(p)	__sferror(p)
#define	clearerr(p)	__sclearerr(p)

#if 0 /*ndef __STRICT_ANSI__ - FIXME: must initialize stdio first, use fn */
#define	fileno(p)	__sfileno(p)
#endif

#ifndef __CYGWIN__
#ifndef lint
#define	getc(fp)	__sgetc(fp)
#define putc(x, fp)	__sputc(x, fp)
#endif /* lint */
#endif /* __CYGWIN__ */

#define	getchar()	getc(stdin)
#define	putchar(x)	putc(x, stdout)

#ifndef __STRICT_ANSI__
/* fast always-buffered version, true iff error */
#define	fast_putc(x,p) (--(p)->_w < 0 ? \
	__swbuf((int)(x), p) == EOF : (*(p)->_p = (x), (p)->_p++, 0))

#define	L_cuserid	9		/* posix says it goes in stdio.h :( */
#ifdef __CYGWIN__
#define L_ctermid       16
#endif
#endif

_END_STD_C

#endif /* _STDIO_H_ */
