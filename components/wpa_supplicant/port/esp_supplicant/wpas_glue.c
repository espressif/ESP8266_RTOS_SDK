// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
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

#ifdef EMBEDDED_SUPP

#include "utils/includes.h"
#include "utils/common.h"
#include "common/eapol_common.h"
#include "rsn_supp/wpa.h"
#define EP_OFFSET 36

#define wpa_malloc_dram(s) heap_caps_malloc(s, MALLOC_CAP_8BIT)
#define wpa_calloc_dram(n, s) heap_caps_calloc(n, s, MALLOC_CAP_8BIT)

static u8* wpa_alloc_eapol(struct wpa_sm *sm, u8 type,
                         const void *data, u16 data_len,
                         size_t *msg_len, void **data_pos)
{
    struct ieee802_1x_hdr* hdr;

    *msg_len = sizeof(*hdr) + data_len;
    hdr = (struct ieee802_1x_hdr*)((int)sm->wpadata + sizeof(struct l2_ethhdr)); //keep head byte remain for filling later

    if (hdr == NULL) {
        return NULL;
    }

    hdr->version = sm->eapol_version;
    hdr->type = type;
    hdr->length = host_to_be16(data_len);

    if (data) {
        memcpy(hdr + 1, data, data_len);
    } else {
        memset(hdr + 1, 0, data_len);
    }

    if (data_pos) {
        *data_pos = hdr + 1;
    }

    return (u8 *) hdr;
}


u8* ICACHE_FLASH_ATTR wpa_sm_alloc_eapol(struct wpa_sm* sm, u8 type,
        const void* data, u16 data_len,
        size_t* msg_len, void** data_pos)
{
    sm->wpadata = wpa_malloc_dram(256 + EP_OFFSET);
    sm->wpadata += EP_OFFSET;
    return wpa_alloc_eapol(sm, type, data, data_len, msg_len, data_pos);


    return NULL;
}

void  wpa_sm_deauthenticate(struct wpa_sm *sm, u8 reason_code)
{

    /*only need send deauth frame when associated*/
    if (WPA_SM_STATE(sm) >= WPA_ASSOCIATED) {
        sm->wpa_deauthenticate(reason_code);
    }
}

/**
 * mlme_setprotection - MLME-SETPROTECTION.request primitive
 * @priv: Private driver interface data
 * @addr: Address of the station for which to set protection (may be
 * %NULL for group keys)
 * @protect_type: MLME_SETPROTECTION_PROTECT_TYPE_*
 * @key_type: MLME_SETPROTECTION_KEY_TYPE_*
 * Returns: 0 on success, -1 on failure
 *
 * This is an optional function that can be used to set the driver to
 * require protection for Tx and/or Rx frames. This uses the layer
 * interface defined in IEEE 802.11i-2004 clause 10.3.22.1
 * (MLME-SETPROTECTION.request). Many drivers do not use explicit
 * set protection operation; instead, they set protection implicitly
 * based on configured keys.
 */
int  wpa_sm_mlme_setprotection(struct wpa_sm *sm, const u8 *addr,
                               int protect_type, int key_type)
{
    return 0;
}

/*
 *use above two functions to get wpa_ie and rsn_ie, then don't need wpa_sm_get_beacon_ie function
*/
int  wpa_sm_get_beacon_ie(struct wpa_sm *sm)
{
    return 0;
}

/**
 * wpa_supplicant_disassociate - Disassociate the current connection
 * @wpa_s: Pointer to wpa_supplicant data
 * @reason_code: IEEE 802.11 reason code for the disassociate frame
 *
 * This function is used to request %wpa_supplicant to disassociate with the
 * current AP.
 */
void  wpa_sm_disassociate(struct wpa_sm *sm, int reason_code)
{
    /*check if need clear internal state and data value*/
}
#endif
