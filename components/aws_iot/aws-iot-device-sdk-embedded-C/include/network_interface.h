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
 * @file network_interface.h
 * @brief Network interface definition for MQTT client.
 *
 * Defines an interface to the TLS layer to be used by the MQTT client.
 * Starting point for porting the SDK to the networking layer of a new platform.
 */

#ifndef __NETWORK_INTERFACE_H_
#define __NETWORK_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <aws_iot_error.h>
#include "timer_interface.h"
#include "network_platform.h"

/**
 * @brief Network Type
 *
 * Defines a type for the network struct.  See structure definition below.
 */
typedef struct Network Network;

/**
 * @brief TLS Connection Parameters
 *
 * Defines a type containing TLS specific parameters to be passed down to the
 * TLS networking layer to create a TLS secured socket.
 */
typedef struct {
	const char *pRootCALocation;                ///< Pointer to string containing the filename (including path) of the root CA file.
	const char *pDeviceCertLocation;            ///< Pointer to string containing the filename (including path) of the device certificate.
	const char *pDevicePrivateKeyLocation;    ///< Pointer to string containing the filename (including path) of the device private key file.
	const char *pDestinationURL;                ///< Pointer to string containing the endpoint of the MQTT service.
	uint16_t DestinationPort;            ///< Integer defining the connection port of the MQTT service.
	uint32_t timeout_ms;                ///< Unsigned integer defining the TLS handshake timeout value in milliseconds.
	bool ServerVerificationFlag;        ///< Boolean.  True = perform server certificate hostname validation.  False = skip validation \b NOT recommended.
} TLSConnectParams;

/**
 * @brief Network Structure
 *
 * Structure for defining a network connection.
 */
struct Network {
	IoT_Error_t (*connect)(Network *, TLSConnectParams *);

	IoT_Error_t (*read)(Network *, unsigned char *, size_t, Timer *, size_t *);    ///< Function pointer pointing to the network function to read from the network
	IoT_Error_t (*write)(Network *, unsigned char *, size_t, Timer *, size_t *);    ///< Function pointer pointing to the network function to write to the network
	IoT_Error_t (*disconnect)(Network *);    ///< Function pointer pointing to the network function to disconnect from the network
	IoT_Error_t (*isConnected)(Network *);    ///< Function pointer pointing to the network function to check if TLS is connected
	IoT_Error_t (*destroy)(Network *);        ///< Function pointer pointing to the network function to destroy the network object

	TLSConnectParams tlsConnectParams;        ///< TLSConnect params structure containing the common connection parameters
	TLSDataParams tlsDataParams;            ///< TLSData params structure containing the connection data parameters that are specific to the library being used
};

/**
 * @brief Initialize the TLS implementation
 *
 * Perform any initialization required by the TLS layer.
 * Connects the interface to implementation by setting up
 * the network layer function pointers to platform implementations.
 *
 * @param pNetwork - Pointer to a Network struct defining the network interface.
 * @param pRootCALocation - Path of the location of the Root CA
 * @param pDeviceCertLocation - Path to the location of the Device Cert
 * @param pDevicyPrivateKeyLocation - Path to the location of the device private key file
 * @param pDestinationURL - The target endpoint to connect to
 * @param DestinationPort - The port on the target to connect to
 * @param timeout_ms - The value to use for timeout of operation
 * @param ServerVerificationFlag - used to decide whether server verification is needed or not
 *
 * @return IoT_Error_t - successful initialization or TLS error
 */
IoT_Error_t iot_tls_init(Network *pNetwork, const char *pRootCALocation, const char *pDeviceCertLocation,
						 const char *pDevicePrivateKeyLocation, const char *pDestinationURL,
						 uint16_t DestinationPort, uint32_t timeout_ms, bool ServerVerificationFlag);

/**
 * @brief Create a TLS socket and open the connection
 *
 * Creates an open socket connection including TLS handshake.
 *
 * @param pNetwork - Pointer to a Network struct defining the network interface.
 * @param TLSParams - TLSConnectParams defines the properties of the TLS connection.
 * @return IoT_Error_t - successful connection or TLS error
 */
IoT_Error_t iot_tls_connect(Network *pNetwork, TLSConnectParams *TLSParams);

/**
 * @brief Write bytes to the network socket
 *
 * @param Network - Pointer to a Network struct defining the network interface.
 * @param unsigned char pointer - buffer to write to socket
 * @param integer - number of bytes to write
 * @param Timer * - operation timer
 * @return integer - number of bytes written or TLS error
 * @return IoT_Error_t - successful write or TLS error code
 */
IoT_Error_t iot_tls_write(Network *, unsigned char *, size_t, Timer *, size_t *);

/**
 * @brief Read bytes from the network socket
 *
 * @param Network - Pointer to a Network struct defining the network interface.
 * @param unsigned char pointer - pointer to buffer where read bytes should be copied
 * @param size_t - number of bytes to read
 * @param Timer * - operation timer
 * @param size_t - pointer to store number of bytes read
 * @return IoT_Error_t - successful read or TLS error code
 */
IoT_Error_t iot_tls_read(Network *, unsigned char *, size_t, Timer *, size_t *);

/**
 * @brief Disconnect from network socket
 *
 * @param Network - Pointer to a Network struct defining the network interface.
 * @return IoT_Error_t - successful read or TLS error code
 */
IoT_Error_t iot_tls_disconnect(Network *pNetwork);

/**
 * @brief Perform any tear-down or cleanup of TLS layer
 *
 * Called to cleanup any resources required for the TLS layer.
 *
 * @param Network - Pointer to a Network struct defining the network interface
 * @return IoT_Error_t - successful cleanup or TLS error code
 */
IoT_Error_t iot_tls_destroy(Network *pNetwork);

/**
 * @brief Check if TLS layer is still connected
 *
 * Called to check if the TLS layer is still connected or not.
 *
 * @param Network - Pointer to a Network struct defining the network interface
 * @return IoT_Error_t - TLS error code indicating status of network physical layer connection
 */
IoT_Error_t iot_tls_is_connected(Network *pNetwork);

#ifdef __cplusplus
}
#endif

#endif //__NETWORK_INTERFACE_H_
