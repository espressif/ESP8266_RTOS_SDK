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
#include <nopoll_conn_opts.h>
#include <nopoll_private.h>

/** 
 * \defgroup nopoll_conn_opts noPoll Connection Options: API to change default connection options.
 */

/** 
 * \addtogroup nopoll_conn_opts
 * @{
 */

/** 
 * @brief Create a new connection options object.
 *
 * @return A newly created connection options object. In general you don't have to worry about releasing this object because this is automatically done by functions using this object. However, if you call to \ref nopoll_conn_opts_set_reuse (opts, nopoll_true), then you'll have to use \ref nopoll_conn_opts_free to release the object after it is no longer used. The function may return NULL in case of memory allocation problems. Creating an object without setting anything will cause the library to provide same default behaviour as not providing it.
 */
noPollConnOpts * nopoll_conn_opts_new (void)
{
	noPollConnOpts * result;

	/* create configuration object */
	result = nopoll_new (noPollConnOpts, 1);
	if (! result)
		return NULL;

	result->reuse        = nopoll_false; /* this is not needed, just to clearly state defaults */
	result->ssl_protocol = NOPOLL_METHOD_TLSV1;

	result->mutex        = nopoll_mutex_create ();
	result->refs         = 1;

	/* by default, disable ssl peer verification */
	result->disable_ssl_verify = nopoll_true;

	return result;
}

/** 
 * @brief Set ssl protocol method to be used on the API receiving this
 * configuration object.
 *
 * @param opts The connection options object. 
 *
 * @param ssl_protocol SSL protocol to use. See \ref noPollSslProtocol for more information.
 */
void nopoll_conn_opts_set_ssl_protocol (noPollConnOpts * opts, noPollSslProtocol ssl_protocol)
{
	if (opts == NULL)
		return;
	opts->ssl_protocol = ssl_protocol;
	return;
}


/** 
 * @brief Allows to certificate, private key and optional chain
 * certificate and ca for on a particular options that can be used for
 * a client and a listener connection.
 *
 * @param opts The connection options where these settings will be
 * applied.
 *
 * @param certificate The certificate to use on the connection.
 *
 * @param private_key client_certificate private key.
 *
 * @param chain_certificate Optional chain certificate to use 
 *
 * @param ca_certificate Optional CA certificate to use during the
 * process.
 *
 * @return nopoll_true in the case all certificate files provided are
 * reachable.
 */
nopoll_bool        nopoll_conn_opts_set_ssl_certs    (noPollConnOpts * opts, 
						      const char     * certificate,
						      const char     * private_key,
						      const char     * chain_certificate,
						      const char     * ca_certificate)
{
	if (opts == NULL)
		return nopoll_false;
	
	/* store certificate settings */
	opts->certificate        = nopoll_strdup (certificate);
	if (opts->certificate)
//		if (access (opts->certificate, R_OK) != 0)
//			return nopoll_false;
	opts->private_key        = nopoll_strdup (private_key);
	if (opts->private_key)
//		if (access (opts->private_key, R_OK) != 0)
//			return nopoll_false;
	opts->chain_certificate  = nopoll_strdup (chain_certificate);
	if (opts->chain_certificate)
//		if (access (opts->chain_certificate, R_OK) != 0)
//			return nopoll_false;
	opts->ca_certificate     = nopoll_strdup (ca_certificate);
	if (opts->ca_certificate)
//		if (access (opts->ca_certificate, R_OK) != 0)
//			return nopoll_false;

	return nopoll_true;
}

/** 
 * @brief Allows to disable peer ssl certificate verification. This is
 * not recommended for production enviroment. This affects in a
 * different manner to a listener connection and a client connection.
 *
 * For a client connection, by default, peer verification is enabled
 * and this function may help to disable it during development or
 * other reasons.
 *
 * In the case of the servers (created by using \ref
 * nopoll_listener_new for example) this is not required because by
 * default peer verification is disabled by default.
 *
 * @param opts The connection option to configure.
 *
 * @param verify nopoll_true to disable verification
 * otherwise, nopoll_false should be used. By default SSL verification
 * is enabled.
 *
 */
void nopoll_conn_opts_ssl_peer_verify (noPollConnOpts * opts, nopoll_bool verify)
{
	if (opts == NULL)
		return;
	opts->disable_ssl_verify = ! verify;
	return;
}

/** 
 * @brief Allows to set Cookie header content to be sent during the
 * connection handshake. If configured and the remote side server is a
 * noPoll peer, use \ref nopoll_conn_get_cookie to get this value.
 *
 * @param opts The connection option to configure.
 *
 * @param cookie_content Content for the cookie. If you pass NULL the
 * cookie is unset.
 */
void        nopoll_conn_opts_set_cookie (noPollConnOpts * opts, const char * cookie_content)
{
	if (opts == NULL)
		return;

	if (cookie_content) {
		/* configure cookie content to be sent */
		opts->cookie = nopoll_strdup (cookie_content);
	} else {
		nopoll_free (opts->cookie);
		opts->cookie = NULL;
	} /* end if */

	return;
}


/** 
 * @brief Allows to increase a reference to the connection options
 * provided. 
 *
 * @param opts The connection option reference over which a connection
 * reference is needed.
 *
 * @return nopoll_true in the case the operation went ok, otherwise
 * nopoll_false is returned.
 */
nopoll_bool nopoll_conn_opts_ref (noPollConnOpts * opts)
{
	if (opts == NULL)
		return nopoll_false;

	/* lock the mutex */
	nopoll_mutex_lock (opts->mutex);
	if (opts->refs <= 0) {
		/* unlock the mutex */
		nopoll_mutex_unlock (opts->mutex);
		return nopoll_false;
	}
	
	opts->refs++;

	/* release here the mutex */
	nopoll_mutex_unlock (opts->mutex);

	return nopoll_true;
}

/** 
 * @brief Allows to unref a reference acquired by \ref nopoll_conn_opts_ref
 *
 * @param opts The connection opts to release.
 */
void        nopoll_conn_opts_unref (noPollConnOpts * opts)
{
	/* call free implementation */
	nopoll_conn_opts_free (opts);
	return;
}


/** 
 * @brief Set reuse-flag be used on the API receiving this
 * configuration object. By setting nopoll_true will cause the API to
 * not release the object when finished. Instead, the caller will be
 * able to use this object in additional API calls but, after
 * finishing, a call to \ref nopoll_conn_opts_set_reuse function is
 * required.
 *
 * @param opts The connection options object. 
 *
 * @param reuse nopoll_true to reuse the object across calls,
 * otherwise nopoll_false to make the API function to release the
 * object when done.
 */
void nopoll_conn_opts_set_reuse        (noPollConnOpts * opts, nopoll_bool reuse)
{
	if (opts == NULL)
		return;
	opts->reuse = reuse;
	return;
}

void __nopoll_conn_opts_free_common  (noPollConnOpts * opts)
{
	if (opts == NULL)
		return;

	/* acquire here the mutex */
	nopoll_mutex_lock (opts->mutex);

	opts->refs--;
	if (opts->refs != 0) {
		/* release here the mutex */
		nopoll_mutex_unlock (opts->mutex);
		return;
	}
	/* release here the mutex */
	nopoll_mutex_unlock (opts->mutex);

	nopoll_free (opts->certificate);
	nopoll_free (opts->private_key);
	nopoll_free (opts->chain_certificate);
	nopoll_free (opts->ca_certificate);

	/* cookie */
	nopoll_free (opts->cookie);

	/* release mutex */
	nopoll_mutex_destroy (opts->mutex);
	nopoll_free (opts);
	return;
}

/** 
 * @brief Allows to release a connection object reported by \ref nopoll_conn_opts_new
 *
 * IMPORTANT NOTE: do not use this function over a \ref noPollConnOpts if it is not flagged with \ref nopoll_conn_opts_set_reuse (opts, nopoll_true).
 *
 * Default behaviour provided by the API implies that every connection
 * options object created by \ref nopoll_conn_opts_new is
 * automatically released by the API consuming that object.
 */
void nopoll_conn_opts_free (noPollConnOpts * opts)
{
	__nopoll_conn_opts_free_common (opts);
	return;
} /* end if */

/** 
 * @internal API. Do not use it. It may change at any time without any
 * previous indication.
 */
void __nopoll_conn_opts_release_if_needed (noPollConnOpts * options)
{
	if (! options)
		return;
	if (options && options->reuse)
		return;
	__nopoll_conn_opts_free_common (options);
	return;
}

/* @} */
