/* pdu.c -- CoAP message structure
 *
 * Copyright (C) 2010--2016 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use. 
 */

#include "coap_config.h"

#if defined(HAVE_ASSERT_H) && !defined(assert)
# include <assert.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "debug.h"
#include "pdu.h"
#include "option.h"
#include "encode.h"
#include "mem.h"

void
coap_pdu_clear(coap_pdu_t *pdu, size_t size) {
  assert(pdu);

#ifdef WITH_LWIP
  /* the pdu itself is not wiped as opposed to the other implementations,
   * because we have to rely on the pbuf to be set there. */
  pdu->hdr = pdu->pbuf->payload;
#else
  pdu->max_delta = 0;
  pdu->data = NULL;
#endif
  memset(pdu->hdr, 0, size);
  pdu->max_size = size;
  pdu->hdr->version = COAP_DEFAULT_VERSION;

  /* data is NULL unless explicitly set by coap_add_data() */
  pdu->length = sizeof(coap_hdr_t);
}

#ifdef WITH_LWIP
coap_pdu_t *
coap_pdu_from_pbuf(struct pbuf *pbuf)
{
  if (pbuf == NULL) return NULL;

  LWIP_ASSERT("Can only deal with contiguous PBUFs", pbuf->tot_len == pbuf->len);
  LWIP_ASSERT("coap_read needs to receive an exclusive copy of the incoming pbuf", pbuf->ref == 1);

  coap_pdu_t *result = coap_malloc_type(COAP_PDU, sizeof(coap_pdu_t));
  if (!result) {
	  pbuf_free(pbuf);
	  return NULL;
  }

  memset(result, 0, sizeof(coap_pdu_t));

  result->max_size = pbuf->tot_len;
  result->length = pbuf->tot_len;
  result->hdr = pbuf->payload;
  result->pbuf = pbuf;

  return result;
}
#endif

coap_pdu_t *
coap_pdu_init(unsigned char type, unsigned char code, 
	      unsigned short id, size_t size) {
  coap_pdu_t *pdu;
#ifdef WITH_LWIP
    struct pbuf *p;
#endif

  assert(size <= COAP_MAX_PDU_SIZE);
  /* Size must be large enough to fit the header. */
  if (size < sizeof(coap_hdr_t) || size > COAP_MAX_PDU_SIZE)
    return NULL;

  /* size must be large enough for hdr */
#if defined(WITH_POSIX) || defined(WITH_CONTIKI)
  pdu = coap_malloc_type(COAP_PDU, sizeof(coap_pdu_t));
  if (!pdu) return NULL;
  pdu->hdr = coap_malloc_type(COAP_PDU_BUF, size);
  if (pdu->hdr == NULL) {
    coap_free_type(COAP_PDU, pdu);
    pdu = NULL;
  }
#endif /* WITH_POSIX or WITH_CONTIKI */
#ifdef WITH_LWIP
  pdu = (coap_pdu_t*)coap_malloc_type(COAP_PDU, sizeof(coap_pdu_t));
  if (!pdu) return NULL;
  p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);
  if (p == NULL) {
    coap_free_type(COAP_PDU, pdu);
    pdu = NULL;
  }
#endif
  if (pdu) {
#ifdef WITH_LWIP
    pdu->pbuf = p;
#endif
    coap_pdu_clear(pdu, size);
    pdu->hdr->id = id;
    pdu->hdr->type = type;
    pdu->hdr->code = code;
  } 
  return pdu;
}

coap_pdu_t *
coap_new_pdu(void) {
  coap_pdu_t *pdu;
  
#ifndef WITH_CONTIKI
  pdu = coap_pdu_init(0, 0, ntohs(COAP_INVALID_TID), COAP_MAX_PDU_SIZE);
#else /* WITH_CONTIKI */
  pdu = coap_pdu_init(0, 0, uip_ntohs(COAP_INVALID_TID), COAP_MAX_PDU_SIZE);
#endif /* WITH_CONTIKI */

#ifndef NDEBUG
  if (!pdu)
    coap_log(LOG_CRIT, "coap_new_pdu: cannot allocate memory for new PDU\n");
#endif
  return pdu;
}

void
coap_delete_pdu(coap_pdu_t *pdu) {
#if defined(WITH_POSIX) || defined(WITH_CONTIKI)
  if (pdu != NULL) {
    if (pdu->hdr != NULL) {
      coap_free_type(COAP_PDU_BUF, pdu->hdr);
    }
    coap_free_type(COAP_PDU, pdu);
  }
#endif
#ifdef WITH_LWIP
  if (pdu != NULL) /* accepting double free as the other implementation accept that too */
    pbuf_free(pdu->pbuf);
  coap_free_type(COAP_PDU, pdu);
#endif
}

int
coap_add_token(coap_pdu_t *pdu, size_t len, const unsigned char *data) {
  const size_t HEADERLENGTH = len + 4;
  /* must allow for pdu == NULL as callers may rely on this */
  if (!pdu || len > 8 || pdu->max_size < HEADERLENGTH)
    return 0;

  pdu->hdr->token_length = len;
  if (len)
    memcpy(pdu->hdr->token, data, len);
  pdu->max_delta = 0;
  pdu->length = HEADERLENGTH;
  pdu->data = NULL;

  return 1;
}

/** @FIXME de-duplicate code with coap_add_option_later */
size_t
coap_add_option(coap_pdu_t *pdu, unsigned short type, unsigned int len, const unsigned char *data) {
  size_t optsize;
  coap_opt_t *opt;
  
  assert(pdu);
  pdu->data = NULL;

  if (type < pdu->max_delta) {
    warn("coap_add_option: options are not in correct order\n");
    return 0;
  }

  opt = (unsigned char *)pdu->hdr + pdu->length;

  /* encode option and check length */
  optsize = coap_opt_encode(opt, pdu->max_size - pdu->length, 
			    type - pdu->max_delta, data, len);

  if (!optsize) {
    warn("coap_add_option: cannot add option\n");
    /* error */
    return 0;
  } else {
    pdu->max_delta = type;
    pdu->length += optsize;
  }

  return optsize;
}

/** @FIXME de-duplicate code with coap_add_option */
unsigned char*
coap_add_option_later(coap_pdu_t *pdu, unsigned short type, unsigned int len) {
  size_t optsize;
  coap_opt_t *opt;

  assert(pdu);
  pdu->data = NULL;

  if (type < pdu->max_delta) {
    warn("coap_add_option: options are not in correct order\n");
    return NULL;
  }

  opt = (unsigned char *)pdu->hdr + pdu->length;

  /* encode option and check length */
  optsize = coap_opt_encode(opt, pdu->max_size - pdu->length,
			    type - pdu->max_delta, NULL, len);

  if (!optsize) {
    warn("coap_add_option: cannot add option\n");
    /* error */
    return NULL;
  } else {
    pdu->max_delta = type;
    pdu->length += optsize;
  }

  return ((unsigned char*)opt) + optsize - len;
}

int
coap_add_data(coap_pdu_t *pdu, unsigned int len, const unsigned char *data) {
  assert(pdu);
  assert(pdu->data == NULL);

  if (len == 0)
    return 1;

  if (pdu->length + len + 1 > pdu->max_size) {
    warn("coap_add_data: cannot add: data too large for PDU\n");
    assert(pdu->data == NULL);
    return 0;
  }

  pdu->data = (unsigned char *)pdu->hdr + pdu->length;
  *pdu->data = COAP_PAYLOAD_START;
  pdu->data++;

  memcpy(pdu->data, data, len);
  pdu->length += len + 1;
  return 1;
}

int
coap_get_data(coap_pdu_t *pdu, size_t *len, unsigned char **data) {
  assert(pdu);
  assert(len);
  assert(data);

  if (pdu->data) {
    *len = (unsigned char *)pdu->hdr + pdu->length - pdu->data;
    *data = pdu->data;
  } else {			/* no data, clear everything */
    *len = 0;
    *data = NULL;
  }

  return *data != NULL;
}

#ifndef SHORT_ERROR_RESPONSE
typedef struct {
  unsigned char code;
  char *phrase;
} error_desc_t;

/* if you change anything here, make sure, that the longest string does not 
 * exceed COAP_ERROR_PHRASE_LENGTH. */
error_desc_t coap_error[] = {
  { COAP_RESPONSE_CODE(201), "Created" },
  { COAP_RESPONSE_CODE(202), "Deleted" },
  { COAP_RESPONSE_CODE(203), "Valid" },
  { COAP_RESPONSE_CODE(204), "Changed" },
  { COAP_RESPONSE_CODE(205), "Content" },
  { COAP_RESPONSE_CODE(231), "Continue" },
  { COAP_RESPONSE_CODE(400), "Bad Request" },
  { COAP_RESPONSE_CODE(401), "Unauthorized" },
  { COAP_RESPONSE_CODE(402), "Bad Option" },
  { COAP_RESPONSE_CODE(403), "Forbidden" },
  { COAP_RESPONSE_CODE(404), "Not Found" },
  { COAP_RESPONSE_CODE(405), "Method Not Allowed" },
  { COAP_RESPONSE_CODE(406), "Not Acceptable" },
  { COAP_RESPONSE_CODE(408), "Request Entity Incomplete" },
  { COAP_RESPONSE_CODE(412), "Precondition Failed" },
  { COAP_RESPONSE_CODE(413), "Request Entity Too Large" },
  { COAP_RESPONSE_CODE(415), "Unsupported Content-Format" },
  { COAP_RESPONSE_CODE(500), "Internal Server Error" },
  { COAP_RESPONSE_CODE(501), "Not Implemented" },
  { COAP_RESPONSE_CODE(502), "Bad Gateway" },
  { COAP_RESPONSE_CODE(503), "Service Unavailable" },
  { COAP_RESPONSE_CODE(504), "Gateway Timeout" },
  { COAP_RESPONSE_CODE(505), "Proxying Not Supported" },
  { 0, NULL }			/* end marker */
};

char *
coap_response_phrase(unsigned char code) {
  int i;
  for (i = 0; coap_error[i].code; ++i) {
    if (coap_error[i].code == code)
      return coap_error[i].phrase;
  }
  return NULL;
}
#endif

/**
 * Advances *optp to next option if still in PDU. This function 
 * returns the number of bytes opt has been advanced or @c 0
 * on error.
 */
static size_t
next_option_safe(coap_opt_t **optp, size_t *length) {
  coap_option_t option;
  size_t optsize;

  assert(optp); assert(*optp); 
  assert(length);

  optsize = coap_opt_parse(*optp, *length, &option);
  if (optsize) {
    assert(optsize <= *length);

    *optp += optsize;
    *length -= optsize;
  }

  return optsize;
}

int
coap_pdu_parse(unsigned char *data, size_t length, coap_pdu_t *pdu) {
  coap_opt_t *opt;

  assert(data);
  assert(pdu);

  if (pdu->max_size < length) {
    debug("insufficient space to store parsed PDU\n");
    return 0;
  }

  if (length < sizeof(coap_hdr_t)) {
    debug("discarded invalid PDU\n");
  }

#ifdef WITH_LWIP
  /* this verifies that with the classical copy-at-parse-time and lwip's
   * zerocopy-into-place approaches, both share the same idea of destination
   * addresses */
  LWIP_ASSERT("coap_pdu_parse with unexpected addresses", data == (void*)pdu->hdr);
  LWIP_ASSERT("coap_pdu_parse with unexpected length", length == pdu->length);
#else

  pdu->hdr->version = data[0] >> 6;
  pdu->hdr->type = (data[0] >> 4) & 0x03;
  pdu->hdr->token_length = data[0] & 0x0f;
  pdu->hdr->code = data[1];
#endif
  pdu->data = NULL;

  /* sanity checks */
  if (pdu->hdr->code == 0) {
    if (length != sizeof(coap_hdr_t) || pdu->hdr->token_length) {
      debug("coap_pdu_parse: empty message is not empty\n");
      goto discard;
    }
  }

  if (length < sizeof(coap_hdr_t) + pdu->hdr->token_length
      || pdu->hdr->token_length > 8) {
    debug("coap_pdu_parse: invalid Token\n");
    goto discard;
  }

#ifndef WITH_LWIP
  /* Copy message id in network byte order, so we can easily write the
   * response back to the network. */
  memcpy(&pdu->hdr->id, data + 2, 2);

  /* Append data (including the Token) to pdu structure, if any. */
  if (length > sizeof(coap_hdr_t)) {
    memcpy(pdu->hdr + 1, data + sizeof(coap_hdr_t), length - sizeof(coap_hdr_t));
  }
  pdu->length = length;
 
  /* Finally calculate beginning of data block and thereby check integrity
   * of the PDU structure. */
#endif

  /* skip header + token */
  length -= (pdu->hdr->token_length + sizeof(coap_hdr_t));
  opt = (unsigned char *)(pdu->hdr + 1) + pdu->hdr->token_length;

  while (length && *opt != COAP_PAYLOAD_START) {
    if (!next_option_safe(&opt, (size_t *)&length)) {
      debug("coap_pdu_parse: drop\n");
      goto discard;
    }
  }

  /* end of packet or start marker */
  if (length) {
    assert(*opt == COAP_PAYLOAD_START);
    opt++; length--;

    if (!length) {
      debug("coap_pdu_parse: message ending in payload start marker\n");
      goto discard;
    }

    debug("set data to %p (pdu ends at %p)\n", (unsigned char *)opt, 
	  (unsigned char *)pdu->hdr + pdu->length);
    pdu->data = (unsigned char *)opt;
  }

  return 1;

 discard:
  return 0;
}
