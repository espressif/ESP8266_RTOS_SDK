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
#ifndef AWS_IOT_SDK_SRC_IOT_SHADOW_H_
#define AWS_IOT_SDK_SRC_IOT_SHADOW_H_

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @file aws_iot_shadow_interface.h
 * @brief Interface for thing shadow
 *
 * These are the functions and structs to manage/interact the Thing Shadow(in the cloud).
 * This SDK will let you interact with your own thing shadow or any other shadow using its Thing Name.
 * There are totally 3 actions a device can perform on the shadow - Get, Update and Delete.
 *
 * Currently the device should use MQTT/S underneath. In the future this will also support other protocols. As it supports MQTT, the shadow needs to connect and disconnect.
 * It will also work on the pub/sub model. On performing any action, the acknowledgment will be received in either accepted or rejected. For Example:
 * If we want to perform a GET on the thing shadow the following messages will be sent and received:
 * 1. A MQTT Publish on the topic - $aws/things/{thingName}/shadow/get
 * 2. Subscribe to MQTT topics - $aws/things/{thingName}/shadow/get/accepted and $aws/things/{thingName}/shadow/get/rejected.
 * If the request was successful we will receive the things json document in the accepted topic.
 *
 *
 */
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_json_data.h"

/*!
 * @brief Shadow Initialization parameters
 *
 * As the Shadow SDK uses MQTT underneath, it could be connected and disconnected on events to save some battery.
 * @note Always use the \c ShadowIniTParametersDefault to initialize this struct
 *
 *
 *
 */
typedef struct {
	char *pHost; ///< This will be unique to a customer and can be retrieved from the console
	uint16_t port; ///< By default the port is 8883
	const char *pRootCA; ///< Location with the Filename of the Root CA
	const char *pClientCRT; ///< Location of Device certs signed by AWS IoT service
	const char *pClientKey; ///< Location of Device private key
	bool enableAutoReconnect;        ///< Set to true to enable auto reconnect
	iot_disconnect_handler disconnectHandler;    ///< Callback to be invoked upon connection loss.
} ShadowInitParameters_t;

/*!
 * @brief Shadow Connect parameters
 *
 * As the Shadow SDK uses MQTT underneath, it could be connected and disconnected on events to save some battery.
 * @note Always use the \c ShadowConnectParametersDefault to initialize this struct
 *
 *d
 *
 */
typedef struct {
	const char *pMyThingName; ///< Every device has a Thing Shadow and this is the placeholder for name
	const char *pMqttClientId; ///< Currently the Shadow uses MQTT to connect and it is important to ensure we have unique client id
	uint16_t mqttClientIdLen; ///< Currently the Shadow uses MQTT to connect and it is important to ensure we have unique client id
	pApplicationHandler_t deleteActionHandler;	///< Callback to be invoked when Thing shadow for this device is deleted
} ShadowConnectParameters_t;

/*!
 * @brief This is set to defaults from the configuration file
 * The certs are set to NULL because they need the path to the file. shadow_sample.c file demonstrates on how to get the relative path
 *
 * \relates ShadowInitParameters_t
 */
extern const ShadowInitParameters_t ShadowInitParametersDefault;

/*!
 * @brief This is set to defaults from the configuration file
 * The length of the client id is initialized as 0. This is due to C language limitations of using constant literals
 * only for creating const variables. The client id will be assigned using the value from aws_iot_config.h but the
 * length needs to be assigned in code. shadow_sample.c file demonstrates this.
 *
 * \relates ShadowConnectParameters_t
 */
extern const ShadowConnectParameters_t ShadowConnectParametersDefault;


/**
 * @brief Initialize the Thing Shadow before use
 *
 * This function takes care of initializing the internal book-keeping data structures and initializing the IoT client.
 *
 * @param pClient A new MQTT Client to be used as the protocol layer. Will be initialized with pParams.
 * @return An IoT Error Type defining successful/failed Initialization
 */
IoT_Error_t aws_iot_shadow_init(AWS_IoT_Client *pClient, const ShadowInitParameters_t *pParams);

/**
 * @brief Connect to the AWS IoT Thing Shadow service over MQTT
 *
 * This function does the TLSv1.2 handshake and establishes the MQTT connection
 *
 * @param pClient	MQTT Client used as the protocol layer
 * @param pParams	Shadow Conenction parameters like TLS cert location
 * @return An IoT Error Type defining successful/failed Connection
 */
IoT_Error_t aws_iot_shadow_connect(AWS_IoT_Client *pClient, const ShadowConnectParameters_t *pParams);

/**
 * @brief Yield function to let the background tasks of MQTT and Shadow
 *
 * This function could be use in a separate thread waiting for the incoming messages, ensuring the connection is kept alive with the AWS Service.
 * It also ensures the expired requests of Shadow actions are cleared and Timeout callback is executed.
 * @note All callbacks ever used in the SDK will be executed in the context of this function.
 *
 * @param pClient	MQTT Client used as the protocol layer
 * @param timeout	in milliseconds, This is the maximum time the yield function will wait for a message and/or read the messages from the TLS buffer
 * @return An IoT Error Type defining successful/failed Yield
 */
IoT_Error_t aws_iot_shadow_yield(AWS_IoT_Client *pClient, uint32_t timeout);

/**
 * @brief Disconnect from the AWS IoT Thing Shadow service over MQTT
 *
 * This will close the underlying TCP connection, MQTT connection will also be closed
 *
 * @param pClient	MQTT Client used as the protocol layer
 * @return An IoT Error Type defining successful/failed disconnect status
 */
IoT_Error_t aws_iot_shadow_disconnect(AWS_IoT_Client *pClient);

/**
 * @brief Thing Shadow Acknowledgment enum
 *
 * This enum type is use in the callback for the action response
 *
 */
typedef enum {
	SHADOW_ACK_TIMEOUT, SHADOW_ACK_REJECTED, SHADOW_ACK_ACCEPTED
} Shadow_Ack_Status_t;

/**
 * @brief Thing Shadow Action type enum
 *
 * This enum type is use in the callback for the action response
 *
 */
typedef enum {
	SHADOW_GET, SHADOW_UPDATE, SHADOW_DELETE
} ShadowActions_t;


/**
 * @brief Function Pointer typedef used as the callback for every action
 *
 * This function will be called from the context of \c aws_iot_shadow_yield() context
 *
 * @param pThingName Thing Name of the response received
 * @param action The response of the action
 * @param status Informs if the action was Accepted/Rejected or Timed out
 * @param pReceivedJsonDocument Received JSON document
 * @param pContextData the void* data passed in during the action call(update, get or delete)
 *
 */
typedef void (*fpActionCallback_t)(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
								   const char *pReceivedJsonDocument, void *pContextData);

/**
 * @brief This function is the one used to perform an Update action to a Thing Name's Shadow.
 *
 * update is one of the most frequently used functionality by a device. In most cases the device may be just reporting few params to update the thing shadow in the cloud
 * Update Action if no callback or if the JSON document does not have a client token then will just publish the update and not track it.
 *
 * @note The update has to subscribe to two topics update/accepted and update/rejected. This function waits 2 seconds to ensure the subscriptions are registered before publishing the update message.
 * The following steps are performed on using this function:
 * 1. Subscribe to Shadow topics - $aws/things/{thingName}/shadow/update/accepted and $aws/things/{thingName}/shadow/update/rejected
 * 2. wait for 2 seconds for the subscription to take effect
 * 3. Publish on the update topic - $aws/things/{thingName}/shadow/update
 * 4. In the \c aws_iot_shadow_yield() function the response will be handled. In case of timeout or if the response is received, the subscription to shadow response topics are un-subscribed from.
 *    On the contrary if the persistent subscription is set to true then the un-subscribe will not be done. The topics will always be listened to.
 *
 * @param pClient	MQTT Client used as the protocol layer
 * @param pThingName Thing Name of the shadow that needs to be Updated
 * @param pJsonString The update action expects a JSON document to send. The JSON String should be a null terminated string. This JSON document should adhere to the AWS IoT Thing Shadow specification. To help in the process of creating this document- SDK provides apis in \c aws_iot_shadow_json_data.h
 * @param callback This is the callback that will be used to inform the caller of the response from the AWS IoT Shadow service.Callback could be set to NULL if response is not important
 * @param pContextData This is an extra parameter that could be passed along with the callback. It should be set to NULL if not used
 * @param timeout_seconds It is the time the SDK will wait for the response on either accepted/rejected before declaring timeout on the action
 * @param isPersistentSubscribe As mentioned above, every  time if a device updates the same shadow then this should be set to true to avoid repeated subscription and unsubscription. If the Thing Name is one off update then this should be set to false
 * @return An IoT Error Type defining successful/failed update action
 */
IoT_Error_t aws_iot_shadow_update(AWS_IoT_Client *pClient, const char *pThingName, char *pJsonString,
								  fpActionCallback_t callback, void *pContextData, uint8_t timeout_seconds,
								  bool isPersistentSubscribe);

/**
 * @brief This function is the one used to perform an Get action to a Thing Name's Shadow.
 *
 * One use of this function is usually to get the config of a device at boot up.
 * It is similar to the Update function internally except it does not take a JSON document as the input. The entire JSON document will be sent over the accepted topic
 *
 * @param pClient	MQTT Client used as the protocol layer
 * @param pThingName Thing Name of the JSON document that is needed
 * @param callback This is the callback that will be used to inform the caller of the response from the AWS IoT Shadow service.Callback could be set to NULL if response is not important
 * @param pContextData This is an extra parameter that could be passed along with the callback. It should be set to NULL if not used
 * @param timeout_seconds It is the time the SDK will wait for the response on either accepted/rejected before declaring timeout on the action
 * @param isPersistentSubscribe As mentioned above, every  time if a device gets the same Sahdow (JSON document) then this should be set to true to avoid repeated subscription and un-subscription. If the Thing Name is one off get then this should be set to false
 * @return An IoT Error Type defining successful/failed get action
 */
IoT_Error_t aws_iot_shadow_get(AWS_IoT_Client *pClient, const char *pThingName, fpActionCallback_t callback,
							   void *pContextData, uint8_t timeout_seconds, bool isPersistentSubscribe);

/**
 * @brief This function is the one used to perform an Delete action to a Thing Name's Shadow.
 *
 * This is not a very common use case for  device. It is generally the responsibility of the accompanying app to do the delete.
 * It is similar to the Update function internally except it does not take a JSON document as the input. The Thing Shadow referred by the ThingName will be deleted.
 *
 * @param pClient MQTT Client used as the protocol layer
 * @param pThingName Thing Name of the Shadow that should be deleted
 * @param callback This is the callback that will be used to inform the caller of the response from the AWS IoT Shadow service.Callback could be set to NULL if response is not important
 * @param pContextData This is an extra parameter that could be passed along with the callback. It should be set to NULL if not used
 * @param timeout_seconds It is the time the SDK will wait for the response on either accepted/rejected before declaring timeout on the action
 * @param isPersistentSubscribe As mentioned above, every  time if a device deletes the same Shadow (JSON document) then this should be set to true to avoid repeated subscription and un-subscription. If the Thing Name is one off delete then this should be set to false
 * @return An IoT Error Type defining successful/failed delete action
 */
IoT_Error_t aws_iot_shadow_delete(AWS_IoT_Client *pClient, const char *pThingName, fpActionCallback_t callback,
								  void *pContextData, uint8_t timeout_seconds, bool isPersistentSubscriptions);

/**
 * @brief This function is used to listen on the delta topic of #AWS_IOT_MY_THING_NAME mentioned in the aws_iot_config.h file.
 *
 * Any time a delta is published the Json document will be delivered to the pStruct->cb. If you don't want the parsing done by the SDK then use the jsonStruct_t key set to "state". A good example of this is displayed in the sample_apps/shadow_console_echo.c
 *
 * @param pClient MQTT Client used as the protocol layer
 * @param pStruct The struct used to parse JSON value
 * @return An IoT Error Type defining successful/failed delta registering
 */
IoT_Error_t aws_iot_shadow_register_delta(AWS_IoT_Client *pClient, jsonStruct_t *pStruct);

/**
 * @brief Reset the last received version number to zero.
 * This will be useful if the Thing Shadow is deleted and would like to to reset the local version
 * @return no return values
 *
 */
void aws_iot_shadow_reset_last_received_version(void);

/**
 * @brief Version of a document is received with every accepted/rejected and the SDK keeps track of the last received version of the JSON document of #AWS_IOT_MY_THING_NAME shadow
 *
 * One exception to this version tracking is that, the SDK will ignore the version from update/accepted topic. Rest of the responses will be scanned to update the version number.
 * Accepting version change for update/accepted may cause version conflicts for delta message if the update message is received before the delta.
 *
 * @return version number of the last received response
 *
 */
uint32_t aws_iot_shadow_get_last_received_version(void);

/**
 * @brief Enable the ignoring of delta messages with old version number
 *
 * As we use MQTT underneath, there could be more than 1 of the same message if we use QoS 0. To avoid getting called for the same message, this functionality should be enabled. All the old message will be ignored
 */
void aws_iot_shadow_enable_discard_old_delta_msgs(void);

/**
 * @brief Disable the ignoring of delta messages with old version number
 */
void aws_iot_shadow_disable_discard_old_delta_msgs(void);

/**
 * @brief This function is used to enable or disable autoreconnect
 *
 * Any time a disconnect happens the underlying MQTT client attempts to reconnect if this is set to true
 *
 * @param pClient MQTT Client used as the protocol layer
 * @param newStatus The new status to set the autoreconnect option to
 *
 * @return An IoT Error Type defining successful/failed operation
 */
IoT_Error_t aws_iot_shadow_set_autoreconnect_status(AWS_IoT_Client *pClient, bool newStatus);

#ifdef __cplusplus
}
#endif

#endif //AWS_IOT_SDK_SRC_IOT_SHADOW_H_
