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
 *    Ian Craggs - fix for https://bugs.eclipse.org/bugs/show_bug.cgi?id=453144
 *******************************************************************************/

/**
 * @file aws_iot_mqtt_client_publish.c
 * @brief MQTT client publish API definitions
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "aws_iot_mqtt_client_common_internal.h"

/**
 * @param stringVar pointer to the String into which the data is to be read
 * @param stringLen pointer to variable which has the length of the string
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param enddata pointer to the end of the data: do not read beyond
 * @return SUCCESS if successful, FAILURE if not
 */
static IoT_Error_t _aws_iot_mqtt_read_string_with_len(char **stringVar, uint16_t *stringLen,
													  unsigned char **pptr, unsigned char *enddata) {
	IoT_Error_t rc = FAILURE;

	FUNC_ENTRY;
	/* the first two bytes are the length of the string */
	/* enough length to read the integer? */
	if(enddata - (*pptr) > 1) {
		*stringLen = aws_iot_mqtt_internal_read_uint16_t(pptr); /* increments pptr to point past length */
		if(&(*pptr)[*stringLen] <= enddata) {
			*stringVar = (char *) *pptr;
			*pptr += *stringLen;
			rc = SUCCESS;
		}
	}

	FUNC_EXIT_RC(rc);
}

/**
  * Serializes the supplied publish data into the supplied buffer, ready for sending
  * @param pTxBuf the buffer into which the packet will be serialized
  * @param txBufLen the length in bytes of the supplied buffer
  * @param dup uint8_t - the MQTT dup flag
  * @param qos QoS - the MQTT QoS value
  * @param retained uint8_t - the MQTT retained flag
  * @param packetId uint16_t - the MQTT packet identifier
  * @param pTopicName char * - the MQTT topic in the publish
  * @param topicNameLen uint16_t - the length of the Topic Name
  * @param pPayload byte buffer - the MQTT publish payload
  * @param payloadLen size_t - the length of the MQTT payload
  * @param pSerializedLen uint32_t - pointer to the variable that stores serialized len
  *
  * @return An IoT Error Type defining successful/failed call
  */
static IoT_Error_t _aws_iot_mqtt_internal_serialize_publish(unsigned char *pTxBuf, size_t txBufLen, uint8_t dup,
															QoS qos, uint8_t retained, uint16_t packetId,
															const char *pTopicName, uint16_t topicNameLen,
															const unsigned char *pPayload, size_t payloadLen,
															uint32_t *pSerializedLen) {
	unsigned char *ptr;
	uint32_t rem_len;
	IoT_Error_t rc;
	MQTTHeader header = {0};

	FUNC_ENTRY;
	if(NULL == pTxBuf || NULL == pPayload || NULL == pSerializedLen) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	ptr = pTxBuf;
	rem_len = 0;

	rem_len += (uint32_t) (topicNameLen + payloadLen + 2);
	if(qos > 0) {
		rem_len += 2; /* packetId */
	}
	if(aws_iot_mqtt_internal_get_final_packet_length_from_remaining_length(rem_len) > txBufLen) {
		FUNC_EXIT_RC(MQTT_TX_BUFFER_TOO_SHORT_ERROR);
	}

	rc = aws_iot_mqtt_internal_init_header(&header, PUBLISH, qos, dup, retained);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}
	aws_iot_mqtt_internal_write_char(&ptr, header.byte); /* write header */

	ptr += aws_iot_mqtt_internal_write_len_to_buffer(ptr, rem_len); /* write remaining length */;

	aws_iot_mqtt_internal_write_utf8_string(&ptr, pTopicName, topicNameLen);

	if(qos > 0) {
		aws_iot_mqtt_internal_write_uint_16(&ptr, packetId);
	}

	memcpy(ptr, pPayload, payloadLen);
	ptr += payloadLen;

	*pSerializedLen = (uint32_t) (ptr - pTxBuf);

	FUNC_EXIT_RC(SUCCESS);
}

/**
  * Serializes the ack packet into the supplied buffer.
  * @param pTxBuf the buffer into which the packet will be serialized
  * @param txBufLen the length in bytes of the supplied buffer
  * @param msgType the MQTT packet type
  * @param dup the MQTT dup flag
  * @param packetId the MQTT packet identifier
  * @param pSerializedLen uint32_t - pointer to the variable that stores serialized len
  *
  * @return An IoT Error Type defining successful/failed call
  */
IoT_Error_t aws_iot_mqtt_internal_serialize_ack(unsigned char *pTxBuf, size_t txBufLen,
												MessageTypes msgType, uint8_t dup, uint16_t packetId,
												uint32_t *pSerializedLen) {
	unsigned char *ptr;
	QoS requestQoS;
	IoT_Error_t rc;
	MQTTHeader header = {0};
	FUNC_ENTRY;
	if(NULL == pTxBuf || pSerializedLen == NULL) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	ptr = pTxBuf;

	/* Minimum byte length required by ACK headers is
	 * 2 for fixed and 2 for variable part */
	if(4 > txBufLen) {
		FUNC_EXIT_RC(MQTT_TX_BUFFER_TOO_SHORT_ERROR);
	}

	requestQoS = (PUBREL == msgType) ? QOS1 : QOS0;
	rc = aws_iot_mqtt_internal_init_header(&header, msgType, requestQoS, dup, 0);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}
	aws_iot_mqtt_internal_write_char(&ptr, header.byte); /* write header */

	ptr += aws_iot_mqtt_internal_write_len_to_buffer(ptr, 2); /* write remaining length */
	aws_iot_mqtt_internal_write_uint_16(&ptr, packetId);
	*pSerializedLen = (uint32_t) (ptr - pTxBuf);

	FUNC_EXIT_RC(SUCCESS);
}

/**
 * @brief Publish an MQTT message on a topic
 *
 * Called to publish an MQTT message on a topic.
 * @note Call is blocking.  In the case of a QoS 0 message the function returns
 * after the message was successfully passed to the TLS layer.  In the case of QoS 1
 * the function returns after the receipt of the PUBACK control packet.
 * This is the internal function which is called by the publish API to perform the operation.
 * Not meant to be called directly as it doesn't do validations or client state changes
 *
 * @param pClient Reference to the IoT Client
 * @param pTopicName Topic Name to publish to
 * @param topicNameLen Length of the topic name
 * @param pParams Pointer to Publish Message parameters
 *
 * @return An IoT Error Type defining successful/failed publish
 */
static IoT_Error_t _aws_iot_mqtt_internal_publish(AWS_IoT_Client *pClient, const char *pTopicName,
												  uint16_t topicNameLen, IoT_Publish_Message_Params *pParams) {
	Timer timer;
	uint32_t len = 0;
	uint16_t packet_id;
	unsigned char dup, type;
	IoT_Error_t rc;

	FUNC_ENTRY;

	init_timer(&timer);
	countdown_ms(&timer, pClient->clientData.commandTimeoutMs);

	if(QOS1 == pParams->qos) {
		pParams->id = aws_iot_mqtt_get_next_packet_id(pClient);
	}

	rc = _aws_iot_mqtt_internal_serialize_publish(pClient->clientData.writeBuf, pClient->clientData.writeBufSize, 0,
												  pParams->qos, pParams->isRetained, pParams->id, pTopicName,
												  topicNameLen, (unsigned char *) pParams->payload,
												  pParams->payloadLen, &len);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	/* send the publish packet */
	rc = aws_iot_mqtt_internal_send_packet(pClient, len, &timer);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	/* Wait for ack if QoS1 */
	if(QOS1 == pParams->qos) {
		rc = aws_iot_mqtt_internal_wait_for_read(pClient, PUBACK, &timer);
		if(SUCCESS != rc) {
			FUNC_EXIT_RC(rc);
		}

		rc = aws_iot_mqtt_internal_deserialize_ack(&type, &dup, &packet_id, pClient->clientData.readBuf,
												   pClient->clientData.readBufSize);
		if(SUCCESS != rc) {
			FUNC_EXIT_RC(rc);
		}
	}

	FUNC_EXIT_RC(SUCCESS);
}

/**
 * @brief Publish an MQTT message on a topic
 *
 * Called to publish an MQTT message on a topic.
 * @note Call is blocking.  In the case of a QoS 0 message the function returns
 * after the message was successfully passed to the TLS layer.  In the case of QoS 1
 * the function returns after the receipt of the PUBACK control packet.
 * This is the outer function which does the validations and calls the internal publish above
 * to perform the actual operation. It is also responsible for client state changes
 *
 * @param pClient Reference to the IoT Client
 * @param pTopicName Topic Name to publish to
 * @param topicNameLen Length of the topic name
 * @param pParams Pointer to Publish Message parameters
 *
 * @return An IoT Error Type defining successful/failed publish
 */
IoT_Error_t aws_iot_mqtt_publish(AWS_IoT_Client *pClient, const char *pTopicName, uint16_t topicNameLen,
								 IoT_Publish_Message_Params *pParams) {
	IoT_Error_t rc, pubRc;
	ClientState clientState;

	FUNC_ENTRY;

	if(NULL == pClient || NULL == pTopicName || 0 == topicNameLen || NULL == pParams) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	if(!aws_iot_mqtt_is_client_connected(pClient)) {
		FUNC_EXIT_RC(NETWORK_DISCONNECTED_ERROR);
	}

	clientState = aws_iot_mqtt_get_client_state(pClient);
	if(CLIENT_STATE_CONNECTED_IDLE != clientState && CLIENT_STATE_CONNECTED_WAIT_FOR_CB_RETURN != clientState) {
		FUNC_EXIT_RC(MQTT_CLIENT_NOT_IDLE_ERROR);
	}

	rc = aws_iot_mqtt_set_client_state(pClient, clientState, CLIENT_STATE_CONNECTED_PUBLISH_IN_PROGRESS);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	pubRc = _aws_iot_mqtt_internal_publish(pClient, pTopicName, topicNameLen, pParams);

	rc = aws_iot_mqtt_set_client_state(pClient, CLIENT_STATE_CONNECTED_PUBLISH_IN_PROGRESS, clientState);
	if(SUCCESS == pubRc && SUCCESS != rc) {
		pubRc = rc;
	}

	FUNC_EXIT_RC(pubRc);
}

/**
  * Deserializes the supplied (wire) buffer into publish data
  * @param dup returned uint8_t - the MQTT dup flag
  * @param qos returned QoS type - the MQTT QoS value
  * @param retained returned uint8_t - the MQTT retained flag
  * @param pPacketId returned uint16_t - the MQTT packet identifier
  * @param pTopicName returned String - the MQTT topic in the publish
  * @param topicNameLen returned uint16_t - the length of the MQTT topic in the publish
  * @param payload returned byte buffer - the MQTT publish payload
  * @param payloadlen returned size_t - the length of the MQTT payload
  * @param pRxBuf the raw buffer data, of the correct length determined by the remaining length field
  * @param rxBufLen the length in bytes of the data in the supplied buffer
  *
  * @return An IoT Error Type defining successful/failed call
  */
IoT_Error_t aws_iot_mqtt_internal_deserialize_publish(uint8_t *dup, QoS *qos,
													  uint8_t *retained, uint16_t *pPacketId,
													  char **pTopicName, uint16_t *topicNameLen,
													  unsigned char **payload, size_t *payloadLen,
													  unsigned char *pRxBuf, size_t rxBufLen) {
	unsigned char *curData = pRxBuf;
	unsigned char *endData = NULL;
	IoT_Error_t rc = FAILURE;
	uint32_t decodedLen = 0;
	uint32_t readBytesLen = 0;
	MQTTHeader header = {0};

	FUNC_ENTRY;

	if(NULL == dup || NULL == qos || NULL == retained || NULL == pPacketId) {
		FUNC_EXIT_RC(FAILURE);
	}

	/* Publish header size is at least four bytes.
	 * Fixed header is two bytes.
	 * Variable header size depends on QoS And Topic Name.
	 * QoS level 0 doesn't have a message identifier (0 - 2 bytes)
	 * Topic Name length fields decide size of topic name field (at least 2 bytes)
	 * MQTT v3.1.1 Specification 3.3.1 */
	if(4 > rxBufLen) {
		FUNC_EXIT_RC(MQTT_RX_BUFFER_TOO_SHORT_ERROR);
	}

	header.byte = aws_iot_mqtt_internal_read_char(&curData);
	if(PUBLISH != MQTT_HEADER_FIELD_TYPE(header.byte)) {
		FUNC_EXIT_RC(FAILURE);
	}

	*dup = MQTT_HEADER_FIELD_DUP(header.byte);
	*qos = (QoS) MQTT_HEADER_FIELD_QOS(header.byte);
	*retained = MQTT_HEADER_FIELD_RETAIN(header.byte);

	/* read remaining length */
	rc = aws_iot_mqtt_internal_decode_remaining_length_from_buffer(curData, &decodedLen, &readBytesLen);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}
	curData += (readBytesLen);
	endData = curData + decodedLen;

	/* do we have enough data to read the protocol version byte? */
	if(SUCCESS != _aws_iot_mqtt_read_string_with_len(pTopicName, topicNameLen, &curData, endData)
	   || (0 > (endData - curData))) {
		FUNC_EXIT_RC(FAILURE);
	}

	if(QOS0 != *qos) {
		*pPacketId = aws_iot_mqtt_internal_read_uint16_t(&curData);
	}

	*payloadLen = (size_t) (endData - curData);
	*payload = curData;

	FUNC_EXIT_RC(SUCCESS);
}

/**
  * Deserializes the supplied (wire) buffer into an ack
  * @param pPacketType returned integer - the MQTT packet type
  * @param dup returned integer - the MQTT dup flag
  * @param pPacketId returned integer - the MQTT packet identifier
  * @param pRxBuf the raw buffer data, of the correct length determined by the remaining length field
  * @param rxBuflen the length in bytes of the data in the supplied buffer
  *
  * @return An IoT Error Type defining successful/failed call
  */
IoT_Error_t aws_iot_mqtt_internal_deserialize_ack(unsigned char *pPacketType, unsigned char *dup,
												  uint16_t *pPacketId, unsigned char *pRxBuf,
												  size_t rxBuflen) {
	IoT_Error_t rc = FAILURE;
	unsigned char *curdata = pRxBuf;
	unsigned char *enddata = NULL;
	uint32_t decodedLen = 0;
	uint32_t readBytesLen = 0;
	MQTTHeader header = {0};

	FUNC_ENTRY;

	if(NULL == pPacketType || NULL == dup || NULL == pPacketId || NULL == pRxBuf) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	/* PUBACK fixed header size is two bytes, variable header is 2 bytes, MQTT v3.1.1 Specification 3.4.1 */
	if(4 > rxBuflen) {
		FUNC_EXIT_RC(MQTT_RX_BUFFER_TOO_SHORT_ERROR);
	}


	header.byte = aws_iot_mqtt_internal_read_char(&curdata);
	*dup = MQTT_HEADER_FIELD_DUP(header.byte);
	*pPacketType = MQTT_HEADER_FIELD_TYPE(header.byte);

	/* read remaining length */
	rc = aws_iot_mqtt_internal_decode_remaining_length_from_buffer(curdata, &decodedLen, &readBytesLen);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}
	curdata += (readBytesLen);
	enddata = curdata + decodedLen;

	if(enddata - curdata < 2) {
		FUNC_EXIT_RC(FAILURE);
	}

	*pPacketId = aws_iot_mqtt_internal_read_uint16_t(&curdata);

	FUNC_EXIT_RC(SUCCESS);
}

#ifdef __cplusplus
}
#endif
