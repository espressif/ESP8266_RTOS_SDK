/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file aws_iot_error.h
 * @brief Definition of error types for the SDK.
 */

#ifndef AWS_IOT_SDK_SRC_IOT_ERROR_H_
#define AWS_IOT_SDK_SRC_IOT_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Used to avoid warnings in case of unused parameters in function pointers */
#define IOT_UNUSED(x) (void)(x)

/*! \public
 * @brief IoT Error enum
 *
 * Enumeration of return values from the IoT_* functions within the SDK.
 * Values less than -1 are specific error codes
 * Value of -1 is a generic failure response
 * Value of 0 is a generic success response
 * Values greater than 0 are specific non-error return codes
 */
typedef enum {
	/** Returned when the Network physical layer is connected */
			NETWORK_PHYSICAL_LAYER_CONNECTED = 6,
	/** Returned when the Network is manually disconnected */
			NETWORK_MANUALLY_DISCONNECTED = 5,
	/** Returned when the Network is disconnected and the reconnect attempt is in progress */
			NETWORK_ATTEMPTING_RECONNECT = 4,
	/** Return value of yield function to indicate auto-reconnect was successful */
			NETWORK_RECONNECTED = 3,
	/** Returned when a read attempt is made on the TLS buffer and it is empty */
			MQTT_NOTHING_TO_READ = 2,
	/** Returned when a connection request is successful and packet response is connection accepted */
			MQTT_CONNACK_CONNECTION_ACCEPTED = 1,
	/** Success return value - no error occurred */
			SUCCESS = 0,
	/** A generic error. Not enough information for a specific error code */
			FAILURE = -1,
	/** A required parameter was passed as null */
			NULL_VALUE_ERROR = -2,
	/** The TCP socket could not be established */
			TCP_CONNECTION_ERROR = -3,
	/** The TLS handshake failed */
			SSL_CONNECTION_ERROR = -4,
	/** Error associated with setting up the parameters of a Socket */
			TCP_SETUP_ERROR = -5,
	/** A timeout occurred while waiting for the TLS handshake to complete. */
			NETWORK_SSL_CONNECT_TIMEOUT_ERROR = -6,
	/** A Generic write error based on the platform used */
			NETWORK_SSL_WRITE_ERROR = -7,
	/** SSL initialization error at the TLS layer */
			NETWORK_SSL_INIT_ERROR = -8,
	/** An error occurred when loading the certificates.  The certificates could not be located or are incorrectly formatted. */
			NETWORK_SSL_CERT_ERROR = -9,
	/** SSL Write times out */
			NETWORK_SSL_WRITE_TIMEOUT_ERROR = -10,
	/** SSL Read times out */
			NETWORK_SSL_READ_TIMEOUT_ERROR = -11,
	/** A Generic error based on the platform used */
			NETWORK_SSL_READ_ERROR = -12,
	/** Returned when the Network is disconnected and reconnect is either disabled or physical layer is disconnected */
			NETWORK_DISCONNECTED_ERROR = -13,
	/** Returned when the Network is disconnected and the reconnect attempt has timed out */
			NETWORK_RECONNECT_TIMED_OUT_ERROR = -14,
	/** Returned when the Network is already connected and a connection attempt is made */
			NETWORK_ALREADY_CONNECTED_ERROR = -15,
	/** Network layer Error Codes */
	/** Network layer Random number generator seeding failed */
			NETWORK_MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED = -16,
	/** A generic error code for Network layer errors */
			NETWORK_SSL_UNKNOWN_ERROR = -17,
	/** Returned when the physical layer is disconnected */
			NETWORK_PHYSICAL_LAYER_DISCONNECTED = -18,
	/** Returned when the root certificate is invalid */
			NETWORK_X509_ROOT_CRT_PARSE_ERROR = -19,
	/** Returned when the device certificate is invalid */
			NETWORK_X509_DEVICE_CRT_PARSE_ERROR = -20,
	/** Returned when the private key failed to parse */
			NETWORK_PK_PRIVATE_KEY_PARSE_ERROR = -21,
	/** Returned when the network layer failed to open a socket */
			NETWORK_ERR_NET_SOCKET_FAILED = -22,
	/** Returned when the server is unknown */
			NETWORK_ERR_NET_UNKNOWN_HOST = -23,
	/** Returned when connect request failed */
			NETWORK_ERR_NET_CONNECT_FAILED = -24,
	/** Returned when there is nothing to read in the TLS read buffer */
			NETWORK_SSL_NOTHING_TO_READ = -25,
	/** A connection could not be established. */
			MQTT_CONNECTION_ERROR = -26,
	/** A timeout occurred while waiting for the TLS handshake to complete */
			MQTT_CONNECT_TIMEOUT_ERROR = -27,
	/** A timeout occurred while waiting for the TLS request complete */
			MQTT_REQUEST_TIMEOUT_ERROR = -28,
	/** The current client state does not match the expected value */
			MQTT_UNEXPECTED_CLIENT_STATE_ERROR = -29,
	/** The client state is not idle when request is being made */
			MQTT_CLIENT_NOT_IDLE_ERROR = -30,
	/** The MQTT RX buffer received corrupt or unexpected message  */
			MQTT_RX_MESSAGE_PACKET_TYPE_INVALID_ERROR = -31,
	/** The MQTT RX buffer received a bigger message. The message will be dropped  */
			MQTT_RX_BUFFER_TOO_SHORT_ERROR = -32,
	/** The MQTT TX buffer is too short for the outgoing message. Request will fail  */
			MQTT_TX_BUFFER_TOO_SHORT_ERROR = -33,
	/** The client is subscribed to the maximum possible number of subscriptions  */
			MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR = -34,
	/** Failed to decode the remaining packet length on incoming packet */
			MQTT_DECODE_REMAINING_LENGTH_ERROR = -35,
	/** Connect request failed with the server returning an unknown error */
			MQTT_CONNACK_UNKNOWN_ERROR = -36,
	/** Connect request failed with the server returning an unacceptable protocol version error */
			MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR = -37,
	/** Connect request failed with the server returning an identifier rejected error */
			MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR = -38,
	/** Connect request failed with the server returning an unavailable error */
			MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR = -39,
	/** Connect request failed with the server returning a bad userdata error */
			MQTT_CONNACK_BAD_USERDATA_ERROR = -40,
	/** Connect request failed with the server failing to authenticate the request */
			MQTT_CONNACK_NOT_AUTHORIZED_ERROR = -41,
	/** An error occurred while parsing the JSON string.  Usually malformed JSON. */
			JSON_PARSE_ERROR = -42,
	/** Shadow: The response Ack table is currently full waiting for previously published updates */
			SHADOW_WAIT_FOR_PUBLISH = -43,
	/** Any time an snprintf writes more than size value, this error will be returned */
			SHADOW_JSON_BUFFER_TRUNCATED = -44,
	/** Any time an snprintf encounters an encoding error or not enough space in the given buffer */
			SHADOW_JSON_ERROR = -45,
	/** Mutex initialization failed */
			MUTEX_INIT_ERROR = -46,
	/** Mutex lock request failed */
			MUTEX_LOCK_ERROR = -47,
	/** Mutex unlock request failed */
			MUTEX_UNLOCK_ERROR = -48,
	/** Mutex destroy failed */
			MUTEX_DESTROY_ERROR = -49,
} IoT_Error_t;

#ifdef __cplusplus
}
#endif

#endif /* AWS_IOT_SDK_SRC_IOT_ERROR_H_ */
