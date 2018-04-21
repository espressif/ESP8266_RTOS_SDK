/*
 *  LibNoPoll: A websocket library
 *  Copyright (C) 2013 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
 *
 *  For commercial support on build Websocket enabled solutions
 *  contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         Edificio Alius A, Oficina 102,
 *         C/ Antonio Suarez Nº 10,
 *         Alcalá de Henares 28802 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/nopoll
 */

#include <nopoll.h>

#define LOG_DOMAIN "nopoll-win32"

#if defined(NOPOLL_OS_WIN32)

nopoll_bool __nopoll_win32_was_init = nopoll_false;

int  nopoll_win32_init (noPollCtx * ctx)
{
	WORD wVersionRequested; 
	WSADATA wsaData; 
	int error; 

	if (__nopoll_win32_was_init)
		return nopoll_true;
	
	wVersionRequested = MAKEWORD( 2, 2 ); 
	
	error = WSAStartup( wVersionRequested, &wsaData ); 
	if (error != NO_ERROR) {
		nopoll_log (ctx, NOPOLL_LEVEL_CRITICAL, "unable to init winsock api, exiting..");
		return nopoll_false;
	}
	nopoll_log (ctx, NOPOLL_LEVEL_DEBUG, "winsock initialization ok");
	/* flag the library as initialized */
	__nopoll_win32_was_init = nopoll_true;
	return nopoll_true;
}

BOOL APIENTRY DllMain (HINSTANCE hInst,
                       DWORD reason,
                       LPVOID reserved)
{
 
    /* always returns true because nopoll init is done through
     * nopoll_init */
    return nopoll_true;
}

int      __nopoll_win32_blocking_socket_set (NOPOLL_SOCKET socket,
                                             int           status) 
{
        unsigned long enable = status;

	return (ioctlsocket (socket, FIONBIO, &enable) == 0);
}

int      nopoll_win32_nonblocking_enable (NOPOLL_SOCKET socket)
{
        return __nopoll_win32_blocking_socket_set (socket, 1);
}

int      nopoll_win32_blocking_enable (NOPOLL_SOCKET socket)
{
        return __nopoll_win32_blocking_socket_set (socket, 0);
}

#if ! defined(HAVE_GETTIMEOFDAY)


/** 
 * @brief The function obtains the current time, expressed as seconds
 * and microseconds since the Epoch, and store it in the timeval
 * structure pointed to by tv. As posix says gettimeoday should return
 * zero and should not reserve any value for error, this function
 * returns zero.
 *
 * The timeval struct have the following members:
 *
 * \code
 * struct timeval {
 *     long tv_sec;
 *     long tv_usec;
 * } timeval;
 * \endcode
 * 
 * @param tv Timeval struct.
 * @param notUsed Not defined.
 * 
 * @return The function allways return 0.
 */
int nopoll_win32_gettimeofday(struct timeval *tv, noPollPtr notUsed)
{
	union {
		long long ns100;
		FILETIME fileTime;
	} now;
	
	GetSystemTimeAsFileTime (&now.fileTime);
	tv->tv_usec = (long) ((now.ns100 / 10LL) % 1000000LL);
	tv->tv_sec = (long) ((now.ns100 - 116444736000000000LL) / 10000000LL);
	return (0);
} /* end gettimeofday */
#endif /* end ! defined(HAVE_GETTIMEOFDAY) */

#endif 
