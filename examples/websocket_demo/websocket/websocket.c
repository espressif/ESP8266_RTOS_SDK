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
#include "ssl_compat-1.0.h"
#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

nopoll_bool debug = nopoll_false;
nopoll_bool show_critical_only = nopoll_false;
LOCAL xQueueHandle	Web_QueueStop = NULL;

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

LOCAL int websocket_main (char *argv)
{
	int iterator = *argv;

	switch (iterator) {

		case 2:
			if (test_02()) {
				printf("Test 02: Simple request/reply [   OK   ]\n");
			} else {
				printf("Test 02: Simple request/reply [ FAILED ]\n");
			}

			break;

		case 4:
			if (test_04a()) {
				printf(
						"Test 04-a: check non-blocking streaming and message based API  [   OK   ]\n");
			} else {
				printf(
						"Test 04-a: check non-blocking streaming and message based API [ FAILED ]\n");
			}
			break;
		case 5:
			if (test_05()) {
				printf("Test 05: sending utf-8 content [   OK   ]\n");
			} else {
				printf("Test 05: sending utf-8 content [ FAILED ]\n");
			}

			break;
		case 6:
			if (test_06()) {
				printf("Test 06: testing basic TLS connect [   OK   ]\n");
			} else {
				printf("Test 06: testing basic TLS connect [ FAILED ]\n");
			}

			break;
		case 7:
			if (test_07()) {
				printf("Test 07: testing TLS request/reply [   OK   ]\n");
			} else {
				printf("Test 07: testing TLS request/reply [ FAILED ]\n");
			}

			break;
		case 8:
			if (test_08()) {
				printf("Test 08: test normal connect to TLS port [   OK   ]\n");
			} else {
				printf("Test 08: test normal connect to TLS port [ FAILED ]\n");
			}

			break;

		default:
			break;
	}

	/* call to cleanup */
	nopoll_cleanup_library ();
	printf ("All tests ok!!\n");

	return 0;
}

LOCAL void websocket_task(void *pvParameters)
{
	bool ValueFromReceive = false;
	portBASE_TYPE xStatus;
	struct ip_info ip_config;
	struct station_config sta_config;
	bzero(&sta_config, sizeof(struct station_config));

	sprintf(sta_config.ssid, "B-LINK_845R");
	sprintf(sta_config.password, "000");
	wifi_station_set_config(&sta_config);
	os_printf("%s\n", __func__);
	wifi_get_ip_info(STATION_IF, &ip_config);
	while(ip_config.ip.addr == 0){
		vTaskDelay(1000 / portTICK_RATE_MS);
		wifi_get_ip_info(STATION_IF, &ip_config);
	}
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
