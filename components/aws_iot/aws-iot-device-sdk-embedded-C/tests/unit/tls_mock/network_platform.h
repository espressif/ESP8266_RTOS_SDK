//
// Created by chaurah on 3/22/16.
//

#ifndef IOTSDKC_NETWORK_MBEDTLS_PLATFORM_H_H

/**
 * @brief TLS Connection Parameters
 *
 * Defines a type containing TLS specific parameters to be passed down to the
 * TLS networking layer to create a TLS secured socket.
 */
typedef struct _TLSDataParams {
	uint32_t flags;
}TLSDataParams;

#define IOTSDKC_NETWORK_MBEDTLS_PLATFORM_H_H

#endif //IOTSDKC_NETWORK_MBEDTLS_PLATFORM_H_H
