#include <string.h>
#include <stdio.h>
#include <network_interface.h>

#include "network_interface.h"
#include "aws_iot_tests_unit_mock_tls_params.h"


void _iot_tls_set_connect_params(Network *pNetwork, char *pRootCALocation, char *pDeviceCertLocation,
								 char *pDevicePrivateKeyLocation, char *pDestinationURL,
								 uint16_t destinationPort, uint32_t timeout_ms, bool ServerVerificationFlag) {
	pNetwork->tlsConnectParams.DestinationPort = destinationPort;
	pNetwork->tlsConnectParams.pDestinationURL = pDestinationURL;
	pNetwork->tlsConnectParams.pDeviceCertLocation = pDeviceCertLocation;
	pNetwork->tlsConnectParams.pDevicePrivateKeyLocation = pDevicePrivateKeyLocation;
	pNetwork->tlsConnectParams.pRootCALocation = pRootCALocation;
	pNetwork->tlsConnectParams.timeout_ms = timeout_ms;
	pNetwork->tlsConnectParams.ServerVerificationFlag = ServerVerificationFlag;
}

IoT_Error_t iot_tls_init(Network *pNetwork, char *pRootCALocation, char *pDeviceCertLocation,
						 char *pDevicePrivateKeyLocation, char *pDestinationURL,
						 uint16_t destinationPort, uint32_t timeout_ms, bool ServerVerificationFlag) {
	_iot_tls_set_connect_params(pNetwork, pRootCALocation, pDeviceCertLocation, pDevicePrivateKeyLocation,
								pDestinationURL, destinationPort, timeout_ms, ServerVerificationFlag);

	pNetwork->connect = iot_tls_connect;
	pNetwork->read = iot_tls_read;
	pNetwork->write = iot_tls_write;
	pNetwork->disconnect = iot_tls_disconnect;
	pNetwork->isConnected = iot_tls_is_connected;
	pNetwork->destroy = iot_tls_destroy;

	return SUCCESS;
}

IoT_Error_t iot_tls_connect(Network *pNetwork, TLSConnectParams *params) {
	IOT_UNUSED(pNetwork);

	if(NULL != params) {
		_iot_tls_set_connect_params(pNetwork, params->pRootCALocation, params->pDeviceCertLocation,
									params->pDevicePrivateKeyLocation, params->pDestinationURL, params->DestinationPort,
									params->timeout_ms, params->ServerVerificationFlag);
	}

	if(NULL != invalidEndpointFilter && 0 == strcmp(invalidEndpointFilter, pNetwork->tlsConnectParams.pDestinationURL)) {
		return NETWORK_ERR_NET_UNKNOWN_HOST;
	}

	if(invalidPortFilter == pNetwork->tlsConnectParams.DestinationPort) {
		return NETWORK_ERR_NET_CONNECT_FAILED;
	}

	if(NULL != invalidRootCAPathFilter && 0 == strcmp(invalidRootCAPathFilter, pNetwork->tlsConnectParams.pRootCALocation)) {
		return NETWORK_ERR_NET_CONNECT_FAILED;
	}

	if(NULL != invalidCertPathFilter && 0 == strcmp(invalidCertPathFilter, pNetwork->tlsConnectParams.pDeviceCertLocation)) {
		return NETWORK_ERR_NET_CONNECT_FAILED;
	}

	if(NULL != invalidPrivKeyPathFilter && 0 == strcmp(invalidPrivKeyPathFilter, pNetwork->tlsConnectParams.pDevicePrivateKeyLocation)) {
		return NETWORK_ERR_NET_CONNECT_FAILED;
	}
	return SUCCESS;
}

IoT_Error_t iot_tls_is_connected(Network *pNetwork) {
	IOT_UNUSED(pNetwork);

	/* Use this to add implementation which can check for physical layer disconnect */
	return NETWORK_PHYSICAL_LAYER_CONNECTED;
}

IoT_Error_t iot_tls_write(Network *pNetwork, unsigned char *pMsg, size_t len, Timer *timer, size_t *written_len) {
	size_t i = 0;
	uint8_t firstByte, secondByte;
	uint16_t topicNameLen;
	IOT_UNUSED(pNetwork);
	IOT_UNUSED(timer);

	for(i = 0; (i < len) && left_ms(timer) > 0; i++) {
		TxBuffer.pBuffer[i] = pMsg[i];
	}
	TxBuffer.len = len;
	*written_len = len;

	/* Save last two subscribed topics */
	if((TxBuffer.pBuffer[0] == 0x82 ? true : false)) {
		snprintf(SecondLastSubscribeMessage, lastSubscribeMsgLen, "%s", LastSubscribeMessage);
		secondLastSubscribeMsgLen = lastSubscribeMsgLen;

		firstByte = (uint8_t)(TxBuffer.pBuffer[4]);
		secondByte = (uint8_t)(TxBuffer.pBuffer[5]);
		topicNameLen = (uint16_t) (secondByte + (256 * firstByte));

		snprintf(LastSubscribeMessage, topicNameLen + 1u, "%s", &(TxBuffer.pBuffer[6])); // Added one for null character
		lastSubscribeMsgLen = topicNameLen + 1u;
	}

	return SUCCESS;
}

static unsigned char isTimerExpired(struct timeval target_time) {
	unsigned char ret_val = 0;
	struct timeval now, result;

	if(target_time.tv_sec != 0 || target_time.tv_usec != 0) {
		gettimeofday(&now, NULL);
		timersub(&(target_time), &now, &result);
		if(result.tv_sec < 0 || (result.tv_sec == 0 && result.tv_usec <= 0)) {
			ret_val = 1;
		}
	} else {
		ret_val = 1;
	}
	return ret_val;
}

IoT_Error_t iot_tls_read(Network *pNetwork, unsigned char *pMsg, size_t len, Timer *pTimer, size_t *read_len) {
	IOT_UNUSED(pNetwork);
	IOT_UNUSED(pTimer);

	if(RxIndex > TLSMaxBufferSize - 1) {
		RxIndex = TLSMaxBufferSize - 1;
	}

	if(RxBuffer.len <= RxIndex || !isTimerExpired(RxBuffer.expiry_time)) {
		return NETWORK_SSL_NOTHING_TO_READ;
	}

	if((false == RxBuffer.NoMsgFlag) && (RxIndex < RxBuffer.len)) {
		memcpy(pMsg, &(RxBuffer.pBuffer[RxIndex]), len);
		RxIndex += len;
		*read_len = len;
	}

	return SUCCESS;
}

IoT_Error_t iot_tls_disconnect(Network *pNetwork) {
	IOT_UNUSED(pNetwork);
	return SUCCESS;
}

IoT_Error_t iot_tls_destroy(Network *pNetwork) {
	IOT_UNUSED(pNetwork);
	return SUCCESS;
}
