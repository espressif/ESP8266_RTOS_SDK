#ifndef __SYS_LOCK_H__
#define __SYS_LOCK_H__

/* dummy lock routines for single-threaded aps */

typedef int _LOCK_T;
typedef int _LOCK_RECURSIVE_T;

#define __LOCK_INIT(class,lock) static int lock = 0;
#define __LOCK_INIT_RECURSIVE(class,lock) static int lock = 0;
#define __lock_init(lock) (0)
#define __lock_init_recursive(lock) (0)
#define __lock_close(lock) (0)
#define __lock_close_recursive(lock) (0)
#define __lock_acquire(lock) (0)
#define __lock_acquire_recursive(lock) (0)
#define __lock_try_acquire(lock) (0)
#define __lock_try_acquire_recursive(lock) (0)
#define __lock_release(lock) (0)
#define __lock_release_recursive(lock) (0)

#endif /* __SYS_LOCK_H__ */
