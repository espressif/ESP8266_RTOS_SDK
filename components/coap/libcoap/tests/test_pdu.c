/* libcoap unit tests
 *
 * Copyright (C) 2012,2015 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use.
 */

#include "coap_config.h"
#include "test_pdu.h"

#include <coap.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

coap_pdu_t *pdu;	      /* Holds the parsed PDU for most tests */

/************************************************************************
 ** PDU decoder
 ************************************************************************/

static void
t_parse_pdu1(void) {
  uint8_t teststr[] = {  0x40, 0x01, 0x93, 0x34 };
  int result;

  result = coap_pdu_parse((unsigned char *)teststr, sizeof(teststr), pdu);
  CU_ASSERT(result > 0);

  CU_ASSERT(pdu->length == sizeof(teststr));
  CU_ASSERT(pdu->hdr->version == 1);
  CU_ASSERT(pdu->hdr->type == COAP_MESSAGE_CON);
  CU_ASSERT(pdu->hdr->token_length == 0);
  CU_ASSERT(pdu->hdr->code == COAP_REQUEST_GET);
  CU_ASSERT(memcmp(&pdu->hdr->id, teststr + 2, 2) == 0);
  CU_ASSERT_PTR_NULL(pdu->data);
}

static void
t_parse_pdu2(void) {
  uint8_t teststr[] = {  0x55, 0x69, 0x12, 0x34, 't', 'o', 'k', 'e', 'n' };
  int result;

  result = coap_pdu_parse((unsigned char *)teststr, sizeof(teststr), pdu);
  CU_ASSERT(result > 0);

  CU_ASSERT(pdu->length == sizeof(teststr));
  CU_ASSERT(pdu->hdr->version == 1);
  CU_ASSERT(pdu->hdr->type == COAP_MESSAGE_NON);
  CU_ASSERT(pdu->hdr->token_length == 5);
  CU_ASSERT(pdu->hdr->code == 0x69);
  CU_ASSERT(memcmp(&pdu->hdr->id, teststr + 2, 2) == 0);
  CU_ASSERT(memcmp(pdu->hdr->token, teststr + 4, 5) == 0);
  CU_ASSERT_PTR_NULL(pdu->data);
}

static void
t_parse_pdu3(void) {
  uint8_t teststr[] = {  0x53, 0x69, 0x12, 0x34, 't', 'o', 'k', 'e', 'n' };
  int result;

  result = coap_pdu_parse((unsigned char *)teststr, sizeof(teststr), pdu);
  CU_ASSERT(result == 0);
}

static void
t_parse_pdu4(void) {
  /* illegal token length */
  uint8_t teststr[] = {  0x59, 0x69, 0x12, 0x34,
		      't', 'o', 'k', 'e', 'n', '1', '2', '3', '4' };
  int result;

  result = coap_pdu_parse((unsigned char *)teststr, sizeof(teststr), pdu);
  CU_ASSERT(result == 0);

  teststr[0] = 0x5f;

  result = coap_pdu_parse((unsigned char *)teststr, sizeof(teststr), pdu);
  CU_ASSERT(result == 0);
}

static void
t_parse_pdu5(void) {
  /* PDU with options */
  uint8_t teststr[] = {  0x55, 0x73, 0x12, 0x34, 't', 'o', 'k', 'e',
		      'n',  0x00, 0xc1, 0x00
  };
  int result;

  result = coap_pdu_parse((unsigned char *)teststr, sizeof(teststr), pdu);
  CU_ASSERT(result > 0);

  CU_ASSERT(pdu->length == sizeof(teststr));
  CU_ASSERT(pdu->hdr->version == 1);
  CU_ASSERT(pdu->hdr->type == COAP_MESSAGE_NON);
  CU_ASSERT(pdu->hdr->token_length == 5);
  CU_ASSERT(pdu->hdr->code == 0x73);
  CU_ASSERT(memcmp(&pdu->hdr->id, teststr + 2, 2) == 0);
  CU_ASSERT(memcmp(pdu->hdr->token, teststr + 4, 5) == 0);
  CU_ASSERT_PTR_NULL(pdu->data);

  /* FIXME: check options */
}

static void
t_parse_pdu6(void) {
  /* PDU with options that exceed the PDU */
  uint8_t teststr[] = {  0x55, 0x73, 0x12, 0x34, 't', 'o', 'k', 'e',
		      'n',  0x00, 0xc1, 0x00, 0xae, 0xf0, 0x03
  };
  int result;

  result = coap_pdu_parse((unsigned char *)teststr, sizeof(teststr), pdu);
  CU_ASSERT(result == 0);
}

static void
t_parse_pdu7(void) {
  /* PDU with options and payload */
  uint8_t teststr[] = {  0x55, 0x73, 0x12, 0x34, 't', 'o', 'k', 'e',
		      'n',  0x00, 0xc1, 0x00, 0xff, 'p', 'a', 'y',
		      'l', 'o', 'a', 'd'
  };
  int result;

  result = coap_pdu_parse((unsigned char *)teststr, sizeof(teststr), pdu);
  CU_ASSERT(result > 0);

  CU_ASSERT(pdu->length == sizeof(teststr));
  CU_ASSERT(pdu->hdr->version == 1);
  CU_ASSERT(pdu->hdr->type == COAP_MESSAGE_NON);
  CU_ASSERT(pdu->hdr->token_length == 5);
  CU_ASSERT(pdu->hdr->code == 0x73);
  CU_ASSERT(memcmp(&pdu->hdr->id, teststr + 2, 2) == 0);
  CU_ASSERT(memcmp(pdu->hdr->token, teststr + 4, 5) == 0);

  /* FIXME: check options */

  CU_ASSERT(pdu->data == (unsigned char *)pdu->hdr + 13);
  CU_ASSERT(memcmp(pdu->data, teststr + 13, 7) == 0);
}

static void
t_parse_pdu8(void) {
  /* PDU without options but with payload */
  uint8_t teststr[] = {  0x50, 0x73, 0x12, 0x34,
		      0xff, 'p', 'a', 'y', 'l', 'o', 'a',
		      'd'
  };
  int result;

  result = coap_pdu_parse((unsigned char *)teststr, sizeof(teststr), pdu);
  CU_ASSERT(result > 0);

  CU_ASSERT(pdu->length == sizeof(teststr));
  CU_ASSERT(pdu->hdr->version == 1);
  CU_ASSERT(pdu->hdr->type == COAP_MESSAGE_NON);
  CU_ASSERT(pdu->hdr->token_length == 0);
  CU_ASSERT(pdu->hdr->code == 0x73);
  CU_ASSERT(memcmp(&pdu->hdr->id, teststr + 2, 2) == 0);

  /* FIXME: check options */

  CU_ASSERT(pdu->data == (unsigned char *)pdu->hdr + 5);
  CU_ASSERT(memcmp(pdu->data, teststr + 5, 7) == 0);
}

static void
t_parse_pdu9(void) {
  /* PDU without options and payload but with payload start marker */
  uint8_t teststr[] = {  0x70, 0x00, 0x12, 0x34, 0xff };
  int result;

  result = coap_pdu_parse((unsigned char *)teststr, sizeof(teststr), pdu);
  CU_ASSERT(result == 0);
}

static void
t_parse_pdu10(void) {
  /* PDU without payload but with options and payload start marker */
  uint8_t teststr[] = {  0x53, 0x73, 0x12, 0x34, 't', 'o', 'k',
		      0x30, 0xc1, 0x00, 0xff
  };
  int result;

  result = coap_pdu_parse((unsigned char *)teststr, sizeof(teststr), pdu);
  CU_ASSERT(result == 0);
}

static void
t_parse_pdu11(void) {
  uint8_t teststr[] = {  0x60, 0x00, 0x12, 0x34 };
  int result;

  result = coap_pdu_parse((unsigned char *)teststr, sizeof(teststr), pdu);
  CU_ASSERT(result > 0);

  CU_ASSERT(pdu->length == sizeof(teststr));
  CU_ASSERT(pdu->hdr->version == 1);
  CU_ASSERT(pdu->hdr->type == COAP_MESSAGE_ACK);
  CU_ASSERT(pdu->hdr->token_length == 0);
  CU_ASSERT(pdu->hdr->code == 0);
  CU_ASSERT(memcmp(&pdu->hdr->id, teststr + 2, 2) == 0);
}

static void
t_parse_pdu12(void) {
  /* RST */
  uint8_t teststr[] = {  0x70, 0x00, 0x12, 0x34 };
  int result;

  result = coap_pdu_parse((unsigned char *)teststr, sizeof(teststr), pdu);
  CU_ASSERT(result > 0);

  CU_ASSERT(pdu->length == sizeof(teststr));
  CU_ASSERT(pdu->hdr->version == 1);
  CU_ASSERT(pdu->hdr->type == COAP_MESSAGE_RST);
  CU_ASSERT(pdu->hdr->token_length == 0);
  CU_ASSERT(pdu->hdr->code == 0);
  CU_ASSERT(memcmp(&pdu->hdr->id, teststr + 2, 2) == 0);
}

static void
t_parse_pdu13(void) {
  /* RST with content */
  uint8_t teststr[] = {  0x70, 0x00, 0x12, 0x34,
		      0xff, 'c', 'o', 'n', 't', 'e', 'n', 't'
  };
  int result;

  result = coap_pdu_parse((unsigned char *)teststr, sizeof(teststr), pdu);
  CU_ASSERT(result == 0);
}

static void
t_parse_pdu14(void) {
  /* ACK with content */
  uint8_t teststr[] = {  0x60, 0x00, 0x12, 0x34,
		      0xff, 'c', 'o', 'n', 't', 'e', 'n', 't'
  };
  int result;

  result = coap_pdu_parse((unsigned char *)teststr, sizeof(teststr), pdu);
  CU_ASSERT(result == 0);
}

/************************************************************************
 ** PDU encoder
 ************************************************************************/

static void
t_encode_pdu1(void) {
  uint8_t teststr[] = { 0x45, 0x01, 0x12, 0x34, 't', 'o', 'k', 'e', 'n' };
  int result;

  coap_pdu_clear(pdu, pdu->max_size);
  pdu->hdr->type = COAP_MESSAGE_CON;
  pdu->hdr->code = COAP_REQUEST_GET;
  pdu->hdr->id = htons(0x1234);

  result = coap_add_token(pdu, 5, (unsigned char *)"token");

  CU_ASSERT(result == 1);
  CU_ASSERT(pdu->length == sizeof(teststr));
  CU_ASSERT_PTR_NULL(pdu->data);
  CU_ASSERT(memcmp(pdu->hdr, teststr, sizeof(teststr)) == 0);
}

static void
t_encode_pdu2(void) {
  size_t old_max = pdu->max_size;
  int result;

  coap_pdu_clear(pdu, 7);	/* set very small PDU size */

  pdu->hdr->type = COAP_MESSAGE_CON;
  pdu->hdr->code = COAP_REQUEST_GET;
  pdu->hdr->id = htons(0x1234);

  result = coap_add_token(pdu, 5, (unsigned char *)"token");

  CU_ASSERT(result == 0);

  coap_pdu_clear(pdu, old_max);	/* restore PDU size */
}

static void
t_encode_pdu3(void) {
  int result;

  result = coap_add_token(pdu, 9, (unsigned char *)"123456789");

  CU_ASSERT(result == 0);
}

static void
t_encode_pdu4(void) {
  /* PDU with options */
  uint8_t teststr[] = { 0x60, 0x99, 0x12, 0x34, 0x3d, 0x05, 0x66, 0x61,
		     0x6e, 0x63, 0x79, 0x70, 0x72, 0x6f, 0x78, 0x79,
		     0x2e, 0x63, 0x6f, 0x61, 0x70, 0x2e, 0x6d, 0x65,
		     0x84, 0x70, 0x61, 0x74, 0x68, 0x00, 0xe8, 0x1e,
		     0x28, 0x66, 0x61, 0x6e, 0x63, 0x79, 0x6f, 0x70,
		     0x74
  };
  int result;

  coap_pdu_clear(pdu, pdu->max_size);	/* clear PDU */

  pdu->hdr->type = COAP_MESSAGE_ACK;
  pdu->hdr->code = 0x99;
  pdu->hdr->id = htons(0x1234);

  CU_ASSERT(pdu->length == 4);

  result = coap_add_option(pdu, COAP_OPTION_URI_HOST,
       18, (unsigned char *)"fancyproxy.coap.me");

  CU_ASSERT(result == 20);
  CU_ASSERT(pdu->max_delta == 3);
  CU_ASSERT(pdu->length == 24);
  CU_ASSERT_PTR_NULL(pdu->data);

  result = coap_add_option(pdu, COAP_OPTION_URI_PATH,
			   4, (unsigned char *)"path");

  CU_ASSERT(result == 5);
  CU_ASSERT(pdu->max_delta == 11);
  CU_ASSERT(pdu->length == 29);
  CU_ASSERT_PTR_NULL(pdu->data);

  result = coap_add_option(pdu, COAP_OPTION_URI_PATH, 0, NULL);

  CU_ASSERT(result == 1);
  CU_ASSERT(pdu->max_delta == 11);
  CU_ASSERT(pdu->length == 30);
  CU_ASSERT_PTR_NULL(pdu->data);

  result = coap_add_option(pdu, 8000, 8, (unsigned char *)"fancyopt");

  CU_ASSERT(result == 11);
  CU_ASSERT(pdu->max_delta == 8000);
  CU_ASSERT(pdu->length == 41);
  CU_ASSERT_PTR_NULL(pdu->data);

  CU_ASSERT(pdu->length == sizeof(teststr));
  CU_ASSERT(memcmp(pdu->hdr, teststr, sizeof(teststr)) == 0);
}

static void
t_encode_pdu5(void) {
  /* PDU with token and options */
  uint8_t teststr[] = { 0x68, 0x84, 0x12, 0x34, '1',  '2',  '3',  '4',
                     '5',  '6',  '7',  '8',  0x18, 0x41, 0x42, 0x43,
		     0x44, 0x45, 0x46, 0x47, 0x48, 0xd1, 0x03, 0x12
  };
  int result;

  coap_pdu_clear(pdu, pdu->max_size);	/* clear PDU */

  pdu->hdr->type = COAP_MESSAGE_ACK;
  pdu->hdr->code = COAP_RESPONSE_CODE(404);
  pdu->hdr->id = htons(0x1234);

  CU_ASSERT(pdu->length == 4);

  result = coap_add_token(pdu, 8, (unsigned char *)"12345678");

  CU_ASSERT(pdu->length == 12);

  result = coap_add_option(pdu, COAP_OPTION_IF_MATCH,
			   8, (unsigned char *)"ABCDEFGH");

  CU_ASSERT(result == 9);
  CU_ASSERT(pdu->max_delta == 1);
  CU_ASSERT(pdu->length == 21);
  CU_ASSERT_PTR_NULL(pdu->data);

  result = coap_add_option(pdu, COAP_OPTION_ACCEPT,
			   1, (unsigned char *)"\x12");

  CU_ASSERT(result == 3);
  CU_ASSERT(pdu->max_delta == 17);
  CU_ASSERT(pdu->length == 24);
  CU_ASSERT_PTR_NULL(pdu->data);

  CU_ASSERT(pdu->length == sizeof(teststr));
  CU_ASSERT(memcmp(pdu->hdr, teststr, sizeof(teststr)) == 0);
}

static void
t_encode_pdu6(void) {
  /* PDU with data */
  uint8_t teststr[] = { 0x50, 0x02, 0x12, 0x34, 0xff, '1',  '2',  '3',
		     '4', '5',  '6',  '7',  '8'
  };
  coap_pdu_clear(pdu, pdu->max_size);	/* clear PDU */

  pdu->hdr->type = COAP_MESSAGE_NON;
  pdu->hdr->code = COAP_REQUEST_POST;
  pdu->hdr->id = htons(0x1234);

  CU_ASSERT(pdu->length == 4);
  CU_ASSERT_PTR_NULL(pdu->data);

  coap_add_data(pdu, 8, (unsigned char *)"12345678");

  CU_ASSERT(pdu->length == sizeof(teststr));
  CU_ASSERT(memcmp(pdu->hdr, teststr, sizeof(teststr)) == 0);
}

static void
t_encode_pdu7(void) {
  /* PDU with empty data */
  uint8_t teststr[] = { 0x40, 0x43, 0x12, 0x34 };
  int result;
  coap_pdu_clear(pdu, pdu->max_size);	/* clear PDU */

  pdu->hdr->type = COAP_MESSAGE_CON;
  pdu->hdr->code = COAP_RESPONSE_CODE(203);
  pdu->hdr->id = htons(0x1234);

  CU_ASSERT(pdu->length == 4);

  result = coap_add_data(pdu, 0, NULL);

  CU_ASSERT(result > 0);
  CU_ASSERT(pdu->length == 4);
  CU_ASSERT_PTR_NULL(pdu->data);

  CU_ASSERT(pdu->length == sizeof(teststr));
  CU_ASSERT(memcmp(pdu->hdr, teststr, sizeof(teststr)) == 0);
}

static void
t_encode_pdu8(void) {
  /* PDU with token and data */
  uint8_t teststr[] = { 0x42, 0x43, 0x12, 0x34, 0x00, 0x01, 0xff, 0x00 };
  int result;
  coap_pdu_clear(pdu, pdu->max_size);	/* clear PDU */

  pdu->hdr->type = COAP_MESSAGE_CON;
  pdu->hdr->code = COAP_RESPONSE_CODE(203);
  pdu->hdr->id = htons(0x1234);

  CU_ASSERT(pdu->length == 4);

  result = coap_add_token(pdu, 2, (unsigned char *)"\x00\x01");

  CU_ASSERT(result > 0);

  result = coap_add_data(pdu, 1, (unsigned char *)"\0");

  CU_ASSERT(result > 0);
  CU_ASSERT(pdu->length == 8);
  CU_ASSERT(pdu->data == (unsigned char *)pdu->hdr + 7);

  CU_ASSERT(pdu->length == sizeof(teststr));
  CU_ASSERT(memcmp(pdu->hdr, teststr, sizeof(teststr)) == 0);
}

static void
t_encode_pdu9(void) {
  /* PDU with options and data */
  uint8_t teststr[] = { 0x60, 0x44, 0x12, 0x34, 0x48, 's',  'o',  'm',
		     'e',  'e',  't',  'a',  'g',  0x10, 0xdd, 0x11,
		     0x04, 's',  'o',  'm',  'e',  'r',  'a',  't',
		     'h',  'e',  'r',  'l',  'o',  'n',  'g',  'u',
		     'r',  'i',  0xff, 'd',  'a',  't',  'a'
  };
  int result;

  coap_pdu_clear(pdu, pdu->max_size);	/* clear PDU */

  pdu->hdr->type = COAP_MESSAGE_ACK;
  pdu->hdr->code = COAP_RESPONSE_CODE(204);
  pdu->hdr->id = htons(0x1234);

  CU_ASSERT(pdu->length == 4);

  result = coap_add_option(pdu, COAP_OPTION_ETAG, 8, (unsigned char *)"someetag");

  CU_ASSERT(result == 9);
  CU_ASSERT(pdu->max_delta == 4);
  CU_ASSERT(pdu->length == 13);
  CU_ASSERT_PTR_NULL(pdu->data);

  result = coap_add_option(pdu, COAP_OPTION_IF_NONE_MATCH, 0, NULL);

  CU_ASSERT(result == 1);
  CU_ASSERT(pdu->max_delta == 5);
  CU_ASSERT(pdu->length == 14);
  CU_ASSERT_PTR_NULL(pdu->data);

  result = coap_add_option(pdu, COAP_OPTION_PROXY_URI,
			   17, (unsigned char *)"someratherlonguri");

  CU_ASSERT(result == 20);
  CU_ASSERT(pdu->max_delta == 35);
  CU_ASSERT(pdu->length == 34);
  CU_ASSERT_PTR_NULL(pdu->data);

  result = coap_add_data(pdu, 4, (unsigned char *)"data");

  CU_ASSERT(result > 0);
  CU_ASSERT(pdu->length == 39);
  CU_ASSERT(pdu->data == (unsigned char *)pdu->hdr + 35);

  CU_ASSERT(pdu->length == sizeof(teststr));
  CU_ASSERT(memcmp(pdu->hdr, teststr, sizeof(teststr)) == 0);
}

static void
t_encode_pdu10(void) {
  /* PDU with token, options and data */
  uint8_t teststr[] = { 0x62, 0x44, 0x12, 0x34, 0x00, 0x00, 0x8d, 0xf2,
		     'c',  'o',  'a',  'p',  ':',  '/',  '/',  'e',
		     'x',  'a',  'm',  'p',  'l',  'e',  '.',  'c',
		     'o',  'm',  '/',  '1',  '2',  '3',  '4',  '5',
		     '/',  '%',  '3',  'F',  'x',  'y',  'z',  '/',
		     '3',  '0',  '4',  '8',  '2',  '3',  '4',  '2',
		     '3',  '4',  '/',  '2',  '3',  '4',  '0',  '2',
		     '3',  '4',  '8',  '2',  '3',  '4',  '/',  '2',
		     '3',  '9',  '0',  '8',  '4',  '2',  '3',  '4',
		     '-',  '2',  '3',  '/',  '%',  'A',  'B',  '%',
		     '3',  '0',  '%',  'a',  'f',  '/',  '+',  '1',
		     '2',  '3',  '/',  'h',  'f',  'k',  's',  'd',
		     'h',  '/',  '2',  '3',  '4',  '8',  '0',  '-',
		     '2',  '3',  '4',  '-',  '9',  '8',  '2',  '3',
		     '5',  '/',  '1',  '2',  '0',  '4',  '/',  '2',
		     '4',  '3',  '5',  '4',  '6',  '3',  '4',  '5',
		     '3',  '4',  '5',  '2',  '4',  '3',  '/',  '0',
		     '1',  '9',  '8',  's',  'd',  'n',  '3',  '-',
		     'a',  '-',  '3',  '/',  '/',  '/',  'a',  'f',
		     'f',  '0',  '9',  '3',  '4',  '/',  '9',  '7',
		     'u',  '2',  '1',  '4',  '1',  '/',  '0',  '0',
		     '0',  '2',  '/',  '3',  '9',  '3',  '2',  '4',
		     '2',  '3',  '5',  '3',  '2',  '/',  '5',  '6',
		     '2',  '3',  '4',  '0',  '2',  '3',  '/',  '-',
		     '-',  '-',  '-',  '/',  '=',  '1',  '2',  '3',
		     '4',  '=',  '/',  '0',  '9',  '8',  '1',  '4',
		     '1',  '-',  '9',  '5',  '6',  '4',  '6',  '4',
		     '3',  '/',  '2',  '1',  '9',  '7',  '0',  '-',
		     '-',  '-',  '-',  '-',  '/',  '8',  '2',  '3',
		     '6',  '4',  '9',  '2',  '3',  '4',  '7',  '2',
		     'w',  'e',  'r',  'e',  'r',  'e',  'w',  'r',
		     '0',  '-',  '9',  '2',  '1',  '-',  '3',  '9',
		     '1',  '2',  '3',  '-',  '3',  '4',  '/',  0x0d,
		     0x01, '/',  '/',  '4',  '9',  '2',  '4',  '0',
		     '3',  '-',  '-',  '0',  '9',  '8',  '/',  0xc1,
		     '*',  0xff, 'd',  'a',  't',  'a'
  };
  int result;

  coap_pdu_clear(pdu, pdu->max_size);	/* clear PDU */

  pdu->hdr->type = COAP_MESSAGE_ACK;
  pdu->hdr->code = COAP_RESPONSE_CODE(204);
  pdu->hdr->id = htons(0x1234);

  CU_ASSERT(pdu->length == 4);

  result = coap_add_token(pdu, 2, (unsigned char *)"\0\0");

  CU_ASSERT(result > 0);
  result = coap_add_option(pdu, COAP_OPTION_LOCATION_PATH, 255,
			   (unsigned char *)"coap://example.com/12345/%3Fxyz/3048234234/23402348234/239084234-23/%AB%30%af/+123/hfksdh/23480-234-98235/1204/243546345345243/0198sdn3-a-3///aff0934/97u2141/0002/3932423532/56234023/----/=1234=/098141-9564643/21970-----/82364923472wererewr0-921-39123-34/");

  CU_ASSERT(result == 257);
  CU_ASSERT(pdu->max_delta == 8);
  CU_ASSERT(pdu->length == 263);
  CU_ASSERT_PTR_NULL(pdu->data);

  result = coap_add_option(pdu, COAP_OPTION_LOCATION_PATH, 14,
			   (unsigned char *)"//492403--098/");

  CU_ASSERT(result == 16);
  CU_ASSERT(pdu->max_delta == 8);
  CU_ASSERT(pdu->length == 279);
  CU_ASSERT_PTR_NULL(pdu->data);

  result = coap_add_option(pdu, COAP_OPTION_LOCATION_QUERY,
			   1, (unsigned char *)"*");

  CU_ASSERT(result == 2);
  CU_ASSERT(pdu->max_delta == 20);
  CU_ASSERT(pdu->length == 281);
  CU_ASSERT_PTR_NULL(pdu->data);

  result = coap_add_data(pdu, 4, (unsigned char *)"data");

  CU_ASSERT(result > 0);
  CU_ASSERT(pdu->length == 286);
  CU_ASSERT(pdu->data == (unsigned char *)pdu->hdr + 282);

  CU_ASSERT(pdu->length == sizeof(teststr));
  CU_ASSERT(memcmp(pdu->hdr, teststr, sizeof(teststr)) == 0);
}

static void
t_encode_pdu11(void) {
  coap_log_t level = coap_get_log_level();
  /* data too long for PDU */
  size_t old_max = pdu->max_size;
  int result;

  coap_pdu_clear(pdu, 8);	/* clear PDU, with small maximum */

  CU_ASSERT(pdu->data == NULL);
  coap_set_log_level(LOG_CRIT);
  result = coap_add_data(pdu, 10, (unsigned char *)"0123456789");
  coap_set_log_level(level);

  CU_ASSERT(result == 0);
  CU_ASSERT(pdu->data == NULL);

  pdu->max_size = old_max;
}

static int
t_pdu_tests_create(void) {
  pdu = coap_pdu_init(0, 0, 0, COAP_MAX_PDU_SIZE);

  return pdu == NULL;
}

static int
t_pdu_tests_remove(void) {
  coap_delete_pdu(pdu);
  return 0;
}

CU_pSuite
t_init_pdu_tests(void) {
  CU_pSuite suite[2];

  suite[0] = CU_add_suite("pdu parser", t_pdu_tests_create, t_pdu_tests_remove);
  if (!suite[0]) {			/* signal error */
    fprintf(stderr, "W: cannot add pdu parser test suite (%s)\n",
	    CU_get_error_msg());

    return NULL;
  }

#define PDU_TEST(s,t)						      \
  if (!CU_ADD_TEST(s,t)) {					      \
    fprintf(stderr, "W: cannot add pdu parser test (%s)\n",	      \
	    CU_get_error_msg());				      \
  }

  PDU_TEST(suite[0], t_parse_pdu1);
  PDU_TEST(suite[0], t_parse_pdu2);
  PDU_TEST(suite[0], t_parse_pdu3);
  PDU_TEST(suite[0], t_parse_pdu4);
  PDU_TEST(suite[0], t_parse_pdu5);
  PDU_TEST(suite[0], t_parse_pdu6);
  PDU_TEST(suite[0], t_parse_pdu7);
  PDU_TEST(suite[0], t_parse_pdu8);
  PDU_TEST(suite[0], t_parse_pdu9);
  PDU_TEST(suite[0], t_parse_pdu10);
  PDU_TEST(suite[0], t_parse_pdu11);
  PDU_TEST(suite[0], t_parse_pdu12);
  PDU_TEST(suite[0], t_parse_pdu13);
  PDU_TEST(suite[0], t_parse_pdu14);

  suite[1] = CU_add_suite("pdu encoder", t_pdu_tests_create, t_pdu_tests_remove);
  if (suite[1]) {
#define PDU_ENCODER_TEST(s,t)						      \
  if (!CU_ADD_TEST(s,t)) {					      \
    fprintf(stderr, "W: cannot add pdu encoder test (%s)\n",	      \
	    CU_get_error_msg());				      \
  }
    PDU_ENCODER_TEST(suite[1], t_encode_pdu1);
    PDU_ENCODER_TEST(suite[1], t_encode_pdu2);
    PDU_ENCODER_TEST(suite[1], t_encode_pdu3);
    PDU_ENCODER_TEST(suite[1], t_encode_pdu4);
    PDU_ENCODER_TEST(suite[1], t_encode_pdu5);
    PDU_ENCODER_TEST(suite[1], t_encode_pdu6);
    PDU_ENCODER_TEST(suite[1], t_encode_pdu7);
    PDU_ENCODER_TEST(suite[1], t_encode_pdu8);
    PDU_ENCODER_TEST(suite[1], t_encode_pdu9);
    PDU_ENCODER_TEST(suite[1], t_encode_pdu10);
    PDU_ENCODER_TEST(suite[1], t_encode_pdu11);

  } else 			/* signal error */
    fprintf(stderr, "W: cannot add pdu parser test suite (%s)\n",
	    CU_get_error_msg());

  return suite[0];
}

