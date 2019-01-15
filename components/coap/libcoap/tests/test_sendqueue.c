/* libcoap unit tests
 *
 * Copyright (C) 2013,2015 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use.
 */

#include "coap_config.h"
#include "test_sendqueue.h"

#include <coap.h>

#include <stdio.h>

static coap_queue_t *sendqueue;

/* timestamps for tests. The first element in this array denotes the
 * base time in ticks, the following elements are timestamps relative
 * to this basetime.
 */
static coap_tick_t timestamp[] = {
  0, 100, 200, 30, 160
};

/* nodes for testing. node[0] is left empty */
coap_queue_t *node[5];

static coap_tick_t
add_timestamps(coap_queue_t *queue, size_t num) {
  coap_tick_t t = 0;
  while (queue && num--) {
    t += queue->t;
    queue = queue->next;
  }

  return t;
}

static void
t_sendqueue1(void) {
  int result = coap_insert_node(&sendqueue, node[1]);

  CU_ASSERT(result > 0);
  CU_ASSERT_PTR_NOT_NULL(sendqueue);
  CU_ASSERT_PTR_EQUAL(sendqueue, node[1]);
  CU_ASSERT(node[1]->t == timestamp[1]);
}

static void
t_sendqueue2(void) {
  int result;

  result = coap_insert_node(&sendqueue, node[2]);

  CU_ASSERT(result > 0);
  CU_ASSERT_PTR_EQUAL(sendqueue, node[1]);
  CU_ASSERT_PTR_EQUAL(sendqueue->next, node[2]);

  CU_ASSERT(sendqueue->t == timestamp[1]);
  CU_ASSERT(node[2]->t == timestamp[2] - timestamp[1]);
}

/* insert new node as first element in queue */
static void
t_sendqueue3(void) {
  int result;
  result = coap_insert_node(&sendqueue, node[3]);

  CU_ASSERT(result > 0);

  CU_ASSERT_PTR_EQUAL(sendqueue, node[3]);
  CU_ASSERT(node[3]->t == timestamp[3]);

  CU_ASSERT_PTR_NOT_NULL(sendqueue->next);
  CU_ASSERT_PTR_NOT_NULL(sendqueue->next->next);

  CU_ASSERT(sendqueue->next->t == timestamp[1] - timestamp[3]);
  CU_ASSERT(sendqueue->next->next->t == timestamp[2] - timestamp[1]);
}

/* insert new node as fourth element in queue */
static void
t_sendqueue4(void) {
  int result;

  result = coap_insert_node(&sendqueue, node[4]);

  CU_ASSERT(result > 0);

  CU_ASSERT_PTR_EQUAL(sendqueue, node[3]);

  CU_ASSERT_PTR_NOT_NULL(sendqueue->next);
  CU_ASSERT_PTR_EQUAL(sendqueue->next, node[1]);

  CU_ASSERT_PTR_NOT_NULL(sendqueue->next->next);
  CU_ASSERT_PTR_EQUAL(sendqueue->next->next, node[4]);

  CU_ASSERT_PTR_NOT_NULL(sendqueue->next->next->next);
  CU_ASSERT_PTR_EQUAL(sendqueue->next->next->next, node[2]);

  CU_ASSERT(sendqueue->next->t == timestamp[1] - timestamp[3]);
  CU_ASSERT(add_timestamps(sendqueue, 1) == timestamp[3]);
  CU_ASSERT(add_timestamps(sendqueue, 2) == timestamp[1]);
  CU_ASSERT(add_timestamps(sendqueue, 3) == timestamp[4]);
  CU_ASSERT(add_timestamps(sendqueue, 4) == timestamp[2]);
}

static void
t_sendqueue5(void) {
  const coap_tick_diff_t delta1 = 20, delta2 = 130;
  unsigned int result;
  coap_tick_t now;
  struct coap_context_t ctx;

  /* space for saving the current node timestamps */
  static coap_tick_t times[sizeof(timestamp)/sizeof(coap_tick_t)];
  coap_queue_t *p;
  int i;

  /* save timestamps of nodes in the sendqueue in their actual order */
  memset(times, 0, sizeof(times));
  for (p = sendqueue, i = 0; p; p = p->next, i++) {
    times[i] = p->t;
  }

  coap_ticks(&now);
  ctx.sendqueue = sendqueue;
  ctx.sendqueue_basetime = now;

  now -= delta1;
  result = coap_adjust_basetime(&ctx, now);

  CU_ASSERT(result == 0);
  CU_ASSERT_PTR_NOT_NULL(ctx.sendqueue);
  CU_ASSERT(ctx.sendqueue_basetime == now);
  CU_ASSERT(ctx.sendqueue->t == timestamp[3] + delta1);

  now += delta2;
  result = coap_adjust_basetime(&ctx, now);
  CU_ASSERT(result == 2);
  CU_ASSERT(ctx.sendqueue_basetime == now);
  CU_ASSERT_PTR_NOT_NULL(ctx.sendqueue);
  CU_ASSERT(ctx.sendqueue->t == 0);

  CU_ASSERT_PTR_NOT_NULL(ctx.sendqueue->next);
  CU_ASSERT(ctx.sendqueue->next->t == 0);

  CU_ASSERT_PTR_NOT_NULL(ctx.sendqueue->next->next);
  CU_ASSERT(ctx.sendqueue->next->next->t == delta2 - delta1 - timestamp[1]);

  /* restore timestamps of nodes in the sendqueue */
  for (p = sendqueue, i = 0; p; p = p->next, i++) {
    p->t = times[i];
  }
}

static void
t_sendqueue6(void) {
  unsigned int result;
  coap_tick_t now;
  const coap_tick_diff_t delta = 20;
  struct coap_context_t ctx;

  /* space for saving the current node timestamps */
  static coap_tick_t times[sizeof(timestamp)/sizeof(coap_tick_t)];
  coap_queue_t *p;
  int i;

  /* save timestamps of nodes in the sendqueue in their actual order */
  memset(times, 0, sizeof(times));
  for (p = sendqueue, i = 0; p; p = p->next, i++) {
    times[i] = p->t;
  }

  coap_ticks(&now);
  ctx.sendqueue = NULL;
  ctx.sendqueue_basetime = now;

  result = coap_adjust_basetime(&ctx, now + delta);

  CU_ASSERT(result == 0);
  CU_ASSERT(ctx.sendqueue_basetime == now + delta);

  /* restore timestamps of nodes in the sendqueue */
  for (p = sendqueue, i = 0; p; p = p->next, i++) {
    p->t = times[i];
  }
}

static void
t_sendqueue7(void) {
  int result;
  coap_queue_t *tmp_node;

  CU_ASSERT_PTR_NOT_NULL(sendqueue);
  CU_ASSERT_PTR_EQUAL(sendqueue, node[3]);

  CU_ASSERT_PTR_NOT_NULL(sendqueue->next);
  CU_ASSERT_PTR_EQUAL(sendqueue->next, node[1]);

  result = coap_remove_from_queue(&sendqueue, 3, &tmp_node);

  CU_ASSERT(result == 1);
  CU_ASSERT_PTR_NOT_NULL(tmp_node);
  CU_ASSERT_PTR_EQUAL(tmp_node, node[3]);

  CU_ASSERT_PTR_NOT_NULL(sendqueue);
  CU_ASSERT_PTR_EQUAL(sendqueue, node[1]);

  CU_ASSERT(sendqueue->t == timestamp[1]);
}

static void
t_sendqueue8(void) {
  int result;
  coap_queue_t *tmp_node;

  result = coap_remove_from_queue(&sendqueue, 4, &tmp_node);

  CU_ASSERT(result == 1);
  CU_ASSERT_PTR_NOT_NULL(tmp_node);
  CU_ASSERT_PTR_EQUAL(tmp_node, node[4]);

  CU_ASSERT_PTR_NOT_NULL(sendqueue);
  CU_ASSERT_PTR_EQUAL(sendqueue, node[1]);
  CU_ASSERT(sendqueue->t == timestamp[1]);

  CU_ASSERT_PTR_NOT_NULL(sendqueue->next);
  CU_ASSERT_PTR_EQUAL(sendqueue->next, node[2]);
  CU_ASSERT(sendqueue->next->t == timestamp[2] - timestamp[1]);

  CU_ASSERT_PTR_NULL(sendqueue->next->next);
}

static void
t_sendqueue9(void) {
  coap_queue_t *tmp_node;
  struct coap_context_t ctx;

  /* Initialize a fake context that points to our global sendqueue
   * Note that all changes happen on ctx.sendqueue. */
  ctx.sendqueue = sendqueue;
  tmp_node = coap_peek_next(&ctx);
  sendqueue = ctx.sendqueue;	/* must update global sendqueue for correct result */

  CU_ASSERT_PTR_NOT_NULL(tmp_node);
  CU_ASSERT_PTR_EQUAL(tmp_node, node[1]);
  CU_ASSERT_PTR_EQUAL(tmp_node, ctx.sendqueue);

  tmp_node = coap_pop_next(&ctx);
  sendqueue = ctx.sendqueue;	/* must update global sendqueue for correct result */

  CU_ASSERT_PTR_NOT_NULL(tmp_node);
  CU_ASSERT_PTR_EQUAL(tmp_node, node[1]);

  CU_ASSERT_PTR_NOT_NULL(sendqueue);
  CU_ASSERT_PTR_EQUAL(sendqueue, node[2]);

  CU_ASSERT(tmp_node->t == timestamp[1]);
  CU_ASSERT(sendqueue->t == timestamp[2]);

  CU_ASSERT_PTR_NULL(sendqueue->next);
}

static void
t_sendqueue10(void) {
  coap_queue_t *tmp_node;
  struct coap_context_t ctx;

  /* Initialize a fake context that points to our global sendqueue
   * Note that all changes happen on ctx.sendqueue. */
  ctx.sendqueue = sendqueue;

  tmp_node = coap_pop_next(&ctx);
  sendqueue = ctx.sendqueue;	/* must update global sendqueue for correct result */

  CU_ASSERT_PTR_NOT_NULL(tmp_node);
  CU_ASSERT_PTR_EQUAL(tmp_node, node[2]);

  CU_ASSERT_PTR_NULL(sendqueue);

  CU_ASSERT(tmp_node->t == timestamp[2]);
}

/* This function creates a set of nodes for testing. These nodes
 * will exist for all tests and are modified by coap_insert_node()
 * and coap_remove_from_queue().
 */
static int
t_sendqueue_tests_create(void) {
  size_t n, error = 0;
  sendqueue = NULL;
  coap_ticks(&timestamp[0]);

  memset(node, 0, sizeof(node));
  for (n = 1; n < sizeof(node)/sizeof(coap_queue_t *); n++) {
    node[n] = coap_new_node();
    if (!node[n]) {
      error = 1;
      break;
    }

    node[n]->id = n;
    node[n]->t = timestamp[n];
  }

  if (error) {
    /* destroy all test nodes and set entry to zero */
    for (n = 0; n < sizeof(node)/sizeof(coap_queue_t *); n++) {
      if (node[n]) {
	coap_delete_node(node[n]);
	node[n] = NULL;
      }
    }
  }

  return error;
}

static int
t_sendqueue_tests_remove(void) {
  size_t n;

  /* destroy all test nodes */
  for (n = 0; n < sizeof(node)/sizeof(coap_queue_t *); n++) {
    if (node[n]) {
      coap_delete_node(node[n]);
    }
  }

  return 0;
}

CU_pSuite
t_init_sendqueue_tests(void) {
  CU_pSuite suite;

  suite = CU_add_suite("sendqueue",
		       t_sendqueue_tests_create, t_sendqueue_tests_remove);
  if (!suite) {			/* signal error */
    fprintf(stderr, "W: cannot add sendqueue test suite (%s)\n",
	    CU_get_error_msg());

    return NULL;
  }

#define SENDQUEUE_TEST(s,t)					      \
  if (!CU_ADD_TEST(s,t)) {					      \
    fprintf(stderr, "W: cannot add sendqueue test (%s)\n",	      \
	    CU_get_error_msg());				      \
  }

  SENDQUEUE_TEST(suite, t_sendqueue1);
  SENDQUEUE_TEST(suite, t_sendqueue2);
  SENDQUEUE_TEST(suite, t_sendqueue3);
  SENDQUEUE_TEST(suite, t_sendqueue4);
  SENDQUEUE_TEST(suite, t_sendqueue5);
  SENDQUEUE_TEST(suite, t_sendqueue6);
  SENDQUEUE_TEST(suite, t_sendqueue7);
  SENDQUEUE_TEST(suite, t_sendqueue8);
  SENDQUEUE_TEST(suite, t_sendqueue9);
  SENDQUEUE_TEST(suite, t_sendqueue10);

  return suite;
}

