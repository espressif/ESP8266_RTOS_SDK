/* This header file provides the reentrancy.  */

/* The reentrant system calls here serve two purposes:

   1) Provide reentrant versions of the system calls the ANSI C library
      requires.
   2) Provide these system calls in a namespace clean way.

   It is intended that *all* system calls that the ANSI C library needs
   be declared here.  It documents them all in one place.  All library access
   to the system is via some form of these functions.

   There are three ways a target may provide the needed syscalls.

   1) Define the reentrant versions of the syscalls directly.
      (eg: _open_r, _close_r, etc.).  Please keep the namespace clean.
      When you do this, set "syscall_dir" to "syscalls" and add
      -DREENTRANT_SYSCALLS_PROVIDED to newlib_cflags in configure.host.

   2) Define namespace clean versions of the system calls by prefixing
      them with '_' (eg: _open, _close, etc.).  Technically, there won't be
      true reentrancy at the syscall level, but the library will be namespace
      clean.
      When you do this, set "syscall_dir" to "syscalls" in configure.host.

   3) Define or otherwise provide the regular versions of the syscalls
      (eg: open, close, etc.).  The library won't be reentrant nor namespace
      clean, but at least it will work.
      When you do this, add -DMISSING_SYSCALL_NAMES to newlib_cflags in
      configure.host.

   Stubs of the reentrant versions of the syscalls exist in the libc/reent
   source directory and are used if REENTRANT_SYSCALLS_PROVIDED isn't defined.
   They use the native system calls: _open, _close, etc. if they're available
   (MISSING_SYSCALL_NAMES is *not* defined), otherwise open, close, etc.
   (MISSING_SYSCALL_NAMES *is* defined).  */

/* WARNING: All identifiers here must begin with an underscore.  This file is
   included by stdio.h and others and we therefore must only use identifiers
   in the namespace allotted to us.  */

#ifndef _REENT_H_
#ifdef __cplusplus
extern "C" {
#endif
#define _REENT_H_

#include <sys/reent.h>
#include <sys/_types.h>
#include <machine/types.h>

#define __need_size_t
#define __need_ptrdiff_t
#include <stddef.h>

/* FIXME: not namespace clean */
struct stat;
struct tms;
struct timeval;
struct timezone;

/* Reentrant versions of system calls.  */

extern int _close_r _PARAMS ((struct _reent *, int));
extern int _execve_r _PARAMS ((struct _reent *, char *, char **, char **));
extern int _fcntl_r _PARAMS ((struct _reent *, int, int, int));
extern int _fork_r _PARAMS ((struct _reent *));
extern int _fstat_r _PARAMS ((struct _reent *, int, struct stat *));
extern int _getpid_r _PARAMS ((struct _reent *));
extern int _kill_r _PARAMS ((struct _reent *, int, int));
extern int _link_r _PARAMS ((struct _reent *, const char *, const char *));
extern _off_t _lseek_r _PARAMS ((struct _reent *, int, _off_t, int));
extern int _open_r _PARAMS ((struct _reent *, const char *, int, int));
extern _ssize_t _read_r _PARAMS ((struct _reent *, int, void *, size_t));
extern void *_sbrk_r _PARAMS ((struct _reent *, ptrdiff_t));
extern int _stat_r _PARAMS ((struct _reent *, const char *, struct stat *));
extern _CLOCK_T_ _times_r _PARAMS ((struct _reent *, struct tms *));
extern int _unlink_r _PARAMS ((struct _reent *, const char *));
extern int _wait_r _PARAMS ((struct _reent *, int *));
extern _ssize_t _write_r _PARAMS ((struct _reent *, int, const void *, size_t));

/* This one is not guaranteed to be available on all targets.  */
extern int _gettimeofday_r _PARAMS ((struct _reent *, struct timeval *tp, struct timezone *tzp));

#ifdef __LARGE64_FILES

#if defined(__CYGWIN__) && defined(_COMPILING_NEWLIB)
#define stat64 __stat64
#endif

struct stat64;

extern _off64_t _lseek64_r _PARAMS ((struct _reent *, int, _off64_t, int));
extern int _fstat64_r _PARAMS ((struct _reent *, int, struct stat64 *));
extern int _open64_r _PARAMS ((struct _reent *, const char *, int, int));
#endif

#ifdef __cplusplus
}
#endif
#endif /* _REENT_H_ */
