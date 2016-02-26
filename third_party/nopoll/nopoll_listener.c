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
#include <nopoll_listener.h>
#include <nopoll_private.h>

/** 
 * \defgroup nopoll_listener noPoll Listener: functions required to create WebSocket listener connections.
 */

/** 
 * \addtogroup nopoll_listener
 * @{
 */

/** 
 * @internal Creates a listener socket on the provided port.
 */
NOPOLL_SOCKET     nopoll_listener_sock_listen      (noPollCtx   * ctx,
						    const char  * host,
						    const char  * port)
{
	struct hostent     * he;
	struct in_addr     * haddr;
	struct sockaddr_in   saddr;
	struct sockaddr_in   sin;
	NOPOLL_SOCKET        fd;
	int                  tries;

#if defined(NOPOLL_OS_WIN32)
	int                  sin_size  = sizeof (sin);
#else    	
	int                  unit      = 1; 
	socklen_t            sin_size  = sizeof (sin);
#endif	
	uint16_t             int_port;
	int                  bind_res;

	nopoll_return_val_if_fail (ctx, ctx,  -2);
	nopoll_return_val_if_fail (ctx, host, -2);
	nopoll_return_val_if_fail (ctx, port || strlen (port) == 0, -2);

	/* resolve hostname */
	he = gethostbyname (host);
        if (he == NULL) {
		nopoll_log (ctx, NOPOLL_LEVEL_CRITICAL, "unable to get hostname by calling gethostbyname");
		return -1;
	} /* end if */

	haddr = ((struct in_addr *) (he->h_addr_list)[0]);
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) <= 2) {
		/* do not allow creating sockets reusing stdin (0),
		   stdout (1), stderr (2) */
		nopoll_log (ctx, NOPOLL_LEVEL_DEBUG, "failed to create listener socket: %d (errno=%d)", fd, errno);
		return -1;
        } /* end if */

#if defined(NOPOLL_OS_WIN32)
	/* Do not issue a reuse addr which causes on windows to reuse
	 * the same address:port for the same process. Under linux,
	 * reusing the address means that consecutive process can
	 * reuse the address without being blocked by a wait
	 * state.  */
	/* setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char  *)&unit, sizeof(BOOL)); */
#else
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &unit, sizeof (unit));
#endif 

	/* get integer port */
	int_port  = (uint16_t) atoi (port);

	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family          = AF_INET;
	saddr.sin_port            = htons(int_port);
	memcpy(&saddr.sin_addr, haddr, sizeof(struct in_addr));

	/* call to bind */
	tries    = 0;
	while (1) {
		bind_res = bind(fd, (struct sockaddr *)&saddr,  sizeof (struct sockaddr_in));
		if (bind_res == NOPOLL_SOCKET_ERROR) {
			/* check if we can retry */
			tries++;
			if (tries < 25) {
				nopoll_log (ctx, NOPOLL_LEVEL_WARNING, 
					    "unable to bind address (port:%u already in use or insufficient permissions, errno=%d : %s), retrying=%d on socket: %d", 
					    int_port, errno, strerror (errno), tries, fd);
				nopoll_sleep (100000);
				continue;
			} /* end if */

			nopoll_log (ctx, NOPOLL_LEVEL_CRITICAL, 
				    "unable to bind address (port:%u already in use or insufficient permissions, errno=%d : %s). Closing socket: %d", 
				    int_port, errno, strerror (errno), fd);
			nopoll_close_socket (fd);
			return -1;
		} /* end if */

		/* reached this point, bind was ok */
		break;
	} /* end while */
	
	if (listen(fd, ctx->backlog) == NOPOLL_SOCKET_ERROR) {
		nopoll_log (ctx, NOPOLL_LEVEL_CRITICAL, "an error have occur while executing listen");
		return -1;
        } /* end if */

	/* notify listener */
	if (getsockname (fd, (struct sockaddr *) &sin, &sin_size) < -1) {
		return -1;
	} /* end if */

	/* report and return fd */
	nopoll_log  (ctx, NOPOLL_LEVEL_DEBUG, "running listener at %s:%d (socket: %d)", inet_ntoa(sin.sin_addr), ntohs (sin.sin_port), fd);
	return fd;
}

/** 
 * @brief Creates a new websocket server listener on the provided host
 * name and port. 
 *
 * @param ctx The context where the operation will take place.
 *
 * @param host The hostname or address interface to bind on.
 *
 * @param port The port where to listen, or NULL to use default port: 80.
 *
 * @return A reference to a \ref noPollConn object representing the
 * listener or NULL if it fails.
 */
noPollConn      * nopoll_listener_new (noPollCtx  * ctx,
				       const char * host,
				       const char * port)
{
	return nopoll_listener_new_opts (ctx, NULL, host, port);
}

/** 
 * @brief Creates a new websocket server listener on the provided host
 * name and port.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param opts Optional connection options to configure this listener.
 *
 * @param host The hostname or address interface to bind on.
 *
 * @param port The port where to listen, or NULL to use default port: 80.
 *
 * @return A reference to a \ref noPollConn object representing the
 * listener or NULL if it fails.
 */
noPollConn      * nopoll_listener_new_opts (noPollCtx      * ctx,
					    noPollConnOpts * opts,
					    const char     * host,
					    const char     * port)
{
	NOPOLL_SOCKET   session;
	noPollConn    * listener;

	nopoll_return_val_if_fail (ctx, ctx && host, NULL);

	/* call to create the socket */
	session = nopoll_listener_sock_listen (ctx, host, port);
	if (session == NOPOLL_INVALID_SOCKET) {
		nopoll_log (ctx, NOPOLL_LEVEL_CRITICAL, "Failed to start listener error was: %d", errno);
		return NULL;
	} /* end if */

	/* create noPollConn ection object */
	listener          = nopoll_new (noPollConn, 1);
	listener->refs    = 1;
	listener->session = session;
	listener->ctx     = ctx;
	listener->role    = NOPOLL_ROLE_MAIN_LISTENER;

	/* record host and port */
	listener->host    = nopoll_strdup (host);
	listener->port    = nopoll_strdup (port);

	/* register connection into context */
	nopoll_ctx_register_conn (ctx, listener);

	/* configure default handlers */
	listener->receive = nopoll_conn_default_receive;
	listener->sends    = nopoll_conn_default_send;

	nopoll_log (ctx, NOPOLL_LEVEL_DEBUG, "Listener created, started: %s:%s (socket: %d)", listener->host, listener->port, listener->session);

	return listener;
}

/** 
 * @brief Allows to create a new WebSocket listener but expecting the
 * incoming connection to be under TLS supervision. The function works
 * like \ref nopoll_listener_new (providing wss:// services).
 *
 * @param ctx The context where the operation will take place.
 *
 * @param host The hostname or address interface to bind on.
 *
 * @param port The port where to listen, or NULL to use default port: 80.
 *
 * @return A reference to a \ref noPollConn object representing the
 * listener or NULL if it fails.
 */
noPollConn      * nopoll_listener_tls_new (noPollCtx  * ctx,
					   const char * host,
					   const char * port)
{
	return nopoll_listener_tls_new_opts (ctx, NULL, host, port);
}


/** 
 * @brief Allows to create a new WebSocket listener but expecting the
 * incoming connection to be under TLS supervision. The function works
 * like \ref nopoll_listener_new (providing wss:// services).
 *
 * @param ctx The context where the operation will take place.
 *
 * @param opts The connection options to configure this particular
 * listener.
 *
 * @param host The hostname or address interface to bind on.
 *
 * @param port The port where to listen, or NULL to use default port: 80.
 *
 * @return A reference to a \ref noPollConn object representing the
 * listener or NULL if it fails.
 */
noPollConn      * nopoll_listener_tls_new_opts (noPollCtx      * ctx,
						noPollConnOpts * opts,
						const char     * host,
						const char     * port)
{
	noPollConn * listener;

	/* call to get listener from base function */
	listener = nopoll_listener_new (ctx, host, port);
	if (! listener)
		return listener;

	/* setup TLS support */
	listener->tls_on = nopoll_true;
	listener->opts   = opts;

	return listener;
}

/** 
 * @brief Allows to configure the TLS certificate and key to be used
 * on the provided connection.
 *
 * @param listener The listener that is going to be configured with the providing certificate and key.
 *
 * @param certificate The path to the public certificate file (PEM
 * format) to be used for every TLS connection received under the
 * provided listener.
 *
 * @param private_key The path to the key file (PEM format) to be used for
 * every TLS connection received under the provided listener.
 *
 * @param chain_file The path to additional chain certificates (PEM
 * format). You can safely pass here a NULL value.
 *
 * @return nopoll_true if the certificates were configured, otherwise
 * nopoll_false is returned.
 */
nopoll_bool           nopoll_listener_set_certificate (noPollConn * listener,
						       const char * certificate,
						       const char * private_key,
						       const char * chain_file)
{
//	FILE * handle;
//
//	if (! listener || ! certificate || ! private_key)
//		return nopoll_false;
//
//	/* check certificate file */
//	handle = fopen (certificate, "r");
//	if (! handle) {
//		nopoll_log (listener->ctx, NOPOLL_LEVEL_CRITICAL, "Failed to open certificate file from %s", certificate);
//		return nopoll_false;
//	} /* end if */
//	fclose (handle);
//
//	/* check private file */
//	handle = fopen (private_key, "r");
//	if (! handle) {
//		nopoll_log (listener->ctx, NOPOLL_LEVEL_CRITICAL, "Failed to open private key file from %s", private_key);
//		return nopoll_false;
//	} /* end if */
//	fclose (handle);
//
//	if (chain_file) {
//		/* check private file */
//		handle = fopen (chain_file, "r");
//		if (! handle) {
//			nopoll_log (listener->ctx, NOPOLL_LEVEL_CRITICAL, "Failed to open chain certificate file from %s", private_key);
//			return nopoll_false;
//		} /* end if */
//		fclose (handle);
//	} /* end if */
//
//	/* copy certificates to be used */
//	listener->certificate   = nopoll_strdup (certificate);
//	listener->private_key   = nopoll_strdup (private_key);
//	if (chain_file)
//		listener->chain_certificate = nopoll_strdup (chain_file);
//
//	nopoll_log (listener->ctx, NOPOLL_LEVEL_DEBUG, "Configured certificate: %s, key: %s, for conn id: %d",
//		    listener->certificate, listener->private_key, listener->id);

	/* certificates configured */
	return nopoll_true;
}

/** 
 * @brief Creates a websocket listener from the socket provided.
 *
 * @param ctx The context where the listener will be associated.
 *
 * @param session The session to associate to the listener.
 *
 * @return A reference to a listener connection object or NULL if it
 * fails.
 */
noPollConn   * nopoll_listener_from_socket (noPollCtx      * ctx,
					    NOPOLL_SOCKET    session)
{
	noPollConn         * listener;
	struct sockaddr_in   sin;
#if defined(NOPOLL_OS_WIN32)
	/* windows flavors */
	int                  sin_size = sizeof (sin);
#else
	/* unix flavors */
	socklen_t            sin_size = sizeof (sin);
#endif

	nopoll_return_val_if_fail (ctx, ctx && session > 0, NULL);
	
	/* create noPollConn ection object */
	listener          = nopoll_new (noPollConn, 1);
	listener->refs    = 1;
	listener->session = session;
	listener->ctx     = ctx;
	listener->role    = NOPOLL_ROLE_LISTENER;

	/* get peer value */
	memset (&sin, 0, sizeof (struct sockaddr_in));
	if (getpeername (session, (struct sockaddr *) &sin, &sin_size) < -1) {
		nopoll_log (ctx, NOPOLL_LEVEL_CRITICAL, "unable to get remote hostname and port");
		return NULL;
	} /* end if */

	/* record host and port */
	/* lock mutex here to protect inet_ntoa */
	listener->host    = nopoll_strdup (inet_ntoa (sin.sin_addr));
	/* release mutex here to protect inet_ntoa */
	listener->port    = nopoll_strdup_printf ("%d", ntohs (sin.sin_port));

	/* configure default handlers */
	listener->receive = nopoll_conn_default_receive;
	listener->sends    = nopoll_conn_default_send;

	/* register connection into context */
	if (! nopoll_ctx_register_conn (ctx, listener)) {
		nopoll_log (ctx, NOPOLL_LEVEL_CRITICAL, "Failed to register connection into the context, unable to create connection");
		nopoll_conn_ref (listener);
		return NULL;
	} /* end if */

	nopoll_log (ctx, NOPOLL_LEVEL_DEBUG, "Listener created, started: %s:%s (socket: %d)", listener->host, listener->port, listener->session);

	/* reduce reference counting here because ctx_register_conn
	 * already acquired a reference */
	nopoll_conn_unref (listener); 
	
	return listener;
}

/** 
 * @internal Public function that performs a TCP listener accept.
 *
 * @param server_socket The listener socket where the accept()
 * operation will be called.
 *
 * @return Returns a connected socket descriptor or -1 if it fails.
 */
NOPOLL_SOCKET nopoll_listener_accept (NOPOLL_SOCKET server_socket)
{
	struct sockaddr_in inet_addr;
#if defined(NOPOLL_OS_WIN32)
	int               addrlen;
#else
	socklen_t         addrlen;
#endif
	addrlen       = sizeof(struct sockaddr_in);

	/* accept the connection new connection */
	return accept (server_socket, (struct sockaddr *)&inet_addr, &addrlen);
}

/* @} */
