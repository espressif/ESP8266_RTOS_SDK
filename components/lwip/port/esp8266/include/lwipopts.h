/**
 * @file
 *
 * lwIP Options Configuration
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * NOTE: || defined __DOXYGEN__ is a workaround for doxygen bug -
 * without this, doxygen does not see the actual #define
 */

#ifndef __LWIP_HDR_LWIPOPTS_H__
#define __LWIP_HDR_LWIPOPTS_H__

#include "sdkconfig.h"
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include "esp_libc.h"
#include "esp_system.h"
#include "driver/soc.h"

#define ESP_LWIP 1
#define ESP_IP4_ATON 1
#define ESP_DHCP 1

#ifdef CONFIG_ESP_DNS
#define ESP_DNS  1
#else
#define ESP_DNS  0
#endif

#ifdef CONFIG_LWIP_IPV6_MLD_SOCK
#define ESP_LWIP_IPV6_MLD 1
#else
#define ESP_LWIP_IPV6_MLD 0
#endif

#ifdef CONFIG_ESP_UDP_SYNC_SEND
#define ESP_UDP  1
#endif

#ifdef CONFIG_LWIP_SOCKET_MULTITHREAD
#define SOCKETS_MT
#endif

#if CONFIG_ENABLE_NONBLOCK_SPEEDUP
#define ESP_NONBLOCK    1
#else
#define ESP_NONBLOCK    0
#endif

#ifdef CONFIG_ESP_LWIP_RECV_REST_DATA
#define ESP_LWIP_RECV_REST_DATA 1
#else
#define ESP_LWIP_RECV_REST_DATA 0
#endif

//#define SOCKETS_TCP_TRACE

#define TCP_HIGH_SPEED_RETRANSMISSION CONFIG_TCP_HIGH_SPEED_RETRANSMISSION

/**
 * @brief System
 */
#define SYS_ARCH_DECL_PROTECT(_lev)    esp_irqflag_t _lev
#define SYS_ARCH_PROTECT(_lev)         _lev = soc_save_local_irq()
#define SYS_ARCH_UNPROTECT(_lev)       soc_restore_local_irq(_lev)

 /*
   ------------------------------------
   -------------- NO SYS --------------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_nosys NO_SYS
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * NO_SYS==1: Use lwIP without OS-awareness (no thread, semaphores, mutexes or
 * mboxes). This means threaded APIs cannot be used (socket, netconn,
 * i.e. everything in the 'api' folder), only the callback-style raw API is
 * available (and you have to watch out for yourself that you don't access
 * lwIP functions/structures from more than one context at a time!)
 */
#define NO_SYS                          0
/**
 * @}
 */

/**
 * @defgroup lwip_opts_timers Timers
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * LWIP_TIMERS==0: Drop support for sys_timeout and lwip-internal cyclic timers.
 * (the array of lwip-internal cyclic timers is still provided)
 * (check NO_SYS_NO_TIMERS for compatibility to old versions)
 */
#define LWIP_TIMERS                     1

/**
 * LWIP_TIMERS_CUSTOM==1: Provide your own timer implementation.
 * Function prototypes in timeouts.h and the array of lwip-internal cyclic timers
 * are still included, but the implementation is not. The following functions
 * will be required: sys_timeouts_init(), sys_timeout(), sys_untimeout(),
 *                   sys_timeouts_mbox_fetch()
 */
#define LWIP_TIMERS_CUSTOM              0
/**
 * @}
 */

/**
 * @defgroup lwip_opts_memcpy memcpy
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * MEMCPY: override this if you have a faster implementation at hand than the
 * one included in your C library
 */
#define MEMCPY(dst,src,len)             memcpy(dst,src,len)

/**
 * SMEMCPY: override this with care! Some compilers (e.g. gcc) can inline a
 * call to memcpy() if the length is known at compile time and is small.
 */
#define SMEMCPY(dst,src,len)            memcpy(dst,src,len)

#define LWIP_RAND	esp_random

/**
 * @}
 */

/*
   ------------------------------------
   ----------- Core locking -----------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_lock Core locking and MPU
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * LWIP_MPU_COMPATIBLE: enables special memory management mechanism
 * which makes lwip able to work on MPU (Memory Protection Unit) system
 * by not passing stack-pointers to other threads
 * (this decreases performance as memory is allocated from pools instead
 * of keeping it on the stack)
 */
#define LWIP_MPU_COMPATIBLE             0

/**
 * LWIP_TCPIP_CORE_LOCKING
 * Creates a global mutex that is held during TCPIP thread operations.
 * Can be locked by client code to perform lwIP operations without changing
 * into TCPIP thread using callbacks. See LOCK_TCPIP_CORE() and
 * UNLOCK_TCPIP_CORE().
 * Your system should provide mutexes supporting priority inversion to use this.
 */
#define LWIP_TCPIP_CORE_LOCKING         0

/**
 * LWIP_TCPIP_CORE_LOCKING_INPUT: when LWIP_TCPIP_CORE_LOCKING is enabled,
 * this lets tcpip_input() grab the mutex for input packets as well,
 * instead of allocating a message and passing it to tcpip_thread.
 *
 * ATTENTION: this does not work when tcpip_input() is called from
 * interrupt context!
 */
#define LWIP_TCPIP_CORE_LOCKING_INPUT   0

/**
 * SYS_LIGHTWEIGHT_PROT==1: enable inter-task protection (and task-vs-interrupt
 * protection) for certain critical regions during buffer allocation, deallocation
 * and memory allocation and deallocation.
 * ATTENTION: This is required when using lwIP from more than one context! If
 * you disable this, you must be sure what you are doing!
 */
#define SYS_LIGHTWEIGHT_PROT            1
/**
 * @}
 */

/*
   ------------------------------------
   ---------- Memory options ----------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_mem Heap and memory pools
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * MEM_LIBC_MALLOC==1: Use malloc/free/realloc provided by your C-library
 * instead of the lwip internal allocator. Can save code size if you
 * already use it.
 */
#define MEM_LIBC_MALLOC                 1

#ifdef ESP_LWIP

#ifdef CONFIG_ESP_LWIP_MEM_DBG
#define ESP_LWIP_MEM_DBG                    1
#endif

size_t memp_malloc_get_size(size_t type);

/*
 * @brief allocate an only DRAM memory block for LWIP pbuf
 * 
 * @param s memory size
 * 
 * @return memory block pointer
 */
#define mem_malloc_ll(s)         heap_caps_malloc(s, MALLOC_CAP_8BIT)

/*
 * @brief allocate an only DRAM memory pool for LWIP pbuf
 * 
 * @param type memory type
 * 
 * @return memory pool pointer
 */
#define memp_malloc_ll(type)     heap_caps_malloc(memp_pools[type]->size, MALLOC_CAP_8BIT)
#endif

/**
 * Use DRAM instead of IRAM
 */
#if CONFIG_LWIP_USE_IRAM
#define mem_clib_free free
#define mem_clib_malloc malloc
#define mem_clib_calloc calloc
#else
#define mem_clib_free os_free
#define mem_clib_malloc os_malloc
#define mem_clib_calloc os_calloc
#endif

/**
 * MEMP_MEM_MALLOC==1: Use mem_malloc/mem_free instead of the lwip pool allocator.
 * Especially useful with MEM_LIBC_MALLOC but handle with care regarding execution
 * speed (heap alloc can be much slower than pool alloc) and usage from interrupts
 * (especially if your netif driver allocates PBUF_POOL pbufs for received frames
 * from interrupt)!
 * ATTENTION: Currently, this uses the heap for ALL pools (also for private pools,
 * not only for internal pools defined in memp_std.h)!
 */
#define MEMP_MEM_MALLOC                 1

/**
 * MEM_ALIGNMENT: should be set to the alignment of the CPU
 *    4 byte alignment -> \#define MEM_ALIGNMENT 4
 *    2 byte alignment -> \#define MEM_ALIGNMENT 2
 */
#define MEM_ALIGNMENT                   4

/*
   ------------------------------------------------
   ---------- Internal Memory Pool Sizes ----------
   ------------------------------------------------
*/
/**
 * @defgroup lwip_opts_memp Internal memory pools
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * MEMP_NUM_PBUF: the number of memp struct pbufs (used for PBUF_ROM and PBUF_REF).
 * If the application sends a lot of data out of ROM (or other static memory),
 * this should be set high.
 */
#define MEMP_NUM_PBUF                   16

/**
 * MEMP_NUM_RAW_PCB: Number of raw connection PCBs
 * (requires the LWIP_RAW option)
 */
#define MEMP_NUM_RAW_PCB                CONFIG_LWIP_MAX_RAW_PCBS

/**
 * MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
 * per active UDP "connection".
 * (requires the LWIP_UDP option)
 */
#define MEMP_NUM_UDP_PCB                CONFIG_LWIP_MAX_UDP_PCBS

/**
 * MEMP_NUM_TCP_PCB: the number of simultaneously active TCP connections.
 * (requires the LWIP_TCP option)
 */
#define MEMP_NUM_TCP_PCB                CONFIG_LWIP_MAX_ACTIVE_TCP

/**
 * MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP connections.
 * (requires the LWIP_TCP option)
 */
#define MEMP_NUM_TCP_PCB_LISTEN         CONFIG_LWIP_MAX_LISTENING_TCP

/**
 * MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP segments.
 * (requires the LWIP_TCP option)
 */
#define MEMP_NUM_TCP_SEG                16

/**
 * MEMP_NUM_REASSDATA: the number of IP packets simultaneously queued for
 * reassembly (whole packets, not fragments!)
 */
#define MEMP_NUM_REASSDATA              5

/**
 * MEMP_NUM_FRAG_PBUF: the number of IP fragments simultaneously sent
 * (fragments, not whole packets!).
 * This is only used with LWIP_NETIF_TX_SINGLE_PBUF==0 and only has to be > 1
 * with DMA-enabled MACs where the packet is not yet sent when netif->output
 * returns.
 */
#define MEMP_NUM_FRAG_PBUF              15

/**
 * MEMP_NUM_ARP_QUEUE: the number of simultaneously queued outgoing
 * packets (pbufs) that are waiting for an ARP request (to resolve
 * their destination address) to finish.
 * (requires the ARP_QUEUEING option)
 */
#define MEMP_NUM_ARP_QUEUE              30

/**
 * MEMP_NUM_IGMP_GROUP: The number of multicast groups whose network interfaces
 * can be members at the same time (one per netif - allsystems group -, plus one
 * per netif membership).
 * (requires the LWIP_IGMP option)
 */
#define MEMP_NUM_IGMP_GROUP             8

/**
 * MEMP_NUM_SYS_TIMEOUT: the number of simultaneously active timeouts.
 * The default number of timeouts is calculated here for all enabled modules.
 * The formula expects settings to be either '0' or '1'.
 */
#if ESP_LWIP
#define MEMP_NUM_SYS_TIMEOUT            (LWIP_TCP + IP_REASSEMBLY + LWIP_ARP + (2*LWIP_DHCP) + LWIP_AUTOIP + LWIP_IGMP + LWIP_DNS + (PPP_SUPPORT*6*MEMP_NUM_PPP_PCB) + (LWIP_IPV6 ? (1 + LWIP_IPV6_REASS + LWIP_IPV6_MLD) : 0)) + (ESP_GRATUITOUS_ARP ? 1 : 0)
#else
#define MEMP_NUM_SYS_TIMEOUT            (LWIP_TCP + IP_REASSEMBLY + LWIP_ARP + (2*LWIP_DHCP) + LWIP_AUTOIP + LWIP_IGMP + LWIP_DNS + (PPP_SUPPORT*6*MEMP_NUM_PPP_PCB) + (LWIP_IPV6 ? (1 + LWIP_IPV6_REASS + LWIP_IPV6_MLD) : 0))
#endif

/**
 * MEMP_NUM_NETBUF: the number of struct netbufs.
 * (only needed if you use the sequential API, like api_lib.c)
 */
#define MEMP_NUM_NETBUF                 2

/**
 * MEMP_NUM_NETCONN: the number of struct netconns.
 * (only needed if you use the sequential API, like api_lib.c)
 */
#define MEMP_NUM_NETCONN                CONFIG_LWIP_MAX_SOCKETS


/**
 * MEMP_NUM_TCPIP_MSG_API: the number of struct tcpip_msg, which are used
 * for callback/timeout API communication.
 * (only needed if you use tcpip.c)
 */
#define MEMP_NUM_TCPIP_MSG_API          8

/**
 * MEMP_NUM_TCPIP_MSG_INPKT: the number of struct tcpip_msg, which are used
 * for incoming packets.
 * (only needed if you use tcpip.c)
 */
#define MEMP_NUM_TCPIP_MSG_INPKT        8

/**
 * MEMP_NUM_NETDB: the number of concurrently running lwip_addrinfo() calls
 * (before freeing the corresponding memory using lwip_freeaddrinfo()).
 */
#define MEMP_NUM_NETDB                  1

/**
 * MEMP_NUM_LOCALHOSTLIST: the number of host entries in the local host list
 * if DNS_LOCAL_HOSTLIST_IS_DYNAMIC==1.
 */
#define MEMP_NUM_LOCALHOSTLIST          1

/**
 * PBUF_POOL_SIZE: the number of buffers in the pbuf pool.
 */
#define PBUF_POOL_SIZE                  16

/** MEMP_NUM_API_MSG: the number of concurrently active calls to various
 * socket, netconn, and tcpip functions
 */
#define MEMP_NUM_API_MSG                MEMP_NUM_TCPIP_MSG_API

/** MEMP_NUM_DNS_API_MSG: the number of concurrently active calls to netconn_gethostbyname
 */
#define MEMP_NUM_DNS_API_MSG            MEMP_NUM_TCPIP_MSG_API

/** MEMP_NUM_SOCKET_SETGETSOCKOPT_DATA: the number of concurrently active calls
 * to getsockopt/setsockopt
 */
#define MEMP_NUM_SOCKET_SETGETSOCKOPT_DATA MEMP_NUM_TCPIP_MSG_API

/** MEMP_NUM_NETIFAPI_MSG: the number of concurrently active calls to the
 * netifapi functions
 */
#define MEMP_NUM_NETIFAPI_MSG           MEMP_NUM_TCPIP_MSG_API
/**
 * @}
 */

/*
   ---------------------------------
   ---------- ARP options ----------
   ---------------------------------
*/
/**
 * @defgroup lwip_opts_arp ARP
 * @ingroup lwip_opts_ipv4
 * @{
 */
/**
 * LWIP_ARP==1: Enable ARP functionality.
 */
#define LWIP_ARP                        1

/**
 * ARP_TABLE_SIZE: Number of active MAC-IP address pairs cached.
 */
#define ARP_TABLE_SIZE                  CONFIG_LWIP_ARP_TABLE_SIZE

/** the time an ARP entry stays valid after its last update,
 *  for ARP_TMR_INTERVAL = 1000, this is
 *  (60 * 5) seconds = 5 minutes.
 */
#define ARP_MAXAGE                      CONFIG_LWIP_ARP_MAXAGE

/**
 * ARP_QUEUEING==1: Multiple outgoing packets are queued during hardware address
 * resolution. By default, only the most recent packet is queued per IP address.
 * This is sufficient for most protocols and mainly reduces TCP connection
 * startup time. Set this to 1 if you know your application sends more than one
 * packet in a row to an IP address that is not in the ARP cache.
 */
#define ARP_QUEUEING                    1

/** The maximum number of packets which may be queued for each
 *  unresolved address by other network layers. Defaults to 3, 0 means disabled.
 *  Old packets are dropped, new packets are queued.
 */
#define ARP_QUEUE_LEN                   3

/**
 * ETHARP_SUPPORT_VLAN==1: support receiving and sending ethernet packets with
 * VLAN header. See the description of LWIP_HOOK_VLAN_CHECK and
 * LWIP_HOOK_VLAN_SET hooks to check/set VLAN headers.
 * Additionally, you can define ETHARP_VLAN_CHECK to an u16_t VLAN ID to check.
 * If ETHARP_VLAN_CHECK is defined, only VLAN-traffic for this VLAN is accepted.
 * If ETHARP_VLAN_CHECK is not defined, all traffic is accepted.
 * Alternatively, define a function/define ETHARP_VLAN_CHECK_FN(eth_hdr, vlan)
 * that returns 1 to accept a packet or 0 to drop a packet.
 */
#define ETHARP_SUPPORT_VLAN             0

/** LWIP_ETHERNET==1: enable ethernet support even though ARP might be disabled
 */
#define LWIP_ETHERNET                   LWIP_ARP

/** ETH_PAD_SIZE: number of bytes added before the ethernet header to ensure
 * alignment of payload after that header. Since the header is 14 bytes long,
 * without this padding e.g. addresses in the IP header will not be aligned
 * on a 32-bit boundary, so setting this to 2 can speed up 32-bit-platforms.
 */
#define ETH_PAD_SIZE                    0

/** ETHARP_SUPPORT_STATIC_ENTRIES==1: enable code to support static ARP table
 * entries (using etharp_add_static_entry/etharp_remove_static_entry).
 */
#define ETHARP_SUPPORT_STATIC_ENTRIES   0

/** ETHARP_TABLE_MATCH_NETIF==1: Match netif for ARP table entries.
 * If disabled, duplicate IP address on multiple netifs are not supported
 * (but this should only occur for AutoIP).
 */
#define ETHARP_TABLE_MATCH_NETIF        0
/**
 * @}
 */

/*
   --------------------------------
   ---------- IP options ----------
   --------------------------------
*/
/**
 * @defgroup lwip_opts_ipv4 IPv4
 * @ingroup lwip_opts
 * @{
 */
/**
 * LWIP_IPV4==1: Enable IPv4
 */
#define LWIP_IPV4                       1

/**
 * IP_FORWARD==1: Enables the ability to forward IP packets across network
 * interfaces. If you are going to run lwIP on a device with only one network
 * interface, define this to 0.
 */
#define IP_FORWARD                      0

/**
 * IP_REASSEMBLY==1: Reassemble incoming fragmented IP packets. Note that
 * this option does not affect outgoing packet sizes, which can be controlled
 * via IP_FRAG.
 */
#define IP_REASSEMBLY                   CONFIG_LWIP_IP_REASSEMBLY

/**
 * IP_FRAG==1: Fragment outgoing IP packets if their size exceeds MTU. Note
 * that this option does not affect incoming packet sizes, which can be
 * controlled via IP_REASSEMBLY.
 */
#define IP_FRAG                         CONFIG_LWIP_IP_FRAG

/**
 * IP_OPTIONS_ALLOWED: Defines the behavior for IP options.
 *      IP_OPTIONS_ALLOWED==0: All packets with IP options are dropped.
 *      IP_OPTIONS_ALLOWED==1: IP options are allowed (but not parsed).
 */
#define IP_OPTIONS_ALLOWED              1

/**
 * IP_REASS_MAXAGE: Maximum time (in multiples of IP_TMR_INTERVAL - so seconds, normally)
 * a fragmented IP packet waits for all fragments to arrive. If not all fragments arrived
 * in this time, the whole packet is discarded.
 */
#define IP_REASS_MAXAGE                 3

/**
 * IP_REASS_MAX_PBUFS: Total maximum amount of pbufs waiting to be reassembled.
 * Since the received pbufs are enqueued, be sure to configure
 * PBUF_POOL_SIZE > IP_REASS_MAX_PBUFS so that the stack is still able to receive
 * packets even if the maximum amount of fragments is enqueued for reassembly!
 */
#define IP_REASS_MAX_PBUFS              CONFIG_LWIP_IP_REASS_MAX_PBUFS

/**
 * IP_DEFAULT_TTL: Default value for Time-To-Live used by transport layers.
 */
#define IP_DEFAULT_TTL                  255

/**
 * IP_SOF_BROADCAST=1: Use the SOF_BROADCAST field to enable broadcast
 * filter per pcb on udp and raw send operations. To enable broadcast filter
 * on recv operations, you also have to set IP_SOF_BROADCAST_RECV=1.
 */
#define IP_SOF_BROADCAST                CONFIG_LWIP_IP_SOF_BROADCAST

/**
 * IP_SOF_BROADCAST_RECV (requires IP_SOF_BROADCAST=1) enable the broadcast
 * filter on recv operations.
 */
#define IP_SOF_BROADCAST_RECV           CONFIG_LWIP_IP_SOF_BROADCAST_RECV

/**
 * IP_FORWARD_ALLOW_TX_ON_RX_NETIF==1: allow ip_forward() to send packets back
 * out on the netif where it was received. This should only be used for
 * wireless networks.
 * ATTENTION: When this is 1, make sure your netif driver correctly marks incoming
 * link-layer-broadcast/multicast packets as such using the corresponding pbuf flags!
 */
#define IP_FORWARD_ALLOW_TX_ON_RX_NETIF 0

/**
 * LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS==1: randomize the local port for the first
 * local TCP/UDP pcb (default==0). This can prevent creating predictable port
 * numbers after booting a device.
 */
#define LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS 0
/**
 * @}
 */

/*
   ----------------------------------
   ---------- ICMP options ----------
   ----------------------------------
*/
/**
 * @defgroup lwip_opts_icmp ICMP
 * @ingroup lwip_opts_ipv4
 * @{
 */
/**
 * LWIP_ICMP==1: Enable ICMP module inside the IP stack.
 * Be careful, disable that make your product non-compliant to RFC1122
 */
#define LWIP_ICMP                       CONFIG_LWIP_ICMP

/**
 * ICMP_TTL: Default value for Time-To-Live used by ICMP packets.
 */
#define ICMP_TTL                       (IP_DEFAULT_TTL)

/**
 * LWIP_BROADCAST_PING==1: respond to broadcast pings (default is unicast only)
 */
#define LWIP_BROADCAST_PING             CONFIG_LWIP_BROADCAST_PING

/**
 * LWIP_MULTICAST_PING==1: respond to multicast pings (default is unicast only)
 */
#define LWIP_MULTICAST_PING             CONFIG_LWIP_MULTICAST_PING
/**
 * @}
 */

/*
   ---------------------------------
   ---------- RAW options ----------
   ---------------------------------
*/
/**
 * @defgroup lwip_opts_raw RAW
 * @ingroup lwip_opts_callback
 * @{
 */
/**
 * LWIP_RAW==1: Enable application layer to hook into the IP layer itself.
 */
#define LWIP_RAW                        CONFIG_LWIP_RAW

/**
 * LWIP_RAW==1: Enable application layer to hook into the IP layer itself.
 */
#define RAW_TTL                        (IP_DEFAULT_TTL)
/**
 * @}
 */

/*
   ----------------------------------
   ---------- DHCP options ----------
   ----------------------------------
*/
/**
 * @defgroup lwip_opts_dhcp DHCP
 * @ingroup lwip_opts_ipv4
 * @{
 */
/**
 * LWIP_DHCP==1: Enable DHCP module.
 */
#define LWIP_DHCP                       1

/**
 * DHCP_DOES_ARP_CHECK==1: Do an ARP check on the offered address.
 */
#define DHCP_DOES_ARP_CHECK             CONFIG_LWIP_DHCP_DOES_ARP_CHECK

/**
 * LWIP_DHCP_CHECK_LINK_UP==1: dhcp_start() only really starts if the netif has
 * NETIF_FLAG_LINK_UP set in its flags. As this is only an optimization and
 * netif drivers might not set this flag, the default is off. If enabled,
 * netif_set_link_up() must be called to continue dhcp starting.
 */
#define LWIP_DHCP_CHECK_LINK_UP         0

/**
 * LWIP_DHCP_BOOTP_FILE==1: Store offered_si_addr and boot_file_name.
 */
#define LWIP_DHCP_BOOTP_FILE            0

/**
 * LWIP_DHCP_GETS_NTP==1: Request NTP servers with discover/select. For each
 * response packet, an callback is called, which has to be provided by the port:
 * void dhcp_set_ntp_servers(u8_t num_ntp_servers, ip_addr_t* ntp_server_addrs);
*/
#define LWIP_DHCP_GET_NTP_SRV           0

/**
 * The maximum of NTP servers requested
 */
#define LWIP_DHCP_MAX_NTP_SERVERS       CONFIG_LWIP_DHCP_MAX_NTP_SERVERS

/**
 * LWIP_DHCP_MAX_DNS_SERVERS > 0: Request DNS servers with discover/select.
 * DHCP servers received in the response are passed to DNS via @ref dns_setserver()
 * (up to the maximum limit defined here).
 */
#define LWIP_DHCP_MAX_DNS_SERVERS       DNS_MAX_SERVERS

/**
 * LWIP_DHCP_DISCOVER_RETRANSMISSION_INTERVAL: DHCP discover retry backoff time. 
 * (default is 250, a conservative default, the maximum is 1000.)
 * Since for embedded devices it's not that hard to miss a discover packet, so 
 * it is necessary to reduce the discovery retry backoff time.
 */
#define LWIP_DHCP_DISCOVER_RETRANSMISSION_INTERVAL       CONFIG_LWIP_DHCP_DISCOVER_RETRANSMISSION_INTERVAL
/**
 * @}
 */

/*
   ------------------------------------
   ---------- AUTOIP options ----------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_autoip AUTOIP
 * @ingroup lwip_opts_ipv4
 * @{
 */
/**
 * LWIP_AUTOIP==1: Enable AUTOIP module.
 */
#define LWIP_AUTOIP                     CONFIG_LWIP_AUTOIP

/**
 * LWIP_DHCP_AUTOIP_COOP==1: Allow DHCP and AUTOIP to be both enabled on
 * the same interface at the same time.
 */
#define LWIP_DHCP_AUTOIP_COOP           CONFIG_LWIP_AUTOIP

/**
 * LWIP_DHCP_AUTOIP_COOP_TRIES: Set to the number of DHCP DISCOVER probes
 * that should be sent before falling back on AUTOIP (the DHCP client keeps
 * running in this case). This can be set as low as 1 to get an AutoIP address
 * very  quickly, but you should be prepared to handle a changing IP address
 * when DHCP overrides AutoIP.
 */
#define LWIP_DHCP_AUTOIP_COOP_TRIES     CONFIG_LWIP_DHCP_AUTOIP_COOP_TRIES
/**
 * @}
 */

/*
   ----------------------------------
   ----- SNMP MIB2 support      -----
   ----------------------------------
*/
/**
 * @defgroup lwip_opts_mib2 SNMP MIB2 callbacks
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * LWIP_MIB2_CALLBACKS==1: Turn on SNMP MIB2 callbacks.
 * Turn this on to get callbacks needed to implement MIB2.
 * Usually MIB2_STATS should be enabled, too.
 */
#define LWIP_MIB2_CALLBACKS             0
/**
 * @}
 */

#define SNTP_SERVER_DNS                 1

#include <sys/time.h>

#define SNTP_SET_SYSTEM_TIME_US(sec, us)  \
    do { \
        struct timeval tv = { .tv_sec = sec, .tv_usec = us }; \
        settimeofday(&tv, NULL); \
    } while (0);

/*
   ----------------------------------
   ----- Multicast/IGMP options -----
   ----------------------------------
*/
/**
 * @defgroup lwip_opts_igmp IGMP
 * @ingroup lwip_opts_ipv4
 * @{
 */
/**
 * LWIP_IGMP==1: Turn on IGMP module.
 */
#define LWIP_IGMP                       CONFIG_LWIP_IGMP

/**
 * LWIP_MULTICAST_TX_OPTIONS==1: Enable multicast TX support like the socket options
 * IP_MULTICAST_TTL/IP_MULTICAST_IF/IP_MULTICAST_LOOP
 */
#define LWIP_MULTICAST_TX_OPTIONS       (LWIP_IGMP && LWIP_UDP)
/**
 * @}
 */

/*
   ----------------------------------
   ---------- DNS options -----------
   ----------------------------------
*/
/**
 * @defgroup lwip_opts_dns DNS
 * @ingroup lwip_opts_callback
 * @{
 */
/**
 * LWIP_DNS==1: Turn on DNS module. UDP must be available for DNS
 * transport.
 */
#define LWIP_DNS                        1

/** DNS maximum number of entries to maintain locally. */
#define DNS_TABLE_SIZE                  4

/** DNS maximum host name length supported in the name table. */
#define DNS_MAX_NAME_LENGTH             256

/** The maximum of DNS servers
 * The first server can be initialized automatically by defining
 * DNS_SERVER_ADDRESS(ipaddr), where 'ipaddr' is an 'ip_addr_t*'
 */
#define DNS_MAX_SERVERS                 CONFIG_DNS_MAX_SERVERS
#define DNS_FALLBACK_SERVER_INDEX       (DNS_MAX_SERVERS - 1)

/** DNS do a name checking between the query and the response. */
#define DNS_DOES_NAME_CHECK             1

/** LWIP_DNS_SECURE: controls the security level of the DNS implementation
 * Use all DNS security features by default.
 * This is overridable but should only be needed by very small targets
 * or when using against non standard DNS servers. */
#define LWIP_DNS_SECURE (LWIP_DNS_SECURE_RAND_XID | LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING | LWIP_DNS_SECURE_RAND_SRC_PORT)

/* A list of DNS security features follows */
#define LWIP_DNS_SECURE_RAND_XID                1
#define LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING 2
#define LWIP_DNS_SECURE_RAND_SRC_PORT           4

/** DNS_LOCAL_HOSTLIST: Implements a local host-to-address list. If enabled, you have to define an initializer:
 *  \#define DNS_LOCAL_HOSTLIST_INIT {DNS_LOCAL_HOSTLIST_ELEM("host_ip4", IPADDR4_INIT_BYTES(1,2,3,4)), \
 *                                    DNS_LOCAL_HOSTLIST_ELEM("host_ip6", IPADDR6_INIT_HOST(123, 234, 345, 456)}
 *
 *  Instead, you can also use an external function:
 *  \#define DNS_LOOKUP_LOCAL_EXTERN(x) extern err_t my_lookup_function(const char *name, ip_addr_t *addr, u8_t dns_addrtype)
 *  that looks up the IP address and returns ERR_OK if found (LWIP_DNS_ADDRTYPE_xxx is passed in dns_addrtype).
 */
#define DNS_LOCAL_HOSTLIST              0

/** If this is turned on, the local host-list can be dynamically changed
 *  at runtime. */
#define DNS_LOCAL_HOSTLIST_IS_DYNAMIC   0

/** Set this to 1 to enable querying ".local" names via mDNS
 *  using a One-Shot Multicast DNS Query */
#define LWIP_DNS_SUPPORT_MDNS_QUERIES  0
/**
 * @}
 */

/*
   ---------------------------------
   ---------- UDP options ----------
   ---------------------------------
*/
/**
 * @defgroup lwip_opts_udp UDP
 * @ingroup lwip_opts_callback
 * @{
 */
/**
 * LWIP_UDP==1: Turn on UDP.
 */
#define LWIP_UDP                        1

/**
 * LWIP_UDPLITE==1: Turn on UDP-Lite. (Requires LWIP_UDP)
 */
#define LWIP_UDPLITE                    0

/**
 * UDP_TTL: Default Time-To-Live value.
 */
#define UDP_TTL                         (IP_DEFAULT_TTL)

/**
 * LWIP_NETBUF_RECVINFO==1: append destination addr and port to every netbuf.
 */
#define LWIP_NETBUF_RECVINFO            0
/**
 * @}
 */

/*
   ---------------------------------
   ---------- TCP options ----------
   ---------------------------------
*/
/**
 * @defgroup lwip_opts_tcp TCP
 * @ingroup lwip_opts_callback
 * @{
 */
/**
 * LWIP_TCP==1: Turn on TCP.
 */
#define LWIP_TCP                        1

/**
 * TCP_TTL: Default Time-To-Live value.
 */
#define TCP_TTL                         (IP_DEFAULT_TTL)

/**
 * TCP_WND: The size of a TCP window.  This must be at least
 * (2 * TCP_MSS) for things to work well.
 * ATTENTION: when using TCP_RCV_SCALE, TCP_WND is the total size
 * with scaling applied. Maximum window value in the TCP header
 * will be TCP_WND >> TCP_RCV_SCALE
 */
#define TCP_WND                         CONFIG_TCP_WND_DEFAULT

/**
 * TCP_MAXRTX: Maximum number of retransmissions of data segments.
 */
#define TCP_MAXRTX                      CONFIG_TCP_MAXRTX

/**
 * TCP_SYNMAXRTX: Maximum number of retransmissions of SYN segments.
 */
#define TCP_SYNMAXRTX                   CONFIG_TCP_SYNMAXRTX

/**
 * TCP_QUEUE_OOSEQ==1: TCP will queue segments that arrive out of order.
 * Define to 0 if your device is low on memory.
 */
#define TCP_QUEUE_OOSEQ                 CONFIG_TCP_QUEUE_OOSEQ

/**
 * TCP_MSS: TCP Maximum segment size. (default is 536, a conservative default,
 * you might want to increase this.)
 * For the receive side, this MSS is advertised to the remote side
 * when opening a connection. For the transmit size, this MSS sets
 * an upper limit on the MSS advertised by the remote host.
 */
#define TCP_MSS                         CONFIG_TCP_MSS

/**
 * TCP_CALCULATE_EFF_SEND_MSS: "The maximum size of a segment that TCP really
 * sends, the 'effective send MSS,' MUST be the smaller of the send MSS (which
 * reflects the available reassembly buffer size at the remote host) and the
 * largest size permitted by the IP layer" (RFC 1122)
 * Setting this to 1 enables code that checks TCP_MSS against the MTU of the
 * netif used for a connection and limits the MSS if it would be too big otherwise.
 */
#define TCP_CALCULATE_EFF_SEND_MSS      1


/**
 * TCP_SND_BUF: TCP sender buffer space (bytes).
 * To achieve good performance, this should be at least 2 * TCP_MSS.
 */
#define TCP_SND_BUF                     CONFIG_TCP_SND_BUF_DEFAULT

/**
 * TCP_SND_QUEUELEN: TCP sender buffer space (pbufs). This must be at least
 * as much as (2 * TCP_SND_BUF/TCP_MSS) for things to work.
 */
#define TCP_SND_QUEUELEN                ((4 * (TCP_SND_BUF) + (TCP_MSS - 1))/(TCP_MSS))

/**
 * TCP_SNDLOWAT: TCP writable space (bytes). This must be less than
 * TCP_SND_BUF. It is the amount of space which must be available in the
 * TCP snd_buf for select to return writable (combined with TCP_SNDQUEUELOWAT).
 */
#define TCP_SNDLOWAT                    LWIP_MIN(LWIP_MAX(((TCP_SND_BUF)/2), (2 * TCP_MSS) + 1), (TCP_SND_BUF) - 1)

/**
 * TCP_SNDQUEUELOWAT: TCP writable bufs (pbuf count). This must be less
 * than TCP_SND_QUEUELEN. If the number of pbufs queued on a pcb drops below
 * this number, select returns writable (combined with TCP_SNDLOWAT).
 */
#define TCP_SNDQUEUELOWAT               LWIP_MAX(((TCP_SND_QUEUELEN)/2), 5)

/**
 * TCP_OOSEQ_MAX_BYTES: The maximum number of bytes queued on ooseq per pcb.
 * Default is 0 (no limit). Only valid for TCP_QUEUE_OOSEQ==1.
 */
#define TCP_OOSEQ_MAX_BYTES             0

/**
 * TCP_OOSEQ_MAX_PBUFS: The maximum number of pbufs queued on ooseq per pcb.
 * Default is 0 (no limit). Only valid for TCP_QUEUE_OOSEQ==1.
 */
#define TCP_OOSEQ_MAX_PBUFS             0

/**
 * TCP_LISTEN_BACKLOG: Enable the backlog option for tcp listen pcb.
 */
#define TCP_LISTEN_BACKLOG              1

/**
 * The maximum allowed backlog for TCP listen netconns.
 * This backlog is used unless another is explicitly specified.
 * 0xff is the maximum (u8_t).
 */
#define TCP_DEFAULT_LISTEN_BACKLOG      0xff

/**
 * TCP_OVERSIZE: The maximum number of bytes that tcp_write may
 * allocate ahead of time in an attempt to create shorter pbuf chains
 * for transmission. The meaningful range is 0 to TCP_MSS. Some
 * suggested values are:
 *
 * 0:         Disable oversized allocation. Each tcp_write() allocates a new
              pbuf (old behaviour).
 * 1:         Allocate size-aligned pbufs with minimal excess. Use this if your
 *            scatter-gather DMA requires aligned fragments.
 * 128:       Limit the pbuf/memory overhead to 20%.
 * TCP_MSS:   Try to create unfragmented TCP packets.
 * TCP_MSS/4: Try to create 4 fragments or less per TCP packet.
 */
#ifdef CONFIG_TCP_OVERSIZE_MSS
#define TCP_OVERSIZE                    TCP_MSS
#endif
#ifdef CONFIG_TCP_OVERSIZE_QUARTER_MSS
#define TCP_OVERSIZE                    (TCP_MSS/4)
#endif
#ifdef CONFIG_TCP_OVERSIZE_DISABLE
#define TCP_OVERSIZE                    0
#endif
#ifndef TCP_OVERSIZE
#error "One of CONFIG_TCP_OVERSIZE_xxx options should be set by sdkconfig"
#endif

/**
 * LWIP_TCP_TIMESTAMPS==1: support the TCP timestamp option.
 * The timestamp option is currently only used to help remote hosts, it is not
 * really used locally. Therefore, it is only enabled when a TS option is
 * received in the initial SYN packet from a remote host.
 */
#define LWIP_TCP_TIMESTAMPS             0

/**
 * TCP_WND_UPDATE_THRESHOLD: difference in window to trigger an
 * explicit window update
 */
#define TCP_WND_UPDATE_THRESHOLD   LWIP_MIN((TCP_WND / 4), (TCP_MSS * 4))

/**
 * LWIP_EVENT_API and LWIP_CALLBACK_API: Only one of these should be set to 1.
 *     LWIP_EVENT_API==1: The user defines lwip_tcp_event() to receive all
 *         events (accept, sent, etc) that happen in the system.
 *     LWIP_CALLBACK_API==1: The PCB callback function is called directly
 *         for the event. This is the default.
 */
#define LWIP_EVENT_API                  0
#define LWIP_CALLBACK_API               1

/**
 * LWIP_WND_SCALE and TCP_RCV_SCALE:
 * Set LWIP_WND_SCALE to 1 to enable window scaling.
 * Set TCP_RCV_SCALE to the desired scaling factor (shift count in the
 * range of [0..14]).
 * When LWIP_WND_SCALE is enabled but TCP_RCV_SCALE is 0, we can use a large
 * send window while having a small receive window only.
 */
#define LWIP_WND_SCALE                  0
#define TCP_RCV_SCALE                   0

/**
 * LWIP_TCP_RTO_TIME: make TCP RTO time configurable.
 * Default is 1 second.
 */
#define LWIP_TCP_RTO_TIME             CONFIG_LWIP_TCP_RTO_TIME
/**
 * @}
 */

/*
   ----------------------------------
   ---------- Pbuf options ----------
   ----------------------------------
*/
/**
 * @defgroup lwip_opts_pbuf PBUF
 * @ingroup lwip_opts
 * @{
 */
/**
 * PBUF_LINK_HLEN: the number of bytes that should be allocated for a
 * link level header. The default is 14, the standard value for
 * Ethernet.
 */
#define PBUF_LINK_HLEN                  (14 + ETH_PAD_SIZE)

/**
 * PBUF_LINK_ENCAPSULATION_HLEN: the number of bytes that should be allocated
 * for an additional encapsulation header before ethernet headers (e.g. 802.11)
 */
#define PBUF_LINK_ENCAPSULATION_HLEN    36u

#define LWIP_SUPPORT_CUSTOM_PBUF    1

/**
 * PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. The default is
 * designed to accommodate single full size TCP frame in one pbuf, including
 * TCP_MSS, IP header, and link header.
 */
#define PBUF_POOL_BUFSIZE               LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_ENCAPSULATION_HLEN+PBUF_LINK_HLEN)
/**
 * @}
 */

/*
   ------------------------------------------------
   ---------- Network Interfaces options ----------
   ------------------------------------------------
*/
/**
 * @defgroup lwip_opts_netif NETIF
 * @ingroup lwip_opts
 * @{
 */
/**
 * LWIP_NETIF_HOSTNAME==1: use DHCP_OPTION_HOSTNAME with netif's hostname
 * field.
 */
#define LWIP_NETIF_HOSTNAME             1

/**
 * LWIP_NETIF_API==1: Support netif api (in netifapi.c)
 */
#define LWIP_NETIF_API                  0

/**
 * LWIP_NETIF_STATUS_CALLBACK==1: Support a callback function whenever an interface
 * changes its up/down status (i.e., due to DHCP IP acquisition)
 */
#define LWIP_NETIF_STATUS_CALLBACK      0

/**
 * LWIP_NETIF_LINK_CALLBACK==1: Support a callback function from an interface
 * whenever the link changes (i.e., link down)
 */
#define LWIP_NETIF_LINK_CALLBACK        0

/**
 * LWIP_NETIF_REMOVE_CALLBACK==1: Support a callback function that is called
 * when a netif has been removed
 */
#define LWIP_NETIF_REMOVE_CALLBACK      0

/**
 * LWIP_NETIF_HWADDRHINT==1: Cache link-layer-address hints (e.g. table
 * indices) in struct netif. TCP and UDP can make use of this to prevent
 * scanning the ARP table for every sent packet. While this is faster for big
 * ARP tables or many concurrent connections, it might be counterproductive
 * if you have a tiny ARP table or if there never are concurrent connections.
 */
#define LWIP_NETIF_HWADDRHINT           0

/**
 * LWIP_NETIF_TX_SINGLE_PBUF: if this is set to 1, lwIP tries to put all data
 * to be sent into one single pbuf. This is for compatibility with DMA-enabled
 * MACs that do not support scatter-gather.
 * Beware that this might involve CPU-memcpy before transmitting that would not
 * be needed without this flag! Use this only if you need to!
 *
 * @todo: TCP and IP-frag do not work with this, yet:
 */
#define LWIP_NETIF_TX_SINGLE_PBUF             1

/**
 * LWIP_NUM_NETIF_CLIENT_DATA: Number of clients that may store
 * data in client_data member array of struct netif.
 */
#define LWIP_NUM_NETIF_CLIENT_DATA            0
/**
 * @}
 */

/*
   ------------------------------------
   ---------- LOOPIF options ----------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_loop Loopback interface
 * @ingroup lwip_opts_netif
 * @{
 */
/**
 * LWIP_HAVE_LOOPIF==1: Support loop interface (127.0.0.1).
 * This is only needed when no real netifs are available. If at least one other
 * netif is available, loopback traffic uses this netif.
 */
#define LWIP_HAVE_LOOPIF                LWIP_NETIF_LOOPBACK

/**
 * LWIP_LOOPIF_MULTICAST==1: Support multicast/IGMP on loop interface (127.0.0.1).
 */
#define LWIP_LOOPIF_MULTICAST               0

/**
 * LWIP_NETIF_LOOPBACK==1: Support sending packets with a destination IP
 * address equal to the netif IP address, looping them back up the stack.
 */
#define LWIP_NETIF_LOOPBACK             CONFIG_LWIP_NETIF_LOOPBACK

/**
 * LWIP_LOOPBACK_MAX_PBUFS: Maximum number of pbufs on queue for loopback
 * sending for each netif (0 = disabled)
 */
#define LWIP_LOOPBACK_MAX_PBUFS         CONFIG_LWIP_LOOPBACK_MAX_PBUFS

/**
 * LWIP_NETIF_LOOPBACK_MULTITHREADING: Indicates whether threading is enabled in
 * the system, as netifs must change how they behave depending on this setting
 * for the LWIP_NETIF_LOOPBACK option to work.
 * Setting this is needed to avoid reentering non-reentrant functions like
 * tcp_input().
 *    LWIP_NETIF_LOOPBACK_MULTITHREADING==1: Indicates that the user is using a
 *       multithreaded environment like tcpip.c. In this case, netif->input()
 *       is called directly.
 *    LWIP_NETIF_LOOPBACK_MULTITHREADING==0: Indicates a polling (or NO_SYS) setup.
 *       The packets are put on a list and netif_poll() must be called in
 *       the main application loop.
 */
#define LWIP_NETIF_LOOPBACK_MULTITHREADING    (!NO_SYS)
/**
 * @}
 */

/*
   ------------------------------------
   ---------- Thread options ----------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_thread Threading
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * TCPIP_THREAD_NAME: The name assigned to the main tcpip thread.
 */
#define TCPIP_THREAD_NAME              "tiT"

/**
 * TCPIP_THREAD_STACKSIZE: The stack size used by the main tcpip thread.
 * The stack size value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#define TCPIP_THREAD_STACKSIZE          CONFIG_TCPIP_TASK_STACK_SIZE

/**
 * TCPIP_THREAD_PRIO: The priority assigned to the main tcpip thread.
 * The priority value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#define TCPIP_THREAD_PRIO               (configMAX_PRIORITIES-7)

/**
 * TCPIP_MBOX_SIZE: The mailbox size for the tcpip thread messages
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when tcpip_init is called.
 */
#define TCPIP_MBOX_SIZE                 CONFIG_TCPIP_RECVMBOX_SIZE

/**
 * Define this to something that triggers a watchdog. This is called from
 * tcpip_thread after processing a message.
 */
#define LWIP_TCPIP_THREAD_ALIVE()

/**
 * SLIPIF_THREAD_NAME: The name assigned to the slipif_loop thread.
 */
#define SLIPIF_THREAD_NAME             "slipif_loop"

/**
 * SLIP_THREAD_STACKSIZE: The stack size used by the slipif_loop thread.
 * The stack size value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#define SLIPIF_THREAD_STACKSIZE         0

/**
 * SLIPIF_THREAD_PRIO: The priority assigned to the slipif_loop thread.
 * The priority value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#define SLIPIF_THREAD_PRIO              1

/**
 * DEFAULT_THREAD_NAME: The name assigned to any other lwIP thread.
 */
#define DEFAULT_THREAD_NAME            "lwIP"

/**
 * DEFAULT_THREAD_STACKSIZE: The stack size used by any other lwIP thread.
 * The stack size value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#define DEFAULT_THREAD_STACKSIZE        TCPIP_THREAD_STACKSIZE

/**
 * DEFAULT_THREAD_PRIO: The priority assigned to any other lwIP thread.
 * The priority value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#define DEFAULT_THREAD_PRIO             TCPIP_THREAD_PRIO

/**
 * DEFAULT_RAW_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_RAW. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created.
 */
#define DEFAULT_RAW_RECVMBOX_SIZE       6

/**
 * DEFAULT_UDP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_UDP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created.
 */
#define DEFAULT_UDP_RECVMBOX_SIZE       CONFIG_UDP_RECVMBOX_SIZE

/**
 * DEFAULT_TCP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_TCP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created.
 */
#define DEFAULT_TCP_RECVMBOX_SIZE       CONFIG_TCP_RECVMBOX_SIZE

/**
 * DEFAULT_ACCEPTMBOX_SIZE: The mailbox size for the incoming connections.
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when the acceptmbox is created.
 */
#define DEFAULT_ACCEPTMBOX_SIZE         6

/*
   ----------------------------------------------
   ---------- Sequential layer options ----------
   ----------------------------------------------
*/
/**
 * @defgroup lwip_opts_netconn Netconn
 * @ingroup lwip_opts_threadsafe_apis
 * @{
 */
/**
 * LWIP_NETCONN==1: Enable Netconn API (require to use api_lib.c)
 */
#define LWIP_NETCONN                    1

/** LWIP_TCPIP_TIMEOUT==1: Enable tcpip_timeout/tcpip_untimeout to create
 * timers running in tcpip_thread from another thread.
 */
#define LWIP_TCPIP_TIMEOUT              0

/** LWIP_NETCONN_SEM_PER_THREAD==1: Use one (thread-local) semaphore per
 * thread calling socket/netconn functions instead of allocating one
 * semaphore per netconn (and per select etc.)
 * ATTENTION: a thread-local semaphore for API calls is needed:
 * - LWIP_NETCONN_THREAD_SEM_GET() returning a sys_sem_t*
 * - LWIP_NETCONN_THREAD_SEM_ALLOC() creating the semaphore
 * - LWIP_NETCONN_THREAD_SEM_FREE() freeing the semaphore
 * The latter 2 can be invoked up by calling netconn_thread_init()/netconn_thread_cleanup().
 * Ports may call these for threads created with sys_thread_new().
 */
#define LWIP_NETCONN_SEM_PER_THREAD     1

/** LWIP_NETCONN_FULLDUPLEX==1: Enable code that allows reading from one thread,
 * writing from a 2nd thread and closing from a 3rd thread at the same time.
 * ATTENTION: This is currently really alpha! Some requirements:
 * - LWIP_NETCONN_SEM_PER_THREAD==1 is required to use one socket/netconn from
 *   multiple threads at once
 * - sys_mbox_free() has to unblock receive tasks waiting on recvmbox/acceptmbox
 *   and prevent a task pending on this during/after deletion
 */
#define LWIP_NETCONN_FULLDUPLEX         0
/**
 * @}
 */

/*
   ------------------------------------
   ---------- Socket options ----------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_socket Sockets
 * @ingroup lwip_opts_threadsafe_apis
 * @{
 */
/**
 * LWIP_SOCKET==1: Enable Socket API (require to use sockets.c)
 */
#define LWIP_SOCKET                     1

/* LWIP_SOCKET_SET_ERRNO==1: Set errno when socket functions cannot complete
 * successfully, as required by POSIX. Default is POSIX-compliant.
 */
#define LWIP_SOCKET_SET_ERRNO           1

/**
 * LWIP_COMPAT_SOCKETS==1: Enable BSD-style sockets functions names through defines.
 * LWIP_COMPAT_SOCKETS==2: Same as ==1 but correctly named functions are created.
 * While this helps code completion, it might conflict with existing libraries.
 * (only used if you use sockets.c)
 */
#define LWIP_COMPAT_SOCKETS             1

/**
 * LWIP_POSIX_SOCKETS_IO_NAMES==1: Enable POSIX-style sockets functions names.
 * Disable this option if you use a POSIX operating system that uses the same
 * names (read, write & close). (only used if you use sockets.c)
 */
#ifdef CONFIG_USING_ESP_VFS
#define LWIP_POSIX_SOCKETS_IO_NAMES     0
#include <unistd.h>  /* close/read/write */
#else
#define LWIP_POSIX_SOCKETS_IO_NAMES     1
#endif

/**
 * LWIP_SOCKET_OFFSET==n: Increases the file descriptor number created by LwIP with n.
 * This can be useful when there are multiple APIs which create file descriptors.
 * When they all start with a different offset and you won't make them overlap you can
 * re implement read/write/close/ioctl/fnctl to send the requested action to the right
 * library (sharing select will need more work though).
 */
#ifdef CONFIG_USING_ESP_VFS
#define LWIP_SOCKET_OFFSET              (FD_SETSIZE - CONFIG_LWIP_MAX_SOCKETS)
#else
#define LWIP_SOCKET_OFFSET              0
#endif

/**
 * LWIP_TCP_KEEPALIVE==1: Enable TCP_KEEPIDLE, TCP_KEEPINTVL and TCP_KEEPCNT
 * options processing. Note that TCP_KEEPIDLE and TCP_KEEPINTVL have to be set
 * in seconds. (does not require sockets.c, and will affect tcp.c)
 */
#define LWIP_TCP_KEEPALIVE              1

/**
 * LWIP_SO_SNDTIMEO==1: Enable send timeout for sockets/netconns and
 * SO_SNDTIMEO processing.
 */
#define LWIP_SO_SNDTIMEO                1

/**
 * LWIP_SO_RCVTIMEO==1: Enable receive timeout for sockets/netconns and
 * SO_RCVTIMEO processing.
 */
#define LWIP_SO_RCVTIMEO                1

/**
 * LWIP_SO_SNDRCVTIMEO_NONSTANDARD==1: SO_RCVTIMEO/SO_SNDTIMEO take an int
 * (milliseconds, much like winsock does) instead of a struct timeval (default).
 */
#define LWIP_SO_SNDRCVTIMEO_NONSTANDARD 0

/**
 * LWIP_SO_RCVBUF==1: Enable SO_RCVBUF processing.
 */
#define LWIP_SO_RCVBUF                  CONFIG_LWIP_SO_RCVBUF

/**
 * LWIP_SO_LINGER==1: Enable SO_LINGER processing.
 */
#define LWIP_SO_LINGER                  1

#define SET_SOLINGER_DEFAULT            CONFIG_SET_SOLINGER_DEFAULT

/**
 * If LWIP_SO_RCVBUF is used, this is the default value for recv_bufsize.
 */
#define RECV_BUFSIZE_DEFAULT            CONFIG_LWIP_RECV_BUFSIZE_DEFAULT

/**
 * By default, TCP socket/netconn close waits 20 seconds max to send the FIN
 */
#define LWIP_TCP_CLOSE_TIMEOUT_MS_DEFAULT CONFIG_LWIP_TCP_CLOSE_TIMEOUT_MS_DEFAULT

/**
 * SO_REUSE==1: Enable SO_REUSEADDR option.
 */
#define SO_REUSE                        CONFIG_LWIP_SO_REUSE

/**
 * SO_REUSE_RXTOALL==1: Pass a copy of incoming broadcast/multicast packets
 * to all local matches if SO_REUSEADDR is turned on.
 * WARNING: Adds a memcpy for every packet if passing to more than one pcb!
 */
#if CONFIG_LWIP_SO_REUSE_RXTOALL
#define SO_REUSE_RXTOALL                1
#else
#define SO_REUSE_RXTOALL                0
#endif

/**
 * LWIP_FIONREAD_LINUXMODE==0 (default): ioctl/FIONREAD returns the amount of
 * pending data in the network buffer. This is the way windows does it. It's
 * the default for lwIP since it is smaller.
 * LWIP_FIONREAD_LINUXMODE==1: ioctl/FIONREAD returns the size of the next
 * pending datagram in bytes. This is the way linux does it. This code is only
 * here for compatibility.
 */
#define LWIP_FIONREAD_LINUXMODE         0
/**
 * @}
 */

/*
   ----------------------------------------
   ---------- Statistics options ----------
   ----------------------------------------
*/
/**
 * @defgroup lwip_opts_stats Statistics
 * @ingroup lwip_opts_debug
 * @{
 */
/**
 * LWIP_STATS==1: Enable statistics collection in lwip_stats.
 */
#define LWIP_STATS                      CONFIG_LWIP_STATS

#if LWIP_STATS

/**
 * LWIP_STATS_DISPLAY==1: Compile in the statistics output functions.
 */
#define LWIP_STATS_DISPLAY              0

/**
 * LINK_STATS==1: Enable link stats.
 */
#define LINK_STATS                      1

/**
 * ETHARP_STATS==1: Enable etharp stats.
 */
#define ETHARP_STATS                    (LWIP_ARP)

/**
 * IP_STATS==1: Enable IP stats.
 */
#define IP_STATS                        1

/**
 * IPFRAG_STATS==1: Enable IP fragmentation stats. Default is
 * on if using either frag or reass.
 */
#define IPFRAG_STATS                    (IP_REASSEMBLY || IP_FRAG)

/**
 * ICMP_STATS==1: Enable ICMP stats.
 */
#define ICMP_STATS                      1

/**
 * IGMP_STATS==1: Enable IGMP stats.
 */
#define IGMP_STATS                      (LWIP_IGMP)

/**
 * UDP_STATS==1: Enable UDP stats. Default is on if
 * UDP enabled, otherwise off.
 */
#define UDP_STATS                       (LWIP_UDP)

/**
 * TCP_STATS==1: Enable TCP stats. Default is on if TCP
 * enabled, otherwise off.
 */
#define TCP_STATS                       (LWIP_TCP)

/**
 * MEM_STATS==1: Enable mem.c stats.
 */
#define MEM_STATS                       ((MEM_LIBC_MALLOC == 0) && (MEM_USE_POOLS == 0))

/**
 * MEMP_STATS==1: Enable memp.c pool stats.
 */
#define MEMP_STATS                      (MEMP_MEM_MALLOC == 0)

/**
 * SYS_STATS==1: Enable system stats (sem and mbox counts, etc).
 */
#define SYS_STATS                       (NO_SYS == 0)

/**
 * IP6_STATS==1: Enable IPv6 stats.
 */
#define IP6_STATS                       (LWIP_IPV6)

/**
 * ICMP6_STATS==1: Enable ICMP for IPv6 stats.
 */
#define ICMP6_STATS                     (LWIP_IPV6 && LWIP_ICMP6)

/**
 * IP6_FRAG_STATS==1: Enable IPv6 fragmentation stats.
 */
#define IP6_FRAG_STATS                  (LWIP_IPV6 && (LWIP_IPV6_FRAG || LWIP_IPV6_REASS))

/**
 * MLD6_STATS==1: Enable MLD for IPv6 stats.
 */
#define MLD6_STATS                      (LWIP_IPV6 && LWIP_IPV6_MLD)

/**
 * ND6_STATS==1: Enable Neighbor discovery for IPv6 stats.
 */
#define ND6_STATS                       (LWIP_IPV6)

/**
 * MIB2_STATS==1: Stats for SNMP MIB2.
 */
#define MIB2_STATS                      0

#else

#define LINK_STATS                      0
#define ETHARP_STATS                    0
#define IP_STATS                        0
#define IPFRAG_STATS                    0
#define ICMP_STATS                      0
#define IGMP_STATS                      0
#define UDP_STATS                       0
#define TCP_STATS                       0
#define MEM_STATS                       0
#define MEMP_STATS                      0
#define SYS_STATS                       0
#define LWIP_STATS_DISPLAY              0
#define IP6_STATS                       0
#define ICMP6_STATS                     0
#define IP6_FRAG_STATS                  0
#define MLD6_STATS                      0
#define ND6_STATS                       0
#define MIB2_STATS                      0

#endif /* LWIP_STATS */
/**
 * @}
 */

/*
   --------------------------------------
   ---------- Checksum options ----------
   --------------------------------------
*/
/**
 * @defgroup lwip_opts_checksum Checksum
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * LWIP_CHECKSUM_CTRL_PER_NETIF==1: Checksum generation/check can be enabled/disabled
 * per netif.
 * ATTENTION: if enabled, the CHECKSUM_GEN_* and CHECKSUM_CHECK_* defines must be enabled!
 */
#define LWIP_CHECKSUM_CTRL_PER_NETIF    0

/**
 * CHECKSUM_GEN_IP==1: Generate checksums in software for outgoing IP packets.
 */
#define CHECKSUM_GEN_IP                 1

/**
 * CHECKSUM_GEN_UDP==1: Generate checksums in software for outgoing UDP packets.
 */
#define CHECKSUM_GEN_UDP                1

/**
 * CHECKSUM_GEN_TCP==1: Generate checksums in software for outgoing TCP packets.
 */
#define CHECKSUM_GEN_TCP                1

/**
 * CHECKSUM_GEN_ICMP==1: Generate checksums in software for outgoing ICMP packets.
 */
#define CHECKSUM_GEN_ICMP               1

/**
 * CHECKSUM_GEN_ICMP6==1: Generate checksums in software for outgoing ICMP6 packets.
 */
#define CHECKSUM_GEN_ICMP6              1

/**
 * CHECKSUM_CHECK_IP==1: Check checksums in software for incoming IP packets.
 */
#define CHECKSUM_CHECK_IP               1

/**
 * CHECKSUM_CHECK_UDP==1: Check checksums in software for incoming UDP packets.
 */
#define CHECKSUM_CHECK_UDP              1

/**
 * CHECKSUM_CHECK_TCP==1: Check checksums in software for incoming TCP packets.
 */
#define CHECKSUM_CHECK_TCP              1

/**
 * CHECKSUM_CHECK_ICMP==1: Check checksums in software for incoming ICMP packets.
 */
#define CHECKSUM_CHECK_ICMP             1

/**
 * CHECKSUM_CHECK_ICMP6==1: Check checksums in software for incoming ICMPv6 packets
 */
#define CHECKSUM_CHECK_ICMP6            1

/**
 * LWIP_CHECKSUM_ON_COPY==1: Calculate checksum when copying data from
 * application buffers to pbufs.
 */
#define LWIP_CHECKSUM_ON_COPY           0
/**
 * @}
 */

/*
   ---------------------------------------
   ---------- IPv6 options ---------------
   ---------------------------------------
*/
/**
 * @defgroup lwip_opts_ipv6 IPv6
 * @ingroup lwip_opts
 * @{
 */
/**
 * LWIP_IPV6==1: Enable IPv6
 */
#define LWIP_IPV6                       CONFIG_LWIP_IPV6

/**
 * LWIP_IPV6_NUM_ADDRESSES: Number of IPv6 addresses per netif.
 */
#define LWIP_IPV6_NUM_ADDRESSES         CONFIG_LWIP_IPV6_NUM_ADDRESSES

/**
 * LWIP_IPV6_FORWARD==1: Forward IPv6 packets across netifs
 */
#define LWIP_IPV6_FORWARD               0

/**
 * LWIP_IPV6_FRAG==1: Fragment outgoing IPv6 packets that are too big.
 */
#define LWIP_IPV6_FRAG                  0

/**
 * LWIP_IPV6_REASS==1: reassemble incoming IPv6 packets that fragmented
 */
#define LWIP_IPV6_REASS                 (LWIP_IPV6)

/**
 * LWIP_IPV6_SEND_ROUTER_SOLICIT==1: Send router solicitation messages during
 * network startup.
 */
#define LWIP_IPV6_SEND_ROUTER_SOLICIT   1

/**
 * LWIP_IPV6_AUTOCONFIG==1: Enable stateless address autoconfiguration as per RFC 4862.
 */
#define LWIP_IPV6_AUTOCONFIG            (LWIP_IPV6)

/**
 * LWIP_IPV6_DUP_DETECT_ATTEMPTS=[0..7]: Number of duplicate address detection attempts.
 */
#define LWIP_IPV6_DUP_DETECT_ATTEMPTS   1
/**
 * @}
 */

/**
 * @defgroup lwip_opts_icmp6 ICMP6
 * @ingroup lwip_opts_ipv6
 * @{
 */
/**
 * LWIP_ICMP6==1: Enable ICMPv6 (mandatory per RFC)
 */
#define LWIP_ICMP6                      (LWIP_IPV6)

/**
 * LWIP_ICMP6_DATASIZE: bytes from original packet to send back in
 * ICMPv6 error messages.
 */
#define LWIP_ICMP6_DATASIZE             8

/**
 * LWIP_ICMP6_HL: default hop limit for ICMPv6 messages
 */
#define LWIP_ICMP6_HL                   255
/**
 * @}
 */

/**
 * @defgroup lwip_opts_mld6 Multicast listener discovery
 * @ingroup lwip_opts_ipv6
 * @{
 */
/**
 * LWIP_IPV6_MLD==1: Enable multicast listener discovery protocol.
 * If LWIP_IPV6 is enabled but this setting is disabled, the MAC layer must
 * indiscriminately pass all inbound IPv6 multicast traffic to lwIP.
 */
#define LWIP_IPV6_MLD                   (LWIP_IPV6)

/**
 * MEMP_NUM_MLD6_GROUP: Max number of IPv6 multicast groups that can be joined.
 * There must be enough groups so that each netif can join the solicited-node
 * multicast group for each of its local addresses, plus one for MDNS if
 * applicable, plus any number of groups to be joined on UDP sockets.
 */
#define MEMP_NUM_MLD6_GROUP             4
/**
 * @}
 */

/**
 * @defgroup lwip_opts_nd6 Neighbor discovery
 * @ingroup lwip_opts_ipv6
 * @{
 */
/**
 * LWIP_ND6_QUEUEING==1: queue outgoing IPv6 packets while MAC address
 * is being resolved.
 */
#define LWIP_ND6_QUEUEING               (LWIP_IPV6)

/**
 * MEMP_NUM_ND6_QUEUE: Max number of IPv6 packets to queue during MAC resolution.
 */
#define MEMP_NUM_ND6_QUEUE              20

/**
 * LWIP_ND6_NUM_NEIGHBORS: Number of entries in IPv6 neighbor cache
 */
#define LWIP_ND6_NUM_NEIGHBORS          10

/**
 * LWIP_ND6_NUM_DESTINATIONS: number of entries in IPv6 destination cache
 */
#define LWIP_ND6_NUM_DESTINATIONS       10

/**
 * LWIP_ND6_NUM_PREFIXES: number of entries in IPv6 on-link prefixes cache
 */
#define LWIP_ND6_NUM_PREFIXES           5

/**
 * LWIP_ND6_NUM_ROUTERS: number of entries in IPv6 default router cache
 */
#define LWIP_ND6_NUM_ROUTERS            3

/**
 * LWIP_ND6_MAX_MULTICAST_SOLICIT: max number of multicast solicit messages to send
 * (neighbor solicit and router solicit)
 */
#define LWIP_ND6_MAX_MULTICAST_SOLICIT  3

/**
 * LWIP_ND6_MAX_UNICAST_SOLICIT: max number of unicast neighbor solicitation messages
 * to send during neighbor reachability detection.
 */
#define LWIP_ND6_MAX_UNICAST_SOLICIT    3

/**
 * Unused: See ND RFC (time in milliseconds).
 */
#define LWIP_ND6_MAX_ANYCAST_DELAY_TIME 1000

/**
 * Unused: See ND RFC
 */
#define LWIP_ND6_MAX_NEIGHBOR_ADVERTISEMENT  3

/**
 * LWIP_ND6_REACHABLE_TIME: default neighbor reachable time (in milliseconds).
 * May be updated by router advertisement messages.
 */
#define LWIP_ND6_REACHABLE_TIME         30000

/**
 * LWIP_ND6_RETRANS_TIMER: default retransmission timer for solicitation messages
 */
#define LWIP_ND6_RETRANS_TIMER          1000

/**
 * LWIP_ND6_DELAY_FIRST_PROBE_TIME: Delay before first unicast neighbor solicitation
 * message is sent, during neighbor reachability detection.
 */
#define LWIP_ND6_DELAY_FIRST_PROBE_TIME 5000

/**
 * LWIP_ND6_ALLOW_RA_UPDATES==1: Allow Router Advertisement messages to update
 * Reachable time and retransmission timers, and netif MTU.
 */
#define LWIP_ND6_ALLOW_RA_UPDATES       1

/**
 * LWIP_ND6_TCP_REACHABILITY_HINTS==1: Allow TCP to provide Neighbor Discovery
 * with reachability hints for connected destinations. This helps avoid sending
 * unicast neighbor solicitation messages.
 */
#define LWIP_ND6_TCP_REACHABILITY_HINTS 1

/**
 * LWIP_ND6_RDNSS_MAX_DNS_SERVERS > 0: Use IPv6 Router Advertisement Recursive
 * DNS Server Option (as per RFC 6106) to copy a defined maximum number of DNS
 * servers to the DNS module.
 */
#define LWIP_ND6_RDNSS_MAX_DNS_SERVERS  CONFIG_LWIP_ND6_RDNSS_MAX_DNS_SERVERS
/**
 * @}
 */

/**
 * LWIP_IPV6_DHCP6==1: enable DHCPv6 stateful address autoconfiguration.
 */
#define LWIP_IPV6_DHCP6                 0

/*
   ---------------------------------------
   ---------- Hook options ---------------
   ---------------------------------------
*/

#define LWIP_HOOK_IP4_ROUTE_SRC(d, s)         (void *)ip4_route_src_hook(d, s)

/**
 * @defgroup lwip_opts_hooks Hooks
 * @ingroup lwip_opts_infrastructure
 * Hooks are undefined by default, define them to a function if you need them.
 * @{
 */

/*
   ---------------------------------------
   ---------- Debugging options ----------
   ---------------------------------------
*/
/**
 * @defgroup lwip_opts_debugmsg Debug messages
 * @ingroup lwip_opts_debug
 * @{
 */
/**
 * LWIP_DBG_MIN_LEVEL: After masking, the value of the debug is
 * compared against this value. If it is smaller, then debugging
 * messages are written.
 * @see debugging_levels
 */
#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_ALL

/**
 * LWIP_DBG_TYPES_ON: A mask that can be used to globally enable/disable
 * debug messages of certain types.
 * @see debugging_levels
 */
#define LWIP_DBG_TYPES_ON               LWIP_DBG_ON

/**
 * Enable debug message printing.
 */
#if CONFIG_LWIP_DEBUG
#define LWIP_DEBUG
#endif

/**
 * ETHARP_DEBUG: Enable debugging in etharp.c.
 */
#if CONFIG_LWIP_ETHARP_DEBUG
#define ETHARP_DEBUG                    LWIP_DBG_ON
#endif

/**
 * NETIF_DEBUG: Enable debugging in netif.c.
 */
#if CONFIG_LWIP_NETIF_DEBUG
#define NETIF_DEBUG                     LWIP_DBG_ON
#endif

/**
 * NETIF_DEBUG: Enable debugging in ethernetif.c.
 */
#if CONFIG_LWIP_ETHERNETIF_DEBUG
#define ETHERNETIF_DEBUG                     LWIP_DBG_ON
#endif

/**
 * PBUF_DEBUG: Enable debugging in pbuf.c.
 */
#if CONFIG_LWIP_PBUF_DEBUG
#define PBUF_DEBUG                      LWIP_DBG_ON
#endif

/**
 * API_LIB_DEBUG: Enable debugging in api_lib.c.
 */
#if CONFIG_LWIP_API_LIB_DEBUG
#define API_LIB_DEBUG                   LWIP_DBG_ON
#endif

/**
 * API_MSG_DEBUG: Enable debugging in api_msg.c.
 */
#if CONFIG_LWIP_API_MSG_DEBUG
#define API_MSG_DEBUG                   LWIP_DBG_ON
#endif

/**
 * SOCKETS_DEBUG: Enable debugging in sockets.c.
 */
#if CONFIG_LWIP_SOCKETS_DEBUG
#define SOCKETS_DEBUG                   LWIP_DBG_ON
#endif

/**
 * ICMP_DEBUG: Enable debugging in icmp.c.
 */
#if CONFIG_LWIP_ICMP_DEBUG
#define ICMP_DEBUG                      LWIP_DBG_ON
#endif

/**
 * IGMP_DEBUG: Enable debugging in igmp.c.
 */
#if CONFIG_LWIP_IGMP_DEBUG
#define IGMP_DEBUG                      LWIP_DBG_ON
#endif

/**
 * INET_DEBUG: Enable debugging in inet.c.
 */
#if CONFIG_LWIP_INET_DEBUG
#define INET_DEBUG                      LWIP_DBG_ON
#endif

/**
 * IP_DEBUG: Enable debugging for IP.
 */
#if CONFIG_LWIP_IP_DEBUG
#define IP_DEBUG                        LWIP_DBG_ON
#endif

/**
 * IP_REASS_DEBUG: Enable debugging in ip_frag.c for both frag & reass.
 */
#if CONFIG_LWIP_IP_REASS_DEBUG
#define IP_REASS_DEBUG                  LWIP_DBG_ON
#endif

/**
 * RAW_DEBUG: Enable debugging in raw.c.
 */
#if CONFIG_LWIP_RAW_DEBUG
#define RAW_DEBUG                       LWIP_DBG_ON
#endif

/**
 * MEM_DEBUG: Enable debugging in mem.c.
 */
#if CONFIG_LWIP_MEM_DEBUG
#define MEM_DEBUG                       LWIP_DBG_ON
#endif

/**
 * MEMP_DEBUG: Enable debugging in memp.c.
 */
#if CONFIG_LWIP_MEMP_DEBUG
#define MEMP_DEBUG                      LWIP_DBG_ON
#endif

/**
 * SYS_DEBUG: Enable debugging in sys.c.
 */
#if CONFIG_LWIP_SYS_DEBUG
#define SYS_DEBUG                       LWIP_DBG_ON
#endif

/**
 * TIMERS_DEBUG: Enable debugging in timers.c.
 */
#if CONFIG_LWIP_TIMERS_DEBUG
#define TIMERS_DEBUG                    LWIP_DBG_ON
#endif

/**
 * TCP_DEBUG: Enable debugging for TCP.
 */
#if CONFIG_LWIP_TCP_DEBUG
#define TCP_DEBUG                       LWIP_DBG_ON
#endif

/**
 * TCP_INPUT_DEBUG: Enable debugging in tcp_in.c for incoming debug.
 */
#if CONFIG_LWIP_TCP_INPUT_DEBUG
#define TCP_INPUT_DEBUG                 LWIP_DBG_ON
#endif

/**
 * TCP_FR_DEBUG: Enable debugging in tcp_in.c for fast retransmit.
 */
#if CONFIG_LWIP_TCP_FR_DEBUG
#define TCP_FR_DEBUG                    LWIP_DBG_ON
#endif

/**
 * TCP_RTO_DEBUG: Enable debugging in TCP for retransmit
 * timeout.
 */
#if CONFIG_LWIP_TCP_RTO_DEBUG
#define TCP_RTO_DEBUG                   LWIP_DBG_ON
#endif

/**
 * TCP_CWND_DEBUG: Enable debugging for TCP congestion window.
 */
#if CONFIG_LWIP_TCP_CWND_DEBUG
#define TCP_CWND_DEBUG                  LWIP_DBG_ON
#endif

/**
 * TCP_WND_DEBUG: Enable debugging in tcp_in.c for window updating.
 */
#if CONFIG_LWIP_TCP_WND_DEBUG
#define TCP_WND_DEBUG                   LWIP_DBG_ON
#endif

/**
 * TCP_OUTPUT_DEBUG: Enable debugging in tcp_out.c output functions.
 */
#if CONFIG_LWIP_TCP_OUTPUT_DEBUG
#define TCP_OUTPUT_DEBUG                LWIP_DBG_ON
#endif

/**
 * TCP_RST_DEBUG: Enable debugging for TCP with the RST message.
 */
#if CONFIG_LWIP_TCP_RST_DEBUG
#define TCP_RST_DEBUG                   LWIP_DBG_ON
#endif

/**
 * TCP_QLEN_DEBUG: Enable debugging for TCP queue lengths.
 */
#if CONFIG_LWIP_TCP_QLEN_DEBUG
#define TCP_QLEN_DEBUG                  LWIP_DBG_ON
#endif

/**
 * UDP_DEBUG: Enable debugging in UDP.
 */
#if CONFIG_LWIP_UDP_DEBUG
#define UDP_DEBUG                       LWIP_DBG_ON
#endif

/**
 * TCPIP_DEBUG: Enable debugging in tcpip.c.
 */
#if CONFIG_LWIP_TCPIP_DEBUG
#define TCPIP_DEBUG                     LWIP_DBG_ON
#endif

/**
 * SLIP_DEBUG: Enable debugging in slipif.c.
 */
#if CONFIG_LWIP_SLIP_DEBUG
#define SLIP_DEBUG                      LWIP_DBG_ON
#endif

/**
 * DHCP_DEBUG: Enable debugging in dhcp.c.
 */
#if CONFIG_LWIP_DHCP_DEBUG
#define DHCP_DEBUG                      LWIP_DBG_ON
#endif

/**
 * AUTOIP_DEBUG: Enable debugging in autoip.c.
 */
#if CONFIG_LWIP_AUTOIP_DEBUG
#define AUTOIP_DEBUG                    LWIP_DBG_ON
#endif

/**
 * DNS_DEBUG: Enable debugging for DNS.
 */
#if CONFIG_LWIP_DNS_DEBUG
#define DNS_DEBUG                       LWIP_DBG_ON
#endif

/**
 * IP6_DEBUG: Enable debugging for IPv6.
 */
#if CONFIG_LWIP_IP6_DEBUG
#define IP6_DEBUG                       LWIP_DBG_ON
#endif

/**
 * SNTP_DEBUG: Enable debugging for SNTP.
 */
#if CONFIG_LWIP_SNTP_DEBUG
#define SNTP_DEBUG                      LWIP_DBG_ON
#endif

#ifdef CONFIG_LWIP_THREAD_SAFE_DEBUG
#define ESP_THREAD_SAFE_DEBUG           LWIP_DBG_ON
#else
#define ESP_THREAD_SAFE_DEBUG           LWIP_DBG_OFF
#endif

#ifdef CONFIG_LWIP_TCP_TXRX_PBUF_DEBUG
#define ESP_TCP_TXRX_PBUF_DEBUG         LWIP_DBG_ON
#define LWIP_SEND_DATA_TO_WIFI                           1
#define LWIP_RESEND_DATA_TO_WIFI_WHEN_WIFI_SEND_FAILED   2
#define LWIP_RECV_DATA_FROM_WIFI                         3
#define LWIP_RETRY_DATA_WHEN_RECV_ACK_TIMEOUT            4
#define LWIP_FETCH_DATA_AT_TCPIP_THREAD                  5
#define WIFI_SEND_DATA_FAILED                            6

void tcp_print_status(int status, void* p, uint32_t tmp1, uint32_t tmp2, uint32_t tmp3);
#else
#define ESP_TCP_TXRX_PBUF_DEBUG         LWIP_DBG_OFF
#endif
/**
 * PBUF_CACHE_DEBUG: Enable debugging for SNTP.
 */
#if CONFIG_LWIP_PBUF_CACHE_DEBUG
#define PBUF_CACHE_DEBUG                      LWIP_DBG_ON
#else
#define PBUF_CACHE_DEBUG                      LWIP_DBG_OFF
#endif

/**
 * @}
 */

/*
   --------------------------------------------------
   ---------- Performance tracking options ----------
   --------------------------------------------------
*/
/**
 * @defgroup lwip_opts_perf Performance
 * @ingroup lwip_opts_debug
 * @{
 */
/**
 * LWIP_PERF: Enable performance testing for lwIP
 * (if enabled, arch/perf.h is included)
 */
#define LWIP_PERF                       0
/**
 * @}
 */

#if ESP_UDP
#if !LWIP_UDP || !LWIP_SOCKET || !ESP_LWIP || !LWIP_NETIF_TX_SINGLE_PBUF
#error "LWIP_UDP & LWIP_SOCKET & ESP_LWIP & LWIP_NETIF_TX_SINGLE_PBUF must be enable"
#else
#include "udp_sync.h"
#endif
#endif

#define ESP_GRATUITOUS_ARP              CONFIG_LWIP_ESP_GRATUITOUS_ARP
#define ESP_PING                        1

#define ESP_DHCP_OPTION                 CONFIG_LWIP_ESP_DHCP_OPTION
#endif /* __LWIP_HDR_LWIPOPTS_H__ */
