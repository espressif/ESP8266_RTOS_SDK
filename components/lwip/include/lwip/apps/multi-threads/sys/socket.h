/**
 * @file
 * This file is a posix wrapper for lwip/sockets.h.
 */

/*
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 */

#ifndef _SOCKET_MT_H_
#define _SOCKET_MT_H_

#include "posix/sys/socket.h"

#ifdef SOCKETS_MT
int lwip_mt_init(void);
int lwip_socket_mt(int domain, int type, int protocol);
int lwip_bind_mt(int s, const struct sockaddr *name, socklen_t namelen);
int lwip_connect_mt(int s, const struct sockaddr *name, socklen_t namelen);
int lwip_listen_mt(int s, int backlog);
int lwip_accept_mt(int s, struct sockaddr *addr, socklen_t *addrlen);
int lwip_getpeername_mt(int s, struct sockaddr *name, socklen_t *namelen);
int lwip_getsockname_mt(int s, struct sockaddr *name, socklen_t *namelen);
int lwip_setsockopt_mt(int s, int level, int optname, const void *optval, socklen_t optlen);
int lwip_getsockopt_mt(int s, int level, int optname, void *optval, socklen_t *optlen);
int lwip_ioctl_mt(int s, long cmd, void *argp);
int lwip_sendto_mt(int s, const void *data, size_t size, int flags,
            const struct sockaddr *to, socklen_t tolen);
int lwip_send_mt(int s, const void *data, size_t size, int flags);
int lwip_recvfrom_mt(int s, void *mem, size_t len, int flags,
            struct sockaddr *from, socklen_t *fromlen);
int lwip_recv_mt(int s, void *mem, size_t len, int flags);
int lwip_read_mt(int s, void *mem, size_t len);
int lwip_write_mt(int s, const void *data, size_t size);
int lwip_shutdown_mt(int s, int how);
int lwip_close_mt(int s);
int lwip_select_mt(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);
int lwip_fcntl_mt(int s, int cmd, int val);

#undef accept
#undef bind
#undef shutdown
#undef connect
#undef getsockname
#undef getpeername
#undef setsockopt
#undef getsockopt
#undef listen
#undef recv
#undef recvfrom
#undef send
#undef sendto
#undef socket
#undef select
#undef ioctlsocket
#undef sendmsg

#define accept(a,b,c)         lwip_accept_mt(a,b,c)
#define bind(a,b,c)           lwip_bind_mt(a,b,c)
#define shutdown(a,b)         lwip_shutdown_mt(a,b)
#define connect(a,b,c)        lwip_connect_mt(a,b,c)
#define getsockname(a,b,c)    lwip_getsockname_mt(a,b,c)
#define getpeername(a,b,c)    lwip_getpeername_mt(a,b,c)
#define setsockopt(a,b,c,d,e) lwip_setsockopt_mt(a,b,c,d,e)
#define getsockopt(a,b,c,d,e) lwip_getsockopt_mt(a,b,c,d,e)
#define listen(a,b)           lwip_listen_mt(a,b)
#define recv(a,b,c,d)         lwip_recv_mt(a,b,c,d)
#define recvfrom(a,b,c,d,e,f) lwip_recvfrom_mt(a,b,c,d,e,f)
#define send(a,b,c,d)         lwip_send_mt(a,b,c,d)
#define sendto(a,b,c,d,e,f)   lwip_sendto_mt(a,b,c,d,e,f)
#define socket(a,b,c)         lwip_socket_mt(a,b,c)
#define select(a,b,c,d,e)     lwip_select_mt(a,b,c,d,e)
#define ioctlsocket(a,b,c)    lwip_ioctl_mt(a,b,c)

#if LWIP_POSIX_SOCKETS_IO_NAMES
#undef read
#undef write
#undef close
#undef fcntl
#undef writev
#undef closesocket
#undef ioctl

#define read(a,b,c)           lwip_read_mt(a,b,c)
#define write(a,b,c)          lwip_write_mt(a,b,c)
#define close(s)              lwip_close_mt(s)
#define closesocket(s)        lwip_close_mt(s)
#define fcntl(a,b,c)          lwip_fcntl_mt(a,b,c)
#endif /* LWIP_POSIX_SOCKETS_IO_NAMES */
#endif /* SOCKETS_MT */

#endif /* _SOCKET_H_ */
