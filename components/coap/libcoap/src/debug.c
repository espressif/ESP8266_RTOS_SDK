/* debug.c -- debug utilities
 *
 * Copyright (C) 2010--2012,2014--2015 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use. 
 */

#include "coap_config.h"

#if defined(HAVE_STRNLEN) && defined(__GNUC__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE 1
#endif

#if defined(HAVE_ASSERT_H) && !defined(assert)
# include <assert.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "block.h"
#include "debug.h"
#include "encode.h"
#include "net.h"

#ifdef WITH_LWIP
# define fprintf(fd, ...) LWIP_PLATFORM_DIAG((__VA_ARGS__))
# define fflush(...)
#endif

#ifdef WITH_CONTIKI
# ifndef DEBUG
#  define DEBUG DEBUG_PRINT
# endif /* DEBUG */
#include "net/ip/uip-debug.h"
#endif

static coap_log_t maxlog = LOG_WARNING;	/* default maximum log level */

const char *coap_package_name(void) {
  return PACKAGE_NAME;
}

const char *coap_package_version(void) {
  return PACKAGE_STRING;
}

coap_log_t 
coap_get_log_level(void) {
  return maxlog;
}

void
coap_set_log_level(coap_log_t level) {
  maxlog = level;
}

/* this array has the same order as the type log_t */
static char *loglevels[] = {
  "EMRG", "ALRT", "CRIT", "ERR", "WARN", "NOTE", "INFO", "DEBG" 
};

#ifdef HAVE_TIME_H

static inline size_t
print_timestamp(char *s, size_t len, coap_tick_t t) {
  struct tm *tmp;
  time_t now = coap_ticks_to_rt(t);
  tmp = localtime(&now);
  return strftime(s, len, "%b %d %H:%M:%S", tmp);
}

#else /* alternative implementation: just print the timestamp */

static inline size_t
print_timestamp(char *s, size_t len, coap_tick_t t) {
#ifdef HAVE_SNPRINTF
  return snprintf(s, len, "%u.%03u", 
		  (unsigned int)coap_ticks_to_rt(t),
		  (unsigned int)(t % COAP_TICKS_PER_SECOND));
#else /* HAVE_SNPRINTF */
  /* @todo do manual conversion of timestamp */
  return 0;
#endif /* HAVE_SNPRINTF */
}

#endif /* HAVE_TIME_H */

#ifndef NDEBUG

#ifndef HAVE_STRNLEN
/** 
 * A length-safe strlen() fake. 
 * 
 * @param s      The string to count characters != 0.
 * @param maxlen The maximum length of @p s.
 * 
 * @return The length of @p s.
 */
static inline size_t
strnlen(const char *s, size_t maxlen) {
  size_t n = 0;
  while(*s++ && n < maxlen)
    ++n;
  return n;
}
#endif /* HAVE_STRNLEN */

static unsigned int
print_readable( const unsigned char *data, unsigned int len,
		unsigned char *result, unsigned int buflen, int encode_always ) {
  const unsigned char hex[] = "0123456789ABCDEF";
  unsigned int cnt = 0;
  assert(data || len == 0);

  if (buflen == 0) { /* there is nothing we can do here but return */
    return 0;
  }

  while (len) {
    if (!encode_always && isprint(*data)) {
      if (cnt+1 < buflen) { /* keep one byte for terminating zero */
      *result++ = *data;
      ++cnt;
      } else {
	break;
      }
    } else {
      if (cnt+4 < buflen) { /* keep one byte for terminating zero */
	*result++ = '\\';
	*result++ = 'x';
	*result++ = hex[(*data & 0xf0) >> 4];
	*result++ = hex[*data & 0x0f];
	cnt += 4;
      } else
	break;
    }

    ++data; --len;
  }

  *result = '\0'; /* add a terminating zero */
  return cnt;
}

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

size_t
coap_print_addr(const struct coap_address_t *addr, unsigned char *buf, size_t len) {
#ifdef HAVE_ARPA_INET_H
  const void *addrptr = NULL;
  in_port_t port;
  unsigned char *p = buf;

  switch (addr->addr.sa.sa_family) {
  case AF_INET: 
    addrptr = &addr->addr.sin.sin_addr;
    port = ntohs(addr->addr.sin.sin_port);
    break;
#if COAP_IPV6
  case AF_INET6:
    if (len < 7) /* do not proceed if buffer is even too short for [::]:0 */
      return 0;

    *p++ = '[';

    addrptr = &addr->addr.sin6.sin6_addr;
    port = ntohs(addr->addr.sin6.sin6_port);

    break;
#endif
  default:
    memcpy(buf, "(unknown address type)", min(22, len));
    return min(22, len);
  }

  if (inet_ntop(addr->addr.sa.sa_family, addrptr, (char *)p, len) == 0) {
    perror("coap_print_addr");
    return 0;
  }

  p += strnlen((char *)p, len);

  if (addr->addr.sa.sa_family == AF_INET6) {
    if (p < buf + len) {
      *p++ = ']';
    } else 
      return 0;
  }

  p += snprintf((char *)p, buf + len - p + 1, ":%d", port);

  return buf + len - p;
#else /* HAVE_ARPA_INET_H */
# if WITH_CONTIKI
  unsigned char *p = buf;
  uint8_t i;
#  if NETSTACK_CONF_WITH_IPV6
  const unsigned char hex[] = "0123456789ABCDEF";

  if (len < 41)
    return 0;

  *p++ = '[';

  for (i=0; i < 16; i += 2) {
    if (i) {
      *p++ = ':';
    }
    *p++ = hex[(addr->addr.u8[i] & 0xf0) >> 4];
    *p++ = hex[(addr->addr.u8[i] & 0x0f)];
    *p++ = hex[(addr->addr.u8[i+1] & 0xf0) >> 4];
    *p++ = hex[(addr->addr.u8[i+1] & 0x0f)];
  }
  *p++ = ']';
#  else /* WITH_UIP6 */
#   warning "IPv4 network addresses will not be included in debug output"

  if (len < 21)
    return 0;
#  endif /* WITH_UIP6 */
  if (buf + len - p < 6)
    return 0;

#ifdef HAVE_SNPRINTF
  p += snprintf((char *)p, buf + len - p + 1, ":%d", uip_htons(addr->port));
#else /* HAVE_SNPRINTF */
  /* @todo manual conversion of port number */
#endif /* HAVE_SNPRINTF */

  return p - buf;
# else /* WITH_CONTIKI */
  /* TODO: output addresses manually */
#   warning "inet_ntop() not available, network addresses will not be included in debug output"
# endif /* WITH_CONTIKI */
  return 0;
#endif
}

#ifdef WITH_CONTIKI
# define fprintf(fd, ...) PRINTF(__VA_ARGS__)
# define fflush(...)

# ifdef HAVE_VPRINTF
#  define vfprintf(fd, ...) vprintf(__VA_ARGS__)
# else /* HAVE_VPRINTF */
#  define vfprintf(fd, ...) PRINTF(__VA_ARGS__)
# endif /* HAVE_VPRINTF */
#endif /* WITH_CONTIKI */

/** Returns a textual description of the message type @p t. */
static const char *
msg_type_string(uint8_t t) {
  static char *types[] = { "CON", "NON", "ACK", "RST", "???" };

  return types[min(t, sizeof(types)/sizeof(char *) - 1)];
}

/** Returns a textual description of the method or response code. */
static const char *
msg_code_string(uint8_t c) {
  static char *methods[] = { "0.00", "GET", "POST", "PUT", "DELETE", "PATCH" };
  static char buf[5];

  if (c < sizeof(methods)/sizeof(char *)) {
    return methods[c];
  } else {
    snprintf(buf, sizeof(buf), "%u.%02u", c >> 5, c & 0x1f);
    return buf;
  }
}

/** Returns a textual description of the option name. */
static const char *
msg_option_string(uint16_t option_type) {
  struct option_desc_t {
    uint16_t type;
    const char *name;
  };

  static struct option_desc_t options[] = {
    { COAP_OPTION_IF_MATCH, "If-Match" },
    { COAP_OPTION_URI_HOST, "Uri-Host" },
    { COAP_OPTION_ETAG, "ETag" },
    { COAP_OPTION_IF_NONE_MATCH, "If-None-Match" },
    { COAP_OPTION_OBSERVE, "Observe" },
    { COAP_OPTION_URI_PORT, "Uri-Port" },
    { COAP_OPTION_LOCATION_PATH, "Location-Path" },
    { COAP_OPTION_URI_PATH, "Uri-Path" },
    { COAP_OPTION_CONTENT_FORMAT, "Content-Format" },
    { COAP_OPTION_MAXAGE, "Max-Age" },
    { COAP_OPTION_URI_QUERY, "Uri-Query" },
    { COAP_OPTION_ACCEPT, "Accept" },
    { COAP_OPTION_LOCATION_QUERY, "Location-Query" },
    { COAP_OPTION_BLOCK2, "Block2" },
    { COAP_OPTION_BLOCK1, "Block1" },
    { COAP_OPTION_PROXY_URI, "Proxy-Uri" },
    { COAP_OPTION_PROXY_SCHEME, "Proxy-Scheme" },
    { COAP_OPTION_SIZE1, "Size1" },
    { COAP_OPTION_NORESPONSE, "No-Response" }
  };

  static char buf[6];
  size_t i;

  /* search option_type in list of known options */
  for (i = 0; i < sizeof(options)/sizeof(struct option_desc_t); i++) {
    if (option_type == options[i].type) {
      return options[i].name;
    }
  }

  /* unknown option type, just print to buf */
  snprintf(buf, sizeof(buf), "%u", option_type);
  return buf;
}

static unsigned int
print_content_format(unsigned int format_type,
		     unsigned char *result, unsigned int buflen) {
  struct desc_t {
    unsigned int type;
    const char *name;
  };

  static struct desc_t formats[] = {
    { COAP_MEDIATYPE_TEXT_PLAIN, "text/plain" },
    { COAP_MEDIATYPE_APPLICATION_LINK_FORMAT, "application/link-format" },
    { COAP_MEDIATYPE_APPLICATION_XML, "application/xml" },
    { COAP_MEDIATYPE_APPLICATION_OCTET_STREAM, "application/octet-stream" },
    { COAP_MEDIATYPE_APPLICATION_EXI, "application/exi" },
    { COAP_MEDIATYPE_APPLICATION_JSON, "application/json" },
    { COAP_MEDIATYPE_APPLICATION_CBOR, "application/cbor" }
  };

  size_t i;

  /* search format_type in list of known content formats */
  for (i = 0; i < sizeof(formats)/sizeof(struct desc_t); i++) {
    if (format_type == formats[i].type) {
      return snprintf((char *)result, buflen, "%s", formats[i].name);
    }
  }

  /* unknown content format, just print numeric value to buf */
  return snprintf((char *)result, buflen, "%d", format_type);
}

/**
 * Returns 1 if the given @p content_format is either unknown or known
 * to carry binary data. The return value @c 0 hence indicates
 * printable data which is also assumed if @p content_format is @c 01.
 */
static inline int
is_binary(int content_format) {
  return !(content_format == -1 ||
	   content_format == COAP_MEDIATYPE_TEXT_PLAIN ||
	   content_format == COAP_MEDIATYPE_APPLICATION_LINK_FORMAT ||
	   content_format == COAP_MEDIATYPE_APPLICATION_XML ||
	   content_format == COAP_MEDIATYPE_APPLICATION_JSON);
}

void
coap_show_pdu(const coap_pdu_t *pdu) {
  unsigned char buf[COAP_MAX_PDU_SIZE]; /* need some space for output creation */
  size_t buf_len = 0; /* takes the number of bytes written to buf */
  int encode = 0, have_options = 0, i;
  coap_opt_iterator_t opt_iter;
  coap_opt_t *option;
  int content_format = -1;
  size_t data_len;
  unsigned char *data;

  fprintf(COAP_DEBUG_FD, "v:%d t:%s c:%s i:%04x {",
	  pdu->hdr->version, msg_type_string(pdu->hdr->type),
	  msg_code_string(pdu->hdr->code), ntohs(pdu->hdr->id));

  for (i = 0; i < pdu->hdr->token_length; i++) {
    fprintf(COAP_DEBUG_FD, "%02x", pdu->hdr->token[i]);
  }
  fprintf(COAP_DEBUG_FD, "}");

  /* show options, if any */
  coap_option_iterator_init((coap_pdu_t *)pdu, &opt_iter, COAP_OPT_ALL);

  fprintf(COAP_DEBUG_FD, " [");
  while ((option = coap_option_next(&opt_iter))) {
    if (!have_options) {
      have_options = 1;
    } else {
      fprintf(COAP_DEBUG_FD, ",");
    }

    switch (opt_iter.type) {
    case COAP_OPTION_CONTENT_FORMAT:
      content_format = (int)coap_decode_var_bytes(COAP_OPT_VALUE(option),
						  COAP_OPT_LENGTH(option));

      buf_len = print_content_format(content_format, buf, sizeof(buf));
      break;

    case COAP_OPTION_BLOCK1:
    case COAP_OPTION_BLOCK2:
      /* split block option into number/more/size where more is the
       * letter M if set, the _ otherwise */
      buf_len = snprintf((char *)buf, sizeof(buf), "%u/%c/%u",
			 coap_opt_block_num(option), /* block number */
			 COAP_OPT_BLOCK_MORE(option) ? 'M' : '_', /* M bit */
			 (1 << (COAP_OPT_BLOCK_SZX(option) + 4))); /* block size */

      break;

    case COAP_OPTION_URI_PORT:
    case COAP_OPTION_MAXAGE:
    case COAP_OPTION_OBSERVE:
    case COAP_OPTION_SIZE1:
      /* show values as unsigned decimal value */
      buf_len = snprintf((char *)buf, sizeof(buf), "%u",
			 coap_decode_var_bytes(COAP_OPT_VALUE(option),
					       COAP_OPT_LENGTH(option)));
      break;

    default:
      /* generic output function for all other option types */
      if (opt_iter.type == COAP_OPTION_URI_PATH ||
	  opt_iter.type == COAP_OPTION_PROXY_URI ||
	  opt_iter.type == COAP_OPTION_URI_HOST ||
	  opt_iter.type == COAP_OPTION_LOCATION_PATH ||
	  opt_iter.type == COAP_OPTION_LOCATION_QUERY ||
	  opt_iter.type == COAP_OPTION_URI_QUERY) {
	encode = 0;
      } else {
	encode = 1;
      }

      buf_len = print_readable(COAP_OPT_VALUE(option),
			       COAP_OPT_LENGTH(option),
			       buf, sizeof(buf), encode);
    }

    fprintf(COAP_DEBUG_FD, " %s:%.*s", msg_option_string(opt_iter.type),
	    (int)buf_len, buf);
  }

  fprintf(COAP_DEBUG_FD, " ]");
  
  if (coap_get_data((coap_pdu_t *)pdu, &data_len, &data)) {

    fprintf(COAP_DEBUG_FD, " :: ");

    if (is_binary(content_format)) {
      fprintf(COAP_DEBUG_FD, "<<");
      while (data_len--) {
	fprintf(COAP_DEBUG_FD, "%02x", *data++);
      }
      fprintf(COAP_DEBUG_FD, ">>");
    } else {
      if (print_readable(data, data_len, buf, sizeof(buf), 0)) {
	fprintf(COAP_DEBUG_FD, "'%s'", buf);
      }
    }
  }

  fprintf(COAP_DEBUG_FD, "\n");
  fflush(COAP_DEBUG_FD);
}


#endif /* NDEBUG */

void 
coap_log_impl(coap_log_t level, const char *format, ...) {
  char timebuf[32];
  coap_tick_t now;
  va_list ap;
  FILE *log_fd;

  if (maxlog < level)
    return;
  
  log_fd = level <= LOG_CRIT ? COAP_ERR_FD : COAP_DEBUG_FD;

  coap_ticks(&now);
  if (print_timestamp(timebuf,sizeof(timebuf), now))
    fprintf(log_fd, "%s ", timebuf);

  if (level <= LOG_DEBUG)
    fprintf(log_fd, "%s ", loglevels[level]);

  va_start(ap, format);
  vfprintf(log_fd, format, ap);
  va_end(ap);
  fflush(log_fd);
}

