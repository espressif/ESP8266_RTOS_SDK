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
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

/**
 * @file aws_iot_mqtt_client_connect.c
 * @brief MQTT client connect API definition and related functions
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include <aws_iot_mqtt_client.h>
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_mqtt_client_common_internal.h"

typedef union {
	uint8_t all;    /**< all connect flags */
#if defined(REVERSED)
	struct
	{
		unsigned int username : 1;		/**< 3.1 user name */
		unsigned int password : 1;		/**< 3.1 password */
		unsigned int willRetain : 1;	/**< will retain setting */
		unsigned int willQoS : 2;		/**< will QoS value */
		unsigned int will : 1;			/**< will flag */
		unsigned int cleansession : 1;	/**< clean session flag */
		unsigned int : 1;				/**< unused */
	} bits;
#else
	struct {
		unsigned int : 1;
		/**< unused */
		unsigned int cleansession : 1;
		/**< cleansession flag */
		unsigned int will : 1;
		/**< will flag */
		unsigned int willQoS : 2;
		/**< will QoS value */
		unsigned int willRetain : 1;
		/**< will retain setting */
		unsigned int password : 1;
		/**< 3.1 password */
		unsigned int username : 1;        /**< 3.1 user name */
	} bits;
#endif
} MQTT_Connect_Header_Flags;
/**< connect flags byte */

typedef union {
	uint8_t all;                            /**< all connack flags */
#if defined(REVERSED)
	struct
	{
		unsigned int sessionpresent : 1;	/**< session present flag */
		unsigned int : 7;					/**< unused */
	} bits;
#else
	struct {
		unsigned int : 7;
		/**< unused */
		unsigned int sessionpresent : 1;    /**< session present flag */
	} bits;
#endif
} MQTT_Connack_Header_Flags;
/**< connack flags byte */

typedef enum {
	CONNACK_CONNECTION_ACCEPTED = 0,
	CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR = 1,
	CONNACK_IDENTIFIER_REJECTED_ERROR = 2,
	CONNACK_SERVER_UNAVAILABLE_ERROR = 3,
	CONNACK_BAD_USERDATA_ERROR = 4,
	CONNACK_NOT_AUTHORIZED_ERROR = 5
} MQTT_Connack_Return_Codes;    /**< Connect request response codes from server */


/**
  * Determines the length of the MQTT connect packet that would be produced using the supplied connect options.
  * @param options the options to be used to build the connect packet
  * @param the length of buffer needed to contain the serialized version of the packet
  * @return IoT_Error_t indicating function execution status
  */
static uint32_t _aws_iot_get_connect_packet_length(IoT_Client_Connect_Params *pConnectParams) {
	uint32_t len;
	/* Enable when adding further MQTT versions */
	/*size_t len = 0;
	switch(pConnectParams->MQTTVersion) {
		case MQTT_3_1_1:
			len = 10;
			break;
	}*/
	FUNC_ENTRY;

	len = 10; // Len = 10 for MQTT_3_1_1
	len = len + pConnectParams->clientIDLen + 2;

	if(pConnectParams->isWillMsgPresent) {
		len = len + pConnectParams->will.topicNameLen + 2 + pConnectParams->will.msgLen + 2;
	}

	if(NULL != pConnectParams->pUsername) {
		len = len + pConnectParams->usernameLen + 2;
	}

	if(NULL != pConnectParams->pPassword) {
		len = len + pConnectParams->passwordLen + 2;
	}

	FUNC_EXIT_RC(len);
}

/**
  * Serializes the connect options into the buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param len the length in bytes of the supplied buffer
  * @param options the options to be used to build the connect packet
  * @param serialized length
  * @return IoT_Error_t indicating function execution status
  */
static IoT_Error_t _aws_iot_mqtt_serialize_connect(unsigned char *pTxBuf, size_t txBufLen,
												   IoT_Client_Connect_Params *pConnectParams,
												   size_t *pSerializedLen) {
	unsigned char *ptr;
	uint32_t len;
	IoT_Error_t rc;
	MQTTHeader header = {0};
	MQTT_Connect_Header_Flags flags = {0};

	FUNC_ENTRY;

	if(NULL == pTxBuf || NULL == pConnectParams || NULL == pSerializedLen ||
	   (NULL == pConnectParams->pClientID && 0 != pConnectParams->clientIDLen) ||
	   (NULL != pConnectParams->pClientID && 0 == pConnectParams->clientIDLen)) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	/* Check needed here before we start writing to the Tx buffer */
	switch(pConnectParams->MQTTVersion) {
		case MQTT_3_1_1:
			break;
		default:
			return MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR;
	}

	ptr = pTxBuf;
	len = _aws_iot_get_connect_packet_length(pConnectParams);
	if(aws_iot_mqtt_internal_get_final_packet_length_from_remaining_length(len) > txBufLen) {
		FUNC_EXIT_RC(MQTT_TX_BUFFER_TOO_SHORT_ERROR);
	}

	rc = aws_iot_mqtt_internal_init_header(&header, CONNECT, QOS0, 0, 0);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	aws_iot_mqtt_internal_write_char(&ptr, header.byte); /* write header */

	ptr += aws_iot_mqtt_internal_write_len_to_buffer(ptr, len); /* write remaining length */

	// Enable if adding support for more versions
	//if(MQTT_3_1_1 == pConnectParams->MQTTVersion) {
	aws_iot_mqtt_internal_write_utf8_string(&ptr, "MQTT", 4);
	aws_iot_mqtt_internal_write_char(&ptr, (unsigned char) pConnectParams->MQTTVersion);
	//}

	flags.all = 0;
	if (pConnectParams->isCleanSession)
	{
		flags.all |= 1 << 1;
	}

	if (pConnectParams->isWillMsgPresent)
	{
		flags.all |= 1 << 2;
		flags.all |= pConnectParams->will.qos << 3;
		flags.all |= pConnectParams->will.isRetained << 5;
	}

	if(pConnectParams->pPassword) {
		flags.all |= 1 << 6;
	}

	if(pConnectParams->pUsername) {
		flags.all |= 1 << 7;
	}

	aws_iot_mqtt_internal_write_char(&ptr, flags.all);
	aws_iot_mqtt_internal_write_uint_16(&ptr, pConnectParams->keepAliveIntervalInSec);

	/* If the code have passed the check for incorrect values above, no client id was passed as argument */
	if(NULL == pConnectParams->pClientID) {
		aws_iot_mqtt_internal_write_uint_16(&ptr, 0);
	} else {
		aws_iot_mqtt_internal_write_utf8_string(&ptr, pConnectParams->pClientID, pConnectParams->clientIDLen);
	}

	if(pConnectParams->isWillMsgPresent) {
		aws_iot_mqtt_internal_write_utf8_string(&ptr, pConnectParams->will.pTopicName,
												pConnectParams->will.topicNameLen);
		aws_iot_mqtt_internal_write_utf8_string(&ptr, pConnectParams->will.pMessage, pConnectParams->will.msgLen);
	}

	if(pConnectParams->pUsername) {
		aws_iot_mqtt_internal_write_utf8_string(&ptr, pConnectParams->pUsername, pConnectParams->usernameLen);
	}

	if(pConnectParams->pPassword) {
		aws_iot_mqtt_internal_write_utf8_string(&ptr, pConnectParams->pPassword, pConnectParams->passwordLen);
	}

	*pSerializedLen = (size_t) (ptr - pTxBuf);

	FUNC_EXIT_RC(SUCCESS);
}

/**
  * Deserializes the supplied (wire) buffer into connack data - return code
  * @param sessionPresent the session present flag returned (only for MQTT 3.1.1)
  * @param connack_rc returned integer value of the connack return code
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return IoT_Error_t indicating function execution status
  */
static IoT_Error_t _aws_iot_mqtt_deserialize_connack(unsigned char *pSessionPresent, IoT_Error_t *pConnackRc,
													 unsigned char *pRxBuf, size_t rxBufLen) {
	unsigned char *curdata, *enddata;
	unsigned char connack_rc_char;
	uint32_t decodedLen, readBytesLen;
	IoT_Error_t rc;
	MQTT_Connack_Header_Flags flags = {0};
	MQTTHeader header = {0};

	FUNC_ENTRY;

	if(NULL == pSessionPresent || NULL == pConnackRc || NULL == pRxBuf) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	/* CONNACK header size is fixed at two bytes for fixed and 2 bytes for variable,
	 * using that as minimum size
	 * MQTT v3.1.1 Specification 3.2.1 */
	if(4 > rxBufLen) {
		FUNC_EXIT_RC(MQTT_RX_BUFFER_TOO_SHORT_ERROR);
	}

	curdata = pRxBuf;
	enddata = NULL;
	decodedLen = 0;
	readBytesLen = 0;

	header.byte = aws_iot_mqtt_internal_read_char(&curdata);
	if(CONNACK != MQTT_HEADER_FIELD_TYPE(header.byte)) {
		FUNC_EXIT_RC(FAILURE);
	}

	/* read remaining length */
	rc = aws_iot_mqtt_internal_decode_remaining_length_from_buffer(curdata, &decodedLen, &readBytesLen);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	/* CONNACK remaining length should always be 2 as per MQTT 3.1.1 spec */
	curdata += (readBytesLen);
	enddata = curdata + decodedLen;
	if(2 != (enddata - curdata)) {
		FUNC_EXIT_RC(MQTT_DECODE_REMAINING_LENGTH_ERROR);
	}

	flags.all = aws_iot_mqtt_internal_read_char(&curdata);
	*pSessionPresent = flags.bits.sessionpresent;
	connack_rc_char = aws_iot_mqtt_internal_read_char(&curdata);
	switch(connack_rc_char) {
		case CONNACK_CONNECTION_ACCEPTED:
			*pConnackRc = MQTT_CONNACK_CONNECTION_ACCEPTED;
			break;
		case CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR:
			*pConnackRc = MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR;
			break;
		case CONNACK_IDENTIFIER_REJECTED_ERROR:
			*pConnackRc = MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR;
			break;
		case CONNACK_SERVER_UNAVAILABLE_ERROR:
			*pConnackRc = MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR;
			break;
		case CONNACK_BAD_USERDATA_ERROR:
			*pConnackRc = MQTT_CONNACK_BAD_USERDATA_ERROR;
			break;
		case CONNACK_NOT_AUTHORIZED_ERROR:
			*pConnackRc = MQTT_CONNACK_NOT_AUTHORIZED_ERROR;
			break;
		default:
			*pConnackRc = MQTT_CONNACK_UNKNOWN_ERROR;
			break;
	}

	FUNC_EXIT_RC(SUCCESS);
}

/**
 * @brief Check if client state is valid for a connect request
 *
 * Called to check if client state is valid for a connect request
 * @param pClient Reference to the IoT Client
 *
 * @return bool true = state is valid, false = not valid
 */
static bool _aws_iot_mqtt_is_client_state_valid_for_connect(ClientState clientState) {
	bool isValid = false;

	switch(clientState) {
		case CLIENT_STATE_INVALID:
			isValid = false;
			break;
		case CLIENT_STATE_INITIALIZED:
			isValid = true;
			break;
		case CLIENT_STATE_CONNECTING:
		case CLIENT_STATE_CONNECTED_IDLE:
		case CLIENT_STATE_CONNECTED_YIELD_IN_PROGRESS:
		case CLIENT_STATE_CONNECTED_PUBLISH_IN_PROGRESS:
		case CLIENT_STATE_CONNECTED_SUBSCRIBE_IN_PROGRESS:
		case CLIENT_STATE_CONNECTED_UNSUBSCRIBE_IN_PROGRESS:
		case CLIENT_STATE_CONNECTED_RESUBSCRIBE_IN_PROGRESS:
		case CLIENT_STATE_CONNECTED_WAIT_FOR_CB_RETURN:
		case CLIENT_STATE_DISCONNECTING:
			isValid = false;
			break;
		case CLIENT_STATE_DISCONNECTED_ERROR:
		case CLIENT_STATE_DISCONNECTED_MANUALLY:
		case CLIENT_STATE_PENDING_RECONNECT:
			isValid = true;
			break;
		default:
			break;
	}

	return isValid;
}

/**
 * @brief MQTT Connection Function
 *
 * Called to establish an MQTT connection with the AWS IoT Service
 * This is the internal function which is called by the connect API to perform the operation.
 * Not meant to be called directly as it doesn't do validations or client state changes
 *
 * @param pClient Reference to the IoT Client
 * @param pConnectParams Pointer to MQTT connection parameters
 *
 * @return An IoT Error Type defining successful/failed connection
 */
static IoT_Error_t _aws_iot_mqtt_internal_connect(AWS_IoT_Client *pClient, const IoT_Client_Connect_Params *pConnectParams) {
	Timer connect_timer;
	IoT_Error_t connack_rc = FAILURE;
	char sessionPresent = 0;
	size_t len = 0;
	IoT_Error_t rc = FAILURE;

	FUNC_ENTRY;

	if(NULL != pConnectParams) {
		/* override default options if new options were supplied */
		rc = aws_iot_mqtt_set_connect_params(pClient, pConnectParams);
		if(SUCCESS != rc) {
			FUNC_EXIT_RC(MQTT_CONNECTION_ERROR);
		}
	}

	rc = pClient->networkStack.connect(&(pClient->networkStack), NULL);
	if(SUCCESS != rc) {
		/* TLS Connect failed, return error */
		FUNC_EXIT_RC(rc);
	}

	init_timer(&connect_timer);
	countdown_ms(&connect_timer, pClient->clientData.commandTimeoutMs);

	pClient->clientData.keepAliveInterval = pClient->clientData.options.keepAliveIntervalInSec;
	rc = _aws_iot_mqtt_serialize_connect(pClient->clientData.writeBuf, pClient->clientData.writeBufSize,
										 &(pClient->clientData.options), &len);
	if(SUCCESS != rc || 0 >= len) {
		FUNC_EXIT_RC(rc);
	}

	/* send the connect packet */
	rc = aws_iot_mqtt_internal_send_packet(pClient, len, &connect_timer);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	/* this will be a blocking call, wait for the CONNACK */
	rc = aws_iot_mqtt_internal_wait_for_read(pClient, CONNACK, &connect_timer);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	/* Received CONNACK, check the return code */
	rc = _aws_iot_mqtt_deserialize_connack((unsigned char *) &sessionPresent, &connack_rc, pClient->clientData.readBuf,
										   pClient->clientData.readBufSize);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	if(MQTT_CONNACK_CONNECTION_ACCEPTED != connack_rc) {
		FUNC_EXIT_RC(connack_rc);
	}

	pClient->clientStatus.isPingOutstanding = false;
	countdown_sec(&pClient->pingTimer, pClient->clientData.keepAliveInterval);

	FUNC_EXIT_RC(SUCCESS);
}

/**
 * @brief MQTT Connection Function
 *
 * Called to establish an MQTT connection with the AWS IoT Service
 * This is the outer function which does the validations and calls the internal connect above
 * to perform the actual operation. It is also responsible for client state changes
 *
 * @param pClient Reference to the IoT Client
 * @param pConnectParams Pointer to MQTT connection parameters
 *
 * @return An IoT Error Type defining successful/failed connection
 */
IoT_Error_t aws_iot_mqtt_connect(AWS_IoT_Client *pClient, const IoT_Client_Connect_Params *pConnectParams) {
	IoT_Error_t rc, disconRc;
	ClientState clientState;
	FUNC_ENTRY;

	if(NULL == pClient) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	clientState = aws_iot_mqtt_get_client_state(pClient);

	if(false == _aws_iot_mqtt_is_client_state_valid_for_connect(clientState)) {
		/* Don't send connect packet again if we are already connected
		 * or in the process of connecting/disconnecting */
		FUNC_EXIT_RC(NETWORK_ALREADY_CONNECTED_ERROR);
	}

	aws_iot_mqtt_set_client_state(pClient, clientState, CLIENT_STATE_CONNECTING);

	rc = _aws_iot_mqtt_internal_connect(pClient, pConnectParams);

	if(SUCCESS != rc) {
		pClient->networkStack.disconnect(&(pClient->networkStack));
		disconRc = pClient->networkStack.destroy(&(pClient->networkStack));
		if (SUCCESS != disconRc) {
			FUNC_EXIT_RC(NETWORK_DISCONNECTED_ERROR);
		}
		aws_iot_mqtt_set_client_state(pClient, CLIENT_STATE_CONNECTING, CLIENT_STATE_DISCONNECTED_ERROR);
	} else {
		aws_iot_mqtt_set_client_state(pClient, CLIENT_STATE_CONNECTING, CLIENT_STATE_CONNECTED_IDLE);
	}

	FUNC_EXIT_RC(rc);
}

/**
 * @brief Disconnect an MQTT Connection
 *
 * Called to send a disconnect message to the broker.
 * This is the internal function which is called by the disconnect API to perform the operation.
 * Not meant to be called directly as it doesn't do validations or client state changes
 *
 * @param pClient Reference to the IoT Client
 *
 * @return An IoT Error Type defining successful/failed send of the disconnect control packet.
 */
IoT_Error_t _aws_iot_mqtt_internal_disconnect(AWS_IoT_Client *pClient) {
	/* We might wait for incomplete incoming publishes to complete */
	Timer timer;
	size_t serialized_len = 0;
	IoT_Error_t rc;

	FUNC_ENTRY;

	rc = aws_iot_mqtt_internal_serialize_zero(pClient->clientData.writeBuf, pClient->clientData.writeBufSize,
											  DISCONNECT,
											  &serialized_len);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	init_timer(&timer);
	countdown_ms(&timer, pClient->clientData.commandTimeoutMs);

	/* send the disconnect packet */
	if(serialized_len > 0) {
		aws_iot_mqtt_internal_send_packet(pClient, serialized_len, &timer);
	}

	/* Clean network stack */
	pClient->networkStack.disconnect(&(pClient->networkStack));
	rc = pClient->networkStack.destroy(&(pClient->networkStack));
	if(0 != rc) {
		/* TLS Destroy failed, return error */
		FUNC_EXIT_RC(FAILURE);
	}

	FUNC_EXIT_RC(SUCCESS);
}

/**
 * @brief Disconnect an MQTT Connection
 *
 * Called to send a disconnect message to the broker.
 * This is the outer function which does the validations and calls the internal disconnect above
 * to perform the actual operation. It is also responsible for client state changes
 *
 * @param pClient Reference to the IoT Client
 *
 * @return An IoT Error Type defining successful/failed send of the disconnect control packet.
 */
IoT_Error_t aws_iot_mqtt_disconnect(AWS_IoT_Client *pClient) {
	ClientState clientState;
	IoT_Error_t rc;

	FUNC_ENTRY;

	if(NULL == pClient) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	clientState = aws_iot_mqtt_get_client_state(pClient);
	if(!aws_iot_mqtt_is_client_connected(pClient)) {
		/* Network is already disconnected. Do nothing */
		FUNC_EXIT_RC(NETWORK_DISCONNECTED_ERROR);
	}

	rc = aws_iot_mqtt_set_client_state(pClient, clientState, CLIENT_STATE_DISCONNECTING);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	rc = _aws_iot_mqtt_internal_disconnect(pClient);

	if(SUCCESS != rc) {
		pClient->clientStatus.clientState = clientState;
	} else {
		/* If called from Keepalive, this gets set to CLIENT_STATE_DISCONNECTED_ERROR */
		pClient->clientStatus.clientState = CLIENT_STATE_DISCONNECTED_MANUALLY;
	}

	FUNC_EXIT_RC(rc);
}

/**
 * @brief MQTT Manual Re-Connection Function
 *
 * Called to establish an MQTT connection with the AWS IoT Service
 * using parameters from the last time a connection was attempted
 * Use after disconnect to start the reconnect process manually
 * Makes only one reconnect attempt. Sets the client state to
 * pending reconnect in case of failure
 *
 * @param pClient Reference to the IoT Client
 *
 * @return An IoT Error Type defining successful/failed connection
 */
IoT_Error_t aws_iot_mqtt_attempt_reconnect(AWS_IoT_Client *pClient) {
	IoT_Error_t rc;

	FUNC_ENTRY;

	if(NULL == pClient) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	if(aws_iot_mqtt_is_client_connected(pClient)) {
		FUNC_EXIT_RC(NETWORK_ALREADY_CONNECTED_ERROR);
	}

	/* Ignoring return code. failures expected if network is disconnected */
	rc = aws_iot_mqtt_connect(pClient, NULL);

	/* If still disconnected handle disconnect */
	if(CLIENT_STATE_CONNECTED_IDLE != aws_iot_mqtt_get_client_state(pClient)) {
		aws_iot_mqtt_set_client_state(pClient, CLIENT_STATE_DISCONNECTED_ERROR, CLIENT_STATE_PENDING_RECONNECT);
		FUNC_EXIT_RC(NETWORK_ATTEMPTING_RECONNECT);
	}

	rc = aws_iot_mqtt_resubscribe(pClient);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	FUNC_EXIT_RC(NETWORK_RECONNECTED);
}

#ifdef __cplusplus
}
#endif
