/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 * Tuan PM <tuanpm at live dot com>
 */
#ifndef _MQTT_CONFIG_H_
#define _MQTT_CONFIG_H_

#include "sdkconfig.h"

#define MQTT_PROTOCOL_311           CONFIG_MQTT_PROTOCOL_311
#define MQTT_RECONNECT_TIMEOUT_MS   (10*1000)

#if CONFIG_MQTT_BUFFER_SIZE
#define MQTT_BUFFER_SIZE_BYTE       CONFIG_MQTT_BUFFER_SIZE
#else
#define MQTT_BUFFER_SIZE_BYTE       1024
#endif

#define MQTT_MAX_HOST_LEN           64
#define MQTT_MAX_CLIENT_LEN         32
#define MQTT_MAX_USERNAME_LEN       32
#define MQTT_MAX_PASSWORD_LEN       65
#define MQTT_MAX_LWT_TOPIC          32
#define MQTT_MAX_LWT_MSG            128
#define MQTT_TASK_PRIORITY          5

#if CONFIG_MQTT_TASK_STACK_SIZE
#define MQTT_TASK_STACK             CONFIG_MQTT_TASK_STACK_SIZE
#else
#define MQTT_TASK_STACK             (6*1024)
#endif

#define MQTT_KEEPALIVE_TICK         (120)
#define MQTT_CMD_QUEUE_SIZE         (10)
#define MQTT_NETWORK_TIMEOUT_MS     (10000)

#ifdef CONFIG_MQTT_TCP_DEFAULT_PORT
#define MQTT_TCP_DEFAULT_PORT       CONFIG_MQTT_TCP_DEFAULT_PORT
#else
#define MQTT_TCP_DEFAULT_PORT       1883
#endif

#ifdef CONFIG_MQTT_SSL_DEFAULT_PORT
#define MQTT_SSL_DEFAULT_PORT       CONFIG_MQTT_SSL_DEFAULT_PORT
#else
#define MQTT_SSL_DEFAULT_PORT       8883
#endif

#ifdef CONFIG_MQTT_WS_DEFAULT_PORT
#define MQTT_WS_DEFAULT_PORT        CONFIG_MQTT_WS_DEFAULT_PORT
#else
#define MQTT_WS_DEFAULT_PORT        80
#endif

#ifdef MQTT_WSS_DEFAULT_PORT
#define MQTT_WSS_DEFAULT_PORT       CONFIG_MQTT_WSS_DEFAULT_PORT
#else
#define MQTT_WSS_DEFAULT_PORT       443
#endif

#define MQTT_CORE_SELECTION_ENABLED CONFIG_MQTT_TASK_CORE_SELECTION_ENABLED

#ifdef CONFIG_MQTT_USE_CORE_0
	#define MQTT_TASK_CORE	0
#else
	#ifdef CONFIG_MQTT_USE_CORE_1
		#define MQTT_TASK_CORE 1
	#else
		#define MQTT_TASK_CORE 0
	#endif
#endif


#define MQTT_ENABLE_SSL             CONFIG_MQTT_TRANSPORT_SSL
#define MQTT_ENABLE_WS              CONFIG_MQTT_TRANSPORT_WEBSOCKET
#define MQTT_ENABLE_WSS             CONFIG_MQTT_TRANSPORT_WEBSOCKET_SECURE

#define OUTBOX_EXPIRED_TIMEOUT_MS   (30*1000)
#define OUTBOX_MAX_SIZE             (4*1024)
#endif
