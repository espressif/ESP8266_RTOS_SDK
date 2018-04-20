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

#ifndef _SOCKIO_H
#define _SOCKIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* espressif specific socket ioctls */
#define SIOESPSTART         0x5500              /* start of espressif specific code */

/* routing table calls. */
#define SIOCADDRT           0x890B              /* add routing table entry	*/
#define SIOCDELRT           0x890C              /* delete routing table entry	*/
#define SIOCRTMSG           0x890D              /* call to routing system	*/

/* socket configuration controls. */
#define SIOCGIFADDR         0x8915              /* get PA address		*/
#define SIOCSIFADDR         0x8916              /* set PA address		*/
#define SIOCGIFBRDADDR      0x8919              /* get broadcast PA address	*/
#define SIOCSIFBRDADDR      0x891a              /* set broadcast PA address	*/
#define SIOCGIFNETMASK      0x891b              /* get network PA mask		*/
#define SIOCSIFNETMASK      0x891c              /* set network PA mask		*/
#define SIOCSIFHWADDR       0x8924              /* set hardware address 	*/
#define SIOCGIFHWADDR       0x8927              /* Get hardware address		*/
#define SIOCGIFINDEX        0x8933              /* name -> if_index mapping	*/
#define SIOGIFINDEX         SIOCGIFINDEX        /* misprint compatibility :-)	*/

#ifdef __cplusplus
}
#endif

#endif /* _SOCKIO_H */
