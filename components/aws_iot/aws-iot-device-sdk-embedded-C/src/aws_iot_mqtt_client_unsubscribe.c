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
 * @file aws_iot_mqtt_client_unsubscribe.c
 * @brief MQTT client unsubscribe API definitions
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "aws_iot_mqtt_client_common_internal.h"

/**
  * Serializes the supplied unsubscribe data into the supplied buffer, ready for sending
  * @param pTxBuf the raw buffer data, of the correct length determined by the remaining length field
  * @param txBufLen the length in bytes of the data in the supplied buffer
  * @param dup integer - the MQTT dup flag
  * @param packetId integer - the MQTT packet identifier
  * @param count - number of members in the topicFilters array
  * @param pTopicNameList - array of topic filter names
  * @param pTopicNameLenList - array of length of topic filter names in pTopicNameList
  * @param pSerializedLen - the length of the serialized data
  * @return IoT_Error_t indicating function execution status
  */
static IoT_Error_t _aws_iot_mqtt_serialize_unsubscribe(unsigned char *pTxBuf, size_t txBufLen,
													   uint8_t dup, uint16_t packetId,
													   uint32_t count, const char **pTopicNameList,
													   uint16_t *pTopicNameLenList, uint32_t *pSerializedLen) {
	unsigned char *ptr = pTxBuf;
	uint32_t i = 0;
	uint32_t rem_len = 2; /* packetId */
	IoT_Error_t rc;
	MQTTHeader header = {0};

	FUNC_ENTRY;

	for(i = 0; i < count; ++i) {
		rem_len += (uint32_t) (pTopicNameLenList[i] + 2); /* topic + length */
	}

	if(aws_iot_mqtt_internal_get_final_packet_length_from_remaining_length(rem_len) > txBufLen) {
		FUNC_EXIT_RC(MQTT_TX_BUFFER_TOO_SHORT_ERROR);
	}

	rc = aws_iot_mqtt_internal_init_header(&header, UNSUBSCRIBE, QOS1, dup, 0);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}
	aws_iot_mqtt_internal_write_char(&ptr, header.byte); /* write header */

	ptr += aws_iot_mqtt_internal_write_len_to_buffer(ptr, rem_len); /* write remaining length */

	aws_iot_mqtt_internal_write_uint_16(&ptr, packetId);

	for(i = 0; i < count; ++i) {
		aws_iot_mqtt_internal_write_utf8_string(&ptr, pTopicNameList[i], pTopicNameLenList[i]);
	}

	*pSerializedLen = (uint32_t) (ptr - pTxBuf);

	FUNC_EXIT_RC(SUCCESS);
}


/**
  * Deserializes the supplied (wire) buffer into unsuback data
  * @param pPacketId returned integer - the MQTT packet identifier
  * @param pRxBuf the raw buffer data, of the correct length determined by the remaining length field
  * @param rxBufLen the length in bytes of the data in the supplied buffer
  * @return IoT_Error_t indicating function execution status
  */
static IoT_Error_t _aws_iot_mqtt_deserialize_unsuback(uint16_t *pPacketId, unsigned char *pRxBuf, size_t rxBufLen) {
	unsigned char type = 0;
	unsigned char dup = 0;
	IoT_Error_t rc;

	FUNC_ENTRY;

	rc = aws_iot_mqtt_internal_deserialize_ack(&type, &dup, pPacketId, pRxBuf, rxBufLen);
	if(SUCCESS == rc && UNSUBACK != type) {
		rc = FAILURE;
	}

	FUNC_EXIT_RC(rc);
}

/**
 * @brief Unsubscribe to an MQTT topic.
 *
 * Called to send an unsubscribe message to the broker requesting removal of a subscription
 * to an MQTT topic.
 * @note Call is blocking.  The call returns after the receipt of the UNSUBACK control packet.
 * This is the internal function which is called by the unsubscribe API to perform the operation.
 * Not meant to be called directly as it doesn't do validations or client state changes
 *
 * @param pClient Reference to the IoT Client
 * @param pTopicName Topic Name to publish to
 * @param topicNameLen Length of the topic name
 *
 * @return An IoT Error Type defining successful/failed unsubscribe call
 */
static IoT_Error_t _aws_iot_mqtt_internal_unsubscribe(AWS_IoT_Client *pClient, const char *pTopicFilter,
													  uint16_t topicFilterLen) {
	/* No NULL checks because this is a static internal function */

	Timer timer;

	uint16_t packet_id;
	uint32_t serializedLen = 0;
	uint32_t i = 0;
	IoT_Error_t rc;
	bool subscriptionExists = false;

	FUNC_ENTRY;

	/* Remove from message handler array */
	for(i = 0; i < AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS; ++i) {
		if(pClient->clientData.messageHandlers[i].topicName != NULL &&
		   (strcmp(pClient->clientData.messageHandlers[i].topicName, pTopicFilter) == 0)) {
			subscriptionExists = true;
		}
	}

	if(false == subscriptionExists) {
		FUNC_EXIT_RC(FAILURE);
	}

	init_timer(&timer);
	countdown_ms(&timer, pClient->clientData.commandTimeoutMs);

	rc = _aws_iot_mqtt_serialize_unsubscribe(pClient->clientData.writeBuf, pClient->clientData.writeBufSize, 0,
											 aws_iot_mqtt_get_next_packet_id(pClient), 1, &pTopicFilter,
											 &topicFilterLen, &serializedLen);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	/* send the unsubscribe packet */
	rc = aws_iot_mqtt_internal_send_packet(pClient, serializedLen, &timer);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	rc = aws_iot_mqtt_internal_wait_for_read(pClient, UNSUBACK, &timer);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	rc = _aws_iot_mqtt_deserialize_unsuback(&packet_id, pClient->clientData.readBuf, pClient->clientData.readBufSize);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	/* Remove from message handler array */
	for(i = 0; i < AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS; ++i) {
		if(pClient->clientData.messageHandlers[i].topicName != NULL &&
		   (strcmp(pClient->clientData.messageHandlers[i].topicName, pTopicFilter) == 0)) {
			pClient->clientData.messageHandlers[i].topicName = NULL;
			/* We don't want to break here, in case the same topic is registered
             * with 2 callbacks. Unlikely scenario */
		}
	}

	FUNC_EXIT_RC(SUCCESS);
}

/**
 * @brief Unsubscribe to an MQTT topic.
 *
 * Called to send an unsubscribe message to the broker requesting removal of a subscription
 * to an MQTT topic.
 * @note Call is blocking.  The call returns after the receipt of the UNSUBACK control packet.
 * This is the outer function which does the validations and calls the internal unsubscribe above
 * to perform the actual operation. It is also responsible for client state changes
 *
 * @param pClient Reference to the IoT Client
 * @param pTopicName Topic Name to publish to
 * @param topicNameLen Length of the topic name
 *
 * @return An IoT Error Type defining successful/failed unsubscribe call
 */
IoT_Error_t aws_iot_mqtt_unsubscribe(AWS_IoT_Client *pClient, const char *pTopicFilter, uint16_t topicFilterLen) {
	IoT_Error_t rc, unsubRc;
	ClientState clientState;

	if(NULL == pClient || NULL == pTopicFilter) {
		return NULL_VALUE_ERROR;
	}

	if(!aws_iot_mqtt_is_client_connected(pClient)) {
		return NETWORK_DISCONNECTED_ERROR;
	}

	clientState = aws_iot_mqtt_get_client_state(pClient);
	if(CLIENT_STATE_CONNECTED_IDLE != clientState && CLIENT_STATE_CONNECTED_WAIT_FOR_CB_RETURN != clientState) {
		return MQTT_CLIENT_NOT_IDLE_ERROR;
	}

	rc = aws_iot_mqtt_set_client_state(pClient, clientState, CLIENT_STATE_CONNECTED_UNSUBSCRIBE_IN_PROGRESS);
	if(SUCCESS != rc) {
		rc = aws_iot_mqtt_set_client_state(pClient, CLIENT_STATE_CONNECTED_UNSUBSCRIBE_IN_PROGRESS, clientState);
		return rc;
	}

	unsubRc = _aws_iot_mqtt_internal_unsubscribe(pClient, pTopicFilter, topicFilterLen);

	rc = aws_iot_mqtt_set_client_state(pClient, CLIENT_STATE_CONNECTED_UNSUBSCRIBE_IN_PROGRESS, clientState);
	if(SUCCESS == unsubRc && SUCCESS != rc) {
		unsubRc = rc;
	}

	return unsubRc;
}

#ifdef __cplusplus
}
#endif

