/* async.c -- state management for asynchronous messages
 *
 * Copyright (C) 2010,2011 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use. 
 */

/** 
 * @file async.c
 * @brief state management for asynchronous messages
 */

#ifndef WITHOUT_ASYNC

#include "coap_config.h"
#include "coap.h"
#include "async.h"
#include "debug.h"
#include "mem.h"
#include "utlist.h"

coap_async_state_t *
coap_register_async(coap_context_t *context, coap_address_t *peer,
		    coap_pdu_t *request, unsigned char flags, void *data) {
  coap_async_state_t *s;
  coap_tid_t id;

  coap_transaction_id(peer, request, &id);
  LL_SEARCH_SCALAR(context->async_state,s,id,id);

  if (s != NULL) {
    /* We must return NULL here as the caller must know that he is
     * responsible for releasing @p data. */
    debug("asynchronous state for transaction %d already registered\n", id);
    return NULL;
  }

  /* store information for handling the asynchronous task */
  s = (coap_async_state_t *)coap_malloc(sizeof(coap_async_state_t) + 
					request->hdr->token_length);
  if (!s) {
    coap_log(LOG_CRIT, "coap_register_async: insufficient memory\n");
    return NULL;
  }

  memset(s, 0, sizeof(coap_async_state_t) + request->hdr->token_length);

  /* set COAP_ASYNC_CONFIRM according to request's type */
  s->flags = flags & ~COAP_ASYNC_CONFIRM;
  if (request->hdr->type == COAP_MESSAGE_CON)
    s->flags |= COAP_ASYNC_CONFIRM;

  s->appdata = data;

  memcpy(&s->peer, peer, sizeof(coap_address_t));

  if (request->hdr->token_length) {
    s->tokenlen = request->hdr->token_length;
    memcpy(s->token, request->hdr->token, request->hdr->token_length);
  }
    
  memcpy(&s->id, &id, sizeof(coap_tid_t));

  coap_touch_async(s);

  LL_PREPEND(context->async_state, s);

  return s;
}

coap_async_state_t *
coap_find_async(coap_context_t *context, coap_tid_t id) {
  coap_async_state_t *tmp;
  LL_SEARCH_SCALAR(context->async_state,tmp,id,id);  
  return tmp;
}

int
coap_remove_async(coap_context_t *context, coap_tid_t id, 
		  coap_async_state_t **s) {
  coap_async_state_t *tmp = coap_find_async(context, id);

  if (tmp)
    LL_DELETE(context->async_state,tmp);

  *s = tmp;
  return tmp != NULL;
}

void 
coap_free_async(coap_async_state_t *s) {
  if (s && (s->flags & COAP_ASYNC_RELEASE_DATA) != 0)
    coap_free(s->appdata);
  coap_free(s); 
}

#else
void does_not_exist();	/* make some compilers happy */
#endif /* WITHOUT_ASYNC */
