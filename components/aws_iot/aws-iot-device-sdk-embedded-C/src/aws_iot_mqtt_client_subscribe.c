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
 * @file aws_iot_mqtt_client_subscribe.c
 * @brief MQTT client subscribe API definitions
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "aws_iot_mqtt_client_common_internal.h"

/**
  * Serializes the supplied subscribe data into the supplied buffer, ready for sending
  * @param pTxBuf the buffer into which the packet will be serialized
  * @param txBufLen the length in bytes of the supplied buffer
  * @param dup unsigned char - the MQTT dup flag
  * @param packetId uint16_t - the MQTT packet identifier
  * @param topicCount - number of members in the topicFilters and reqQos arrays
  * @param pTopicNameList - array of topic filter names
  * @param pTopicNameLenList - array of length of topic filter names
  * @param pRequestedQoSs - array of requested QoS
  * @param pSerializedLen - the length of the serialized data
  *
  * @return An IoT Error Type defining successful/failed operation
  */
static IoT_Error_t _aws_iot_mqtt_serialize_subscribe(unsigned char *pTxBuf, size_t txBufLen,
													 unsigned char dup, uint16_t packetId, uint32_t topicCount,
													 const char **pTopicNameList, uint16_t *pTopicNameLenList,
													 QoS *pRequestedQoSs, uint32_t *pSerializedLen) {
	unsigned char *ptr;
	uint32_t itr, rem_len;
	IoT_Error_t rc;
	MQTTHeader header = {0};

	FUNC_ENTRY;
	if(NULL == pTxBuf || NULL == pSerializedLen) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	ptr = pTxBuf;
	rem_len = 2; /* packetId */

	for(itr = 0; itr < topicCount; ++itr) {
		rem_len += (uint32_t) (pTopicNameLenList[itr] + 2 + 1); /* topic + length + req_qos */
	}

	if(aws_iot_mqtt_internal_get_final_packet_length_from_remaining_length(rem_len) > txBufLen) {
		FUNC_EXIT_RC(MQTT_TX_BUFFER_TOO_SHORT_ERROR);
	}

	rc = aws_iot_mqtt_internal_init_header(&header, SUBSCRIBE, QOS1, dup, 0);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}
	/* write header */
	aws_iot_mqtt_internal_write_char(&ptr, header.byte);

	/* write remaining length */
	ptr += aws_iot_mqtt_internal_write_len_to_buffer(ptr, rem_len);

	aws_iot_mqtt_internal_write_uint_16(&ptr, packetId);

	for(itr = 0; itr < topicCount; ++itr) {
		aws_iot_mqtt_internal_write_utf8_string(&ptr, pTopicNameList[itr], pTopicNameLenList[itr]);
		aws_iot_mqtt_internal_write_char(&ptr, (unsigned char) pRequestedQoSs[itr]);
	}

	*pSerializedLen = (uint32_t) (ptr - pTxBuf);

	FUNC_EXIT_RC(SUCCESS);
}

/**
  * Deserializes the supplied (wire) buffer into suback data
  * @param pPacketId returned integer - the MQTT packet identifier
  * @param maxExpectedQoSCount - the maximum number of members allowed in the grantedQoSs array
  * @param pGrantedQoSCount returned uint32_t - number of members in the grantedQoSs array
  * @param pGrantedQoSs returned array of QoS type - the granted qualities of service
  * @param pRxBuf the raw buffer data, of the correct length determined by the remaining length field
  * @param rxBufLen the length in bytes of the data in the supplied buffer
  *
  * @return An IoT Error Type defining successful/failed operation
  */
static IoT_Error_t _aws_iot_mqtt_deserialize_suback(uint16_t *pPacketId, uint32_t maxExpectedQoSCount,
													uint32_t *pGrantedQoSCount, QoS *pGrantedQoSs,
													unsigned char *pRxBuf, size_t rxBufLen) {
	unsigned char *curData, *endData;
	uint32_t decodedLen, readBytesLen;
	IoT_Error_t decodeRc;
	MQTTHeader header = {0};

	FUNC_ENTRY;
	if(NULL == pPacketId || NULL == pGrantedQoSCount || NULL == pGrantedQoSs) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	curData = pRxBuf;
	endData = NULL;
	decodeRc = FAILURE;
	decodedLen = 0;
	readBytesLen = 0;

	/* SUBACK header size is 4 bytes for header and at least one byte for QoS payload
	 * Need at least a 5 bytes buffer. MQTT3.1.1 specification 3.9
	 */
	if(5 > rxBufLen) {
		FUNC_EXIT_RC(MQTT_RX_BUFFER_TOO_SHORT_ERROR);
	}

	header.byte = aws_iot_mqtt_internal_read_char(&curData);
	if(SUBACK != MQTT_HEADER_FIELD_TYPE(header.byte)) {
		FUNC_EXIT_RC(FAILURE);
	}

	/* read remaining length */
	decodeRc = aws_iot_mqtt_internal_decode_remaining_length_from_buffer(curData, &decodedLen, &readBytesLen);
	if(SUCCESS != decodeRc) {
		FUNC_EXIT_RC(decodeRc);
	}

	curData += (readBytesLen);
	endData = curData + decodedLen;
	if(endData - curData < 2) {
		FUNC_EXIT_RC(FAILURE);
	}

	*pPacketId = aws_iot_mqtt_internal_read_uint16_t(&curData);

	*pGrantedQoSCount = 0;
	while(curData < endData) {
		if(*pGrantedQoSCount > maxExpectedQoSCount) {
			FUNC_EXIT_RC(FAILURE);
		}
		pGrantedQoSs[(*pGrantedQoSCount)++] = (QoS) aws_iot_mqtt_internal_read_char(&curData);
	}

	FUNC_EXIT_RC(SUCCESS);
}

/* Returns MAX_MESSAGE_HANDLERS value if no free index is available */
static uint32_t _aws_iot_mqtt_get_free_message_handler_index(AWS_IoT_Client *pClient) {
	uint32_t itr;

	FUNC_ENTRY;

	for(itr = 0; itr < AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS; itr++) {
		if(pClient->clientData.messageHandlers[itr].topicName == NULL) {
			break;
		}
	}

	FUNC_EXIT_RC(itr);
}

/**
 * @brief Subscribe to an MQTT topic.
 *
 * Called to send a subscribe message to the broker requesting a subscription
 * to an MQTT topic. This is the internal function which is called by the
 * subscribe API to perform the operation. Not meant to be called directly as
 * it doesn't do validations or client state changes
 * @note Call is blocking.  The call returns after the receipt of the SUBACK control packet.
 *
 * @param pClient Reference to the IoT Client
 * @param pTopicName Topic Name to publish to
 * @param topicNameLen Length of the topic name
 * @param pApplicationHandler_t Reference to the handler function for this subscription
 *
 * @return An IoT Error Type defining successful/failed subscription
 */
static IoT_Error_t _aws_iot_mqtt_internal_subscribe(AWS_IoT_Client *pClient, const char *pTopicName,
													uint16_t topicNameLen, QoS qos,
													pApplicationHandler_t pApplicationHandler,
													void *pApplicationHandlerData) {
	uint16_t txPacketId, rxPacketId;
	uint32_t serializedLen, indexOfFreeMessageHandler, count;
	IoT_Error_t rc;
	Timer timer;
	QoS grantedQoS[3] = {QOS0, QOS0, QOS0};

	FUNC_ENTRY;
	init_timer(&timer);
	countdown_ms(&timer, pClient->clientData.commandTimeoutMs);

	serializedLen = 0;
	count = 0;
	txPacketId = aws_iot_mqtt_get_next_packet_id(pClient);
	rxPacketId = 0;

	rc = _aws_iot_mqtt_serialize_subscribe(pClient->clientData.writeBuf, pClient->clientData.writeBufSize, 0,
										   txPacketId, 1, &pTopicName, &topicNameLen, &qos, &serializedLen);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	indexOfFreeMessageHandler = _aws_iot_mqtt_get_free_message_handler_index(pClient);
	if(AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS <= indexOfFreeMessageHandler) {
		FUNC_EXIT_RC(MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR);
	}

	/* send the subscribe packet */
	rc = aws_iot_mqtt_internal_send_packet(pClient, serializedLen, &timer);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	/* wait for suback */
	rc = aws_iot_mqtt_internal_wait_for_read(pClient, SUBACK, &timer);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	/* Granted QoS can be 0, 1 or 2 */
	rc = _aws_iot_mqtt_deserialize_suback(&rxPacketId, 1, &count, grantedQoS, pClient->clientData.readBuf,
										  pClient->clientData.readBufSize);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	/* TODO : Figure out how to test this before activating this check */
	//if(txPacketId != rxPacketId) {
	/* Different SUBACK received than expected. Return error
	 * This can cause issues if the request timeout value is too small */
	//	return RX_MESSAGE_INVALID_ERROR;
	//}

	pClient->clientData.messageHandlers[indexOfFreeMessageHandler].topicName =
			pTopicName;
	pClient->clientData.messageHandlers[indexOfFreeMessageHandler].topicNameLen =
			topicNameLen;
	pClient->clientData.messageHandlers[indexOfFreeMessageHandler].pApplicationHandler =
			pApplicationHandler;
	pClient->clientData.messageHandlers[indexOfFreeMessageHandler].pApplicationHandlerData =
			pApplicationHandlerData;
	pClient->clientData.messageHandlers[indexOfFreeMessageHandler].qos = qos;

	FUNC_EXIT_RC(SUCCESS);
}

/**
 * @brief Subscribe to an MQTT topic.
 *
 * Called to send a subscribe message to the broker requesting a subscription
 * to an MQTT topic. This is the outer function which does the validations and
 * calls the internal subscribe above to perform the actual operation.
 * It is also responsible for client state changes
 * @note Call is blocking.  The call returns after the receipt of the SUBACK control packet.
 *
 * @param pClient Reference to the IoT Client
 * @param pTopicName Topic Name to publish to
 * @param topicNameLen Length of the topic name
 * @param pApplicationHandler_t Reference to the handler function for this subscription
 *
 * @return An IoT Error Type defining successful/failed subscription
 */
IoT_Error_t aws_iot_mqtt_subscribe(AWS_IoT_Client *pClient, const char *pTopicName, uint16_t topicNameLen,
								   QoS qos, pApplicationHandler_t pApplicationHandler, void *pApplicationHandlerData) {
	ClientState clientState;
	IoT_Error_t rc, subRc;

	FUNC_ENTRY;

	if(NULL == pClient || NULL == pTopicName || NULL == pApplicationHandler) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	if(!aws_iot_mqtt_is_client_connected(pClient)) {
		FUNC_EXIT_RC(NETWORK_DISCONNECTED_ERROR);
	}

	clientState = aws_iot_mqtt_get_client_state(pClient);
	if(CLIENT_STATE_CONNECTED_IDLE != clientState && CLIENT_STATE_CONNECTED_WAIT_FOR_CB_RETURN != clientState) {
		FUNC_EXIT_RC(MQTT_CLIENT_NOT_IDLE_ERROR);
	}

	rc = aws_iot_mqtt_set_client_state(pClient, clientState, CLIENT_STATE_CONNECTED_SUBSCRIBE_IN_PROGRESS);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	subRc = _aws_iot_mqtt_internal_subscribe(pClient, pTopicName, topicNameLen, qos,
											 pApplicationHandler, pApplicationHandlerData);

	rc = aws_iot_mqtt_set_client_state(pClient, CLIENT_STATE_CONNECTED_SUBSCRIBE_IN_PROGRESS, clientState);
	if(SUCCESS == subRc && SUCCESS != rc) {
		subRc = rc;
	}

	FUNC_EXIT_RC(subRc);
}

/**
 * @brief Subscribe to an MQTT topic.
 *
 * Called to send a subscribe message to the broker requesting a subscription
 * to an MQTT topic.
 * This is the internal function which is called by the resubscribe API to perform the operation.
 * Not meant to be called directly as it doesn't do validations or client state changes
 * @note Call is blocking.  The call returns after the receipt of the SUBACK control packet.
 *
 * @param pClient Reference to the IoT Client
 *
 * @return An IoT Error Type defining successful/failed subscription
 */
static IoT_Error_t _aws_iot_mqtt_internal_resubscribe(AWS_IoT_Client *pClient) {
	uint16_t packetId;
	uint32_t len, count, existingSubCount, itr;
	IoT_Error_t rc;
	Timer timer;
	QoS grantedQoS[3] = {QOS0, QOS0, QOS0};

	FUNC_ENTRY;

	packetId = 0;
	len = 0;
	count = 0;
	existingSubCount = _aws_iot_mqtt_get_free_message_handler_index(pClient);

	for(itr = 0; itr < existingSubCount; itr++) {
		if(pClient->clientData.messageHandlers[itr].topicName == NULL) {
			continue;
		}

		init_timer(&timer);
		countdown_ms(&timer, pClient->clientData.commandTimeoutMs);

		rc = _aws_iot_mqtt_serialize_subscribe(pClient->clientData.writeBuf, pClient->clientData.writeBufSize, 0,
											   aws_iot_mqtt_get_next_packet_id(pClient), 1,
											   &(pClient->clientData.messageHandlers[itr].topicName),
											   &(pClient->clientData.messageHandlers[itr].topicNameLen),
											   &(pClient->clientData.messageHandlers[itr].qos), &len);
		if(SUCCESS != rc) {
			FUNC_EXIT_RC(rc);
		}

		/* send the subscribe packet */
		rc = aws_iot_mqtt_internal_send_packet(pClient, len, &timer);
		if(SUCCESS != rc) {
			FUNC_EXIT_RC(rc);
		}

		/* wait for suback */
		rc = aws_iot_mqtt_internal_wait_for_read(pClient, SUBACK, &timer);
		if(SUCCESS != rc) {
			FUNC_EXIT_RC(rc);
		}

		/* Granted QoS can be 0, 1 or 2 */
		rc = _aws_iot_mqtt_deserialize_suback(&packetId, 1, &count, grantedQoS, pClient->clientData.readBuf,
											  pClient->clientData.readBufSize);
		if(SUCCESS != rc) {
			FUNC_EXIT_RC(rc);
		}
	}

	FUNC_EXIT_RC(SUCCESS);
}

/**
 * @brief Subscribe to an MQTT topic.
 *
 * Called to send a subscribe message to the broker requesting a subscription
 * to an MQTT topic.
 * This is the outer function which does the validations and calls the internal resubscribe above
 * to perform the actual operation. It is also responsible for client state changes
 * @note Call is blocking.  The call returns after the receipt of the SUBACK control packet.
 *
 * @param pClient Reference to the IoT Client
 *
 * @return An IoT Error Type defining successful/failed subscription
 */
IoT_Error_t aws_iot_mqtt_resubscribe(AWS_IoT_Client *pClient) {
	IoT_Error_t rc, resubRc;

	FUNC_ENTRY;

	if(NULL == pClient) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	if(false == aws_iot_mqtt_is_client_connected(pClient)) {
		FUNC_EXIT_RC(NETWORK_DISCONNECTED_ERROR);
	}

	if(CLIENT_STATE_CONNECTED_IDLE != aws_iot_mqtt_get_client_state(pClient)) {
		FUNC_EXIT_RC(MQTT_CLIENT_NOT_IDLE_ERROR);
	}

	rc = aws_iot_mqtt_set_client_state(pClient, CLIENT_STATE_CONNECTED_IDLE,
									   CLIENT_STATE_CONNECTED_RESUBSCRIBE_IN_PROGRESS);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	resubRc = _aws_iot_mqtt_internal_resubscribe(pClient);

	rc = aws_iot_mqtt_set_client_state(pClient, CLIENT_STATE_CONNECTED_RESUBSCRIBE_IN_PROGRESS,
									   CLIENT_STATE_CONNECTED_IDLE);
	if(SUCCESS == resubRc && SUCCESS != rc) {
		resubRc = rc;
	}

	FUNC_EXIT_RC(resubRc);
}

#ifdef __cplusplus
}
#endif

