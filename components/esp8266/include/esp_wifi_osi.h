// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ESP_WIFI_OSI_H_
#define ESP_WIFI_OSI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_WIFI_OS_ADAPTER_VERSION  0x00000001
#define ESP_WIFI_OS_ADAPTER_MAGIC    0xDEADBEAF

#define OSI_FUNCS_TIME_BLOCKING      0xffffffff

#define OSI_QUEUE_SEND_FRONT         0
#define OSI_QUEUE_SEND_BACK          1
#define OSI_QUEUE_SEND_OVERWRITE     2

#ifdef __cplusplus
}
#endif

#endif /* ESP_WIFI_OSI_H_ */
