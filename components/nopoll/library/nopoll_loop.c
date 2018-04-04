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
#include <nopoll_loop.h>
#include <nopoll_private.h>

/** 
 * \defgroup nopoll_loop noPoll Loop: basic support to create a watching loop for WebSocket listeners
 */

/** 
 * \addtogroup nopoll_loop
 * @{
 */

/** 
 * @internal Function used by nopoll_loop_wait to register all
 * connections into the io waiting object.
 */
nopoll_bool nopoll_loop_register (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data)
{
	/* do not add connections that aren't working */
	if (! nopoll_conn_is_ok (conn)) {
		/* remove this connection from registry */
		nopoll_ctx_unregister_conn (ctx, conn);
		return nopoll_false; /* keep foreach, don't stop */
	}

	/* register the connection socket */
	/* nopoll_log (ctx, NOPOLL_LEVEL_DEBUG, "Adding socket id: %d", conn->session);*/
	if (! ctx->io_engine->addto (conn->session, ctx, conn, ctx->io_engine->io_object)) {
		/* remove this connection from registry */
		nopoll_ctx_unregister_conn (ctx, conn);
		nopoll_log (ctx, NOPOLL_LEVEL_WARNING, "Failed to add socket %d to the watching set", conn->session);
	}

	return nopoll_false; /* keep foreach, don't stop */
}

/** 
 * @internal Function used to handle incoming data from from the
 * connection and to notify this data on the connection.
 */
void nopoll_loop_process_data (noPollCtx * ctx, noPollConn * conn)
{
	noPollMsg * msg;

	/* call to get messages from the connection */
	msg = nopoll_conn_get_msg (conn);
	if (msg == NULL)
		return;

	/* found message, notify it */
	if (conn->on_msg) 
		conn->on_msg (ctx, conn, msg, conn->on_msg_data);
	else if (ctx->on_msg)
		ctx->on_msg (ctx, conn, msg, ctx->on_msg_data);

	/* release message */
	nopoll_msg_unref (msg);
	return;
}

/** 
 * @internal Function used to detected which connections has something
 * interesting to be notified.
 *
 */
nopoll_bool nopoll_loop_process (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data)
{
	int        * conn_changed = (int *) user_data;

	/* check if the connection have something to notify */
	if (ctx->io_engine->isset (ctx, conn->session, ctx->io_engine->io_object)) {

		/* call to notify action according to role */
		switch (conn->role) {
		case NOPOLL_ROLE_CLIENT:
		case NOPOLL_ROLE_LISTENER:
			/* received data, notify */
			nopoll_loop_process_data (ctx, conn);
			break;
		case NOPOLL_ROLE_MAIN_LISTENER:
			/* call to handle */
			nopoll_conn_accept (ctx, conn);
			break;
		default:
			nopoll_log (ctx, NOPOLL_LEVEL_CRITICAL, "Found connection with unknown role, closing and dropping");
			nopoll_conn_shutdown (conn);
			break;
		}
		
		/* reduce connection changed */
		(*conn_changed)--;
	} /* end if */
	
	return (*conn_changed) == 0;
}

/** 
 * @internal Function used to init internal io wait mechanism...
 *
 * @param ctx The noPoll context to be initialized if it wasn't
 */
void nopoll_loop_init (noPollCtx * ctx) 
{
	if (ctx == NULL)
		return;

	/* grab the mutex for the following check */
	if (ctx->io_engine == NULL) {
		ctx->io_engine = nopoll_io_get_engine (ctx, NOPOLL_IO_ENGINE_DEFAULT);
		if (ctx->io_engine == NULL) {
			nopoll_log (ctx, NOPOLL_LEVEL_CRITICAL, "Failed to create IO wait engine, unable to implement wait call");
			return;
		} 
	} /* end if */
	/* release the mutex */

	return;
}

/** 
 * @brief Flag to stop the current loop implemented (if any) on the provided context.
 *
 * @param ctx The context where the loop is being done, and wanted to
 * be stopped.
 *
 */
void nopoll_loop_stop (noPollCtx * ctx)
{
	if (! ctx)
		return;
	ctx->keep_looping = nopoll_false;
	return;
} /* end if */

/** 
 * @brief Allows to implement a wait over all connections registered
 * under the provided context during the provided timeout until
 * something is detected meaningful to the user, calling to the action
 * handler defined, optionally receving the user data pointer.
 *
 * @param ctx The context object where the wait will be implemented.
 *
 * @param timeout The timeout to wait for changes. If no changes
 * happens, the function returns. The function will block the caller
 * until a call to \ref nopoll_loop_stop is done in the case timeout
 * passed is 0.
 *
 * @return The function returns 0 when finished or -2 in the case ctx
 * is NULL or timeout is negative.
 */
int nopoll_loop_wait (noPollCtx * ctx, long timeout)
{
	struct timeval start;
	struct timeval stop;
	struct timeval diff;
	long           ellapsed;
	int            wait_status;

	nopoll_return_val_if_fail (ctx, ctx, -2);
	nopoll_return_val_if_fail (ctx, timeout >= 0, -2);
	
	/* call to init io engine */
	nopoll_loop_init (ctx);

	/* get as reference current time */
	if (timeout > 0)
#if defined(NOPOLL_OS_WIN32)
		nopoll_win32_gettimeofday (&start, NULL);
#else
		gettimeofday (&start, NULL);
#endif
	
	/* set to keep looping everything this function is called */
	ctx->keep_looping = nopoll_true;

	while (ctx->keep_looping) {
		/* ok, now implement wait operation */
		ctx->io_engine->clear (ctx, ctx->io_engine->io_object);
		
		/* add all connections */
		/* nopoll_log (ctx, NOPOLL_LEVEL_DEBUG, "Adding connections to watch: %d", ctx->conn_num);  */
		nopoll_ctx_foreach_conn (ctx, nopoll_loop_register, NULL);

		/* if (errno == EBADF) { */
			/* detected some descriptor not properly
			 * working, try to check them */
			/* nopoll_ctx_foreach_conn (ctx, nopoll_loop_clean_descriptors, NULL); */
		/* nopoll_log (ctx, NOPOLL_LEVEL_CRITICAL, "Found some descriptor is not valid (errno==%d)", errno);
		   continue; */ 
		/* } */ /* end if */
		
		/* implement wait operation */
		/* nopoll_log (ctx, NOPOLL_LEVEL_DEBUG, "Waiting for changes into %d connections", ctx->conn_num); */
		wait_status = ctx->io_engine->wait (ctx, ctx->io_engine->io_object);
		/* nopoll_log (ctx, NOPOLL_LEVEL_DEBUG, "Waiting finished with result %d", wait_status);  */
		if (wait_status == -1) {
			nopoll_log (ctx, NOPOLL_LEVEL_CRITICAL, "Received error from wait operation, error code was: %d", errno);
			break;
		} /* end if */

		/* check how many connections changed and restart */
		if (wait_status > 0) {
			/* check and call for connections with something
			 * interesting */
			nopoll_ctx_foreach_conn (ctx, nopoll_loop_process, &wait_status);
		}

		/* check to stop wait operation */
		if (timeout > 0) {
#if defined(NOPOLL_OS_WIN32)
			nopoll_win32_gettimeofday (&stop, NULL);
#else
			gettimeofday (&stop, NULL);
#endif
			nopoll_timeval_substract (&stop, &start, &diff);
			ellapsed = (diff.tv_sec * 1000000) + diff.tv_usec;
			if (ellapsed > timeout)
				break;
		} /* end if */
	} /* end while */

	/* release engine */
	nopoll_io_release_engine (ctx->io_engine);
	ctx->io_engine = NULL;

	return 0;
}

/* @} */


