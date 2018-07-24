// Copyright 2018-2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "lwip/opt.h"

#ifdef SOCKETS_MT

#define SOCKETS_MT_DISABLE_SHUTDOWN

#include "lwip/priv/api_msg.h"

/* disable all LWIP socket API when compiling LWIP raw socket */

#undef lwip_accept
#undef lwip_bind
#undef lwip_shutdown
#undef lwip_getpeername
#undef lwip_getsockname
#undef lwip_setsockopt
#undef lwip_getsockopt
#undef lwip_close
#undef lwip_connect
#undef lwip_listen
#undef lwip_recv
#undef lwip_recvfrom
#undef lwip_send
#undef lwip_sendmsg
#undef lwip_sendto
#undef lwip_socket
#undef lwip_select
#undef lwip_ioctlsocket

#define lwip_accept       lwip_accept_esp
#define lwip_bind         lwip_bind_esp
#define lwip_shutdown     lwip_shutdown_esp
#define lwip_getpeername  lwip_getpeername_esp
#define lwip_getsockname  lwip_getsockname_esp
#define lwip_setsockopt   lwip_setsockopt_esp
#define lwip_getsockopt   lwip_getsockopt_esp
#define lwip_close        lwip_closesocket_esp
#define lwip_connect      lwip_connect_esp
#define lwip_listen       lwip_listen_esp
#define lwip_recv         lwip_recv_esp
#define lwip_recvfrom     lwip_recvfrom_esp
#define lwip_send         lwip_send_esp
#define lwip_sendmsg      lwip_sendmsg_esp
#define lwip_sendto       lwip_sendto_esp
#define lwip_socket       lwip_socket_esp
#define lwip_select       lwip_select_esp
#define lwip_ioctlsocket  lwip_ioctl_esp

#if LWIP_POSIX_SOCKETS_IO_NAMES
#undef lwip_read
#undef lwip_write
#undef lwip_writev
#undef lwip_close
#undef closesocket
#undef lwip_fcntl
#undef lwip_ioctl

#define lwip_read         lwip_read_esp
#define lwip_write        lwip_write_esp
#define lwip_writev       lwip_writev_esp
#define lwip_close        lwip_close_esp
#define lwip_fcntl        lwip_fcntl_esp
#define lwip_ioctl        lwip_ioctl_esp
#endif /* LWIP_POSIX_SOCKETS_IO_NAMES */

#include "../../lwip/src/api/sockets.c"

/* disable macros to enable LWIP function */

#undef lwip_accept
#undef lwip_bind
#undef lwip_shutdown
#undef lwip_getpeername
#undef lwip_getsockname
#undef lwip_setsockopt
#undef lwip_getsockopt
#undef lwip_close
#undef lwip_connect
#undef lwip_listen
#undef lwip_recv
#undef lwip_recvfrom
#undef lwip_send
#undef lwip_sendmsg
#undef lwip_sendto
#undef lwip_socket
#undef lwip_select
#undef lwip_ioctlsocket

#if LWIP_POSIX_SOCKETS_IO_NAMES
#undef lwip_read
#undef lwip_write
#undef lwip_writev
#undef lwip_close
#undef closesocket
#undef lwip_fcntl
#undef lwip_ioctl
#endif /* LWIP_POSIX_SOCKETS_IO_NAMES */

/********************************************************************/

#ifndef LWIP_SYNC_MT_SLEEP_MS
#define LWIP_SYNC_MT_SLEEP_MS 10
#endif

#ifndef SOCK_MT_DEBUG_LEVEL
#define SOCK_MT_DEBUG_LEVEL 255
#endif

typedef struct socket_conn_sync {
    sys_sem_t       *sem;
    struct netconn  *conn;
} socket_conn_sync_t;

typedef int (*lwip_io_mt_fn)(int, int );

enum sock_mt_stat {
    SOCK_MT_STATE_NONE = 0,
    SOCK_MT_STATE_BIND,
    SOCK_MT_STATE_LISTEN,
    SOCK_MT_STATE_ACCEPT,
    SOCK_MT_STATE_CONNECT,
    SOCK_MT_STATE_SEND,
};

enum sock_mt_shutdown {
    SOCK_MT_SHUTDOWN_OK = 0,
    SOCK_MT_SHUTDOWN_NONE
};

enum sock_mt_module {
    SOCK_MT_STATE,
    SOCK_MT_RECV,
    SOCK_MT_IOCTL,
    SOCK_MT_SELECT,

    SOCK_MT_SHUTDOWN,

    SOCK_MT_CLOSE,

    SOCK_MT_MODULE_MAX,
};

enum sock_mt_lock {
    SOCK_MT_STATE_LOCK,
    SOCK_MT_RECV_LOCK,
    SOCK_MT_IOCTRL_LOCK,

    SOCK_MT_LOCK_MAX,
};

struct _sock_mt {

    enum sock_mt_shutdown shutdown;

    enum sock_mt_stat state;

    int sel;

    sys_mutex_t lock[SOCK_MT_LOCK_MAX];
};
typedef struct _sock_mt sock_mt_t;

#if (SOCK_MT_DEBUG_LEVEL < 16)
#define SOCK_MT_DEBUG(level, ...)                                           \
        if (level >= SOCK_MT_DEBUG_LEVEL)                                   \
            printf(__VA_ARGS__);
#else
#define SOCK_MT_DEBUG(level, ...)
#endif

#if 0
#define SOCK_MT_LOCK(s, l)                                                  \
{                                                                           \
    SOCK_MT_DEBUG(1, "s %d l %d enter ", s, l);                             \
    sys_mutex_lock(&sockets_mt[s].lock[l]);                                 \
    SOCK_MT_DEBUG(1, "OK\n");                                               \
}

#define SOCK_MT_LOCK_RET(s, l, r)                                           \
{                                                                           \
    SOCK_MT_LOCK(s, l);                                                     \
    r = ERR_OK;                                                             \
}
#else
#define SOCK_MT_LOCK(s, l)                                                  \
{                                                                           \
    SOCK_MT_DEBUG(1, "s %d l %d enter ", s, l);                             \
    while (1) {                                                             \
        err_t err;                                                          \
        SYS_ARCH_DECL_PROTECT(lev);                                         \
                                                                            \
        SYS_ARCH_PROTECT(lev);                                              \
        if (SOCK_MT_GET_SHUTDOWN(s) != SOCK_MT_SHUTDOWN_NONE)               \
            err = ERR_CLSD;                                                 \
        else                                                                \
            err = ERR_OK;                                                   \
        if (err == ERR_OK) {                                                \
            if (sockets_mt[s].lock[l])                                      \
                err = sys_mutex_trylock(&sockets_mt[s].lock[l]);            \
            else                                                            \
                err = ERR_VAL;                                              \
        }                                                                   \
        SYS_ARCH_UNPROTECT(lev);                                            \
                                                                            \
        if (err == ERR_OK)                                                  \
            break;                                                          \
        else if (err == ERR_VAL || err == ERR_CLSD)                         \
            return -1;                                                      \
                                                                            \
        vTaskDelay(1);                                                      \
    }                                                                       \
    SOCK_MT_DEBUG(1, "OK\n");                                               \
}

#define SOCK_MT_LOCK_RET(s, l, r)                                           \
{                                                                           \
    SOCK_MT_DEBUG(1, "s %d l %d enter ", s, l);                             \
    while (1) {                                                             \
        SYS_ARCH_DECL_PROTECT(lev);                                         \
                                                                            \
        SYS_ARCH_PROTECT(lev);                                              \
        if (SOCK_MT_GET_SHUTDOWN(s) != SOCK_MT_SHUTDOWN_NONE)               \
            r = ERR_CLSD;                                                   \
        else                                                                \
            r = ERR_OK;                                                     \
        if (r == ERR_OK) {                                                  \
            if (sockets_mt[s].lock[l])                                      \
                r = sys_mutex_trylock(&sockets_mt[s].lock[l]);              \
            else                                                            \
                r = ERR_VAL;                                                \
        }                                                                   \
        SYS_ARCH_UNPROTECT(lev);                                            \
                                                                            \
        if (r == ERR_OK || r == ERR_VAL || r == ERR_CLSD)                   \
            break;                                                          \
        vTaskDelay(1);                                                      \
    }                                                                       \
    SOCK_MT_DEBUG(1, "OK\n");                                               \
}
#endif

#define SOCK_MT_TRYLOCK(s, l, r)                                            \
{                                                                           \
    SOCK_MT_DEBUG(1, "s %d l %d try enter ", s, l);                         \
    r = sys_mutex_trylock(&sockets_mt[s].lock[l]);                          \
    SOCK_MT_DEBUG(1, "ret %d OK\n", r);                                     \
}

#define SOCK_MT_UNLOCK(s, l)                                                \
{                                                                           \
    SOCK_MT_DEBUG(1, "s %d l %d exit ", s, l);                              \
    sys_mutex_unlock(&sockets_mt[s].lock[l]);                               \
    SOCK_MT_DEBUG(1, "OK\n");                                               \
}


#define SOCK_MT_SET_STATE(s, stat)                                          \
{                                                                           \
    SYS_ARCH_DECL_PROTECT(lev);                                             \
    SYS_ARCH_PROTECT(lev);                                                  \
    sockets_mt[s].state = stat;                                             \
    SYS_ARCH_UNPROTECT(lev);                                                \
}

#define SOCK_MT_GET_STATE(s)                                                \
    sockets_mt[s].state

#define SOCK_MT_SET_SHUTDOWN(s, d)                                          \
{                                                                           \
    SYS_ARCH_DECL_PROTECT(lev);                                             \
    SYS_ARCH_PROTECT(lev);                                                  \
    sockets_mt[s].shutdown = d;                                             \
    SYS_ARCH_UNPROTECT(lev);                                                \
}

#define SOCK_MT_GET_SHUTDOWN(s)                                             \
    sockets_mt[s].shutdown

#define SOCK_MT_SET_WRITE_SEL(s)                                            \
{                                                                           \
    SYS_ARCH_DECL_PROTECT(lev);                                             \
    SYS_ARCH_PROTECT(lev);                                                  \
    sockets_mt[s].sel |= 0x1;                                               \
    SYS_ARCH_UNPROTECT(lev);                                                \
}

#define SOCK_MT_RESET_WRITE_SEL(s)                                          \
{                                                                           \
    SYS_ARCH_DECL_PROTECT(lev);                                             \
    SYS_ARCH_PROTECT(lev);                                                  \
    sockets_mt[s].sel &= ~0x1;                                              \
    SYS_ARCH_UNPROTECT(lev);                                                \
}

#define SOCK_MT_GET_WRITE_SEL(s)                                            \
    (sockets_mt[s].sel & 0x01)

#define SOCK_MT_SET_READ_SEL(s)                                             \
{                                                                           \
    SYS_ARCH_DECL_PROTECT(lev);                                             \
    SYS_ARCH_PROTECT(lev);                                                  \
    sockets_mt[s].sel |= 0x2;                                               \
    SYS_ARCH_UNPROTECT(lev);                                                \
}

#define SOCK_MT_RESET_READ_SEL(s)                                           \
{                                                                           \
    SYS_ARCH_DECL_PROTECT(lev);                                             \
    SYS_ARCH_PROTECT(lev);                                                  \
    sockets_mt[s].sel &= ~0x2;                                              \
    SYS_ARCH_UNPROTECT(lev);                                                \
}

#define SOCK_MT_GET_READ_SEL(s)                                             \
    (sockets_mt[s].sel & 0x02)

#define SOCK_MT_GET_SEL(s)                                                  \
    sockets_mt[s].sel

#define LWIP_ENTER_MT(s, m, p)                                              \
{                                                                           \
    int r;                                                                  \
    SOCK_MT_DEBUG(1, "enter s %d module %d args %d\n", s, m, p);            \
	r = lwip_enter_mt_table[m](s, p);                                       \
    SOCK_MT_DEBUG(1, "enter s %d ret %d\n", s, r);                          \
    if (r)                                                                  \
        return -1;                                                          \
}

#define LWIP_EXIT_MT(s, m, p)                                               \
{                                                                           \
    int r;                                                                  \
    SOCK_MT_DEBUG(1, "exit s %d module %d args %d\n", s, m, p);             \
	r = lwip_exit_mt_table[m](s, p);                                        \
    SOCK_MT_DEBUG(1, "exit s %d ret %d\n", s, r);                           \
    if (r)                                                                  \
        return -1;                                                          \
}


static sock_mt_t sockets_mt[NUM_SOCKETS];

static int lwip_enter_mt_state(int s, int arg)
{
    if (tryget_socket(s) == NULL ||
        SOCK_MT_GET_STATE(s) != SOCK_MT_STATE_NONE ||
        SOCK_MT_GET_WRITE_SEL(s))
        return -1;

    SOCK_MT_LOCK(s, SOCK_MT_STATE_LOCK);
    SOCK_MT_SET_STATE(s, arg);

    return 0;
}

static int lwip_enter_mt_recv(int s, int arg)
{
    if (tryget_socket(s) == NULL ||
        SOCK_MT_GET_READ_SEL(s))
        return -1;

    SOCK_MT_LOCK(s, SOCK_MT_RECV_LOCK);

    return 0;
}

static int lwip_enter_mt_shutdown(int s, int arg)
{
    if (tryget_socket(s) == NULL
        || SOCK_MT_GET_SHUTDOWN(s) != SOCK_MT_SHUTDOWN_NONE)
        return -1;

    SOCK_MT_SET_SHUTDOWN(s, SOCK_MT_SHUTDOWN_OK);

    return 0;
}

static int lwip_enter_mt_close(int s, int arg)
{
    if (tryget_socket(s) == NULL)
        return -1;

    SOCK_MT_SET_SHUTDOWN(s, SOCK_MT_SHUTDOWN_OK);

    return 0;
}


static int lwip_enter_mt_select(int s, int arg)
{
    int i;
    int *fdset = (int *)arg;
    fd_set *read_set = (fd_set *)fdset[0];
    fd_set *write_set = (fd_set *)fdset[1];

    if (s > NUM_SOCKETS || s < 0)
        return -1;

    for (i = 0; i < s; i++) {
        if(FD_ISSET(i, read_set) || FD_ISSET(i, write_set))
            if (tryget_socket(i) == NULL)
                goto failed1;

        if (FD_ISSET(i, read_set)) {
            err_t err;

            SOCK_MT_SET_READ_SEL(i);
            SOCK_MT_LOCK_RET(i, SOCK_MT_RECV_LOCK, err);
            if (err != ERR_OK) {
                goto failed2;
            }
        }

        if (FD_ISSET(i, write_set)) {
            err_t err;

            SOCK_MT_SET_WRITE_SEL(i);
            SOCK_MT_LOCK_RET(i, SOCK_MT_STATE_LOCK, err);
            if (err != ERR_OK) {
                goto failed3;
            }
        }
    }

    return 0;

failed3:
    SOCK_MT_UNLOCK(i, SOCK_MT_STATE_LOCK);
    SOCK_MT_RESET_WRITE_SEL(i);
failed2:
    if (FD_ISSET(i, read_set)) {
        SOCK_MT_UNLOCK(i, SOCK_MT_RECV_LOCK);
        SOCK_MT_RESET_READ_SEL(i);
    }
failed1:
    for (i--; i >=0; i--) {
        if (FD_ISSET(i, read_set) ) {
            SOCK_MT_UNLOCK(i, SOCK_MT_RECV_LOCK);
            SOCK_MT_RESET_READ_SEL(i);
        }

        if (FD_ISSET(i, write_set)) {
            SOCK_MT_UNLOCK(i, SOCK_MT_STATE_LOCK);
            SOCK_MT_RESET_WRITE_SEL(i);
        }
    }

    return -1;
}

static int lwip_enter_mt_ioctrl(int s, int arg)
{
    if (tryget_socket(s) == NULL)
        return -1;

    SOCK_MT_LOCK(s, SOCK_MT_IOCTRL_LOCK);

    return 0;
}

static int lwip_exit_mt_state(int s, int arg)
{
    SOCK_MT_SET_STATE(s, SOCK_MT_STATE_NONE);
    SOCK_MT_UNLOCK(s, SOCK_MT_STATE_LOCK);

    if (SOCK_MT_GET_SHUTDOWN(s) != SOCK_MT_SHUTDOWN_NONE) {
        return -1;
    }

    return 0;
}

static int lwip_exit_mt_recv(int s, int arg)
{
    SOCK_MT_UNLOCK(s, SOCK_MT_RECV_LOCK);

    if (SOCK_MT_GET_SHUTDOWN(s) != SOCK_MT_SHUTDOWN_NONE) {
        return -1;
    }

    return 0;
}

static int lwip_exit_mt_shutdown(int s, int arg)
{
    //SOCK_MT_SET_SHUTDOWN(s, SOCK_MT_STATE_NONE);
    return 0;
}

static int lwip_exit_mt_close(int s, int arg)
{
    return 0;
}

static int lwip_exit_mt_select(int s, int arg)
{
    int i;
    int *fdset = (int *)arg;
    fd_set *read_set = (fd_set *)fdset[0];
    fd_set *write_set = (fd_set *)fdset[1];

    for (i = 0; i < s; i++) {
        if (FD_ISSET(i, read_set)) {
            SOCK_MT_UNLOCK(i, SOCK_MT_RECV_LOCK);
            SOCK_MT_RESET_READ_SEL(i);
        }

        if (FD_ISSET(i, write_set)) {
            SOCK_MT_UNLOCK(i, SOCK_MT_STATE_LOCK);
            SOCK_MT_RESET_WRITE_SEL(i);
        }
    }

    for (i = 0; i < s; i++) {
        if ((FD_ISSET(i, read_set) || FD_ISSET(i, write_set)) \
                && SOCK_MT_GET_SHUTDOWN(i) != SOCK_MT_SHUTDOWN_NONE) {
            return -1;
        }
    }

    return 0;
}

static int lwip_exit_mt_ioctrl(int s, int arg)
{
    SOCK_MT_UNLOCK(s, SOCK_MT_IOCTRL_LOCK);

    if (SOCK_MT_GET_SHUTDOWN(s) != SOCK_MT_SHUTDOWN_NONE) {
        return -1;
    }

    return 0;
}

static const lwip_io_mt_fn lwip_enter_mt_table[] = {
    lwip_enter_mt_state,
    lwip_enter_mt_recv,
    lwip_enter_mt_ioctrl,
    lwip_enter_mt_select,
    lwip_enter_mt_shutdown,
    lwip_enter_mt_close
};

static const lwip_io_mt_fn lwip_exit_mt_table[] = {
    lwip_exit_mt_state,
    lwip_exit_mt_recv,
    lwip_exit_mt_ioctrl,
    lwip_exit_mt_select,
    lwip_exit_mt_shutdown,
    lwip_exit_mt_close
};

static void lwip_do_sync_send(void *arg)
{
    socket_conn_sync_t *sync = (socket_conn_sync_t *)arg;
    struct netconn *conn = sync->conn;

    SYS_ARCH_DECL_PROTECT(lev);
    SYS_ARCH_PROTECT(lev);
    if (conn->current_msg) {
        conn->current_msg->err = ERR_OK;
        if (conn->current_msg && sys_sem_valid(conn->current_msg->op_completed_sem))
            sys_sem_signal(conn->current_msg->op_completed_sem);
        conn->current_msg = NULL;
    }
    conn->state = NETCONN_NONE;
    SYS_ARCH_UNPROTECT(lev);

    sys_sem_signal(sync->sem);
}

static void lwip_do_sync_rst_state(void *arg)
{
    socket_conn_sync_t *sync = (socket_conn_sync_t *)arg;
    struct netconn *conn = sync->conn;

    SYS_ARCH_DECL_PROTECT(lev);
    SYS_ARCH_PROTECT(lev);
    conn->state = NETCONN_NONE;
    SYS_ARCH_UNPROTECT(lev);

    sys_sem_signal(sync->sem);
}

static void lwip_sync_state_mt(struct lwip_sock *sock, int state)
{
    SOCK_MT_DEBUG(1, "sync state %d\n", state);

    switch (state) {
    case SOCK_MT_STATE_ACCEPT :
        if (sys_mbox_valid(&sock->conn->acceptmbox))
            sys_mbox_trypost(&sock->conn->acceptmbox, NULL);
        break;
    case SOCK_MT_STATE_CONNECT:
    case SOCK_MT_STATE_SEND :
    {
        socket_conn_sync_t sync;

        sync.conn = sock->conn;
        sync.sem = sys_thread_sem_get();

        tcpip_callback(lwip_do_sync_send, &sync);
        sys_arch_sem_wait(sync.sem, 0);
        break;
    }
    default :
        break;
    }
}

static void lwip_sync_recv_mt(struct lwip_sock *sock)
{
    SOCK_MT_DEBUG(1, "sync recv %d\n", sock->conn->socket);
    if (sys_mbox_valid(&sock->conn->recvmbox))
        sys_mbox_trypost(&sock->conn->recvmbox, NULL);
}

static void lwip_sync_select_mt(struct lwip_sock *sock)
{
    SOCK_MT_DEBUG(1, "sync select %d\n", sock->conn->socket);
    event_callback(sock->conn, NETCONN_EVT_ERROR, 0);
}


static inline bool is_need_sync(int s, int how, int module)
{
    int ret;

    switch (module) {
        case SOCK_MT_STATE:
            if (how == SHUT_RD)
                return false;
            break;
        case SOCK_MT_RECV:
            if (how == SHUT_WR)
                return false;
            break;
        default:
            break;
    }

    /*
     * we always lock the mutex in case of other thread entering,
     * other thread will be blocked at "SOCK_MT_LOCK" and poll-check
     */
    SOCK_MT_TRYLOCK(s, module, ret);

    return ret == ERR_OK ? false : true;
}

static void lwip_sync_mt(int s, int how)
{
    int module = 0;
    struct lwip_sock *sock;

    while (module < SOCK_MT_SELECT) {
        extern void sys_arch_msleep(int ms);

        if (is_need_sync(s, how, module) == false) {
            module++;
            continue;
        }

        sock = get_socket(s);
        if (!SOCK_MT_GET_SEL(s)) {
            switch (module) {
            case SOCK_MT_STATE:
                lwip_sync_state_mt(sock, SOCK_MT_GET_STATE(s));
                break;
            case SOCK_MT_RECV:
                lwip_sync_recv_mt(sock);
                break;
            default :
                break;
            }
        } else {
            lwip_sync_select_mt(sock);
        }

        sys_arch_msleep(LWIP_SYNC_MT_SLEEP_MS);
    }

    sock = tryget_socket(s);
    if (sock) {
        socket_conn_sync_t sync;

        sync.conn = sock->conn;
        sync.sem = sys_thread_sem_get();

        tcpip_callback(lwip_do_sync_rst_state, &sync);
        sys_arch_sem_wait(sync.sem, 0);
    }
}

#if LWIP_SO_LINGER
static void lwip_socket_set_so_link(int s, int linger)
{
    struct lwip_sock *sock = get_socket(s);

    if (sock) {
        /*
         * linker:
         *     -1: nothing
         *      0: free sent_buf immediately
         */
        sock->conn->linger = linger;
    }
}
#else
#error "LWIP_SO_LINGER must be enable"
#endif


int lwip_socket(int domain, int type, int protocol)
{
    int s;
    int i;

    s = lwip_socket_esp(domain, type, protocol);
    if (s < 0)
        return -1;

    for (i = 0; i < SOCK_MT_LOCK_MAX; i++) {
        if (sys_mutex_new(&sockets_mt[s].lock[i]) != ERR_OK)
            break;
    }
    if (i < SOCK_MT_LOCK_MAX) {
        for (i-- ; i >= 0; i--) {
            sys_mutex_free(&sockets_mt[s].lock[i]);
            sockets_mt[s].lock[i] = NULL;
        }

        lwip_close_esp(s);
        s = -1;
    }

    if (s >= 0) {
        lwip_socket_set_so_link(s, 0);
        SOCK_MT_SET_SHUTDOWN(s, SOCK_MT_SHUTDOWN_NONE);
    }

    return s;
}

int lwip_bind(int s, const struct sockaddr *name, socklen_t namelen)
{
    int ret;

    LWIP_ENTER_MT(s, SOCK_MT_STATE, SOCK_MT_STATE_BIND);

    ret = lwip_bind_esp(s, name, namelen);

    LWIP_EXIT_MT(s, SOCK_MT_STATE, SOCK_MT_STATE_BIND);


    return ret;
}

int lwip_connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    int ret;

    LWIP_ENTER_MT(s, SOCK_MT_STATE, SOCK_MT_STATE_CONNECT);

    ret = lwip_connect_esp(s, name, namelen);

    LWIP_EXIT_MT(s, SOCK_MT_STATE, SOCK_MT_STATE_CONNECT);

    return ret;
}

int lwip_listen(int s, int backlog)
{
    int ret;

    LWIP_ENTER_MT(s, SOCK_MT_STATE, SOCK_MT_STATE_LISTEN);

    lwip_socket_set_so_link(s, -1);

    ret = lwip_listen_esp(s, backlog);

    LWIP_EXIT_MT(s, SOCK_MT_STATE, SOCK_MT_STATE_LISTEN);

    return ret;
}

int lwip_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int i;
    int ret;

    LWIP_ENTER_MT(s, SOCK_MT_STATE, SOCK_MT_STATE_ACCEPT)

    lwip_socket_set_so_link(s, -1);

    ret = lwip_accept_esp(s, addr, addrlen);

    LWIP_EXIT_MT(s, SOCK_MT_STATE, SOCK_MT_STATE_ACCEPT);

    if (ret < 0)
        return -1;

    for (i = 0; i < SOCK_MT_LOCK_MAX; i++) {
        if (sys_mutex_new(&sockets_mt[ret].lock[i]) != ERR_OK)
            break;
    }
    if (i < SOCK_MT_LOCK_MAX) {
        for (i-- ; i >= 0; i--) {
            sys_mutex_free(&sockets_mt[ret].lock[i]);
            sockets_mt[ret].lock[i] = NULL;
        }

        lwip_close_esp(ret);
        ret = -1;
    }

    if (ret >= 0) {
        lwip_socket_set_so_link(s, 0);
        SOCK_MT_SET_SHUTDOWN(ret, SOCK_MT_SHUTDOWN_NONE);
    }

    return ret;
}

int lwip_getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
    int ret;

    LWIP_ENTER_MT(s, SOCK_MT_IOCTL, 0);

    ret = lwip_getpeername_esp(s, name, namelen);

    LWIP_EXIT_MT(s, SOCK_MT_IOCTL, 0);

    return ret;
}

int lwip_getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
    int ret;

    LWIP_ENTER_MT(s, SOCK_MT_IOCTL, 0);

    ret = lwip_getsockname_esp(s, name, namelen);

    LWIP_EXIT_MT(s, SOCK_MT_IOCTL, 0);

    return ret;
}

int lwip_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
    int ret;

    LWIP_ENTER_MT(s, SOCK_MT_IOCTL, 0);

    ret = lwip_setsockopt_esp(s, level, optname, optval, optlen);

    LWIP_EXIT_MT(s, SOCK_MT_IOCTL, 0);

    return ret;
}

int lwip_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
    int ret;

    if (optname == SO_ERROR) {
        int retval = 0;

        if (SOCK_MT_GET_SHUTDOWN(s) != SOCK_MT_SHUTDOWN_NONE)
            retval = ENOTCONN;

        if (retval) {
            *(int *)optval = retval;
            return 0;
        }
    }

    LWIP_ENTER_MT(s, SOCK_MT_IOCTL, 0);

    ret = lwip_getsockopt_esp(s, level, optname, optval, optlen);

    LWIP_EXIT_MT(s, SOCK_MT_IOCTL, 0);

    return ret;
}

int lwip_ioctl(int s, long cmd, void *argp)
{
    int ret;

    LWIP_ENTER_MT(s, SOCK_MT_IOCTL, 0)

    ret = lwip_ioctl_esp(s, cmd, argp);

    LWIP_EXIT_MT(s, SOCK_MT_IOCTL, 0);

    return ret;
}

int lwip_sendto(int s, const void *data, size_t size, int flags,
                   const struct sockaddr *to, socklen_t tolen)
{
    int ret;

    LWIP_ENTER_MT(s, SOCK_MT_STATE, SOCK_MT_STATE_SEND);

    ret = lwip_sendto_esp(s, data, size, flags, to, tolen);

    LWIP_EXIT_MT(s, SOCK_MT_STATE, SOCK_MT_STATE_SEND);

    return ret;
}

int lwip_send(int s, const void *data, size_t size, int flags)
{
    int ret;

    LWIP_ENTER_MT(s, SOCK_MT_STATE, SOCK_MT_STATE_SEND);

    ret = lwip_send_esp(s, data, size, flags);

    LWIP_EXIT_MT(s, SOCK_MT_STATE, SOCK_MT_STATE_SEND);

    return ret;
}

int lwip_recvfrom(int s, void *mem, size_t len, int flags,
                     struct sockaddr *from, socklen_t *fromlen)
{
    int ret;

    LWIP_ENTER_MT(s, SOCK_MT_RECV, 0);

    ret = lwip_recvfrom_esp(s, mem, len, flags, from, fromlen);

    LWIP_EXIT_MT(s, SOCK_MT_RECV, 0);

    return ret;
}

int lwip_recv(int s, void *mem, size_t len, int flags)
{
    return lwip_recvfrom(s, mem, len, flags, NULL, NULL);
}

int lwip_read(int s, void *mem, size_t len)
{
    return lwip_recvfrom(s, mem, len, 0, NULL, NULL);
}

int lwip_write(int s, const void *data, size_t size)
{
    return lwip_send(s, data, size, 0);
}

int lwip_fcntl(int s, int cmd, int val)
{
    int ret;

    LWIP_ENTER_MT(s, SOCK_MT_IOCTL, 0);

    ret = lwip_fcntl_esp(s, cmd, val);

    LWIP_EXIT_MT(s, SOCK_MT_IOCTL, 0);

    return ret;
}

#ifdef SOCKETS_MT_DISABLE_SHUTDOWN
int lwip_shutdown(int s, int how)
{
    return 0;
}
#else
int lwip_shutdown(int s, int how)
{
    int ret;

    LWIP_ENTER_MT(s, SOCK_MT_SHUTDOWN, 0);

    lwip_sync_mt(s, how);

    ret = lwip_shutdown_esp(s, how);

    LWIP_EXIT_MT(s, SOCK_MT_SHUTDOWN, 0);

    return ret;
}
#endif

int lwip_close(int s)
{
    int ret;
    int i;
    SYS_ARCH_DECL_PROTECT(lev);
    sys_mutex_t lock_tmp[SOCK_MT_LOCK_MAX];

#ifdef SOCKETS_MT_DISABLE_SHUTDOWN
    LWIP_ENTER_MT(s, SOCK_MT_CLOSE, 0);

    lwip_sync_mt(s, SHUT_RDWR);
#else
    lwip_shutdown(s, SHUT_RDWR);

    LWIP_ENTER_MT(s, SOCK_MT_CLOSE, 0);
#endif

    ret = lwip_close_esp(s);

    LWIP_EXIT_MT(s, SOCK_MT_CLOSE, 0);

    SYS_ARCH_PROTECT(lev);
    for (i = 0 ; i < SOCK_MT_LOCK_MAX; i++) {
        lock_tmp[i] = sockets_mt[s].lock[i];
        sockets_mt[s].lock[i] = NULL;
    }
    SYS_ARCH_UNPROTECT(lev);

    for (i = 0 ; i < SOCK_MT_LOCK_MAX; i++) {
        sys_mutex_free(&lock_tmp[i]);
    }

    return ret;
}

int lwip_select(int maxfdp1, fd_set *readset, fd_set *writeset,
                   fd_set *exceptset, struct timeval *timeout)
{
    int ret;
    fd_set read_set, write_set;
    int pset[2] = {(int)&read_set, (int)&write_set};

    FD_ZERO(&read_set);
    FD_ZERO(&write_set);

    if (readset)
        MEMCPY(&read_set, readset, sizeof(fd_set));

    if (writeset)
        MEMCPY(&write_set, writeset, sizeof(fd_set));

    LWIP_ENTER_MT(maxfdp1, SOCK_MT_SELECT, (int)pset);

    ret = lwip_select_esp(maxfdp1, readset, writeset, exceptset, timeout);

    LWIP_EXIT_MT(maxfdp1, SOCK_MT_SELECT, (int)pset);

    return ret;
}

#endif
