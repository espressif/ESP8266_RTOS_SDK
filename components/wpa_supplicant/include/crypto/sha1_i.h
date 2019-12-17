/*
 * SHA1 internal definitions
 * Copyright (c) 2003-2005, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#ifndef SHA1_I_H
#define SHA1_I_H

#include "sdkconfig.h"
#ifdef CONFIG_ESP_SHA
#include "esp_sha.h"

typedef esp_sha1_t SHA1_CTX;

#define SHA1Init(_sha)              esp_sha1_init(_sha)
#define SHA1Update(_sha, _s, _l)    esp_sha1_update(_sha, _s, _l)
#define SHA1Final(_d, _sha)         esp_sha1_finish(_sha, _d)
#else /* CONFIG_ESP_SHA */
struct SHA1Context {
	u32 state[5];
	u32 count[2];
	unsigned char buffer[64];
};

void SHA1Init(struct SHA1Context *context);
void SHA1Update(struct SHA1Context *context, const void *data, u32 len);
void SHA1Final(unsigned char digest[20], struct SHA1Context *context);
void SHA1Transform(u32 state[5], const unsigned char buffer[64]);
#endif /* CONFIG_ESP_SHA */

#endif /* SHA1_I_H */
