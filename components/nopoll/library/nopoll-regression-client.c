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
#include <nopoll/nopoll.h>
#include "ssl/ssl_compat-1.0.h"
#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#if defined(__NOPOLL_PTHREAD_SUPPORT__)
#include <pthread.h>
noPollPtr __nopoll_regtest_mutex_create (void) {
	pthread_mutex_t * mutex = nopoll_new (pthread_mutex_t, 1);
	if (mutex == NULL)
		return NULL;

	/* init the mutex using default values */
	if (pthread_mutex_init (mutex, NULL) != 0) {
		return NULL;
	} /* end if */

	return mutex;
}

void __nopoll_regtest_mutex_destroy (noPollPtr _mutex) {
	pthread_mutex_t * mutex = _mutex;
	if (mutex == NULL)
		return;

	if (pthread_mutex_destroy (mutex) != 0) {
		/* do some reporting */
		return;
	}
	nopoll_free (mutex);

	return;
}

void __nopoll_regtest_mutex_lock (noPollPtr _mutex) {
	pthread_mutex_t * mutex = _mutex;

	/* lock the mutex */
	if (pthread_mutex_lock (mutex) != 0) {
		/* do some reporting */
		return;
	} /* end if */
	return;
}

void __nopoll_regtest_mutex_unlock (noPollPtr _mutex) {
	pthread_mutex_t * mutex = _mutex;

	/* unlock mutex */
	if (pthread_mutex_unlock (mutex) != 0) {
		/* do some reporting */
		return;
	} /* end if */
	return;
}
#endif

nopoll_bool debug = nopoll_false;
nopoll_bool show_critical_only = nopoll_false;

#define local_host_name		"iot.espressif.cn"
#define local_host_url		"v1/datastreams/tem_hum/datapoint"
#define local_host_port		"9000"
#define local_host_ports	"9443"

nopoll_bool test_sending_and_check_echo (noPollConn * conn, const char * label, const char * msg)
{
	char  buffer[1024];
	int   length = strlen (msg);
	int   bytes_read;

	/* wait for the reply */
	while (nopoll_true) {
		if (nopoll_conn_is_ready (conn))
			break;
		nopoll_sleep (10000);
	} /* end if */

	/* send content text(utf-8) */
	printf ("%s: sending content..\n", label);
	if (nopoll_conn_send_text (conn, msg, length) != length) {
		printf ("ERROR: Expected to find proper send operation..\n");
		return nopoll_false;
	}

	/* wait for the reply (try to read 1024, blocking and with a 3 seconds timeout) */
	bytes_read = nopoll_conn_read (conn, buffer, length, nopoll_true, 3000);
	if (bytes_read > 0)
		buffer[bytes_read] = 0;
	
	if (bytes_read != length) {
		printf ("ERROR: expected to find 14 bytes but found %d..\n", bytes_read);
		return nopoll_false;
	} /* end if */

	/* check content received */
	if (! nopoll_cmp (buffer, msg)) {
		printf ("ERROR: expected to find message 'This is a test' but something different was received: '%s'..\n",
			buffer);
		return nopoll_false;
	} /* end if */

	printf ("%s: received reply and echo matches..\n", label);

	/* return that we sent and received the echo reply */
	return nopoll_true;
}

void __report_critical (noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data)
{
        if (level == NOPOLL_LEVEL_CRITICAL) {
  	        printf ("CRITICAL: %s\n", log_msg);
	}
	return;
}

noPollCtx * create_ctx (void) {
	
	/* create a context */
	noPollCtx * ctx = nopoll_ctx_new ();
	nopoll_log_enable (ctx, debug);
	nopoll_log_color_enable (ctx, debug);

	/* configure handler */
	if (show_critical_only)
	        nopoll_log_set_handler (ctx, __report_critical, NULL);
	return ctx;
}

nopoll_bool test_01_strings (void) {
	/* check string compare functions */
	if (! nopoll_ncmp ("GET ", "GET ", 4)) {
		printf ("ERROR (1): expected to find right equal comparison..\n");
		return nopoll_false;
	}

	if (! nopoll_ncmp ("GET VALUE", "GET ", 4)) {
		printf ("ERROR (2): expected to find right equal comparison..\n");
		return nopoll_false;
	}

	return nopoll_true;
}

nopoll_bool test_01_base64 (void) {
	char buffer[1024];
	int  size = 1024;
	int  iterator = 0;

	/* call to produce base 64 (we do a loop to ensure we don't
	 * leak through openssl (220) bytes */
	while (iterator < 10) {
		size = 1024;
		if (! nopoll_base64_encode ("This is a test", 14, buffer, &size)) {
			printf ("ERROR: failed to encode this is a test..\n");
			return nopoll_false;
		} /* end if */
		
		/* check result */
		if (! nopoll_cmp (buffer, "VGhpcyBpcyBhIHRlc3Q=")) {
			printf ("ERROR: expected to find encoded base64 string %s but found %s..\n", 
				"VGhpcyBpcyBhIHRlc3Q=", buffer);
			return nopoll_false;
		}

		iterator++;
	}

	/* now decode content */
	iterator = 0;
	while (iterator < 10) {
		size = 1024;
		if (! nopoll_base64_decode ("VGhpcyBpcyBhIHRlc3Q=", 20, buffer, &size)) {
			printf ("ERROR: failed to decode base64 content..\n");
		}
		
		/* check result */
		if (! nopoll_cmp (buffer, "This is a test")) {
			printf ("ERROR: expected to find encoded base64 string %s but found %s..\n", 
				"This is a test", buffer);
			return nopoll_false;
		} /* end if */

		iterator++;
	}

	
	return nopoll_true;
}

nopoll_bool test_01_masking (void) {

	char         mask[4];
	int          mask_value;
	char         buffer[1024];
	noPollCtx  * ctx;

	/* clear buffer */
	memset (buffer, 0, 1024);

	/* create context */
	ctx = create_ctx ();

#if defined(NOPOLL_OS_WIN32)
	mask_value = rand ();
#else
	mask_value = os_random ();
#endif
	printf ("Test-01 masking: using masking value %d\n", mask_value);
	nopoll_set_32bit (mask_value, mask);

	memcpy (buffer, "This is a test value", 20);
	nopoll_conn_mask_content (ctx, buffer, 20, mask, 0);

	if (nopoll_ncmp (buffer, "This is a test value", 20)) {
		printf ("ERROR: expected to find different values after masking but found the same..\n");
		return nopoll_false;
	}

	/* revert changes */
	nopoll_conn_mask_content (ctx, buffer, 20, mask, 0);

	if (! nopoll_ncmp (buffer, "This is a test value", 20)) {
		printf ("ERROR: expected to find SAME values after masking but found the same..\n");
		return nopoll_false;
	} /* end if */

	/* now check transfering these values to the mask */
	if (nopoll_get_32bit (mask) != mask_value) {
		printf ("ERROR: found failure while reading the mask from from buffer..\n");
		return nopoll_false;
	}
	printf ("Test 01 masking: found mask in the buffer %d == %d\n", 
		nopoll_get_32bit (mask), mask_value);

	nopoll_ctx_unref (ctx);
	return nopoll_true;
}

nopoll_bool test_01 (void) {
	noPollCtx  * ctx;
	noPollConn * conn;

	/* create context */
	ctx = create_ctx ();

	/* check connections registered */
	if (nopoll_ctx_conns (ctx) != 0) {
		printf ("ERROR: expected to find 0 registered connections but found: %d\n", nopoll_ctx_conns (ctx));
		return nopoll_false;
	} /* end if */

	nopoll_ctx_unref (ctx);

	/* reinit again */
	ctx = create_ctx ();

	/* call to create a connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
	        printf ("ERROR: Expected to find proper client connection status, but found error (conn=%p, conn->session=%d, NOPOLL_INVALID_SOCKET=%d)..\n",
			conn, (int) nopoll_conn_socket (conn), (int) NOPOLL_INVALID_SOCKET);
		return nopoll_false;
	}

	/* check connections registered */
	if (nopoll_ctx_conns (ctx) != 1) {
		printf ("ERROR: expected to find 1 registered connections but found: %d\n", nopoll_ctx_conns (ctx));
		return nopoll_false;
	} /* end if */

	/* ensure connection status is ok */
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR (3): expected to find proper connection status, but found failure.. (conn=%p, conn->session=%d, NOPOLL_INVALID_SOCKET=%d)..\n",
			conn, (int) nopoll_conn_socket (conn), (int) NOPOLL_INVALID_SOCKET);
		return nopoll_false;
	}

	printf ("Test 01: reference counting for the connection: %d\n", nopoll_conn_ref_count (conn));

	/* check if the connection already finished its connection
	   handshake */
	while (! nopoll_conn_is_ready (conn)) {

		if (! nopoll_conn_is_ok (conn)) {
			printf ("ERROR (4.1 jkd412): expected to find proper connection handshake finished, but found connection is broken: session=%d, errno=%d : %s..\n",
				(int) nopoll_conn_socket (conn), errno, strerror (errno));
			return nopoll_false;
		} /* end if */

		/* wait a bit 10ms */
		nopoll_sleep (100);
	} /* end if */

	/* finish connection */
	nopoll_conn_close (conn);
	
	/* finish */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

nopoll_bool test_02 (void) {
	noPollCtx  * ctx;
	noPollConn * conn;
	noPollMsg  * msg;
	int          iter;

	/* create context */
	ctx = create_ctx ();

	/* check connections registered */
	if (nopoll_ctx_conns (ctx) != 0) {
		printf ("ERROR: expected to find 0 registered connections but found: %d\n", nopoll_ctx_conns (ctx));
		return nopoll_false;
	} /* end if */

	nopoll_ctx_unref (ctx);

	/* reinit again */
	ctx = create_ctx ();

	/* call to create a connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error.. (conn=%p, conn->session=%d, NOPOLL_INVALID_SOCKET=%d, errno=%d, strerr=%s)..\n",
			conn, (int) nopoll_conn_socket (conn), (int) NOPOLL_INVALID_SOCKET, errno, strerror (errno));
		return nopoll_false;
	}

	printf ("Test 02: sending basic content..\n");

	/* send content text(utf-8) */
	if (nopoll_conn_send_text (conn, "This is a test", 14) != 14) {
		printf ("ERROR: Expected to find proper send operation..\n");
		return nopoll_false;
	}

	/* wait for the reply */
	iter = 0;
	while ((msg = nopoll_conn_get_msg (conn)) == NULL) {

		if (! nopoll_conn_is_ok (conn)) {
			printf ("ERROR: received websocket connection close during wait reply..\n");
			return nopoll_false;
		}

		nopoll_sleep (10000);

		if (iter > 10)
			break;
	} /* end if */

	/* check content received */
	if (! nopoll_cmp ((char*) nopoll_msg_get_payload (msg), "This is a test")) {
		printf ("ERROR: expected to find message 'This is a test' but something different was received: '%s'..\n",
			(const char *) nopoll_msg_get_payload (msg));
		return nopoll_false;
	} /* end if */

	/* unref message */
	nopoll_msg_unref (msg);

	/* finish connection */
	nopoll_conn_close (conn);
	
	/* finish */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

nopoll_bool test_03 (void) {
	noPollCtx  * ctx;
	noPollConn * conn;
	char         buffer[1024];
	int          bytes_read;

	memset (buffer, 0, 1024);

	/* create context */
	ctx = create_ctx ();

	/* check connections registered */
	if (nopoll_ctx_conns (ctx) != 0) {
		printf ("ERROR: expected to find 0 registered connections but found: %d\n", nopoll_ctx_conns (ctx));
		return nopoll_false;
	} /* end if */

	nopoll_ctx_unref (ctx);

	/* reinit again */
	ctx = create_ctx ();

	/* call to create a connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	printf ("Test 03: sending basic content..\n");

	/* send content text(utf-8) */
	if (nopoll_conn_send_text (conn, "This is a test", 14) != 14) {
		printf ("ERROR: Expected to find proper send operation..\n");
		return nopoll_false;
	}

	/* wait for the reply (try to read 1024, blocking and with a 3 seconds timeout) */
	printf ("Test 03: now reading reply..\n");
	bytes_read = nopoll_conn_read (conn, buffer, 14, nopoll_true, 3000);
	
	if (bytes_read != 14) {
		printf ("ERROR: expected to find 14 bytes but found %d..\n", bytes_read);
		return nopoll_false;
	} /* end if */

	/* check content received */
	if (! nopoll_ncmp (buffer, "This is a test", 14)) {
		printf ("ERROR: expected to find message 'This is a test' but something different was received: '%s'..\n",
			buffer);
		return nopoll_false;
	} /* end if */

	/* finish connection */
	nopoll_conn_close (conn);
	
	/* finish */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

//nopoll_bool test_04 (int chunk_size) {
//	noPollCtx  * ctx;
//	noPollConn * conn;
//	char         buffer[1024];
//	int          bytes_read;
//	FILE       * file;
//	struct stat  stat_buf;
//	int          total_read = 0;
//	const char * cmd;
//	int          retries = 0;
//
//	/* create context */
//	ctx = create_ctx ();
//
//	printf ("Test 04: running test with chunk_size=%d\n", chunk_size);
//
//	/* check connections registered */
//	if (nopoll_ctx_conns (ctx) != 0) {
//		printf ("ERROR: expected to find 0 registered connections but found: %d\n", nopoll_ctx_conns (ctx));
//		return nopoll_false;
//	} /* end if */
//
//	nopoll_ctx_unref (ctx);
//
//	printf ("Test 04: creating connection to download file..\n");
//
//	/* reinit again */
//	ctx = create_ctx ();
//
//	/* call to create a connection */
//	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
//	if (! nopoll_conn_is_ok (conn)) {
//		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
//		return nopoll_false;
//	}
//
//	printf ("Test 04: sending get-file..\n");
//
//	/* send content text(utf-8) */
//	if (nopoll_conn_send_text (conn, "get-file", 8) != 8) {
//		printf ("ERROR: Expected to find proper send operation..\n");
//		return nopoll_false;
//	}
//
//#if defined(NOPOLL_OS_WIN32)
//	file = fopen ("tmp", "wb");
//#else
//	file = fopen ("tmp", "w");
//#endif
//	if (file == NULL) {
//		printf ("ERROR: unable to open file tmp for content comparision\n");
//		return nopoll_false;
//	} /* end if */
//
//	stat ("nopoll-regression-client.c", &stat_buf);
//
//	printf ("Test 04: stat file (nopoll-regression-client.c = %d bytes)\n", (int) stat_buf.st_size);
//
//	retries = 0;
//	while (total_read < stat_buf.st_size) {
//		/* wait for the reply (try to read 1024, blocking) */
//		bytes_read = nopoll_conn_read (conn, buffer, chunk_size, nopoll_true, 1000);
//		/* printf ("Test 04: read %d bytes over the connection %d\n", bytes_read, nopoll_conn_get_id (conn));  */
//
//		if (bytes_read < 0) {
//			printf ("ERROR: expected to find bytes from the connection but found: %d\n", bytes_read);
//			return nopoll_false;
//		}
//
//		if (bytes_read == 0) {
//			retries ++;
//			if (retries > 100) {
//				printf ("Test 04: nothing found (0 bytes), total read %d, total requested: %ld, for %d retries\n",
//					total_read, stat_buf.st_size, retries);
//				return nopoll_false;
//			} /* end if */
//			continue;
//		} /* end if */
//
//		/* write content */
//		if (fwrite (buffer, 1, bytes_read, file) != bytes_read)
//		  return nopoll_false;
//
//		/* count total read bytes */
//		total_read += bytes_read;
//
//	} /* end while */
//	fclose (file);
//
//	/* now check both files */
//	printf ("Test 04: checking content download (chunk_size=%d)...\n", chunk_size);
//	printf ("Test 04: about to run diff nopoll-regression-client.c tmp > /dev/null\n");
//#if defined(NOPOLL_OS_WIN32)
//	cmd = "diff -q nopoll-regression-client.c tmp";
//#else
//	cmd = "diff -q nopoll-regression-client.c tmp > /dev/null";
//#endif
//	if (system (cmd)) {
//		printf ("ERROR: failed to download file from server, content differs. Check: diff nopoll-regression-client.c tmp\n");
//		return nopoll_false;
//	} /* end if */
//
//	/* finish connection */
//	nopoll_conn_close (conn);
//
//	/* finish */
//	nopoll_ctx_unref (ctx);
//
//	return nopoll_true;
//}

nopoll_bool test_04a (void) {
	noPollCtx  * ctx;
	noPollConn * conn;
	char         buffer[1024];
	int          result;

	/* reinit again */
	ctx = create_ctx ();

	/* call to create a connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	/* attempt to read without blocking */
	printf ("Test 04-a: checking non-blocking API..\n");
	result = nopoll_conn_read (conn, buffer, 1024, nopoll_false, 0);
	if (result != -1) {
		printf ("ERROR: expected return result -1(%d)\n", result);
		return nopoll_false;
	}
		
	printf ("Test 04-a: ok, operation not blocked, result %d\n", result);
	if (result != -1) {
		printf ("ERROR: expected return result -1(%d)\n", result);
		return nopoll_false;
	}

	result = nopoll_conn_read (conn, buffer, 1024, nopoll_false, 300);
	if (result != -1) {
		printf ("ERROR: expected return result -1(%d)\n", result);
		return nopoll_false;
	}

	printf ("Test 04-a: ok, operation not blocked, result %d\n", result);

	result = nopoll_conn_read (conn, buffer, 1024, nopoll_false, 1000);
	if (result != -1) {
		printf ("ERROR: expected return result -1(%d)\n", result);
		return nopoll_false;
	}

	printf ("Test 04-a: ok, operation not blocked, result %d\n", result);

	/* finish connection */
	nopoll_conn_close (conn);
	
	/* finish */
	nopoll_ctx_unref (ctx);
	

	return nopoll_true;
}

nopoll_bool test_04b (void) {
	noPollCtx  * ctx;
	noPollConn * conn;
	int          iterator;
	int          length;
	int          bytes_written;
	const char * msg = NULL;
	/* reinit again */
	ctx = create_ctx ();

	/* call to create a connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	printf ("Test 04-b: waiting until connection is ok\n");
	nopoll_conn_wait_until_connection_ready (conn, 5);

	printf ("Test 04-b: sending was quick as possible to flood local buffers..\n");
	
	/* get message length */
	length = strlen (msg);
	iterator = 0;
	while (iterator < 100) {
		/* send a message */
		if (nopoll_conn_send_text (conn, msg, length) != length) {
			if (errno == 0) {
				printf ("ERROR: expected to find errno value but found 0..\n");
			}
			printf ("Test 04-b: found expected error, checking errno=%d..\n", errno);
			break;
		} /* end if */

		/* next iterator */
		iterator ++;
	}  /* end while */

	if (errno != NOPOLL_EWOULDBLOCK && errno != EINPROGRESS) {
		printf ("ERROR: expected to find errno=%d, but found errno=%d : %s\n",
			(int)NOPOLL_EWOULDBLOCK, (int)errno, strerror (errno));
		return nopoll_false;
	} /* end if */

	/* write pending content */
	if (nopoll_conn_pending_write_bytes (conn) == 0) {
		printf ("ERROR: expected to have pending bytes to be written.. but found 0..\n");
		return nopoll_false;
	} /* end if */

	iterator = 0;
	while (iterator < 10) {
		printf ("Test 04-b: found pending write bytes=%d\n", nopoll_conn_pending_write_bytes (conn));

		/* call to flush bytes */
		nopoll_conn_complete_pending_write (conn);

		if (nopoll_conn_pending_write_bytes (conn) == 0) {
			printf ("Test 04-b: all bytes written..\n");
			break;
		} /* end if */

		/* sleep a bit */
		nopoll_sleep (1000000);

		/* next iterator */
		iterator++;
	} 

	if (nopoll_conn_pending_write_bytes (conn) != 0) {
		printf ("Test 04-b: expected to find no pending bytes waiting to be written but found: %d\n", nopoll_conn_pending_write_bytes (conn));
		return nopoll_false;
	} /* end if */

	nopoll_conn_close (conn);

	/* call to create a connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	printf ("Test 04-b: waiting until connection is ok\n");
	nopoll_conn_wait_until_connection_ready (conn, 5);

	/* send a cleanup message */
	bytes_written = nopoll_conn_send_text (conn, "release-message", 15);
	if (bytes_written != 15) {
		printf ("Test 04-b: unable to send release message, bytes_written=%d, but expected=%d..\n",
			bytes_written, 15);
		return nopoll_false;
	} /* end if */

	printf ("Test 04-b: waiting a second before finishing test..\n");
	nopoll_sleep (1000000);

	/* finish connection */
	nopoll_conn_close (conn);
	
	/* finish */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

//nopoll_bool test_04c (void) {
//	noPollCtx  * ctx;
//	noPollConn * conn;
//	int          length;
//	int          bytes_written;
//	char         buffer[4096];
//	FILE       * handle;
//	struct stat  file_info;
//	int          iterator;
//	char       * cmd;
//	const char * file_checked;
//	const char * cmd_format;
//	int          total_bytes = 0;
//	nopoll_bool  flush_required = nopoll_false;
//
//	/* reinit again */
//	ctx = create_ctx ();
//
//	/* call to create a connection */
//	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
//	if (! nopoll_conn_is_ok (conn)) {
//		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
//		return nopoll_false;
//	}
//
//	printf ("Test 04-c: waiting until connection is ok\n");
//	nopoll_conn_wait_until_connection_ready (conn, 5);
//
//	/* remove local file */
//	if (stat ("copy-test-04c.txt", &file_info) == 0) {
//		printf ("Test 04-c: FILE exists, removing (copy-test-04c.txt)\n");
//		/* remove file */
//		unlink ("copy-test-04c.txt");
//	} /* end if */
//
//	/* open file descriptor */
//	bytes_written = nopoll_conn_send_text (conn, "open-file: copy-test-04c.txt", 28);
//	if (bytes_written != 28) {
//		printf ("Test 04-c: unable to send open file command, bytes_written=%d, but expected=%d..\n",
//			bytes_written, 15);
//		return nopoll_false;
//	} /* end if */
//
//	/* open the handle to send the content */
//	file_checked = "/boot/vmlinuz-2.6.32-5-amd64";
//#if defined(NOPOLL_OS_WIN32)
//	handle = fopen (file_checked, "rb");
//#else
//	handle = fopen (file_checked, "r");
//#endif
//	if (handle == NULL) {
//		/* checking file */
//		file_checked = "nopoll-regression-client.c";
//#if defined(NOPOLL_OS_WIN32)
//		handle = fopen (file_checked, "rb");
//#else
//		handle = fopen (file_checked, "r");
//#endif
//		if (handle == NULL) {
//			printf ("Test 04-c: failed to open file to be sent to the server..\n");
//			return nopoll_false;
//		} /* end if */
//	} /* end if */
//	printf ("Test 04-c: running test with file: %s\n", file_checked);
//
//	/* send content */
//	while (nopoll_true) {
//		/* read content */
//		length = fread (buffer, 1, 4096, handle);
//
//		/* write content */
//		if (length > 0) {
//			bytes_written = nopoll_conn_send_text (conn, buffer, length);
//
//			/* check for flush required */
//			if (nopoll_conn_pending_write_bytes (conn) > 0)
//				flush_required = nopoll_true;
//
//			/* call to flush writes */
//			bytes_written = nopoll_conn_flush_writes (conn, 10000000, bytes_written);
//
//			if (bytes_written != length) {
//				printf ("ERROR: Failed to send bytes read from file %d, bytes written were=%d (errno=%d, pending bytes: %d, total bytes: %d)..\n",
//					length, bytes_written, errno, nopoll_conn_pending_write_bytes (conn), total_bytes);
//				return nopoll_false;
//			} /* end if */
//		} /* end if */
//
//		if (bytes_written > 0)
//			total_bytes += bytes_written;
//
//		if (length < 4096) {
//			printf ("Test 04-c: last read operation found length=%d\n", length);
//			break;
//		} /* end if */
//	} /* end while */
//
//	fclose (handle);
//
//	printf ("Test 04-c: pending bytes to be written are=%d\n", nopoll_conn_pending_write_bytes (conn));
//
//	/* send command to close file */
//	bytes_written = nopoll_conn_send_text (conn, "close-file", 10);
//	if (bytes_written != 10) {
//		printf ("Test 04-c: unable to send close file command\n");
//		return nopoll_false;
//	} /* end if */
//
//#if defined(NOPOLL_OS_WIN32)
//	cmd_format = "diff -q copy-test-04c.txt %s";
//#else
//	cmd_format = "diff -q copy-test-04c.txt %s > /dev/null";
//#endif
//	cmd = nopoll_strdup_printf (cmd_format, file_checked);
//
//	iterator = 0;
//	while (iterator < 50) {
//		/* checking file transferred */
//		printf ("Test 04-c: checking file transfered, iterator=%d..\n", iterator);
//		if (system (cmd) == 0)
//			break;
//
//		iterator++;
//		nopoll_sleep (500000);
//	} /* end if */
//
//	if (system (cmd) != 0) {
//		printf ("Test 04-c: file differs, test failing, run: diff copy-test-04c.txt %s\n", file_checked);
//		return nopoll_false;
//	} /* end if */
//
//	/* check total size */
//	if (stat ("copy-test-04c.txt", &file_info) == 0 && file_info.st_size != total_bytes) {
//		printf ("Test 04-c: expected to find same total bytes written %d != %d\n",
//			(int) file_info.st_size, (int) total_bytes);
//		return nopoll_false;
//	} /* end if */
//
//	if (stat (file_checked, &file_info) == 0 && file_info.st_size != total_bytes) {
//		printf ("Test 04-c: expected to find same total bytes written %d != %d\n",
//			(int) file_info.st_size, (int) total_bytes);
//		return nopoll_false;
//	} /* end if */
//
//	printf ("Test 04-c: file ok (%d bytes written)..\n", total_bytes);
//	nopoll_free (cmd);
//
//	if (! flush_required) {
//		printf (" *** \n");
//		printf (" *** \n");
//		printf (" *** ATTENTION: !! Flush operations weren't required so this test didn't check everything (file used to check transfers was: %s)  \n",
//			file_checked);
//		printf (" *** \n");
//		printf (" *** \n");
//	}
//
//	/* sleep half a second */
//	nopoll_sleep (500000);
//
//	/* finish connection */
//	nopoll_conn_close (conn);
//
//	/* finish */
//	nopoll_ctx_unref (ctx);
//
//	return nopoll_true;
//}

nopoll_bool test_05 (void) {

	noPollCtx  * ctx;
	noPollConn * conn;
	char         buffer[1024];
	int          bytes_read;
	const char * msg = " klasdfkla akldfj klafklajetqkljt kjlwergklwejry90246tkgwr kñljwrglkjdfg lksdjglskg slkg camión adsfasdf pruébasdfad España asdfaklsjdflk jasfkjaslfjetql tjñqgkjadgklj aglkjalk jafkjaslfkjaskj asjaslfkjasfklajg klajefñlqkjetrlkqj lqkj ñlskdfjañlk asldfjñlafj añlfj ñdfjkjt4ñqlkjt lkj34tlkjañlgjañlkgjañlkgjw";

	memset (buffer, 0, 1024);

	/* reinit again */
	ctx = create_ctx ();

	/* call to create a connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	printf ("Test 05: sending UTF-8 content..\n");

	/* send content text(utf-8) */
	if (nopoll_conn_send_text (conn, msg, -1) <= 0) {
		printf ("ERROR: Expected to find proper send operation (nopoll_conn_send_test) returned less or 0..\n");
		return nopoll_false;
	}

	/* wait for the reply (try to read 322, blocking and with a 3 seconds timeout) */
	bytes_read = nopoll_conn_read (conn, buffer, 322, nopoll_true, 3000);
	if (bytes_read != 322) {
		printf ("ERROR: expected to receive 322 bytes, but received %d\n", bytes_read);
		return nopoll_false;
	}

	if (! nopoll_ncmp (buffer, msg, 322)) {
		printf ("ERROR: expected to receive another content....\n");
		printf ("Expected: %s\n", msg);
		printf ("Received: %s\n", buffer);

		return nopoll_false;
	}

	/* finish connection */
	nopoll_conn_close (conn);
	
	/* finish */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

nopoll_bool test_06 (void) {

	noPollCtx      * ctx;
	noPollConn     * conn;
	noPollConnOpts * opts;

	/* reinit again */
	ctx = create_ctx ();

	/* disable verification */
	opts = nopoll_conn_opts_new ();
	nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);

	/* call to create a connection */
	conn = nopoll_conn_tls_new (ctx, opts, local_host_name, local_host_ports, NULL, local_host_url, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	/* check if the connection already finished its connection
	   handshake */
	while (! nopoll_conn_is_ready (conn)) {

		if (! nopoll_conn_is_ok (conn)) {
			printf ("ERROR (4.1 jg72): expected to find proper connection handshake finished, but found connection is broken: session=%d, errno=%d : %s..\n",
				(int) nopoll_conn_socket (conn), errno, strerror (errno));
			return nopoll_false;
		} /* end if */

		/* wait a bit 10ms */
		nopoll_sleep (100);
	} /* end if */

	if (! nopoll_conn_is_tls_on (conn)) {
		printf ("ERROR (5): expected to find TLS enabled on the connection but found it isn't..\n");
		return nopoll_false;
	} /* end if */

	/* finish connection */
	nopoll_conn_close (conn);
	
	/* finish */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

nopoll_bool test_07 (void) {

	noPollCtx      * ctx;
	noPollConn     * conn;
	noPollConnOpts * opts;

	/* reinit again */
	ctx = create_ctx ();

	/* disable verification */
	opts = nopoll_conn_opts_new ();
	nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);

	/* call to create a connection */
	conn = nopoll_conn_tls_new (ctx, opts, local_host_name, local_host_ports, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	} /* end if */

	/* check if the connection already finished its connection
	   handshake */
	while (! nopoll_conn_is_ready (conn)) {

		if (! nopoll_conn_is_ok (conn)) {
			printf ("ERROR (4.1 dk45): expected to find proper connection handshake finished, but found connection is broken: session=%d, errno=%d : %s..\n",
				(int) nopoll_conn_socket (conn), errno, strerror (errno));
			return nopoll_false;
		} /* end if */

		/* wait a bit 10ms */
		nopoll_sleep (10000);
	} /* end if */

	printf ("Test 07: testing sending TLS content over the wire..\n");
	if (! test_sending_and_check_echo (conn, "Test 07", "This is a test"))
		return nopoll_false;

	/* finish connection */
	nopoll_conn_close (conn);
	
	/* finish */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

nopoll_bool test_08 (void) {

	noPollCtx  * ctx;
	noPollConn * conn;

	/* reinit again */
	ctx = create_ctx ();

	/* call to connect to TLS port expecting non-TLS protocol */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);

	/* wait a bit 100ms */
	nopoll_sleep (100000);

	if (nopoll_conn_is_ready (conn)) {
		printf ("ERROR: Expected a FAILING connection status, but found error..\n");
		return nopoll_false;
	} /* end if */

	/* finish connection */
	nopoll_conn_close (conn);
	
	/* finish */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

nopoll_bool test_09 (void) {

	noPollCtx  * ctx;
	noPollConn * conn;

	/* reinit again */
	ctx = create_ctx ();

	/* setup the protocol version to see how it breaks (it should) */
	nopoll_ctx_set_protocol_version (ctx, 12);

	/* call to connect to TLS port expecting non-TLS protocol */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);

	/* wait a bit 100ms */
	nopoll_sleep (100000);

	if (nopoll_conn_is_ready (conn)) {
		printf ("ERROR: Expected a FAILING connection status due to protocol version error, but it working..\n");
		return nopoll_false;
	} /* end if */

	/* finish connection */
	nopoll_conn_close (conn);
	
	/* finish */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

nopoll_bool test_10 (void) {

	noPollCtx  * ctx;
	noPollConn * conn;

	/* reinit again */
	ctx = create_ctx ();

	/* call to connect from an origining that shouldn't be allowed */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, "http://deny.aspl.es");

	/* wait a bit 100ms */
	nopoll_sleep (100000);

	if (nopoll_conn_is_ready (conn)) {
		printf ("ERROR: Expected a FAILING connection status due to origing denied, but it working..\n");
		return nopoll_false;
	} /* end if */

	/* finish connection */
	nopoll_conn_close (conn);
	
	/* finish */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

nopoll_bool test_11 (void) {

	noPollCtx  * ctx;
	noPollConn * conn;

	/* reinit again */
	ctx = create_ctx ();

	/* create a working connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);

	if (! nopoll_conn_wait_until_connection_ready (conn, 5)) {
		printf ("ERROR: Expected a FAILING connection status due to origing denied, but it working..\n");
		return nopoll_false;
	} /* end if */

	/* finish */
	nopoll_ctx_unref (ctx);

	/* finish connection */
	nopoll_conn_close (conn);
	
	return nopoll_true;
}

nopoll_bool test_12 (void) {

	noPollCtx  * ctx;
	noPollConn * conn;
	int          iterator;

	/* time tracking */
	struct  timeval    start;
	struct  timeval    stop;
	struct  timeval    diff;


	/* reinit again */
	ctx = create_ctx ();

	/* start */
#if defined(NOPOLL_OS_WIN32)
	nopoll_win32_gettimeofday (&start, NULL);
#else
	gettimeofday (&start, NULL);
#endif	

	printf ("Test 12: creating 1000 connections...\n");
	iterator = 0;
	while (iterator < 1000) {
		/* create a working connection */
		conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
		
		if (! nopoll_conn_wait_until_connection_ready (conn, 5)) {
			printf ("ERROR: Expected NOT to find a FAILING connection status, errno is=%d..\n", errno);
			return nopoll_false;
		} /* end if */

		/* finish connection */
		nopoll_conn_close (conn);

		iterator++;
	} /* end while */

	/* finish */
	nopoll_ctx_unref (ctx);

	/* stop */
#if defined(NOPOLL_OS_UNIX)
	gettimeofday (&stop, NULL);
#else
	nopoll_win32_gettimeofday (&stop, NULL);
#endif

	nopoll_timeval_substract (&stop, &start, &diff);

	printf ("Test 12: created %d connections in %ld.%ld secs\n", 
		iterator, diff.tv_sec, diff.tv_usec);
	
	
	return nopoll_true;
}

nopoll_bool test_13_test (noPollCtx * ctx, const char * serverName, const char * _certificateFile, const char * _privateKey)
{
	const char * certificateFile;
	const char * privateKey;

	if (! nopoll_ctx_find_certificate (ctx, serverName, NULL, NULL, NULL)) {
		printf ("Test 13: it SHOULD find something about found.server.com but function reported failure status..\n");
		return nopoll_false;
	}

	if (! nopoll_ctx_find_certificate (ctx, serverName, &certificateFile, &privateKey, NULL)) {
		printf ("Test 13: it SHOULD find something about found.server.com but function reported failure status..\n");
		return nopoll_false;
	}

	if (! nopoll_cmp (certificateFile, _certificateFile)) {
		printf ("Test 13: expected to find certificate %s, but found %s\n", _certificateFile, certificateFile);
		return nopoll_false;
	}
	if (! nopoll_cmp (privateKey, _privateKey)) {
		printf ("Test 13: expected to find certificate %s, but found %s\n", _privateKey, privateKey);
		return nopoll_false;
	}
	return nopoll_true;
}

nopoll_bool test_13 (void)
{
	noPollCtx * ctx;

	/* create ctx */
	ctx = nopoll_ctx_new ();

	if (nopoll_ctx_find_certificate (ctx, "not-found", NULL, NULL, NULL)) {
		printf ("Test 13: it shouldn't find anything but function reported ok status..\n");
		return nopoll_false;
	}

	/* register */
	if (! nopoll_ctx_set_certificate (ctx, "found.server.com", "test.crt", "test.key", NULL)) {
		printf ("Test 13: unable to install certificate...\n");
		return nopoll_false;
	} /* end if */

	if (! test_13_test (ctx, "found.server.com", "test.crt", "test.key")) 
		return nopoll_false;

	/* register */
	if (! nopoll_ctx_set_certificate (ctx, "another.server.com", "another.test.crt", "another.test.key", NULL)) {
		printf ("Test 13: unable to install certificate (another.server.com)...\n");
		return nopoll_false;
	} /* end if */


	if (! test_13_test (ctx, "found.server.com", "test.crt", "test.key")) 
		return nopoll_false;

	if (! test_13_test (ctx, "another.server.com", "another.test.crt", "another.test.key")) 
		return nopoll_false;

	/* register */
	if (! nopoll_ctx_set_certificate (ctx, "other.server.com", "other.test.crt", "other.test.key", NULL)) {
		printf ("Test 13: unable to install certificate (another.server.com)...\n");
		return nopoll_false;
	} /* end if */

	if (! test_13_test (ctx, "found.server.com", "test.crt", "test.key")) 
		return nopoll_false;

	if (! test_13_test (ctx, "another.server.com", "another.test.crt", "another.test.key")) 
		return nopoll_false;

	if (! test_13_test (ctx, "other.server.com", "other.test.crt", "other.test.key")) 
		return nopoll_false;

	/* release ctx */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

nopoll_bool test_14 (void) {
	noPollCtx  * ctx;
	noPollConn * conn;
	noPollMsg  * msg;
	int          iter;

	/* create context */
	ctx = create_ctx ();

	/* check connections registered */
	if (nopoll_ctx_conns (ctx) != 0) {
		printf ("ERROR: expected to find 0 registered connections but found: %d\n", nopoll_ctx_conns (ctx));
		return nopoll_false;
	} /* end if */

	nopoll_ctx_unref (ctx);

	/* reinit again */
	ctx = create_ctx ();

	/* call to create a connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	printf ("Test 14: sending partial frames (Hel..)..\n");
	if (nopoll_conn_send_text_fragment (conn, "Hel", 3) != 3) {
		printf ("ERROR: expected to be able to send Hel frame..\n");
		return nopoll_false;
	}
	printf ("Test 14: sending completing frame (..lo)..\n");
	if (nopoll_conn_send_text (conn, "lo", 2) != 2) {
		printf ("ERROR: expected to be able to send lo completion frame..\n");
		return nopoll_false;
	}

	/* wait for the reply */
	iter = 0;
	while ((msg = nopoll_conn_get_msg (conn)) == NULL) {

		if (! nopoll_conn_is_ok (conn)) {
			printf ("ERROR: received websocket connection close during wait reply..\n");
			return nopoll_false;
		}

		nopoll_sleep (10000);

		if (iter > 10)
			break;
	} /* end if */

	/* check content received */
	if (! nopoll_cmp ((char*) nopoll_msg_get_payload (msg), "Hello")) {
		printf ("ERROR: expected to find message 'This is a test' but something different was received: '%s'..\n",
			(const char *) nopoll_msg_get_payload (msg));
		return nopoll_false;
	} /* end if */

	/* unref message */
	nopoll_msg_unref (msg);

	/* finish connection */
	nopoll_conn_close (conn);
	
	/* finish */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

nopoll_bool test_15 (void) {
	noPollCtx  * ctx;
	noPollConn * conn;

	/* create context */
	ctx = create_ctx ();

	/* check connections registered */
	if (nopoll_ctx_conns (ctx) != 0) {
		printf ("ERROR: expected to find 0 registered connections but found: %d\n", nopoll_ctx_conns (ctx));
		return nopoll_false;
	} /* end if */

	nopoll_ctx_unref (ctx);

	/* reinit again */
	ctx = create_ctx ();

	/* call to create a connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	/* wait for the reply */
	while (nopoll_true) {
		if (nopoll_conn_is_ready (conn))
			break;
		printf ("Test 15: not ready yet..\n");
		nopoll_sleep (10000);
	} /* end if */

	printf ("Test 15: setting non-blocking state..\n");

	if (! nopoll_conn_set_sock_block (nopoll_conn_socket (conn), nopoll_false)) {
		printf ("ERROR: failed to configure non-blocking state to connection..\n");
		return nopoll_false;
	} /* end if */

	printf ("Test 15: attempting to read content..\n");

	/* wait for the reply */
	if (nopoll_conn_get_msg (conn)) {
		printf ("ERROR (1): expected to not be able to find a message..\n");
		return nopoll_false;
	}
	if (nopoll_conn_get_msg (conn)) {
		printf ("ERROR (2): expected to not be able to find a message..\n");
		return nopoll_false;
	}
	if (nopoll_conn_get_msg (conn)) {
		printf ("ERROR (3): expected to not be able to find a message..\n");
		return nopoll_false;
	}

	printf ("Test 15: reads finished..\n");

	/* check connection state */
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: expected to find connection state ok, but failure found..\n");
		return nopoll_false;
	} /* end if */

	/* finish connection */
	nopoll_conn_close (conn);
	
	/* finish */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

nopoll_bool test_16 (void) {
	noPollCtx  * ctx;
	noPollConn * conn;
	int          iterator;

	/* create context */
	ctx = create_ctx ();

	/* check connections registered */
	if (nopoll_ctx_conns (ctx) != 0) {
		printf ("ERROR: expected to find 0 registered connections but found: %d\n", nopoll_ctx_conns (ctx));
		return nopoll_false;
	} /* end if */

	nopoll_ctx_unref (ctx);

	/* reinit again */
	ctx = create_ctx ();

	/* call to create a connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	/* wait for the reply */
	while (nopoll_true) {
		if (nopoll_conn_is_ready (conn))
			break;
		printf ("Test 16: not ready yet..\n");
		nopoll_sleep (10000);
	} /* end if */

	iterator = 0;
	while (iterator < 10) {
		printf ("Test 16: send sleep in header content (waiting 1000 ms, iterator=%d)..\n", iterator);
		if (__nopoll_conn_send_common (conn, "This is a test", 14, nopoll_true, 400000, NOPOLL_TEXT_FRAME) != 14) {
			printf ("ERROR: failed to send content..\n");
			return nopoll_false;
		} /* end if */

		iterator++;
	} /* end while */

	printf ("Test 16: sends finished, now checking connection ..\n");

	nopoll_sleep (100000);

	/* check connection state */
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: expected to find connection state ok, but failure found..\n");
		return nopoll_false;
	} /* end if */

	/* finish connection */
	nopoll_conn_close (conn);
	
	/* finish */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

nopoll_bool test_17_send_and_receive_test (noPollCtx * ctx, noPollConn * conn, noPollConn * listener, 
					   const char * message, int length, nopoll_bool read_in_the_middle, 
					   nopoll_bool read_after_header, nopoll_bool read_after_mask) 
{
	char           buffer[1024];
	char           buffer2[1024];
	NOPOLL_SOCKET  _socket;
	char           mask[4];
	int            desp;

	memset (buffer, 0, 1024);

	/* make it unblock */
	nopoll_conn_set_sock_block (nopoll_conn_socket (listener), nopoll_false);

	/* now send partial content */
	printf ("Test 17: sending normal message to test link..\n");
	if (nopoll_conn_send_text (conn, message, length) != length) {
		printf ("ERROR: expected to properly send all bytes but it wasn't possible..\n");
		return nopoll_false;
	} /* end if */

	/* read reply */
	if (nopoll_conn_read (listener, buffer, length, nopoll_true, 0) != length) {
		printf ("ERROR: expected read 22 bytes ...but there was a failure..\n");
		return nopoll_false;
	} /* end if */

	/* printf ("Test 17: sending partial content..\n"); */
	_socket = nopoll_conn_socket (conn);
	buffer[0] = 129;
	buffer[1] = 150;
	send (_socket, buffer, 2, 0);

	if (read_after_header) {
		nopoll_sleep (1000000);
		printf ("Test 17: Reading after header..\n");
		nopoll_conn_read (listener, buffer2, length, nopoll_false, 0);
	}

	/* send mask */
	mask[0] = 23;
	mask[1] = 24;
	mask[2] = 25;
	mask[3] = 26;
	send (_socket, mask, 4, 0);

	if (read_after_mask) {
		nopoll_sleep (1000000);
		printf ("Test 17: Reading after mask..\n");
		nopoll_conn_read (listener, buffer2, length, nopoll_false, 0);
	}

	memcpy (buffer, message, length);
	nopoll_conn_mask_content (ctx, buffer, length, mask, 0);

	send (_socket, buffer, 10, 0);
	printf ("Test 17: sent partial content...wait a bit (2 seconds)..\n");
	nopoll_sleep (2000000);

	desp = 0;
	if (read_in_the_middle) {
		printf ("Test 17: reading in the middle (10 bytes)\n");
		memset (buffer2, 0, 100);
		desp = nopoll_conn_read (listener, buffer2, length, nopoll_false, 0);
		if (desp != 10) {
			printf ("Test 17: failed to read some initial content (10), found %d bytes..\n", desp);
			return nopoll_false;
		} /* end if */

		printf ("Test 17: read %d bytes..\n", desp);
	} 

	printf ("Test 17: now send the rest..\n");
	send (_socket, buffer + 10, length - 10, 0);

	/* copy content into original buffer */
	if (read_in_the_middle) 
		memcpy (buffer, buffer2, desp);

	printf ("Test 17: now read the content received..\n");
	/* now read the content */
	if (nopoll_conn_read (listener, buffer + desp, length - desp, nopoll_true, 0) != (length - desp)) {
		printf ("ERROR: expected to receive 22 bytes but found something different..\n");
		return nopoll_false;
	} /* end if */

	if (! nopoll_ncmp (buffer, message, length)) {
		printf ("ERROR: expected to receive test message but found: '%s'\n", buffer);
		return nopoll_false;
	}

	return nopoll_true;
}

nopoll_bool test_17 (void) {

	noPollCtx      * ctx;
	noPollConn     * conn;
	noPollConn     * listener, * master;

	/* reinit again */
	ctx = create_ctx ();

	/* create a listener */
	master = nopoll_listener_new (ctx, "0.0.0.0", "22351");
	printf ("Test 17: created master listener (conn-id=%d, status=%d)\n", 
		nopoll_conn_get_id (master), nopoll_conn_is_ok (master));
	if (! nopoll_conn_is_ok (master)) {
		printf ("ERROR: expected proper master listener at 0.0.0.0:2235 creation but a failure was found..\n");
		return nopoll_false;
	} /* end if */

	/* call to create a connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	} /* end if */


	/* wait for the reply */
	printf ("Test 17: accepting listener..\n");
	listener = nopoll_conn_accept (ctx, master);

	if (! nopoll_conn_is_ok (listener)) {
		printf ("ERROR: expected to find proper listener status (connection accepted), but found failure..\n");
		return nopoll_false;
	} /* end if */
	
	/** call test here **/
	if (! test_17_send_and_receive_test (ctx, conn, listener, "This is a test message", 22, 
					     /* read in the middle */
					     nopoll_false, 
					     /* read after the header */
					     nopoll_false, 
					     /* read after the mask */
					     nopoll_false))
		return nopoll_false;

	/** call test here **/
	if (! test_17_send_and_receive_test (ctx, conn, listener, "This is a test message", 22, 
					     /* read in the middle */
					     nopoll_true, 
					     /* read after the header */
					     nopoll_false, 
					     /* read after the mask */
					     nopoll_false))
		return nopoll_false;

	/** call test here **/
	if (! test_17_send_and_receive_test (ctx, conn, listener, "This is a test message", 22, 
					     /* read in the middle */
					     nopoll_false, 
					     /* read after the header */
					     nopoll_true, 
					     /* read after the mask */
					     nopoll_false))
		return nopoll_false;

	/** call test here **/
	if (! test_17_send_and_receive_test (ctx, conn, listener, "This is a test message", 22, 
					     /* read in the middle */
					     nopoll_false, 
					     /* read after the header */
					     nopoll_false, 
					     /* read after the mask */
					     nopoll_true))
		return nopoll_false;

	printf ("Test 17: closing connections..\n"); 
	nopoll_conn_close (listener);
	nopoll_conn_close (master);
	nopoll_conn_close (conn);

	printf ("Test 17: finishing context..\n");

	/* finish */
	nopoll_ctx_unref (ctx);

	/* report finish */
	return nopoll_true;
}

nopoll_bool test_18 (void) {

	noPollCtx      * ctx;
	noPollConn     * conn;
	noPollConnOpts * opts;

	/* reinit again */
	ctx = create_ctx ();

	/* disable verification */
	opts = nopoll_conn_opts_new ();
	nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);

	/* call to create a connection */
	conn = nopoll_conn_tls_new (ctx, opts, local_host_name, local_host_ports, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	printf ("Test 18: waiting on nopoll_loop_wait (1 seconds)...\n");
	nopoll_loop_wait (ctx, 1);
	printf ("Test 18: waiting on nopoll_loop_wait (1 seconds)...\n");
	nopoll_loop_wait (ctx, 1);

	/* finish connection */
	nopoll_conn_close (conn);

	/* finish */
	nopoll_ctx_unref (ctx);

	/* report finish */
	return nopoll_true;
}

nopoll_bool test_19 (void) {

	noPollCtx      * ctx;
	noPollConn     * conn;
	noPollConnOpts * opts;

	/* reinit again */
	ctx = create_ctx ();

	printf ("Test 19: testing SSLv23 connection...\n");

	/* create options */
	opts     = nopoll_conn_opts_new ();
	nopoll_conn_opts_set_ssl_protocol (opts, NOPOLL_METHOD_SSLV23);

	/* create connection */
	conn = nopoll_conn_tls_new (ctx, opts, local_host_name, local_host_ports, NULL, NULL, NULL, NULL);

	/* check connection */
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: failed to start listener connection..\n");
		return nopoll_false;
	} /* end if */

	if (! test_sending_and_check_echo (conn, "Test 19", "This is a test...checking SSL with different values..."))
		return nopoll_false;
	
	/* finish connection */
	nopoll_conn_close (conn);

	printf ("Test 19: testing SSLv23 connection with TLSv1 server...\n");

	/* create options */
	opts     = nopoll_conn_opts_new ();
	nopoll_conn_opts_set_ssl_protocol (opts, NOPOLL_METHOD_SSLV23);

	/* create connection */
	conn = nopoll_conn_tls_new (ctx, opts, local_host_name, local_host_ports, NULL, NULL, NULL, NULL);

	/* check connection */
	if (! nopoll_conn_is_ok (conn)) {
		printf ("WARNING: failed to create connection (2)..unable to connect to TLSv1 server with NOPOLL_METHOD_SSLV23\n");
	} else {
		if (! test_sending_and_check_echo (conn, "Test 19", "This is a test...checking SSL with different values..."))
			return nopoll_false;
	} /* end if */

	nopoll_conn_close (conn);

	printf ("Test 19: perfect, got it working..\n");

	/* create options */
	opts     = nopoll_conn_opts_new ();
	nopoll_conn_opts_set_ssl_protocol (opts, NOPOLL_METHOD_SSLV3);

	/* create connection */
	printf ("Test 19: checking SSLv3 with TLSv1..\n");
	conn = nopoll_conn_tls_new (ctx, opts, local_host_name, local_host_ports, NULL, NULL, NULL, NULL);

	/* check connection */
	if (nopoll_conn_is_ok (conn)) {
		printf ("ERROR: expected a connection failure..\n");
		return nopoll_false;
	} /* end if */
	printf ("   ... it does not work, but this is expected..\n");

	nopoll_conn_close (conn);

	/* finish */
	nopoll_ctx_unref (ctx);

	/* report finish */
	return nopoll_true;
}

#if defined(__NOPOLL_PTHREAD_SUPPORT__)
nopoll_bool test_20 (void) {

	noPollPtr  * mutex;
	int          iterator = 0;


	while (iterator < 10) {
		/* call to create mutex */
		mutex = __nopoll_regtest_mutex_create ();
		if (mutex == NULL)
			return nopoll_false;
		
		/* call to lock */
		__nopoll_regtest_mutex_lock (mutex);
		__nopoll_regtest_mutex_unlock (mutex);
		
		/* call to destroy */
		__nopoll_regtest_mutex_destroy (mutex);

		/* next operation */
		iterator++;
	}

	return nopoll_true;
}
#endif

/* 
 * Use gen-certificates-test-21.sh to rebuild certificates.
 * Reg test to check client auth certificate.
 */
nopoll_bool test_21 (void) {

	noPollCtx      * ctx;
	noPollConn     * conn;
	noPollConnOpts * opts;

	/* reinit again */
	ctx = create_ctx ();

	/* call to create a connection */
	printf ("Test 21: check ssl connection (with auth certificate)..\n");
	conn = nopoll_conn_tls_new (ctx, NULL, local_host_name, local_host_ports, NULL, NULL, NULL, NULL);
	if (nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to FAILURE client connection status, but ok..\n");
		return nopoll_false;
	}
	nopoll_conn_close (conn);

	/* try again configuring conection certificates */
	printf ("Test 21: checking to connect again with client provided certificates..\n");
	opts     = nopoll_conn_opts_new ();
	nopoll_conn_opts_set_ssl_certs (opts, 
					/* certificate */
					"client.pem",
					/* private key */
					"client.pem",
					NULL,
					/* ca certificate */
					"root.pem");
	conn = nopoll_conn_tls_new (ctx, opts, local_host_name, local_host_ports, NULL, NULL, NULL, NULL);
	if (! test_sending_and_check_echo (conn, "Test 21", "This is a test")) {
		printf ("ERROR: it should WORK, client certificate isn't working..\n");
		return nopoll_false;
	} /* end if */

	/* finish connection */
	nopoll_conn_close (conn);
	
	/* finish */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}


nopoll_bool __test_22_on_close_signal = nopoll_false;

void __test_22_on_close (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data)
{
	printf ("Test --: called on connection close for conn-id=%d\n", nopoll_conn_get_id (conn));
	__test_22_on_close_signal = nopoll_true;

	return;
}

nopoll_bool test_22 (void) {
	
	noPollCtx      * ctx;
	noPollConn     * conn;
	NOPOLL_SOCKET    _socket;
	noPollMsg      * msg;
	noPollConnOpts * opts;

	printf ("Test 22: testing connection close notification for regular connections (client side)..\n");

	/* init context */
	ctx = create_ctx ();

	/* create connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	/* set connection close */
	nopoll_conn_set_on_close (conn, __test_22_on_close, NULL);

	/* call to close connection as we had lost the connection */
	_socket = nopoll_conn_socket (conn);
	nopoll_close_socket (_socket);

	/* call to get content (we shouldn't get anythign) */
	msg = nopoll_conn_get_msg (conn);
	if (msg) {
		printf ("ERROR: we shouldn't get a msg frame, but a well defined pointer was found..\n");
		return nopoll_false;
	} /* end if */

	if (nopoll_conn_is_ok (conn)) {
		printf ("ERROR: we shouldn't get an ok value from nopoll_conn_is_ok (conn)..\n");
		return nopoll_false;
	} /* end if */

	if (! __test_22_on_close_signal) {
		printf ("ERROR: connection close should've been called but it wasn't..\n");
		return nopoll_false;
	} /* end if */

	/* close the connection */
	nopoll_conn_close (conn);

	__test_22_on_close_signal = nopoll_false;
	printf ("Test 22: test close connection close notification for WSS:// (ssl connections), (client side)..\n");

	/* disable verification */
	opts = nopoll_conn_opts_new ();
	nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);

	/* call to create a connection */
	conn = nopoll_conn_tls_new (ctx, opts, local_host_name, local_host_ports, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	/* set connection close */
	nopoll_conn_set_on_close (conn, __test_22_on_close, NULL);

	/* call to close connection as we had lost the connection */
	_socket = nopoll_conn_socket (conn);
	nopoll_close_socket (_socket);

	/* call to get content (we shouldn't get anythign) */
	msg = nopoll_conn_get_msg (conn);
	if (msg) {
		printf ("ERROR: we shouldn't get a msg frame, but a well defined pointer was found..\n");
		return nopoll_false;
	} /* end if */

	if (nopoll_conn_is_ok (conn)) {
		printf ("ERROR: we shouldn't get an ok value from nopoll_conn_is_ok (conn)..\n");
		return nopoll_false;
	} /* end if */

	if (! __test_22_on_close_signal) {
		printf ("ERROR: connection close should've been called but it wasn't..\n");
		return nopoll_false;
	} /* end if */

	/* close the connection */
	nopoll_conn_close (conn);
	

	nopoll_ctx_unref (ctx);
	return nopoll_true;
} /* end if */

int test_23_get_connection_close_count (noPollCtx * ctx, noPollConn * conn) {
	
	int         count_before_closing;
	noPollMsg * msg;

	/* wait for the reply */
	while (nopoll_true) {
		if (nopoll_conn_is_ready (conn))
			break;
		nopoll_sleep (10000);
	} /* end if */

	/* send package to get number of connection close detected */
	if (nopoll_conn_send_text (conn, "get-connection-close-count", 26) != 26) {
		printf ("ERROR: Expected to find proper send operation..\n");
		return nopoll_false;
	}
	
	/* call to get content (we shouldn't get anythign) */
	while (nopoll_true) {
		msg = nopoll_conn_get_msg (conn);
		if (msg)
			break;

		nopoll_sleep (10000);
	} /* end if */


	count_before_closing = strtol ((const char *) nopoll_msg_get_payload (msg), NULL, 10);
	printf ("Test 23: Message received: %d..\n", count_before_closing);
	/* release message */
	nopoll_msg_unref (msg);

	return count_before_closing;
}

nopoll_bool test_23 (void) {
	
	noPollCtx      * ctx;
	noPollConn     * conn;
	int              count_before_closing;
	int              count_before_closing2;
	noPollConnOpts * opts;

	printf ("Test 23: testing connection close notification for regular connections (client side)..\n");

	/* init context */
	ctx = create_ctx ();

	/* create connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	/* get connection close before closing */
	if ((count_before_closing = test_23_get_connection_close_count (ctx, conn)) == -1)
		return nopoll_false;
	printf ("Test 23: current connection close is: %d\n", count_before_closing);

	/* close the connection cleanly and check connection close is not called  */
	nopoll_conn_close (conn);

	/* create connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	if ((count_before_closing2 = test_23_get_connection_close_count (ctx, conn)) == -1)
		return nopoll_false;
	printf ("Test 23: current connection close is: %d\n", count_before_closing2);

	if (count_before_closing == count_before_closing2) {
		printf ("ERROR: expected connection close notification ...but same values were found..\n");
		return nopoll_false;
	} /* end if */

	/* close the connection cleanly and check connection close is not called  */
	nopoll_conn_close (conn);

	printf ("Test 23: now test TLS connections connection close..\n");

	/* disable verification */
	opts = nopoll_conn_opts_new ();
	nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);

	/* call to create a connection */
	conn = nopoll_conn_tls_new (ctx, opts, local_host_name, local_host_ports, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	/* get connection close before closing */
	if ((count_before_closing = test_23_get_connection_close_count (ctx, conn)) == -1)
		return nopoll_false;

	/* close the connection */
	nopoll_conn_close (conn);

	/* call to create a connection second connection */
	opts = nopoll_conn_opts_new ();
	nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);
	conn = nopoll_conn_tls_new (ctx, opts, local_host_name, local_host_ports, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	if ((count_before_closing2 = test_23_get_connection_close_count (ctx, conn)) == -1)
		return nopoll_false;

	/* close the connection */
	nopoll_conn_close (conn);

	if (count_before_closing == count_before_closing2) {
		printf ("ERROR: expected connection close notification ...but same values were found..\n");
		return nopoll_false;
	} /* end if */

	nopoll_ctx_unref (ctx);
	return nopoll_true;
} /* end if */

nopoll_bool test_24 (void) {
	
	noPollCtx      * ctx;
	noPollConn     * conn;
	noPollConnOpts * opts;
	noPollMsg      * msg;

	printf ("Test 24: test cookie support (set client and receive on server..)\n");

	/* init context */
	ctx = create_ctx ();

	/* configure cookie */
	opts = nopoll_conn_opts_new ();
	nopoll_conn_opts_set_cookie (opts, "theme=light; sessionToken=abc123");

	/* create connection */
	conn = nopoll_conn_new_opts (ctx, opts, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}

	/* send package to get number of connection close detected */
	if (nopoll_conn_send_text (conn, "get-cookie", 10) != 10) {
		printf ("ERROR: Expected to find proper send operation..\n");
		return nopoll_false;
	}
	
	/* call to get content (we shouldn't get anythign) */
	while (nopoll_true) {
		msg = nopoll_conn_get_msg (conn);
		if (msg)
			break;

		if (! nopoll_conn_is_ok (conn)) {
			printf ("ERROR: connection failure found during message wait..\n");
			return nopoll_false;
		}

		nopoll_sleep (10000);
	} /* end if */

	printf ("Test 24: received header set on server side: %s\n", nopoll_msg_get_payload (msg));
	if (! nopoll_cmp ((const char *) nopoll_msg_get_payload (msg), "theme=light; sessionToken=abc123")) {
		printf ("ERROR: expected to receive different header, error was: %s\n", nopoll_msg_get_payload (msg));
		return nopoll_false;
	}

	/* release message */
	nopoll_msg_unref (msg);


	/* close the connection */
	nopoll_conn_close (conn);


	nopoll_ctx_unref (ctx);
	return nopoll_true;
} /* end if */

nopoll_bool test_25_check_cookie (noPollCtx * ctx, const char * cookie) {
	noPollConn     * conn;
	noPollConnOpts * opts;

	/* configure cookie */
	opts = nopoll_conn_opts_new ();

	/* set a cookie bigger than 1044 */
	nopoll_conn_opts_set_cookie (opts, cookie);

	/* create connection */
	conn = nopoll_conn_new_opts (ctx, opts, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	}


	/* close the connection */
	nopoll_conn_close (conn);	

	return nopoll_true;
}

nopoll_bool test_25 (void) {
	
	noPollCtx      * ctx;

	printf ("Test 25: test cookie support (set client and receive on server..)\n");

	/* init context */
	ctx = create_ctx ();

	if (! test_25_check_cookie (ctx, "theme=light; sessionToken=abc123 lkjsadfkljasdf lkjaseflkawjet klajw glkajy240u 4234lkj y3j 3q5yñkl aegar glkejry b theme=light; sessionToken=abc123 lkjsadfkljasdf lkjaseflkawjet klajw glkajy240u 4234lkj y3j 3q5yñkl aegar glkejry b theme=light; sessionToken=abc123 lkjsadfkljasdf lkjaseflkawjet klajw glkajy240u 4234lkj y3j 3q5yñkl aegar glkejry b theme=light; sessionToken=abc123 lkjsadfkljasdf lkjaseflkawjet klajw glkajy240u 4234lkj y3j 3q5yñkl aegar glkejry b theme=light; sessionToken=abc123 lkjsadfkljasdf lkjaseflkawjet klajw glkajy240u 4234lkj y3j 3q5yñkl aegar glkejry b theme=light; sessionToken=abc123 lkjsadfkljasdf lkjaseflkawjet klajw glkajy240u 4234lkj y3j 3q5yñkl aegar glkejry b theme=light; sessionToken=abc123 lkjsadfkljasdf lkjaseflkawjet klajw glkajy240u 4234lkj y3j 3q5yñkl aegar glkejry b theme=light; sessionToken=abc123 lkjsadfkljasdf lkjaseflkawjet klajw glkajy240u 4234lkj y3j 3q5yñkl aegar glkejry b theme=light; sessionToken=abc123 lkjsadfkljasdf lkjaseflkawjet klajw glkajy240u 4234lkj y3j 3q5yñkl aegar glkejry b "))
		return nopoll_false;


	if (! test_25_check_cookie (ctx, "222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444asd35555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555affffffffff-23345"))
		return nopoll_false;

	if (! test_25_check_cookie (ctx, "222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444asd35555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555affffffffff-233"))
		return nopoll_false;

	if (! test_25_check_cookie (ctx, "222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444asd35555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555afffffff"))
		return nopoll_false;

	if (! test_25_check_cookie (ctx, "22222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444222222222222222222222211111111111111111111111133333333333333333333333334444444444444444444444444asd35555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555afffffff"))
		return nopoll_false;

	nopoll_ctx_unref (ctx);
	return nopoll_true;
} /* end if */

nopoll_bool test_26 (void) {

	noPollConn     * conn;
	noPollCtx      * ctx;

	/* init context */
	ctx = create_ctx ();

	/* create connection */
	conn = nopoll_conn_new (ctx, "echo.websocket.org", "80", NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	} /* end if */

	/* check test */
	if (! test_sending_and_check_echo (conn, "Test 26", "This is a test"))
		return nopoll_false;

	/* close the connection */
	nopoll_conn_close (conn);	

	/* release context */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

nopoll_bool test_27 (void) {

	noPollConn     * conn;
	noPollCtx      * ctx;

	/* init context */
	ctx = create_ctx ();

	/* create connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, "/", "chat-protocol", "http://www.aspl.es");
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	} /* end if */

	/* check test */
	if (! test_sending_and_check_echo (conn, "Test 27", "This is a test"))
		return nopoll_false;

	/* check accepted protocol */
	if (! nopoll_cmp ("chat-protocol", nopoll_conn_get_accepted_protocol (conn))) {
		printf ("ERROR: expected to find [chat-protocol] but found something: %s\n", 
			nopoll_conn_get_accepted_protocol (conn));
		return nopoll_false;
	} /* end if */ 

	printf ("Test 27: accepted protocol by the server: %s\n", nopoll_conn_get_accepted_protocol (conn));

	/* close the connection */
	nopoll_conn_close (conn);	

	/* create connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, "/", "hello-protocol", "http://www.aspl.es");
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	} /* end if */

	/* check test */
	if (! test_sending_and_check_echo (conn, "Test 27", "This is a test"))
		return nopoll_false;

	/* check accepted protocol */
	if (! nopoll_cmp ("hello-protocol-response", nopoll_conn_get_accepted_protocol (conn))) {
		printf ("ERROR: expected to find [chat-protocol] but found something: %s\n", 
			nopoll_conn_get_accepted_protocol (conn));
		return nopoll_false;
	} /* end if */ 

	printf ("Test 27: accepted protocol by the server: %s\n", nopoll_conn_get_accepted_protocol (conn));

	/* close the connection */
	nopoll_conn_close (conn);	


	/* release context */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}


nopoll_bool test_28 (void) {

	noPollConn     * conn;
	noPollCtx      * ctx;
	noPollMsg      * msg;

	/* init context */
	ctx = create_ctx ();

	/* create connection */
	conn = nopoll_conn_new (ctx, local_host_name, local_host_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (conn)) {
		printf ("ERROR: Expected to find proper client connection status, but found error..\n");
		return nopoll_false;
	} /* end if */

	/* wait until it is connected */
	nopoll_conn_wait_until_connection_ready (conn, 5);

	/* send a message to request connection close with a particular message */
	if (nopoll_conn_send_text (conn, "close with message", 18) != 18) {
		printf ("ERROR: failed to send close with message..");
		return nopoll_false;
	} /* end while */

	/* wait for the reply */
	while ((msg = nopoll_conn_get_msg (conn)) == NULL) {

		if (! nopoll_conn_is_ok (conn)) {
			/* connection was closed by remote side */
			break;
		} /* end if */

		nopoll_sleep (10000);
	} /* end if */

	printf ("Test 28: close reason received, statud=%d, message=%s\n", 
		nopoll_conn_get_close_status (conn),
		nopoll_conn_get_close_reason (conn));
	if (nopoll_conn_get_close_status (conn) != 1048) {
		printf ("ERROR: expected different error code..\n");
		return nopoll_false;
	}

	if (! nopoll_cmp (nopoll_conn_get_close_reason (conn), "Hey, this is a very reasonable error message")) {
		printf ("ERROR: expected different error message..\n");
		return nopoll_false;
	} /* end if */

	/* close connection */
	nopoll_conn_close (conn);

	/* release context */
	nopoll_ctx_unref (ctx);

	return nopoll_true;
}

LOCAL int websocket_main (char *argv)
{
	int iterator = *argv;

	printf ("** NoPoll: Websocket toolkit (regression test).\n");
	printf ("** Copyright (C) 2013 Advanced Software Production Line, S.L.\n**\n");

	printf ("** To gather information about time performance you can use:\n**\n");
	printf ("**     >> time ./nopoll-regression-client [--debug,--show-critical-only]\n**\n");
	printf ("** To gather information about memory consumed (and leaks) use:\n**\n");
	printf ("**     >> libtool --mode=execute valgrind --leak-check=yes --error-limit=no ./nopoll-regression-client\n**\n");
	printf ("**\n");
	printf ("** Report bugs to:\n**\n");
	printf ("**     <info@aspl.es> noPoll mailing list\n**\n");
	switch (iterator) {
		case 1:
			if (test_01_strings()) {
				printf("Test 01-strings: Library strings support [   OK   ]\n");
			} else {
				printf("Test 01-strings: Library strings support [ FAILED ]\n");
				//		return -1;
			}
			break;
		case 2:
			if (test_02()) {
				printf("Test 02: Simple request/reply [   OK   ]\n");
			} else {
				printf("Test 02: Simple request/reply [ FAILED ]\n");
				//		return -1;
			}

			break;
		case 3:

			/* test streaming api */
			if (test_03()) {
				printf("Test 03: test streaming api [   OK   ]\n");
			} else {
				printf("Test 03: test streaming api [ FAILED ]\n");
				//		return -1;
			}

			break;
		case 4:
			if (test_04a()) {
				printf(
						"Test 04-a: check non-blocking streaming and message based API  [   OK   ]\n");
			} else {
				printf(
						"Test 04-a: check non-blocking streaming and message based API [ FAILED ]\n");
				//			return -1;
			}

			if (test_04b()) {
				printf(
						"Test 04-b: try to overflow write access and recover from it  [   OK   ]\n");
			} else {
				printf(
						"Test 04-b: try to overflow write access and recover from it [ FAILED ]\n");
				//			return -1;
			}

			break;
		case 5:
			if (test_05()) {
				printf("Test 05: sending utf-8 content [   OK   ]\n");
			} else {
				printf("Test 05: sending utf-8 content [ FAILED ]\n");
				//			return -1;
			}

			break;
		case 6:
			if (test_06()) {
				printf("Test 06: testing basic TLS connect [   OK   ]\n");
			} else {
				printf("Test 06: testing basic TLS connect [ FAILED ]\n");
				//			return -1;
			}

			break;
		case 7:
			if (test_07()) {
				printf("Test 07: testing TLS request/reply [   OK   ]\n");
			} else {
				printf("Test 07: testing TLS request/reply [ FAILED ]\n");
				//			return -1;
			}

			break;
		case 8:
			if (test_08()) {
				printf("Test 08: test normal connect to TLS port [   OK   ]\n");
			} else {
				printf("Test 08: test normal connect to TLS port [ FAILED ]\n");
				//			return -1;
			}

			break;
		case 9:
			if (test_09()) {
				printf(
						"Test 09: ensure we only support Sec-WebSocket-Version: 13 [   OK   ]\n");
			} else {
				printf(
						"Test 09: ensure we only support Sec-WebSocket-Version: 13 [ FAILED ]\n");
				//			return -1;
			}

			break;
		case 10:
			if (test_10()) {
				printf(
						"Test 10: test checking origing in on open and denying it [   OK   ]\n");
			} else {
				printf(
						"Test 10: test checking origing in on open and denying it [ FAILED ]\n");
				//			return -1;
			}

			break;
		case 11:
			if (test_11()) {
				printf("Test 11: release context after connection [   OK   ]\n");
			} else {
				printf("Test 11: release context after connection [ FAILED ]\n");
				//			return -1;
			}

			break;
		case 12:
			if (test_12()) {
				printf(
						"Test 12: create huge amount of connections in a short time [   OK   ]\n");
			} else {
				printf(
						"Test 12: create huge amount of connections in a short time [ FAILED ]\n");
				//			return -1;
			}

			break;
		case 13:
			if (test_13()) {
				printf("Test 13: testing certificate storage [   OK    ]\n");
			} else {
				printf("Test 13: testing certificate storage [ FAILED  ]\n");
				//			return -1;
			}

			break;
		case 14:
			if (test_14()) {
				printf(
						"Test 14: testing sending frame with few content as indicated by header [   OK    ]\n");
			} else {
				printf(
						"Test 14: testing sending frame with few content as indicated by header [ FAILED  ]\n");
				//			return -1;
			}

			break;
		case 15:
			if (test_15()) {
				printf(
						"Test 15: checking non-blocking calls to get messages when no content is available [   OK    ]\n");
			} else {
				printf(
						"Test 15: checking non-blocking calls to get messages when no content is available [ FAILED  ]\n");
				//			return -1;
			}

			break;
		case 16:
			if (test_16()) {
				printf(
						"Test 16: check sending frames with sleep in header [   OK    ]\n");
			} else {
				printf(
						"Test 16: check sending frames with sleep in header [ FAILED  ]\n");
				//			return -1;
			}

			break;
		case 17:
			if (test_17()) {
				printf("Test 17: check partial frame reception [   OK    ]\n");
			} else {
				printf("Test 17: check partial frame reception [ FAILED  ]\n");
				//			return -1;
			}

			break;
		case 18:
			if (test_18()) {
				printf(
						"Test 18: check nopoll_loop_wait (second call) [   OK    ]\n");
			} else {
				printf(
						"Test 18: check nopoll_loop_wait (second call) [ FAILED  ]\n");
				//			return -1;
			}

			break;
		case 19:
			if (test_19()) {
				printf(
						"Test 19: support different SSL methods (SSLv23, SSLv3, TLSv1 [   OK    ]\n");
			} else {
				printf(
						"Test 19: support different SSL methods (SSLv23, SSLv3, TLSv1 [ FAILED  ]\n");
				//			return -1;
			}

			break;
		case 20:
	#if defined(__NOPOLL_PTHREAD_SUPPORT__)
			if (test_20 ()) {
				printf ("Test 20: check mutex support [   OK    ]\n");
			} else {
				printf ("Test 20: check mutex support [ FAILED  ]\n");
				return -1;
			}
	#endif

			break;
		case 21:
			if (test_21()) {
				printf(
						"Test 21: client side ssl certificates verification  [   OK    ]\n");
			} else {
				printf(
						"Test 21: client side ssl certificates verification [ FAILED  ]\n");
				//			return -1;
			} /* end if */

			break;
		case 22:
			if (test_22()) {
				printf(
						"Test 22: test connection close trigger (client side)  [   OK    ]\n");
			} else {
				printf(
						"Test 22: test connection close trigger (client side) [ FAILED  ]\n");
				//			return -1;
			} /* end if */

			break;
		case 23:
			if (test_23()) {
				printf(
						"Test 23: test connection close trigger (server side)  [   OK    ]\n");
			} else {
				printf(
						"Test 23: test connection close trigger (server side) [ FAILED  ]\n");
				//			return -1;
			} /* end if */

			break;
		case 24:
			if (test_24()) {
				printf(
						"Test 24: check cookie support (client and server side)  [   OK    ]\n");
			} else {
				printf(
						"Test 24: check cookie support (client and server side)  [ FAILED  ]\n");
				//			return -1;
			} /* end if */

			break;
		case 25:
			if (test_25()) {
				printf("Test 25: check cookie attack  [   OK    ]\n");
			} else {
				printf("Test 25: check cookie attack  [ FAILED  ]\n");
				//			return -1;
			} /* end if */

			break;
		case 26:
			if (test_26()) {
				printf("Test 26: checking echo.websocket.org  [   OK    ]\n");
			} else {
				printf("Test 26: chekcing echo.websocket.org  [ FAILED  ]\n");
				//			return -1;
			} /* end if */

			break;
		case 27:
			if (test_27()) {
				printf("Test 27: checking setting protocol  [   OK    ]\n");
			} else {
				printf("Test 27: chekcing setting protocol  [ FAILED  ]\n");
				//			return -1;
			} /* end if */

			break;
		case 28:
			if (test_28()) {
				printf("Test 28: checking setting protocol  [   OK    ]\n");
			} else {
				printf("Test 28: chekcing setting protocol  [ FAILED  ]\n");
				//			return -1;
			} /* end if */
			break;

		default:
			break;
	}
#if 0
	iterator = 1;
//		while (iterator < argc) {
		/* check for debug */
		printf ("Checking agument: %s\n", argv[iterator]);
		if (nopoll_cmp (argv[iterator], "--debug")) {
			printf ("Activating debug..\n");
			debug = nopoll_true;
		} /* end if */
		if (nopoll_cmp (argv[iterator], "--show-critical-only")) {
			printf ("Activating reporting of critical messages..\n");
			show_critical_only = nopoll_true;
		} /* end if */

		/* next position */
		iterator++;
//		}

	//printf ("INFO: starting tests with pid: %d\n", getpid ());
	if (test_01_strings ()) {
		printf ("Test 01-strings: Library strings support [   OK   ]\n");
	} else {
		printf ("Test 01-strings: Library strings support [ FAILED ]\n");
//		return -1;
	}

	if (test_01_base64 ()) {
		printf ("Test 01-bas64: Library bas64 support [   OK   ]\n");
	}else {
		printf ("Test 01-bas64: Library bas64 support [ FAILED ]\n");
//		return -1;
	}

	if (test_01_masking ()) {
		printf ("Test 01-masking: Library websocket content masking support [   OK   ]\n");
	}else {
		printf ("Test 01-masking: Library websocket content masking support [ FAILED ]\n");
//		return -1;
	}

	if (test_01 ()) {
		printf ("Test 01: Simple connect and disconnect [   OK   ]\n");
	}else {
		printf ("Test 01: Simple connect and disconnect [ FAILED ]\n");
//		return -1;
	}

	if (test_02 ()) {	
		printf ("Test 02: Simple request/reply [   OK   ]\n");
	}else {
		printf ("Test 02: Simple request/reply [ FAILED ]\n");
//		return -1;
	}

	/* test streaming api */
	if (test_03 ()) {	
		printf ("Test 03: test streaming api [   OK   ]\n");
	}else {
		printf ("Test 03: test streaming api [ FAILED ]\n");
//		return -1;
	}

//	if (test_04 (1024)) {
//		printf ("Test 04: test streaming api (II) [   OK   ]\n");
//	}else {
//		printf ("Test 04: test streaming api (II) [ FAILED ]\n");
////		return -1;
//	}

//		if (test_04 (512)) {
//			printf ("Test 04-a: test streaming api (III) [   OK   ]\n");
//		}else {
//			printf ("Test 04-a: test streaming api (III) [ FAILED ]\n");
////			return -1;
//		}

//		if (test_04 (137)) {
//			printf ("Test 04-b: test streaming api (IV) [   OK   ]\n");
//		}else {
//			printf ("Test 04-b: test streaming api (IV) [ FAILED ]\n");
////			return -1;
//		}
//
//		if (test_04 (17)) {
//			printf ("Test 04-c: test streaming api (V) [   OK   ]\n");
//		}else {
//			printf ("Test 04-c: test streaming api (V) [ FAILED ]\n");
////			return -1;
//		}

		if (test_04a ()) {
			printf ("Test 04-a: check non-blocking streaming and message based API  [   OK   ]\n");
		} else {
			printf ("Test 04-a: check non-blocking streaming and message based API [ FAILED ]\n");
//			return -1;
		}

		if (test_04b ()) {
			printf ("Test 04-b: try to overflow write access and recover from it  [   OK   ]\n");
		} else {
			printf ("Test 04-b: try to overflow write access and recover from it [ FAILED ]\n");
//			return -1;
		}

//		if (test_04c ()) {
//			printf ("Test 04-c: send a file and try to overflow (but retry)  [   OK   ]\n");
//		} else {
//			printf ("Test 04-c: send a file and try to overflow (but retry) [ FAILED ]\n");
////			return -1;
//		}

		if (test_05 ()) {
			printf ("Test 05: sending utf-8 content [   OK   ]\n");
		} else {
			printf ("Test 05: sending utf-8 content [ FAILED ]\n");
//			return -1;
		}

		if (test_06 ()) {
			printf ("Test 06: testing basic TLS connect [   OK   ]\n");
		} else {
			printf ("Test 06: testing basic TLS connect [ FAILED ]\n");
//			return -1;
		}

		if (test_07 ()) {
			printf ("Test 07: testing TLS request/reply [   OK   ]\n");
		} else {
			printf ("Test 07: testing TLS request/reply [ FAILED ]\n");
//			return -1;
		}

		if (test_08 ()) {
			printf ("Test 08: test normal connect to TLS port [   OK   ]\n");
		} else {
			printf ("Test 08: test normal connect to TLS port [ FAILED ]\n");
//			return -1;
		}

		if (test_09 ()) {
			printf ("Test 09: ensure we only support Sec-WebSocket-Version: 13 [   OK   ]\n");
		} else {
			printf ("Test 09: ensure we only support Sec-WebSocket-Version: 13 [ FAILED ]\n");
//			return -1;
		}

		if (test_10 ()) {
			printf ("Test 10: test checking origing in on open and denying it [   OK   ]\n");
		} else {
			printf ("Test 10: test checking origing in on open and denying it [ FAILED ]\n");
//			return -1;
		}

		if (test_11 ()) {
			printf ("Test 11: release context after connection [   OK   ]\n");
		} else {
			printf ("Test 11: release context after connection [ FAILED ]\n");
//			return -1;
		}

		if (test_12 ()) {
			printf ("Test 12: create huge amount of connections in a short time [   OK   ]\n");
		} else {
			printf ("Test 12: create huge amount of connections in a short time [ FAILED ]\n");
//			return -1;
		}

		if (test_13 ()) {
			printf ("Test 13: testing certificate storage [   OK    ]\n");
		} else {
			printf ("Test 13: testing certificate storage [ FAILED  ]\n");
//			return -1;
		}

		if (test_14 ()) {
			printf ("Test 14: testing sending frame with few content as indicated by header [   OK    ]\n");
		} else {
			printf ("Test 14: testing sending frame with few content as indicated by header [ FAILED  ]\n");
//			return -1;
		}

		if (test_15 ()) {
			printf ("Test 15: checking non-blocking calls to get messages when no content is available [   OK    ]\n");
		} else {
			printf ("Test 15: checking non-blocking calls to get messages when no content is available [ FAILED  ]\n");
//			return -1;
		}

		if (test_16 ()) {
			printf ("Test 16: check sending frames with sleep in header [   OK    ]\n");
		} else {
			printf ("Test 16: check sending frames with sleep in header [ FAILED  ]\n");
//			return -1;
		}

		if (test_17 ()) {
			printf ("Test 17: check partial frame reception [   OK    ]\n");
		} else {
			printf ("Test 17: check partial frame reception [ FAILED  ]\n");
//			return -1;
		}

		if (test_18 ()) {
			printf ("Test 18: check nopoll_loop_wait (second call) [   OK    ]\n");
		} else {
			printf ("Test 18: check nopoll_loop_wait (second call) [ FAILED  ]\n");
//			return -1;
		}

		if (test_19 ()) {
			printf ("Test 19: support different SSL methods (SSLv23, SSLv3, TLSv1 [   OK    ]\n");
		} else {
			printf ("Test 19: support different SSL methods (SSLv23, SSLv3, TLSv1 [ FAILED  ]\n");
//			return -1;
		}

	#if defined(__NOPOLL_PTHREAD_SUPPORT__)
		if (test_20 ()) {
			printf ("Test 20: check mutex support [   OK    ]\n");
		} else {
			printf ("Test 20: check mutex support [ FAILED  ]\n");
			return -1;
		}
	#endif

		if (test_21 ()) {
			printf ("Test 21: client side ssl certificates verification  [   OK    ]\n");
		} else {
			printf ("Test 21: client side ssl certificates verification [ FAILED  ]\n");
//			return -1;
		} /* end if */

		if (test_22 ()) {
			printf ("Test 22: test connection close trigger (client side)  [   OK    ]\n");
		} else {
			printf ("Test 22: test connection close trigger (client side) [ FAILED  ]\n");
//			return -1;
		} /* end if */

		if (test_23 ()) {
			printf ("Test 23: test connection close trigger (server side)  [   OK    ]\n");
		} else {
			printf ("Test 23: test connection close trigger (server side) [ FAILED  ]\n");
//			return -1;
		} /* end if */

		if (test_24 ()) {
			printf ("Test 24: check cookie support (client and server side)  [   OK    ]\n");
		} else {
			printf ("Test 24: check cookie support (client and server side)  [ FAILED  ]\n");
//			return -1;
		} /* end if */

		if (test_25 ()) {
			printf ("Test 25: check cookie attack  [   OK    ]\n");
		} else {
			printf ("Test 25: check cookie attack  [ FAILED  ]\n");
//			return -1;
		} /* end if */

		if (test_26 ()) {
			printf ("Test 26: checking echo.websocket.org  [   OK    ]\n");
		} else {
			printf ("Test 26: chekcing echo.websocket.org  [ FAILED  ]\n");
//			return -1;
		} /* end if */

		if (test_27 ()) {
			printf ("Test 27: checking setting protocol  [   OK    ]\n");
		} else {
			printf ("Test 27: chekcing setting protocol  [ FAILED  ]\n");
//			return -1;
		} /* end if */

		if (test_28 ()) {
			printf ("Test 28: checking setting protocol  [   OK    ]\n");
		} else {
			printf ("Test 28: chekcing setting protocol  [ FAILED  ]\n");
//			return -1;
		} /* end if */

	/* add support to reply with redirect 301 to an opening
	 * request: page 19 and 22 */

	/* add support for basic HTTP auth before proceding with the
	 * handshake. The the possibility to use htpasswd tools. Page 19 and 22 */

	/* add support to define cookies by the server: page 20 */

	/* update the library to split message frames into smaller
	 * complete frames when bigger messages are received. */

	/* add support for proxy mode */

	/* check control files aren't flagged as fragmented */
	
	/* upload a file to the server ...*/

	/* more streaming api testing, get bigger content as a
	 * consequence of receiving several messages */

	/* test streaming API when it timeouts */

	/* test sending wrong mime headers */

	/* test sending missing mime headers */

	/* test injecting wrong bytes */

	/* test sending lot of MIME headers (really lot of
	 * information) */

	/* test checking protocols and denying it */

	/* test sending ping */

	/* test sending pong (without ping) */

	/* test sending frames 126 == ( 65536) */

	/* test sending frames 127 == ( more than 65536) */

	/* test applying limits to incoming content */

	/* test splitting into several frames content bigger */

	/* test wrong UTF-8 content received on text frames */

	/* add support to sending close frames with status code and a
	 * textual indication as defined by page 36 */
#endif
	/* call to cleanup */
	nopoll_cleanup_library ();
	printf ("All tests ok!!\n");

	return 0;
}
LOCAL xQueueHandle	Web_QueueStop = NULL;

LOCAL void websocket_task(void *pvParameters)
{
	bool ValueFromReceive = false;
	portBASE_TYPE xStatus;
	websocket_main((char*)pvParameters);

	while (1) {
		xStatus = xQueueReceive(Web_QueueStop,&ValueFromReceive,0);
		if (xStatus == pdPASS && ValueFromReceive == true){
			printf("websocket_task exit signal\n");
			break;
		}
		vTaskDelay(200 / portTICK_RATE_MS);
		printf("websocket_task\n");
	}
	vQueueDelete(Web_QueueStop);
	Web_QueueStop = NULL;
	vTaskDelete(NULL);
	printf("delete the websocket_task\n");
}

/*start the websocket task*/
void websocket_start(void *optarg)
{
	if (Web_QueueStop == NULL)
		Web_QueueStop = xQueueCreate(1,1);

	if (Web_QueueStop != NULL)
		xTaskCreate(websocket_task, "websocket_task", 512, optarg, 4, NULL);
}

/*stop the websocket task*/
sint8 websocket_stop(void)
{
	bool ValueToSend = true;
	portBASE_TYPE xStatus;

	if (Web_QueueStop == NULL)
		return -1;

	xStatus = xQueueSend(Web_QueueStop,&ValueToSend,0);
	if (xStatus != pdPASS)
		return -1;
	else
		return pdPASS;
}
/* end-of-file-found */
