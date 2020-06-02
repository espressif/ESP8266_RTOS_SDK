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

#undef lwip_read
#undef lwip_write
#undef lwip_writev
#undef lwip_close
#undef closesocket
#undef lwip_fcntl
#undef lwip_ioctl

/********************************************************************/
#define LWIP_SYNC_MT_SLEEP_MS 10

#define SOCK_MT_DEBUG_LEVEL 255

typedef struct socket_conn_sync {
    struct tcpip_api_call_data call;
    struct netconn  *conn;
} socket_conn_sync_t;

typedef int (*lwip_io_mt_fn)(int, int );

/* Use about 2 bit */
#define SOCK_MT_STATE_SOCK      0
#define SOCK_MT_STATE_ACCEPT    1
#define SOCK_MT_STATE_CONNECT   2
#define SOCK_MT_STATE_SEND      3
#define SOCK_MT_STATE_ILL       4

#define SOCK_MT_LOCK_SEND      (1 << 0)
#define SOCK_MT_LOCK_RECV       (1 << 1)
#define SOCK_MT_LOCK_IOCTL     (1 << 2)

#define SOCK_MT_LOCK_MIN       SOCK_MT_LOCK_SEND
#define SOCK_MT_LOCK_MAX       SOCK_MT_LOCK_IOCTL

#define SOCK_MT_SELECT_RECV     (1 << 0)
#define SOCK_MT_SELECT_SEND    (1 << 1)

typedef struct _sock_mt {
    uint8_t opened  : 1;
    uint8_t state   : 2;
    uint8_t select  : 2;
    uint8_t lock    : 3;
} sock_mt_t;

#if (SOCK_MT_DEBUG_LEVEL < 16)
#define SOCK_MT_DEBUG(level, ...)                                           \
        if (level >= SOCK_MT_DEBUG_LEVEL)                                   \
            printf(__VA_ARGS__);
#else
#define SOCK_MT_DEBUG(level, ...)
#endif

#define SOCK_MT_ENTER_CHECK(s, l, st)           \
{                                               \
    if (_sock_lock(s, l) != ERR_OK)             \
        return -1;                              \
    if (st != SOCK_MT_STATE_ILL)                \
        _sock_set_state(s, st);                 \
}

#define SOCK_MT_EXIT_CHECK(s, l, st)            \
{                                               \
    if (st != SOCK_MT_STATE_ILL)                \
        _sock_set_state(s, SOCK_MT_STATE_SOCK); \
    if (_sock_unlock(s, l) != ERR_OK)           \
        return -1;                              \
}

static volatile sock_mt_t DRAM_ATTR sockets_mt[NUM_SOCKETS];

static inline void _sock_mt_init(int s)
{
    memset((void *)&sockets_mt[s - LWIP_SOCKET_OFFSET], 0, sizeof(sock_mt_t));
}

static inline int _sock_is_opened(int s)
{
    return sockets_mt[s - LWIP_SOCKET_OFFSET].opened != 0;
}

static inline void _sock_set_open(int s, int opened)
{
    SYS_ARCH_DECL_PROTECT(lev);

    SYS_ARCH_PROTECT(lev);
    sockets_mt[s - LWIP_SOCKET_OFFSET].opened = opened;
    SYS_ARCH_UNPROTECT(lev);
}

static inline void _sock_set_state(int s, int state)
{
    SYS_ARCH_DECL_PROTECT(lev);

    SYS_ARCH_PROTECT(lev);
    sockets_mt[s - LWIP_SOCKET_OFFSET].state = state;
    SYS_ARCH_UNPROTECT(lev);
}

static inline int _sock_get_state(int s)
{
    return sockets_mt[s - LWIP_SOCKET_OFFSET].state;
}

static inline int _sock_get_select(int s, int select)
{
    return sockets_mt[s - LWIP_SOCKET_OFFSET].select & select;
}

static int inline _sock_is_lock(int s, int l)
{
    return sockets_mt[s - LWIP_SOCKET_OFFSET].lock & l;
}

static int inline _sock_next_lock(int lock)
{
    return lock << 1;
}

static void inline _sock_set_select(int s, int select)
{
    SYS_ARCH_DECL_PROTECT(lev);

    SYS_ARCH_PROTECT(lev);
    sockets_mt[s - LWIP_SOCKET_OFFSET].select |= select;
    SYS_ARCH_UNPROTECT(lev);
}

static void inline _sock_reset_select(int s, int select)
{
    SYS_ARCH_DECL_PROTECT(lev);

    SYS_ARCH_PROTECT(lev);
    sockets_mt[s - LWIP_SOCKET_OFFSET].select &= ~select;
    SYS_ARCH_UNPROTECT(lev);
}

static int _sock_try_lock(int s, int l)
{
    int ret = ERR_OK;
    SYS_ARCH_DECL_PROTECT(lev);

    if (!_sock_is_opened(s)) {
        ret = ERR_CLSD;
        goto exit;
    }

    if (sockets_mt[s - LWIP_SOCKET_OFFSET].lock & l) {
        ret = ERR_INPROGRESS;
        goto exit;
    }

    SYS_ARCH_PROTECT(lev);
    sockets_mt[s - LWIP_SOCKET_OFFSET].lock |= l;
    SYS_ARCH_UNPROTECT(lev);

exit:
    return ret;
}

static int _sock_lock(int s, int l)
{
    int ret = ERR_OK;
    if (tryget_socket(s) == NULL)
        return -1;

    SOCK_MT_DEBUG(1, "s %d l %d enter ", s, l);

    while (1) {
        ret = _sock_try_lock(s, l);

        if (ret != ERR_INPROGRESS)
            break;

        vTaskDelay(1);
    }

    SOCK_MT_DEBUG(1, "OK %d\n", ret);

    return ret;
}

static int _sock_unlock(int s, int l)
{
    int ret = 0;
    SYS_ARCH_DECL_PROTECT(lev);

    SOCK_MT_DEBUG(1, "s %d l %d exit ", s, l);

    SYS_ARCH_PROTECT(lev);
    sockets_mt[s - LWIP_SOCKET_OFFSET].lock &= ~l;
    SYS_ARCH_UNPROTECT(lev);

    if (!_sock_is_opened(s)) {
        ret = ERR_CLSD;
        goto exit;
    }

exit:
    SOCK_MT_DEBUG(1, "OK %d\n", ret);

    return ret;
}

static int lwip_enter_mt_select(int s, fd_set *read_set, fd_set *write_set)
{
    int i;

    if (s > NUM_SOCKETS + LWIP_SOCKET_OFFSET || s < LWIP_SOCKET_OFFSET)
        return -1;

    for (i = 0; i < s; i++) {
        if(FD_ISSET(i, read_set) || FD_ISSET(i, write_set))
            if (tryget_socket(i) == NULL)
                goto failed1;

        if (FD_ISSET(i, read_set)) {
            err_t err;

            _sock_set_select(i, SOCK_MT_SELECT_RECV);
            err = _sock_lock(i, SOCK_MT_LOCK_RECV);
            if (err != ERR_OK) {
                goto failed2;
            }
        }

        if (FD_ISSET(i, write_set)) {
            err_t err;

            _sock_set_select(i, SOCK_MT_SELECT_SEND);
            err = _sock_lock(i, SOCK_MT_LOCK_SEND);
            if (err != ERR_OK) {
                goto failed3;
            }
        }
    }

    return 0;

failed3:
    _sock_unlock(i, SOCK_MT_LOCK_SEND);
    _sock_reset_select(i, SOCK_MT_SELECT_SEND);
failed2:
    if (FD_ISSET(i, read_set)) {
        _sock_unlock(i, SOCK_MT_LOCK_RECV);
        _sock_reset_select(i, SOCK_MT_SELECT_RECV);
    }
failed1:
    for (i--; i >=0; i--) {
        if (FD_ISSET(i, read_set) ) {
            _sock_unlock(i, SOCK_MT_LOCK_RECV);
            _sock_reset_select(i, SOCK_MT_SELECT_RECV);
        }

        if (FD_ISSET(i, write_set)) {
            _sock_unlock(i, SOCK_MT_LOCK_SEND);
            _sock_reset_select(i, SOCK_MT_SELECT_SEND);
        }
    }

    return -1;
}

static void lwip_exit_mt_select(int s, fd_set *read_set, fd_set *write_set)
{
    int i;

    for (i = 0; i < s; i++) {
        if (FD_ISSET(i, read_set)) {
            _sock_unlock(i, SOCK_MT_LOCK_RECV);
            _sock_reset_select(i, SOCK_MT_SELECT_RECV);
        }

        if (FD_ISSET(i, write_set)) {
            _sock_unlock(i, SOCK_MT_LOCK_SEND);
            _sock_reset_select(i, SOCK_MT_SELECT_SEND);
        }
    }
}

static err_t lwip_do_sync_accept(struct tcpip_api_call_data *call)
{
    socket_conn_sync_t *sync = (socket_conn_sync_t *)call;
    struct netconn *conn = sync->conn;

    SYS_ARCH_DECL_PROTECT(lev);
    SYS_ARCH_PROTECT(lev);
    if (sys_mbox_valid(&conn->acceptmbox))
        sys_mbox_trypost(&conn->acceptmbox, NULL);
    conn->state = NETCONN_NONE;
    SYS_ARCH_UNPROTECT(lev);

    return ERR_OK;
}

static err_t lwip_do_sync_send(struct tcpip_api_call_data *call)
{
    socket_conn_sync_t *sync = (socket_conn_sync_t *)call;
    struct netconn *conn = sync->conn;

    SYS_ARCH_DECL_PROTECT(lev);
    SYS_ARCH_PROTECT(lev);
    if (conn->current_msg) {
        conn->current_msg->err = ERR_OK;
        if (sys_sem_valid(conn->current_msg->op_completed_sem))
            sys_sem_signal(conn->current_msg->op_completed_sem);
        conn->current_msg = NULL;
    }
    conn->state = NETCONN_NONE;
    SYS_ARCH_UNPROTECT(lev);

    return ERR_OK;
}

static err_t lwip_do_sync_recv_state(struct tcpip_api_call_data *call)
{
    socket_conn_sync_t *sync = (socket_conn_sync_t *)call;
    struct netconn *conn = sync->conn;

    SYS_ARCH_DECL_PROTECT(lev);
    SYS_ARCH_PROTECT(lev);
    SOCK_MT_DEBUG(1, "sync recv %d\n", conn->socket);
    if (sys_mbox_valid(&conn->recvmbox))
        sys_mbox_trypost(&conn->recvmbox, NULL);
    conn->state = NETCONN_NONE;
    SYS_ARCH_UNPROTECT(lev);

    return ERR_OK;
}

static err_t lwip_do_sync_select_state(struct tcpip_api_call_data *call)
{
    socket_conn_sync_t *sync = (socket_conn_sync_t *)call;
    struct netconn *conn = sync->conn;

    SYS_ARCH_DECL_PROTECT(lev);
    SYS_ARCH_PROTECT(lev);
    event_callback(conn, NETCONN_EVT_ERROR, 0);
    conn->state = NETCONN_NONE;
    SYS_ARCH_UNPROTECT(lev);

    return ERR_OK;
}

static void lwip_sync_state_mt(int s)
{
    struct lwip_sock *sock = tryget_socket(s);
    int state = _sock_get_state(s);
    socket_conn_sync_t sync = {
        .conn = sock->conn,
    };

    SOCK_MT_DEBUG(1, "sync state %d\n", state);

    switch (state) {
        case SOCK_MT_STATE_ACCEPT :
            tcpip_api_call(lwip_do_sync_accept, &sync.call);
            break;
        case SOCK_MT_STATE_CONNECT:
        case SOCK_MT_STATE_SEND :
            tcpip_api_call(lwip_do_sync_send, &sync.call);
            break;
        default :
            break;
    }
}

static void lwip_sync_recv_mt(int s)
{
    struct lwip_sock *sock = tryget_socket(s);
    socket_conn_sync_t sync = {
        .conn = sock->conn,
    };

    tcpip_api_call(lwip_do_sync_recv_state, &sync.call);
}

static void lwip_sync_select_mt(int s)
{
    struct lwip_sock *sock = tryget_socket(s);
    socket_conn_sync_t sync = {
        .conn = sock->conn,
    };

    tcpip_api_call(lwip_do_sync_select_state, &sync.call);
}

static void lwip_sync_mt(int s, int how)
{
    int lock = SOCK_MT_LOCK_MIN;

    do {
        if (_sock_is_lock(s, lock)) {
            int need_wait = 0;
            extern void sys_arch_msleep(int ms);

            if (!_sock_get_select(s, SOCK_MT_SELECT_RECV | SOCK_MT_SELECT_SEND)) {
                SOCK_MT_DEBUG(1, "lock state=%d how=%d\n", lock, how);

                switch (lock) {
                    case SOCK_MT_LOCK_SEND:
                        if (how == SHUT_WR || how == SHUT_RDWR) {
                            lwip_sync_state_mt(s);
                            need_wait = 1;
                        } else {
                            lock = _sock_next_lock(lock);
                        }
                        break;
                    case SOCK_MT_LOCK_RECV:
                        if (how == SHUT_RD || how == SHUT_RDWR) {
                            lwip_sync_recv_mt(s);
                            need_wait = 1;
                        } else {
                            lock = _sock_next_lock(lock);
                        }
                        break;
                    default :
                        break;
                }
            } else {
                lwip_sync_select_mt(s);
                need_wait = 1;
            }

            if (need_wait)
                sys_arch_msleep(LWIP_SYNC_MT_SLEEP_MS);
        } else
            lock = _sock_next_lock(lock);
    }  while (lock < SOCK_MT_LOCK_MAX);
}

#if SET_SOLINGER_DEFAULT
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
#endif /* LWIP_SO_LINGER */
#else /* SET_SOLINGER_DEFAULT */
#define lwip_socket_set_so_link(_s, _linger)
#endif /* SET_SOLINGER_DEFAULT */

int lwip_socket(int domain, int type, int protocol)
{
    int s;

    s = lwip_socket_esp(domain, type, protocol);
    if (s < 0)
        return -1;

    lwip_socket_set_so_link(s, 0);
    _sock_mt_init(s);
    _sock_set_open(s, 1);

    return s;
}

int lwip_bind(int s, const struct sockaddr *name, socklen_t namelen)
{
    int ret;

    SOCK_MT_ENTER_CHECK(s, SOCK_MT_LOCK_SEND, SOCK_MT_STATE_ILL);

    ret = lwip_bind_esp(s, name, namelen);

    SOCK_MT_EXIT_CHECK(s, SOCK_MT_LOCK_SEND, SOCK_MT_STATE_ILL);

    return ret;
}

int lwip_connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    int ret;

    SOCK_MT_ENTER_CHECK(s, SOCK_MT_LOCK_SEND, SOCK_MT_STATE_CONNECT);

    ret = lwip_connect_esp(s, name, namelen);

    SOCK_MT_EXIT_CHECK(s, SOCK_MT_LOCK_SEND, SOCK_MT_STATE_CONNECT);

    return ret;
}

int lwip_listen(int s, int backlog)
{
    int ret;

    SOCK_MT_ENTER_CHECK(s, SOCK_MT_LOCK_SEND, SOCK_MT_STATE_ILL);

    lwip_socket_set_so_link(s, -1);

    ret = lwip_listen_esp(s, backlog);

    SOCK_MT_EXIT_CHECK(s, SOCK_MT_LOCK_SEND, SOCK_MT_STATE_ILL);

    return ret;
}

int lwip_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int ret;

    SOCK_MT_ENTER_CHECK(s, SOCK_MT_LOCK_SEND, SOCK_MT_STATE_ACCEPT)

    lwip_socket_set_so_link(s, -1);

    ret = lwip_accept_esp(s, addr, addrlen);

    SOCK_MT_EXIT_CHECK(s, SOCK_MT_LOCK_SEND, SOCK_MT_STATE_ACCEPT);

    if (ret < 0)
        return -1;

    lwip_socket_set_so_link(ret, 0);
    _sock_mt_init(ret);
    _sock_set_open(ret, 1);

    return ret;
}

int lwip_getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
    int ret;

    SOCK_MT_ENTER_CHECK(s, SOCK_MT_LOCK_IOCTL, SOCK_MT_STATE_ILL);

    ret = lwip_getpeername_esp(s, name, namelen);

    SOCK_MT_EXIT_CHECK(s, SOCK_MT_LOCK_IOCTL, SOCK_MT_STATE_ILL);

    return ret;
}

int lwip_getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
    int ret;

    SOCK_MT_ENTER_CHECK(s, SOCK_MT_LOCK_IOCTL, SOCK_MT_STATE_ILL);

    ret = lwip_getsockname_esp(s, name, namelen);

    SOCK_MT_EXIT_CHECK(s, SOCK_MT_LOCK_IOCTL, SOCK_MT_STATE_ILL);

    return ret;
}

int lwip_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
    int ret;

    SOCK_MT_ENTER_CHECK(s, SOCK_MT_LOCK_IOCTL, SOCK_MT_STATE_ILL);

    ret = lwip_setsockopt_esp(s, level, optname, optval, optlen);

    SOCK_MT_EXIT_CHECK(s, SOCK_MT_LOCK_IOCTL, SOCK_MT_STATE_ILL);

    return ret;
}

int lwip_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
    int ret;

    if (tryget_socket(s) == NULL)
        return -1;

    if (optname == SO_ERROR) {
        int retval = 0;

        if (!_sock_is_opened(s))
            retval = ENOTCONN;

        if (retval) {
            *(int *)optval = retval;
            return 0;
        }
    }

    SOCK_MT_ENTER_CHECK(s, SOCK_MT_LOCK_IOCTL, SOCK_MT_STATE_ILL);

    ret = lwip_getsockopt_esp(s, level, optname, optval, optlen);

    SOCK_MT_EXIT_CHECK(s, SOCK_MT_LOCK_IOCTL, SOCK_MT_STATE_ILL);

    return ret;
}

int lwip_ioctl(int s, long cmd, void *argp)
{
    int ret;

    SOCK_MT_ENTER_CHECK(s, SOCK_MT_LOCK_IOCTL, SOCK_MT_STATE_ILL)

    ret = lwip_ioctl_esp(s, cmd, argp);

    SOCK_MT_EXIT_CHECK(s, SOCK_MT_LOCK_IOCTL, SOCK_MT_STATE_ILL);

    return ret;
}

int lwip_sendto(int s, const void *data, size_t size, int flags,
                   const struct sockaddr *to, socklen_t tolen)
{
    int ret;

    SOCK_MT_ENTER_CHECK(s, SOCK_MT_LOCK_SEND, SOCK_MT_STATE_SEND);

    ret = lwip_sendto_esp(s, data, size, flags, to, tolen);

    SOCK_MT_EXIT_CHECK(s, SOCK_MT_LOCK_SEND, SOCK_MT_STATE_SEND);

    return ret;
}

int lwip_send(int s, const void *data, size_t size, int flags)
{
    int ret;

    SOCK_MT_ENTER_CHECK(s, SOCK_MT_LOCK_SEND, SOCK_MT_STATE_SEND);

    ret = lwip_send_esp(s, data, size, flags);

    SOCK_MT_EXIT_CHECK(s, SOCK_MT_LOCK_SEND, SOCK_MT_STATE_SEND);

    return ret;
}

int lwip_recvfrom(int s, void *mem, size_t len, int flags,
                     struct sockaddr *from, socklen_t *fromlen)
{
    int ret;

    SOCK_MT_ENTER_CHECK(s, SOCK_MT_LOCK_RECV, SOCK_MT_STATE_ILL);

    ret = lwip_recvfrom_esp(s, mem, len, flags, from, fromlen);

    SOCK_MT_EXIT_CHECK(s, SOCK_MT_LOCK_RECV, SOCK_MT_STATE_ILL);

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

    SOCK_MT_ENTER_CHECK(s, SOCK_MT_LOCK_IOCTL, SOCK_MT_STATE_ILL);

    ret = lwip_fcntl_esp(s, cmd, val);

    SOCK_MT_EXIT_CHECK(s, SOCK_MT_LOCK_IOCTL, SOCK_MT_STATE_ILL);

    return ret;
}

int lwip_shutdown(int s, int how)
{
    int ret;
    SYS_ARCH_DECL_PROTECT(lev);

    if (tryget_socket(s) == NULL)
        return -1;

    SYS_ARCH_PROTECT(lev);
    if (!_sock_is_opened(s)) {
        SYS_ARCH_UNPROTECT(lev);
        return -1;
    }
    SYS_ARCH_UNPROTECT(lev);

    lwip_sync_mt(s, how);

    ret = lwip_shutdown_esp(s, how);

    return ret;
}

int lwip_close(int s)
{
    int ret;
    SYS_ARCH_DECL_PROTECT(lev);

    if (tryget_socket(s) == NULL)
        return -1;

    SYS_ARCH_PROTECT(lev);
    if (_sock_is_opened(s)) {
        _sock_set_open(s, 0);
        SYS_ARCH_UNPROTECT(lev);
    } else {
        SYS_ARCH_UNPROTECT(lev);
        return -1;
    }

#if ESP_UDP
    struct lwip_sock *sock = get_socket(s);
    if (sock)
        udp_sync_close_netconn(sock->conn);
#endif

    lwip_sync_mt(s, SHUT_RDWR);

    ret = lwip_close_esp(s);

    return ret;
}

int lwip_select(int maxfdp1, fd_set *readset, fd_set *writeset,
                   fd_set *exceptset, struct timeval *timeout)
{
    int ret;
    fd_set read_set, write_set;

    if (readset)
        MEMCPY(&read_set, readset, sizeof(fd_set));
    else
        FD_ZERO(&read_set);

    if (writeset)
        MEMCPY(&write_set, writeset, sizeof(fd_set));
    else
        FD_ZERO(&write_set);

    ret = lwip_enter_mt_select(maxfdp1, &read_set, &write_set);
    if (ret)
        return ret;

    ret = lwip_select_esp(maxfdp1, readset, writeset, exceptset, timeout);

    lwip_exit_mt_select(maxfdp1, &read_set, &write_set);

    return ret;
}

#endif
