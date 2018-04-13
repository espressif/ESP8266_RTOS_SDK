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
#include <nopoll_io.h>
#include <nopoll_private.h>

typedef struct _noPollSelect {
	noPollCtx          * ctx;
	fd_set               set;
	int                  length;
	int                  max_fds;
} noPollSelect;

/** 
 * @internal nopoll implementation to create a compatible "select" IO
 * call fd set reference.
 *
 * @return A newly allocated fd_set reference.
 */
noPollPtr nopoll_io_wait_select_create (noPollCtx * ctx) 
{
	noPollSelect * select = nopoll_new (noPollSelect, 1);

	/* set default behaviour expected for the set */
	select->ctx           = ctx;
	
	/* clear the set */
	FD_ZERO (&(select->set));

	return select;
}

/** 
 * @internal noPoll implementation to destroy the "select" IO call
 * created by the default create.
 * 
 * @param fd_group The fd group to be deallocated.
 */
void    nopoll_io_wait_select_destroy (noPollCtx * ctx, noPollPtr fd_group)
{
	fd_set * __fd_set = (fd_set *) fd_group;

	/* release memory allocated */
	nopoll_free (__fd_set);
	
	/* nothing more to do */
	return;
}

/** 
 * @internal noPoll implementation to clear the "select" IO call
 * created by the default create.
 * 
 * @param fd_group The fd group to be deallocated.
 */
void    nopoll_io_wait_select_clear (noPollCtx * ctx, noPollPtr __fd_group)
{
	noPollSelect * select = (noPollSelect *) __fd_group;

	/* clear the fd set */
	select->length = 0;
	FD_ZERO (&(select->set));

	/* nothing more to do */
	return;
}

/** 
 * @internal Default internal implementation for the wait operation to
 * change its status at least one socket description inside the fd set
 * provided.
 * 
 * @param __fd_group The fd set having all sockets to be watched.
 * @param wait_to The operation requested.
 * 
 * @return Number of connections that changed or -1 if something wailed
 */
int nopoll_io_wait_select_wait (noPollCtx * ctx, noPollPtr __fd_group)
{
	int                 result = -1;
	struct timeval      tv;
	noPollSelect     * _select = (noPollSelect *) __fd_group;

	/* init wait */
	tv.tv_sec    = 0;
	tv.tv_usec   = 500000;
	result       = select (_select->max_fds + 1, &(_select->set), NULL,   NULL, &tv);

	/* check result */
	if ((result == NOPOLL_SOCKET_ERROR) && (errno == NOPOLL_EINTR))
		return -1;
	
	return result;
}

/** 
 * @internal noPoll select implementation for the "add to" on fd set
 * operation.
 * 
 * @param fds The socket descriptor to be added.
 *
 * @param fd_set The fd set where the socket descriptor will be added.
 */
nopoll_bool  nopoll_io_wait_select_add_to (int               fds, 
					   noPollCtx       * ctx,
					   noPollConn      * conn,
					   noPollPtr         __fd_set)
{
	noPollSelect * select = (noPollSelect *) __fd_set;

	if (fds < 0) {
		nopoll_log (ctx, NOPOLL_LEVEL_CRITICAL,
			    "received a non valid socket (%d), unable to add to the set", fds);
		return nopoll_false;
	}

	/* set the value */
	FD_SET (fds, &(select->set));

	/* update length */
	select->length++;

	/* update max fds */
	if (fds > select->max_fds)
		select->max_fds = fds;

	return nopoll_true;
}

/** 
 * @internal
 *
 * @brief Default noPoll implementation for the "is set" on fd
 * set operation.
 * 
 * @param fds The socket descriptor to be checked to be active on the
 * given fd group.
 *
 * @param fd_set The fd set where the socket descriptor will be checked.
 */
nopoll_bool      nopoll_io_wait_select_is_set (noPollCtx   * ctx,
					       int           fds, 
					       noPollPtr      __fd_set)
{
	noPollSelect * select = (noPollSelect *) __fd_set;
	
	return FD_ISSET (fds, &(select->set));
}


/** 
 * @brief Creates an object that represents the best IO wait mechanism
 * found on the current system.
 *
 * @param ctx The context where the engine will be created/associated.
 *
 * @param engine Use \ref NOPOLL_IO_ENGINE_DEFAULT or the engine you
 * want to use.
 *
 * @return The selected IO wait mechanism or NULL if it fails.
 */ 
noPollIoEngine * nopoll_io_get_engine (noPollCtx * ctx, noPollIoEngineType engine_type)
{
	noPollIoEngine * engine = nopoll_new (noPollIoEngine, 1);
	if (engine == NULL)
		return NULL;

	/* configure default implementation */
	engine->create  = nopoll_io_wait_select_create;
	engine->destroy = nopoll_io_wait_select_destroy;
	engine->clear   = nopoll_io_wait_select_clear;
	engine->wait    = nopoll_io_wait_select_wait;
	engine->addto   = nopoll_io_wait_select_add_to;
	engine->isset   = nopoll_io_wait_select_is_set;

	/* call to create the object */
	engine->ctx       = ctx;
	engine->io_object = engine->create (ctx);
	
	/* return the engine that was created */
	return engine;
}

/** 
 * @brief Release the io engine created by \ref nopoll_io_get_engine.
 *
 * @param engine The engine to be released.
 */
void             nopoll_io_release_engine (noPollIoEngine * engine)
{
	if (engine == NULL)
		return;
	engine->destroy (engine->ctx, engine->io_object);
	nopoll_free (engine);
	return;
}


