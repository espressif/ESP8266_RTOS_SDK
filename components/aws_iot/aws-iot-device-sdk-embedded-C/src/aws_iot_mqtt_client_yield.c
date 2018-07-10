/*
* Copyright 2015-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License").
* You may not use this file except in compliance with the License.
* A copy of the License is located at
*
* http://aws.amazon.com/apache2.0
*
* or in the "license" file accompanying this file. This file is distributed
* on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
* express or implied. See the License for the specific language governing
* permissions and limitations under the License.
*/

// Based on Eclipse Paho.
/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander/Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

/**
 * @file aws_iot_mqtt_client_yield.c
 * @brief MQTT client yield API definitions
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "aws_iot_mqtt_client_common_internal.h"

/**
  * This is for the case when the aws_iot_mqtt_internal_send_packet Fails.
  */
static void _aws_iot_mqtt_force_client_disconnect(AWS_IoT_Client *pClient) {
	pClient->clientStatus.clientState = CLIENT_STATE_DISCONNECTED_ERROR;
	pClient->networkStack.disconnect(&(pClient->networkStack));
	pClient->networkStack.destroy(&(pClient->networkStack));
}

static IoT_Error_t _aws_iot_mqtt_handle_disconnect(AWS_IoT_Client *pClient) {
	IoT_Error_t rc;

	FUNC_ENTRY;

	rc = aws_iot_mqtt_disconnect(pClient);
	if(rc != SUCCESS) {
		// If the aws_iot_mqtt_internal_send_packet prevents us from sending a disconnect packet then we have to clean the stack
		_aws_iot_mqtt_force_client_disconnect(pClient);
	}

	if(NULL != pClient->clientData.disconnectHandler) {
		pClient->clientData.disconnectHandler(pClient, pClient->clientData.disconnectHandlerData);
	}

	/* Reset to 0 since this was not a manual disconnect */
	pClient->clientStatus.clientState = CLIENT_STATE_DISCONNECTED_ERROR;
	FUNC_EXIT_RC(NETWORK_DISCONNECTED_ERROR);
}


static IoT_Error_t _aws_iot_mqtt_handle_reconnect(AWS_IoT_Client *pClient) {
	IoT_Error_t rc;

	FUNC_ENTRY;

	if(!has_timer_expired(&(pClient->reconnectDelayTimer))) {
		/* Timer has not expired. Not time to attempt reconnect yet.
		 * Return attempting reconnect */
		FUNC_EXIT_RC(NETWORK_ATTEMPTING_RECONNECT);
	}

	rc = NETWORK_PHYSICAL_LAYER_DISCONNECTED;
	if(NULL != pClient->networkStack.isConnected) {
		rc = pClient->networkStack.isConnected(&(pClient->networkStack));
	}

	if(NETWORK_PHYSICAL_LAYER_CONNECTED == rc) {
		rc = aws_iot_mqtt_attempt_reconnect(pClient);
		if(NETWORK_RECONNECTED == rc) {
			rc = aws_iot_mqtt_set_client_state(pClient, CLIENT_STATE_CONNECTED_IDLE,
											   CLIENT_STATE_CONNECTED_YIELD_IN_PROGRESS);
			if(SUCCESS != rc) {
				FUNC_EXIT_RC(rc);
			}
			FUNC_EXIT_RC(NETWORK_RECONNECTED);
		}
	}

	pClient->clientData.currentReconnectWaitInterval *= 2;

	if(AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL < pClient->clientData.currentReconnectWaitInterval) {
		FUNC_EXIT_RC(NETWORK_RECONNECT_TIMED_OUT_ERROR);
	}
	countdown_ms(&(pClient->reconnectDelayTimer), pClient->clientData.currentReconnectWaitInterval);
	FUNC_EXIT_RC(rc);
}

static IoT_Error_t _aws_iot_mqtt_keep_alive(AWS_IoT_Client *pClient) {
	IoT_Error_t rc = SUCCESS;
	Timer timer;
	size_t serialized_len;

	FUNC_ENTRY;

	if(NULL == pClient) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	if(0 == pClient->clientData.keepAliveInterval) {
		FUNC_EXIT_RC(SUCCESS);
	}

	if(!has_timer_expired(&pClient->pingTimer)) {
		FUNC_EXIT_RC(SUCCESS);
	}

	if(pClient->clientStatus.isPingOutstanding) {
		rc = _aws_iot_mqtt_handle_disconnect(pClient);
		FUNC_EXIT_RC(rc);
	}

	/* there is no ping outstanding - send one */
	init_timer(&timer);

	countdown_ms(&timer, pClient->clientData.commandTimeoutMs);
	serialized_len = 0;
	rc = aws_iot_mqtt_internal_serialize_zero(pClient->clientData.writeBuf, pClient->clientData.writeBufSize,
											  PINGREQ, &serialized_len);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	/* send the ping packet */
	rc = aws_iot_mqtt_internal_send_packet(pClient, serialized_len, &timer);
	if(SUCCESS != rc) {
		//If sending a PING fails we can no longer determine if we are connected.  In this case we decide we are disconnected and begin reconnection attempts
		rc = _aws_iot_mqtt_handle_disconnect(pClient);
		FUNC_EXIT_RC(rc);
	}

	pClient->clientStatus.isPingOutstanding = true;
	/* start a timer to wait for PINGRESP from server */
	countdown_sec(&pClient->pingTimer, pClient->clientData.keepAliveInterval);

	FUNC_EXIT_RC(SUCCESS);
}

/**
 * @brief Yield to the MQTT client
 *
 * Called to yield the current thread to the underlying MQTT client.  This time is used by
 * the MQTT client to manage PING requests to monitor the health of the TCP connection as
 * well as periodically check the socket receive buffer for subscribe messages.  Yield()
 * must be called at a rate faster than the keepalive interval.  It must also be called
 * at a rate faster than the incoming message rate as this is the only way the client receives
 * processing time to manage incoming messages.
 * This is the internal function which is called by the yield API to perform the operation.
 * Not meant to be called directly as it doesn't do validations or client state changes
 *
 * @param pClient Reference to the IoT Client
 * @param timeout_ms Maximum number of milliseconds to pass thread execution to the client.
 *
 * @return An IoT Error Type defining successful/failed client processing.
 *         If this call results in an error it is likely the MQTT connection has dropped.
 *         iot_is_mqtt_connected can be called to confirm.
 */
static IoT_Error_t _aws_iot_mqtt_internal_yield(AWS_IoT_Client *pClient, uint32_t timeout_ms) {
	IoT_Error_t yieldRc = SUCCESS;

	uint8_t packet_type;
	ClientState clientState;
	Timer timer;
	init_timer(&timer);
	countdown_ms(&timer, timeout_ms);

	FUNC_ENTRY;

	// evaluate timeout at the end of the loop to make sure the actual yield runs at least once
	do {
		clientState = aws_iot_mqtt_get_client_state(pClient);
		if(CLIENT_STATE_PENDING_RECONNECT == clientState) {
			if(AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL < pClient->clientData.currentReconnectWaitInterval) {
				yieldRc = NETWORK_RECONNECT_TIMED_OUT_ERROR;
				break;
			}
			yieldRc = _aws_iot_mqtt_handle_reconnect(pClient);
			/* Network reconnect attempted, check if yield timer expired before
			 * doing anything else */
			continue;
		}

		yieldRc = aws_iot_mqtt_internal_cycle_read(pClient, &timer, &packet_type);
		if(SUCCESS == yieldRc) {
			yieldRc = _aws_iot_mqtt_keep_alive(pClient);
		} else {
			// SSL read and write errors are terminal, connection must be closed and retried
			if(NETWORK_SSL_READ_ERROR == yieldRc || NETWORK_SSL_READ_TIMEOUT_ERROR == yieldRc
				|| NETWORK_SSL_WRITE_ERROR == yieldRc || NETWORK_SSL_WRITE_TIMEOUT_ERROR == yieldRc) {
				yieldRc = _aws_iot_mqtt_handle_disconnect(pClient);
			}
		}

		if(NETWORK_DISCONNECTED_ERROR == yieldRc) {
			pClient->clientData.counterNetworkDisconnected++;
			if(1 == pClient->clientStatus.isAutoReconnectEnabled) {
				yieldRc = aws_iot_mqtt_set_client_state(pClient, CLIENT_STATE_DISCONNECTED_ERROR,
														CLIENT_STATE_PENDING_RECONNECT);
				if(SUCCESS != yieldRc) {
					FUNC_EXIT_RC(yieldRc);
				}

				pClient->clientData.currentReconnectWaitInterval = AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL;
				countdown_ms(&(pClient->reconnectDelayTimer), pClient->clientData.currentReconnectWaitInterval);
				/* Depending on timer values, it is possible that yield timer has expired
				 * Set to rc to attempting reconnect to inform client that autoreconnect
				 * attempt has started */
				yieldRc = NETWORK_ATTEMPTING_RECONNECT;
			} else {
				break;
			}
		} else if(SUCCESS != yieldRc) {
			break;
		}
	} while(!has_timer_expired(&timer));

	FUNC_EXIT_RC(yieldRc);
}

/**
 * @brief Yield to the MQTT client
 *
 * Called to yield the current thread to the underlying MQTT client.  This time is used by
 * the MQTT client to manage PING requests to monitor the health of the TCP connection as
 * well as periodically check the socket receive buffer for subscribe messages.  Yield()
 * must be called at a rate faster than the keepalive interval.  It must also be called
 * at a rate faster than the incoming message rate as this is the only way the client receives
 * processing time to manage incoming messages.
 * This is the outer function which does the validations and calls the internal yield above
 * to perform the actual operation. It is also responsible for client state changes
 *
 * @param pClient Reference to the IoT Client
 * @param timeout_ms Maximum number of milliseconds to pass thread execution to the client.
 *
 * @return An IoT Error Type defining successful/failed client processing.
 *         If this call results in an error it is likely the MQTT connection has dropped.
 *         iot_is_mqtt_connected can be called to confirm.
 */
IoT_Error_t aws_iot_mqtt_yield(AWS_IoT_Client *pClient, uint32_t timeout_ms) {
	IoT_Error_t rc, yieldRc;
	ClientState clientState;

	if(NULL == pClient || 0 == timeout_ms) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	clientState = aws_iot_mqtt_get_client_state(pClient);
	/* Check if network was manually disconnected */
	if(CLIENT_STATE_DISCONNECTED_MANUALLY == clientState) {
		FUNC_EXIT_RC(NETWORK_MANUALLY_DISCONNECTED);
	}

	/* If we are in the pending reconnect state, skip other checks.
	 * Pending reconnect state is only set when auto-reconnect is enabled */
	if(CLIENT_STATE_PENDING_RECONNECT != clientState) {
		/* Check if network is disconnected and auto-reconnect is not enabled */
		if(!aws_iot_mqtt_is_client_connected(pClient)) {
			FUNC_EXIT_RC(NETWORK_DISCONNECTED_ERROR);
		}

		/* Check if client is idle, if not another operation is in progress and we should return */
		if(CLIENT_STATE_CONNECTED_IDLE != clientState) {
			FUNC_EXIT_RC(MQTT_CLIENT_NOT_IDLE_ERROR);
		}

		rc = aws_iot_mqtt_set_client_state(pClient, CLIENT_STATE_CONNECTED_IDLE,
										   CLIENT_STATE_CONNECTED_YIELD_IN_PROGRESS);
		if(SUCCESS != rc) {
			FUNC_EXIT_RC(rc);
		}
	}

	yieldRc = _aws_iot_mqtt_internal_yield(pClient, timeout_ms);

	if(NETWORK_DISCONNECTED_ERROR != yieldRc && NETWORK_ATTEMPTING_RECONNECT != yieldRc) {
		rc = aws_iot_mqtt_set_client_state(pClient, CLIENT_STATE_CONNECTED_YIELD_IN_PROGRESS,
										   CLIENT_STATE_CONNECTED_IDLE);
		if(SUCCESS == yieldRc && SUCCESS != rc) {
			yieldRc = rc;
		}
	}

	FUNC_EXIT_RC(yieldRc);
}

#ifdef __cplusplus
}
#endif

