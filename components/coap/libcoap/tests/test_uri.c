/* libcoap unit tests
 *
 * Copyright (C) 2012,2015 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use. 
 */

#include "coap_config.h"
#include "test_uri.h"

#include <coap.h>

#include <stdio.h>

static void
t_parse_uri1(void) {
  char teststr[] = "coap://[::1]/.well-known/core";

  int result;
  coap_uri_t uri;

  result = coap_split_uri((unsigned char *)teststr, strlen(teststr), &uri);
  if (result == 0) {
    CU_ASSERT(uri.host.length == 3);
    CU_ASSERT_NSTRING_EQUAL(uri.host.s, "::1", 3);

    CU_ASSERT(uri.port == COAP_DEFAULT_PORT);

    CU_ASSERT(uri.path.length == 16);
    CU_ASSERT_NSTRING_EQUAL(uri.path.s, ".well-known/core", 16);

    CU_ASSERT(uri.query.length == 0);    
    CU_ASSERT(uri.query.s == NULL);    
  } else {
    CU_FAIL("uri parser error");
  }
}

static void
t_parse_uri2(void) {
  char teststr[] = "coap://[::1]:8000/.well-known/core";
  int result;
  coap_uri_t uri;

  result = coap_split_uri((unsigned char *)teststr, strlen(teststr), &uri);
  if (result == 0) {
    CU_ASSERT(uri.host.length == 3);
    CU_ASSERT_NSTRING_EQUAL(uri.host.s, "::1", 3);

    CU_ASSERT(uri.port == 8000);

    CU_ASSERT(uri.path.length == 16);
    CU_ASSERT_NSTRING_EQUAL(uri.path.s, ".well-known/core", 16);

    CU_ASSERT(uri.query.length == 0);    
    CU_ASSERT(uri.query.s == NULL);    
  } else {
    CU_FAIL("uri parser error");
  }
}

static void
t_parse_uri3(void) {
  char teststr[] = "coap://localhost/?foo&bla=fasel";
  int result;
  coap_uri_t uri;

  result = coap_split_uri((unsigned char *)teststr, strlen(teststr), &uri);
  if (result == 0) {
    CU_ASSERT(uri.host.length == 9);
    CU_ASSERT_NSTRING_EQUAL(uri.host.s, "localhost", 9);

    CU_ASSERT(uri.port == COAP_DEFAULT_PORT);

    CU_ASSERT(uri.path.length == 0);

    CU_ASSERT(uri.query.length == 13);    
    CU_ASSERT_NSTRING_EQUAL(uri.query.s, "foo&bla=fasel", 13);
  } else {
    CU_FAIL("uri parser error");
  }
}

static void
t_parse_uri4(void) {
  char teststr[] = "coap://:100000";
  int result;
  coap_uri_t uri;

  result = coap_split_uri((unsigned char *)teststr, strlen(teststr), &uri);
  CU_ASSERT(result < 0);
}

static void
t_parse_uri5(void) {
  char teststr[] = "coap://foo:100000";
  int result;
  coap_uri_t uri;

  result = coap_split_uri((unsigned char *)teststr, strlen(teststr), &uri);
  if (result == 0) {
    CU_ASSERT(uri.host.length == 3);
    CU_ASSERT_NSTRING_EQUAL(uri.host.s, "foo", 3);

    CU_ASSERT(uri.path.length == 0);
    CU_ASSERT(uri.path.s == NULL);

    CU_ASSERT(uri.query.length == 0);
    CU_ASSERT(uri.query.s == NULL);

    CU_FAIL("invalid port not detected");
  } else {
    CU_PASS("detected invalid port");
  }
}

static void
t_parse_uri6(void) {
  char teststr[] = "coap://134.102.218.2/.well-known/core";
  int result;
  coap_uri_t uri;

  result = coap_split_uri((unsigned char *)teststr, strlen(teststr), &uri);
  if (result == 0) {
    CU_ASSERT(uri.host.length == 13);
    CU_ASSERT_NSTRING_EQUAL(uri.host.s, "134.102.218.2", 13);

    CU_ASSERT(uri.port == COAP_DEFAULT_PORT);

    CU_ASSERT(uri.path.length == 16);
    CU_ASSERT_NSTRING_EQUAL(uri.path.s, ".well-known/core", 16);

    CU_ASSERT(uri.query.length == 0);    
    CU_ASSERT(uri.query.s == NULL);    
  } else {
    CU_FAIL("uri parser error");
  }
}

static void
t_parse_uri7(void) {
  char teststr[] = "coap://foo.bar:5683/some_resource/with/multiple/segments";
  int result;
  coap_uri_t uri;
  unsigned char buf[40];
  size_t buflen = sizeof(buf);

  /* The list of path segments to check against. Each segment is
     preceded by a dummy option indicating that holds the (dummy)
     delta value 0 and the actual segment length. */
  const unsigned char checkbuf[] = {
    0x0d, 0x00, 's', 'o', 'm', 'e', '_', 'r', 'e', 's', 'o', 'u', 'r', 'c', 'e',
    0x04, 'w', 'i', 't', 'h',
    0x08, 'm', 'u', 'l', 't', 'i', 'p', 'l', 'e',
    0x08, 's', 'e', 'g', 'm', 'e', 'n', 't', 's'
  };

  result = coap_split_uri((unsigned char *)teststr, strlen(teststr), &uri);
  if (result == 0) {
    CU_ASSERT(uri.host.length == 7);
    CU_ASSERT_NSTRING_EQUAL(uri.host.s, "foo.bar", 7);

    CU_ASSERT(uri.port == 5683);

    CU_ASSERT(uri.path.length == 36);
    CU_ASSERT_NSTRING_EQUAL(uri.path.s, "some_resource/with/multiple/segments", 36);

    CU_ASSERT(uri.query.length == 0);
    CU_ASSERT(uri.query.s == NULL);    

    /* check path segments */
    result = coap_split_path(uri.path.s, uri.path.length, buf, &buflen);
    CU_ASSERT(result == 4);
    CU_ASSERT(buflen == sizeof(checkbuf));
    CU_ASSERT_NSTRING_EQUAL(buf, checkbuf, buflen);
  } else {
    CU_FAIL("uri parser error");
  }
}

static void
t_parse_uri8(void) {
  char teststr[] = "http://example.com/%7E%AB%13";
  int result;
  coap_uri_t uri;

  result = coap_split_uri((unsigned char *)teststr, strlen(teststr), &uri);
  if (result < 0) {
    CU_PASS("detected non-coap URI");
  } else {
    CU_FAIL("non-coap URI not recognized");
  }
}

static void
t_parse_uri9(void) {
  char teststr[] = "http://example.com/%x";
  int result;
  coap_uri_t uri;

  result = coap_split_uri((unsigned char *)teststr, strlen(teststr), &uri);
  if (result < 0) {
    CU_PASS("detected non-coap URI");
  } else {
    CU_FAIL("non-coap URI not recognized");
  }
}

static void
t_parse_uri10(void) {
  char teststr[] = "/absolute/path";
  int result;
  coap_uri_t uri;

  result = coap_split_uri((unsigned char *)teststr, strlen(teststr), &uri);
  if (result == 0) {
    CU_ASSERT(uri.host.length == 0);
    CU_ASSERT(uri.host.s == NULL);

    CU_ASSERT(uri.port == COAP_DEFAULT_PORT);

    CU_ASSERT(uri.path.length == 13);
    CU_ASSERT_NSTRING_EQUAL(uri.path.s, "absolute/path", 13);

    CU_ASSERT(uri.query.length == 0);    
    CU_ASSERT(uri.query.s == NULL);    
  } else {
    CU_FAIL("uri parser error");
  }
}

static void
t_parse_uri11(void) {
  char teststr[] = 
    "coap://xn--18j4d.example/%E3%81%93%E3%82%93%E3%81%AB%E3%81%A1%E3%81%AF";
  int result;
  coap_uri_t uri;
  unsigned char buf[40];
  size_t buflen = sizeof(buf);

  /* The list of path segments to check against. Each segment is
     preceded by a dummy option indicating that holds the (dummy)
     delta value 0 and the actual segment length. */
  const unsigned char checkbuf[] = {
    0x0d, 0x02, 0xE3, 0x81, 0x93, 0xE3, 0x82, 0x93, 
    0xE3, 0x81, 0xAB, 0xE3, 0x81, 0xA1, 0xE3, 0x81, 
    0xAF
  };

  result = coap_split_uri((unsigned char *)teststr, strlen(teststr), &uri);
  if (result == 0) {
    CU_ASSERT(uri.host.length == 17);
    CU_ASSERT_NSTRING_EQUAL(uri.host.s, "xn--18j4d.example", 17);

    CU_ASSERT(uri.port == COAP_DEFAULT_PORT);

    CU_ASSERT(uri.path.length == 45);
    CU_ASSERT_NSTRING_EQUAL(uri.path.s, 
			    "%E3%81%93%E3%82%93%E3%81%AB%E3%81%A1%E3%81%AF", 45);

    CU_ASSERT(uri.query.length == 0);    
    CU_ASSERT(uri.query.s == NULL);    
    
    /* check path segments */
    result = coap_split_path(uri.path.s, uri.path.length, buf, &buflen);
    CU_ASSERT(result == 1);
    CU_ASSERT(buflen == sizeof(checkbuf));
    CU_ASSERT_NSTRING_EQUAL(buf, checkbuf, buflen);
  } else {
    CU_FAIL("uri parser error");
  }
}

static void
t_parse_uri12(void) {
  char teststr[] = "coap://198.51.100.1:61616//%2F//?%2F%2F&?%26";
  int result;
  coap_uri_t uri;
  unsigned char buf[40];
  size_t buflen = sizeof(buf);

  /* The list of path segments to check against. Each segment is
     preceded by a dummy option indicating that holds the (dummy)
     delta value 0 and the actual segment length. */
  const unsigned char uricheckbuf[] = { 0x00, 0x01, 0x2f, 0x00, 0x00 };
  const unsigned char querycheckbuf[] = { 0x02, 0x2f, 0x2f, 0x02, 0x3f, 0x26 };

  result = coap_split_uri((unsigned char *)teststr, strlen(teststr), &uri);
  if (result == 0) {
    CU_ASSERT(uri.host.length == 12);
    CU_ASSERT_NSTRING_EQUAL(uri.host.s, "198.51.100.1", 12);

    CU_ASSERT(uri.port == 61616);

    CU_ASSERT(uri.path.length == 6);
    CU_ASSERT_NSTRING_EQUAL(uri.path.s, "/%2F//", 6);

    CU_ASSERT(uri.query.length == 11);
    CU_ASSERT_NSTRING_EQUAL(uri.query.s, "%2F%2F&?%26", 11);

    /* check path segments */
    result = coap_split_path(uri.path.s, uri.path.length, buf, &buflen);
    CU_ASSERT(result == 4);
    CU_ASSERT(buflen == sizeof(uricheckbuf));
    CU_ASSERT_NSTRING_EQUAL(buf, uricheckbuf, buflen);

    /* check query segments */
    buflen = sizeof(buf);
    result = coap_split_query(uri.query.s, uri.query.length, buf, &buflen);
    CU_ASSERT(result == 2);
    CU_ASSERT(buflen == sizeof(querycheckbuf));
    CU_ASSERT_NSTRING_EQUAL(buf, querycheckbuf, buflen);
  } else {
    CU_FAIL("uri parser error");
  }
}

static void
t_parse_uri13(void) {
  uint8_t teststr[] __attribute__ ((aligned (8))) = { 
    0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 'f',  'o',
    'o',  0x3b, '.',  'w',  'e',  'l',  'l',  '-',  
    'k',  'n',  'o',  'w',  'n',  0x04,  'c', 'o',  
    'r',  'e'
  };

  coap_pdu_t pdu = { 
    .max_size = sizeof(teststr),
    .hdr = (coap_hdr_t *)teststr,
    .length = sizeof(teststr)
  };

  coap_key_t key;

  coap_hash_request_uri(&pdu, key);
  
  CU_ASSERT(sizeof(key) == sizeof(COAP_DEFAULT_WKC_HASHKEY) - 1);
  CU_ASSERT_NSTRING_EQUAL(key, COAP_DEFAULT_WKC_HASHKEY, sizeof(key));
}

static void
t_parse_uri14(void) {
  char teststr[] =
    "longerthan13lessthan270=0123456789012345678901234567890123456789";
  int result;

  /* buf is large enough to hold sizeof(teststr) - 1 bytes content and
   * 2 bytes for the option header. */
  unsigned char buf[sizeof(teststr) + 1];
  size_t buflen = sizeof(buf);

  result = coap_split_query((unsigned char *)teststr, strlen(teststr),
                            buf, &buflen);
  if (result >= 0) {
    CU_ASSERT(buf[0] == 0x0d);
    CU_ASSERT(buf[1] == strlen(teststr) - 13);

    CU_ASSERT_NSTRING_EQUAL(buf+2, teststr, strlen(teststr));
  } else {
    CU_FAIL("uri parser error");
  }
}

static void
t_parse_uri15(void) {
  char teststr[] =
    "longerthan13lessthan270=0123456789012345678901234567890123456789";
  int result;

  /* buf is too small to hold sizeof(teststr) - 1 bytes content and 2
   * bytes for the option header. */
  unsigned char buf[sizeof(teststr) - 1];
  size_t buflen = sizeof(buf);

  result = coap_split_query((unsigned char *)teststr, strlen(teststr),
                            buf, &buflen);
  CU_ASSERT(result == 0);
}

static void
t_parse_uri16(void) {
  char teststr[] =
    "longerthan13lessthan270=0123456789012345678901234567890123456789";
  int result;

  /* buf is too small to hold the option header. */
  unsigned char buf[1];
  size_t buflen = sizeof(buf);

  result = coap_split_query((unsigned char *)teststr, strlen(teststr),
                            buf, &buflen);
  CU_ASSERT(result == 0);
}

static void
t_parse_uri17(void) {
  char teststr[] =
    "thisislongerthan269="
    "01234567890123456789012345678901234567890123456789"
    "01234567890123456789012345678901234567890123456789"
    "01234567890123456789012345678901234567890123456789"
    "01234567890123456789012345678901234567890123456789"
    "01234567890123456789012345678901234567890123456789";
  int result;

  /* buf is large enough to hold sizeof(teststr) - 1 bytes content and
   * 3 bytes for the option header. */
  unsigned char buf[sizeof(teststr) + 2];
  size_t buflen = sizeof(buf);

  result = coap_split_query((unsigned char *)teststr, strlen(teststr),
                            buf, &buflen);
  if (result >= 0) {
    CU_ASSERT(buf[0] == 0x0e);
    CU_ASSERT(buf[1] == (((strlen(teststr) - 269) >> 8) & 0xff));
    CU_ASSERT(buf[2] == ((strlen(teststr) - 269) & 0xff));

    CU_ASSERT_NSTRING_EQUAL(buf+3, teststr, strlen(teststr));
  } else {
    CU_FAIL("uri parser error");
  }
}

CU_pSuite
t_init_uri_tests(void) {
  CU_pSuite suite;

  suite = CU_add_suite("uri parser", NULL, NULL);
  if (!suite) {			/* signal error */
    fprintf(stderr, "W: cannot add uri parser test suite (%s)\n", 
	    CU_get_error_msg());

    return NULL;
  }

#define URI_TEST(s,t)						      \
  if (!CU_ADD_TEST(s,t)) {					      \
    fprintf(stderr, "W: cannot add uri parser test (%s)\n",	      \
	    CU_get_error_msg());				      \
  }

  URI_TEST(suite, t_parse_uri1);
  URI_TEST(suite, t_parse_uri2);
  URI_TEST(suite, t_parse_uri3);
  URI_TEST(suite, t_parse_uri4);
  URI_TEST(suite, t_parse_uri5);
  URI_TEST(suite, t_parse_uri6);
  URI_TEST(suite, t_parse_uri7);
  URI_TEST(suite, t_parse_uri8);
  URI_TEST(suite, t_parse_uri9);
  URI_TEST(suite, t_parse_uri10);
  URI_TEST(suite, t_parse_uri11);
  URI_TEST(suite, t_parse_uri12);
  URI_TEST(suite, t_parse_uri13);
  URI_TEST(suite, t_parse_uri14);
  URI_TEST(suite, t_parse_uri15);
  URI_TEST(suite, t_parse_uri16);
  URI_TEST(suite, t_parse_uri17);

  return suite;
}

