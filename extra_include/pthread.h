/*  pthread.h
 *
 *  Written by Joel Sherrill <joel@OARcorp.com>.
 *
 *  COPYRIGHT (c) 1989-2000.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  Permission to use, copy, modify, and distribute this software for any
 *  purpose without fee is hereby granted, provided that this entire notice
 *  is included in all copies of any software which is or includes a copy
 *  or modification of this software.
 *
 *  THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 *  WARRANTY.  IN PARTICULAR,  THE AUTHOR MAKES NO REPRESENTATION
 *  OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY OF THIS
 *  SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 *  $Id: pthread.h,v 1.3 2002/10/08 13:03:07 joel Exp $
 */

#ifndef __PTHREAD_h
#define __PTHREAD_h

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>

#if defined(_POSIX_THREADS)

#include <sys/types.h>
#include <time.h>
#include <sys/sched.h>

/* Register Fork Handlers, P1003.1c/Draft 10, P1003.1c/Draft 10, p. 27
  
    If an OS does not support processes, then it falls under this provision
    and may not provide pthread_atfork():
  
    "Either the implementation shall support the pthread_atfork() function
     as described above or the pthread_atfork() funciton shall not be 
     provided."
  
    NOTE: RTEMS does not provide pthread_atfork().  */

#if !defined(__rtems__)
#warning "Add pthread_atfork() prototype"
#endif

/* Mutex Initialization Attributes, P1003.1c/Draft 10, p. 81 */

int	_EXFUN(pthread_mutexattr_init, (pthread_mutexattr_t *attr));
int	_EXFUN(pthread_mutexattr_destroy, (pthread_mutexattr_t *attr));
int	_EXFUN(pthread_mutexattr_getpshared,
		(const pthread_mutexattr_t *attr, int  *pshared));
int	_EXFUN(pthread_mutexattr_setpshared,
		(pthread_mutexattr_t *attr, int pshared));

/* Initializing and Destroying a Mutex, P1003.1c/Draft 10, p. 87 */

int	_EXFUN(pthread_mutex_init,
	(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr));
int	_EXFUN(pthread_mutex_destroy, (pthread_mutex_t *mutex));

/* This is used to statically initialize a pthread_mutex_t. Example:
  
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
 */

#define PTHREAD_MUTEX_INITIALIZER  ((pthread_mutex_t) 0xFFFFFFFF)

/*  Locking and Unlocking a Mutex, P1003.1c/Draft 10, p. 93
    NOTE: P1003.4b/D8 adds pthread_mutex_timedlock(), p. 29 */

int	_EXFUN(pthread_mutex_lock, (pthread_mutex_t *mutex));
int	_EXFUN(pthread_mutex_trylock, (pthread_mutex_t *mutex));
int	_EXFUN(pthread_mutex_unlock, (pthread_mutex_t *mutex));

#if defined(_POSIX_TIMEOUTS)

int	_EXFUN(pthread_mutex_timedlock,
	(pthread_mutex_t *mutex, const struct timespec *timeout));

#endif /* _POSIX_TIMEOUTS */

/* Condition Variable Initialization Attributes, P1003.1c/Draft 10, p. 96 */
 
int	_EXFUN(pthread_condattr_init, (pthread_condattr_t *attr));
int	_EXFUN(pthread_condattr_destroy, (pthread_condattr_t *attr));
int	_EXFUN(pthread_condattr_getpshared,
		(const pthread_condattr_t *attr, int *pshared));
int	_EXFUN(pthread_condattr_setpshared,
		(pthread_condattr_t *attr, int pshared));
 
/* Initializing and Destroying a Condition Variable, P1003.1c/Draft 10, p. 87 */
 
int	_EXFUN(pthread_cond_init,
	(pthread_cond_t *cond, const pthread_condattr_t *attr));
int	_EXFUN(pthread_cond_destroy, (pthread_cond_t *mutex));
 
/* This is used to statically initialize a pthread_cond_t. Example:
  
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
 */
 
#define PTHREAD_COND_INITIALIZER  ((pthread_mutex_t) 0xFFFFFFFF)
 
/* Broadcasting and Signaling a Condition, P1003.1c/Draft 10, p. 101 */
 
int	_EXFUN(pthread_cond_signal, (pthread_cond_t *cond));
int	_EXFUN(pthread_cond_broadcast, (pthread_cond_t *cond));
 
/* Waiting on a Condition, P1003.1c/Draft 10, p. 105 */
 
int	_EXFUN(pthread_cond_wait,
	(pthread_cond_t *cond, pthread_mutex_t *mutex));
 
int	_EXFUN(pthread_cond_timedwait,
		(pthread_cond_t *cond, pthread_mutex_t *mutex,
		const struct timespec *abstime));
 
#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)

/* Thread Creation Scheduling Attributes, P1003.1c/Draft 10, p. 120 */

int	_EXFUN(pthread_attr_setscope,
		(pthread_attr_t *attr, int contentionscope));
int	_EXFUN(pthread_attr_getscope,
	(const pthread_attr_t *attr, int *contentionscope));
int	_EXFUN(pthread_attr_setinheritsched,
		(pthread_attr_t *attr, int inheritsched));
int	_EXFUN(pthread_attr_getinheritsched,
		(const pthread_attr_t *attr, int *inheritsched));
int	_EXFUN(pthread_attr_setschedpolicy, (pthread_attr_t *attr, int policy));
int	_EXFUN(pthread_attr_getschedpolicy,
	(const pthread_attr_t *attr, int *policy));

#endif /* defined(_POSIX_THREAD_PRIORITY_SCHEDULING) */

int	_EXFUN(pthread_attr_setschedparam,
	(pthread_attr_t *attr, const struct sched_param *param));
int	_EXFUN(pthread_attr_getschedparam,
	(const pthread_attr_t *attr, struct sched_param *param));

#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)

/* Dynamic Thread Scheduling Parameters Access, P1003.1c/Draft 10, p. 124 */

int	_EXFUN(pthread_getschedparam,
	(pthread_t thread, int *policy, struct sched_param *param));
int	_EXFUN(pthread_setschedparam,
	(pthread_t thread, int policy, struct sched_param *param));

#endif /* defined(_POSIX_THREAD_PRIORITY_SCHEDULING) */

#if defined(_POSIX_THREAD_PRIO_INHERIT) || defined(_POSIX_THREAD_PRIO_PROTECT)

/* Mutex Initialization Scheduling Attributes, P1003.1c/Draft 10, p. 128 */
 
int	_EXFUN(pthread_mutexattr_setprotocol,
	(pthread_mutexattr_t *attr, int protocol));
int	_EXFUN(pthread_mutexattr_getprotocol,
		(const pthread_mutexattr_t *attr, int *protocol));
int	_EXFUN(pthread_mutexattr_setprioceiling,
	(pthread_mutexattr_t *attr, int prioceiling));
int	_EXFUN(pthread_mutexattr_getprioceiling,
	(const pthread_mutexattr_t *attr, int *prioceiling));

#endif /* _POSIX_THREAD_PRIO_INHERIT || _POSIX_THREAD_PRIO_PROTECT */

#if defined(_POSIX_THREAD_PRIO_PROTECT)

/* Change the Priority Ceiling of a Mutex, P1003.1c/Draft 10, p. 131 */

int	_EXFUN(pthread_mutex_setprioceiling,
	(pthread_mutex_t *mutex, int prioceiling, int *old_ceiling));
int	_EXFUN(pthread_mutex_getprioceiling,
	(pthread_mutex_t *mutex, int *prioceiling));

#endif /* _POSIX_THREAD_PRIO_PROTECT */

/* Thread Creation Attributes, P1003.1c/Draft 10, p, 140 */

int	_EXFUN(pthread_attr_init, (pthread_attr_t *attr));
int	_EXFUN(pthread_attr_destroy, (pthread_attr_t *attr));
int	_EXFUN(pthread_attr_getstacksize,
	(const pthread_attr_t *attr, size_t *stacksize));
int	_EXFUN(pthread_attr_setstacksize,
	(pthread_attr_t *attr, size_t stacksize));
int	_EXFUN(pthread_attr_getstackaddr,
	(const pthread_attr_t *attr, void **stackaddr));
int	_EXFUN(pthread_attr_setstackaddr,
	(pthread_attr_t  *attr, void *stackaddr));
int	_EXFUN(pthread_attr_getdetachstate,
	(const pthread_attr_t *attr, int *detachstate));
int	_EXFUN(pthread_attr_setdetachstate,
	(pthread_attr_t *attr, int detachstate));

/* Thread Creation, P1003.1c/Draft 10, p. 144 */

int	_EXFUN(pthread_create,
	(pthread_t *thread, const pthread_attr_t  *attr,
	void *(*start_routine)( void * ), void *arg));

/* Wait for Thread Termination, P1003.1c/Draft 10, p. 147 */

int	_EXFUN(pthread_join, (pthread_t thread, void **value_ptr));

/* Detaching a Thread, P1003.1c/Draft 10, p. 149 */

int	_EXFUN(pthread_detach, (pthread_t thread));

/* Thread Termination, p1003.1c/Draft 10, p. 150 */

void	_EXFUN(pthread_exit, (void *value_ptr));

/* Get Calling Thread's ID, p1003.1c/Draft 10, p. XXX */

pthread_t	_EXFUN(pthread_self, (void));

/* Compare Thread IDs, p1003.1c/Draft 10, p. 153 */

int	_EXFUN(pthread_equal, (pthread_t t1, pthread_t t2));

/* Dynamic Package Initialization */

/* This is used to statically initialize a pthread_once_t. Example:
  
    pthread_once_t once = PTHREAD_ONCE_INIT;
  
    NOTE:  This is named inconsistently -- it should be INITIALIZER.  */
 
#define PTHREAD_ONCE_INIT  { 1, 0 }  /* is initialized and not run */
 
int	_EXFUN(pthread_once,
	(pthread_once_t *once_control, void (*init_routine)(void)));

/* Thread-Specific Data Key Create, P1003.1c/Draft 10, p. 163 */

int	_EXFUN(pthread_key_create,
	(pthread_key_t *key, void (*destructor)( void * )));

/* Thread-Specific Data Management, P1003.1c/Draft 10, p. 165 */

int	_EXFUN(pthread_setspecific, (pthread_key_t key, const void *value));
void *	_EXFUN(pthread_getspecific, (pthread_key_t key));

/* Thread-Specific Data Key Deletion, P1003.1c/Draft 10, p. 167 */

int	_EXFUN(pthread_key_delete, (pthread_key_t key));

/* Execution of a Thread, P1003.1c/Draft 10, p. 181 */

#define PTHREAD_CANCEL_ENABLE  0
#define PTHREAD_CANCEL_DISABLE 1

#define PTHREAD_CANCEL_DEFERRED 0
#define PTHREAD_CANCEL_ASYNCHRONOUS 1

#define PTHREAD_CANCELED ((void *) -1)

int	_EXFUN(pthread_cancel, (pthread_t thread));

/* Setting Cancelability State, P1003.1c/Draft 10, p. 183 */

int	_EXFUN(pthread_setcancelstate, (int state, int *oldstate));
int	_EXFUN(pthread_setcanceltype, (int type, int *oldtype));
void 	_EXFUN(pthread_testcancel, (void));

/* Establishing Cancellation Handlers, P1003.1c/Draft 10, p. 184 */

void 	_EXFUN(pthread_cleanup_push, (void (*routine)( void * ), void *arg));
void 	_EXFUN(pthread_cleanup_pop, (int execute));

#if defined(_POSIX_THREAD_CPUTIME)
 
/* Accessing a Thread CPU-time Clock, P1003.4b/D8, p. 58 */
 
int	_EXFUN(pthread_getcpuclockid,
	(pthread_t thread_id, clockid_t *clock_id));
 
/* CPU-time Clock Thread Creation Attribute, P1003.4b/D8, p. 59 */

int	_EXFUN(pthread_attr_setcputime,
	(pthread_attr_t *attr, int clock_allowed));

int	_EXFUN(pthread_attr_getcputime,
	(pthread_attr_t *attr, int *clock_allowed));

#endif /* defined(_POSIX_THREAD_CPUTIME) */

#endif /* defined(_POSIX_THREADS) */

#ifdef __cplusplus
}
#endif

#endif
/* end of include file */
