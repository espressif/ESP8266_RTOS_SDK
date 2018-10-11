/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 * -*- */

/* coap -- simple implementation of the Constrained Application Protocol (CoAP)
 *         as defined in RFC 7252
 *
 * Copyright (C) 2010--2015 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see README for terms of
 * use.
 */


/**
 * @file rd.c
 * @brief CoRE resource directory
 *
 * @see http://tools.ietf.org/id/draft-shelby-core-resource-directory
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>

#include "coap_config.h"
#include "utlist.h"
#include "resource.h"
#include "coap.h"

#define COAP_RESOURCE_CHECK_TIME 2

#define RD_ROOT_STR   ((unsigned char *)"rd")
#define RD_ROOT_SIZE  2

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

typedef struct rd_t {
  UT_hash_handle hh;      /**< hash handle (for internal use only) */
  coap_key_t key;         /**< the actual key bytes for this resource */

  size_t etag_len;        /**< actual length of @c etag */
  unsigned char etag[8];  /**< ETag for current description */

  str data;               /**< points to the resource description  */
} rd_t;

rd_t *resources = NULL;

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__ ((unused))
#else /* not a GCC */
#define UNUSED_PARAM
#endif /* GCC */

static inline rd_t *
rd_new(void) {
  rd_t *rd;
  rd = (rd_t *)coap_malloc(sizeof(rd_t));
  if (rd)
    memset(rd, 0, sizeof(rd_t));

  return rd;
}

static inline void
rd_delete(rd_t *rd) {
  if (rd) {
    coap_free(rd->data.s);
    coap_free(rd);
  }
}

/* temporary storage for dynamic resource representations */
static int quit = 0;

/* SIGINT handler: set quit to 1 for graceful termination */
static void
handle_sigint(int signum UNUSED_PARAM) {
  quit = 1;
}

static void
hnd_get_resource(coap_context_t  *ctx UNUSED_PARAM,
                 struct coap_resource_t *resource,
                 const coap_endpoint_t *local_interface UNUSED_PARAM,
                 coap_address_t *peer UNUSED_PARAM,
                 coap_pdu_t *request UNUSED_PARAM,
                 str *token UNUSED_PARAM,
                 coap_pdu_t *response) {
  rd_t *rd = NULL;
  unsigned char buf[3];

  HASH_FIND(hh, resources, resource->key, sizeof(coap_key_t), rd);

  response->hdr->code = COAP_RESPONSE_CODE(205);

  coap_add_option(response,
                  COAP_OPTION_CONTENT_TYPE,
                  coap_encode_var_bytes(buf,
                                        COAP_MEDIATYPE_APPLICATION_LINK_FORMAT),
                                        buf);

  if (rd && rd->etag_len)
    coap_add_option(response, COAP_OPTION_ETAG, rd->etag_len, rd->etag);

  if (rd && rd->data.s)
    coap_add_data(response, rd->data.length, rd->data.s);
}

static void
hnd_put_resource(coap_context_t  *ctx UNUSED_PARAM,
                 struct coap_resource_t *resource UNUSED_PARAM,
                 const coap_endpoint_t *local_interface UNUSED_PARAM,
                 coap_address_t *peer UNUSED_PARAM,
                 coap_pdu_t *request UNUSED_PARAM,
                 str *token UNUSED_PARAM,
                 coap_pdu_t *response) {
#if 1
  response->hdr->code = COAP_RESPONSE_CODE(501);
#else /* FIXME */
  coap_opt_iterator_t opt_iter;
  coap_opt_t *token, *etag;
  coap_pdu_t *response;
  size_t size = sizeof(coap_hdr_t);
  int type = (request->hdr->type == COAP_MESSAGE_CON)
    ? COAP_MESSAGE_ACK : COAP_MESSAGE_NON;
  rd_t *rd = NULL;
  unsigned char code;     /* result code */
  unsigned char *data;
  str tmp;

  HASH_FIND(hh, resources, resource->key, sizeof(coap_key_t), rd);
  if (rd) {
    /* found resource object, now check Etag */
    etag = coap_check_option(request, COAP_OPTION_ETAG, &opt_iter);
    if (!etag || (COAP_OPT_LENGTH(etag) != rd->etag_len)
        || memcmp(COAP_OPT_VALUE(etag), rd->etag, rd->etag_len) != 0) {

      if (coap_get_data(request, &tmp.length, &data)) {

        tmp.s = (unsigned char *)coap_malloc(tmp.length);
        if (!tmp.s) {
          debug("hnd_put_rd: cannot allocate storage for new rd\n");
          code = COAP_RESPONSE_CODE(503);
          goto finish;
        }

        coap_free(rd->data.s);
        rd->data.s = tmp.s;
        rd->data.length = tmp.length;
        memcpy(rd->data.s, data, rd->data.length);
      }
    }

    if (etag) {
      rd->etag_len = min(COAP_OPT_LENGTH(etag), sizeof(rd->etag));
      memcpy(rd->etag, COAP_OPT_VALUE(etag), rd->etag_len);
    }

    code = COAP_RESPONSE_CODE(204);
    /* FIXME: update lifetime */

    } else {

    code = COAP_RESPONSE_CODE(503);
  }

  finish:
  /* FIXME: do not create a new response but use the old one instead */
  response = coap_pdu_init(type, code, request->hdr->id, size);

  if (!response) {
    debug("cannot create response for message %d\n", request->hdr->id);
    return;
  }

  if (request->hdr->token_length)
    coap_add_token(response, request->hdr->token_length, request->hdr->token);

  if (coap_send(ctx, peer, response) == COAP_INVALID_TID) {
    debug("hnd_get_rd: cannot send response for message %d\n",
    request->hdr->id);
  }
  coap_delete_pdu(response);
#endif
}

static void
hnd_delete_resource(coap_context_t  *ctx,
                    struct coap_resource_t *resource,
                    const coap_endpoint_t *local_interface UNUSED_PARAM,
                    coap_address_t *peer UNUSED_PARAM,
                    coap_pdu_t *request UNUSED_PARAM,
                    str *token UNUSED_PARAM,
                    coap_pdu_t *response) {
  rd_t *rd = NULL;

  HASH_FIND(hh, resources, resource->key, sizeof(coap_key_t), rd);
  if (rd) {
    HASH_DELETE(hh, resources, rd);
    rd_delete(rd);
  }
  /* FIXME: link attributes for resource have been created dynamically
   * using coap_malloc() and must be released. */
  coap_delete_resource(ctx, resource->key);

  response->hdr->code = COAP_RESPONSE_CODE(202);
}

static void
hnd_get_rd(coap_context_t  *ctx UNUSED_PARAM,
           struct coap_resource_t *resource UNUSED_PARAM,
           const coap_endpoint_t *local_interface UNUSED_PARAM,
           coap_address_t *peer UNUSED_PARAM,
           coap_pdu_t *request UNUSED_PARAM,
           str *token UNUSED_PARAM,
           coap_pdu_t *response) {
  unsigned char buf[3];

  response->hdr->code = COAP_RESPONSE_CODE(205);

  coap_add_option(response,
                  COAP_OPTION_CONTENT_TYPE,
                  coap_encode_var_bytes(buf,
                                        COAP_MEDIATYPE_APPLICATION_LINK_FORMAT),
                                        buf);

  coap_add_option(response,
                  COAP_OPTION_MAXAGE,
                  coap_encode_var_bytes(buf, 0x2ffff), buf);
}

static int
parse_param(unsigned char *search,
            size_t search_len,
            unsigned char *data,
            size_t data_len,
            str *result) {

  if (result)
    memset(result, 0, sizeof(str));

  if (!search_len)
    return 0;

  while (search_len <= data_len) {

    /* handle parameter if found */
    if (memcmp(search, data, search_len) == 0) {
      data += search_len;
      data_len -= search_len;

      /* key is only valid if we are at end of string or delimiter follows */
      if (!data_len || *data == '=' || *data == '&') {
        while (data_len && *data != '=') {
          ++data; --data_len;
        }

        if (data_len > 1 && result) {
          /* value begins after '=' */

          result->s = ++data;
          while (--data_len && *data != '&') {
            ++data; result->length++;
          }
        }

        return 1;
      }
    }

    /* otherwise proceed to next */
    while (--data_len && *data++ != '&')
      ;
  }

  return 0;
}

static void
add_source_address(struct coap_resource_t *resource,
                   coap_address_t *peer) {
#define BUFSIZE 64
  char *buf;
  size_t n = 1;

  buf = (char *)coap_malloc(BUFSIZE);
  if (!buf)
    return;

  buf[0] = '"';

  switch(peer->addr.sa.sa_family) {

  case AF_INET:
    /* FIXME */
    break;

  case AF_INET6:
    n += snprintf(buf + n, BUFSIZE - n,
      "[%02x%02x:%02x%02x:%02x%02x:%02x%02x" \
      ":%02x%02x:%02x%02x:%02x%02x:%02x%02x]",
      peer->addr.sin6.sin6_addr.s6_addr[0],
      peer->addr.sin6.sin6_addr.s6_addr[1],
      peer->addr.sin6.sin6_addr.s6_addr[2],
      peer->addr.sin6.sin6_addr.s6_addr[3],
      peer->addr.sin6.sin6_addr.s6_addr[4],
      peer->addr.sin6.sin6_addr.s6_addr[5],
      peer->addr.sin6.sin6_addr.s6_addr[6],
      peer->addr.sin6.sin6_addr.s6_addr[7],
      peer->addr.sin6.sin6_addr.s6_addr[8],
      peer->addr.sin6.sin6_addr.s6_addr[9],
      peer->addr.sin6.sin6_addr.s6_addr[10],
      peer->addr.sin6.sin6_addr.s6_addr[11],
      peer->addr.sin6.sin6_addr.s6_addr[12],
      peer->addr.sin6.sin6_addr.s6_addr[13],
      peer->addr.sin6.sin6_addr.s6_addr[14],
      peer->addr.sin6.sin6_addr.s6_addr[15]);

    if (peer->addr.sin6.sin6_port != htons(COAP_DEFAULT_PORT)) {
      n +=
      snprintf(buf + n, BUFSIZE - n, ":%d", peer->addr.sin6.sin6_port);
    }
    break;
    default:
    ;
  }

  if (n < BUFSIZE)
    buf[n++] = '"';

  coap_add_attr(resource,
                (unsigned char *)"A",
                1,
                (unsigned char *)buf,
                n,
                COAP_ATTR_FLAGS_RELEASE_VALUE);
#undef BUFSIZE
}

static rd_t *
make_rd(coap_address_t *peer UNUSED_PARAM, coap_pdu_t *pdu) {
  rd_t *rd;
  unsigned char *data;
  coap_opt_iterator_t opt_iter;
  coap_opt_t *etag;

  rd = rd_new();

  if (!rd) {
    debug("hnd_get_rd: cannot allocate storage for rd\n");
    return NULL;
  }

  if (coap_get_data(pdu, &rd->data.length, &data)) {
    rd->data.s = (unsigned char *)coap_malloc(rd->data.length);
    if (!rd->data.s) {
      debug("hnd_get_rd: cannot allocate storage for rd->data\n");
      rd_delete(rd);
      return NULL;
    }
    memcpy(rd->data.s, data, rd->data.length);
  }

  etag = coap_check_option(pdu, COAP_OPTION_ETAG, &opt_iter);
  if (etag) {
    rd->etag_len = min(COAP_OPT_LENGTH(etag), sizeof(rd->etag));
    memcpy(rd->etag, COAP_OPT_VALUE(etag), rd->etag_len);
  }

  return rd;
}

static void
hnd_post_rd(coap_context_t  *ctx,
            struct coap_resource_t *resource UNUSED_PARAM,
            const coap_endpoint_t *local_interface UNUSED_PARAM,
            coap_address_t *peer,
            coap_pdu_t *request,
            str *token UNUSED_PARAM,
            coap_pdu_t *response) {
  coap_resource_t *r;
  coap_opt_iterator_t opt_iter;
  coap_opt_t *query;
#define LOCSIZE 68
  unsigned char *loc;
  size_t loc_size;
  str h = {0, NULL}, ins = {0, NULL}, rt = {0, NULL}, lt = {0, NULL}; /* store query parameters */
  unsigned char *buf;

  loc = (unsigned char *)coap_malloc(LOCSIZE);
  if (!loc) {
    response->hdr->code = COAP_RESPONSE_CODE(500);
    return;
  }
  memcpy(loc, RD_ROOT_STR, RD_ROOT_SIZE);

  loc_size = RD_ROOT_SIZE;
  loc[loc_size++] = '/';

  /* store query parameters for later use */
  query = coap_check_option(request, COAP_OPTION_URI_QUERY, &opt_iter);
  if (query) {
    parse_param((unsigned char *)"h", 1,
    COAP_OPT_VALUE(query), COAP_OPT_LENGTH(query), &h);
    parse_param((unsigned char *)"ins", 3,
    COAP_OPT_VALUE(query), COAP_OPT_LENGTH(query), &ins);
    parse_param((unsigned char *)"lt", 2,
    COAP_OPT_VALUE(query), COAP_OPT_LENGTH(query), &lt);
    parse_param((unsigned char *)"rt", 2,
    COAP_OPT_VALUE(query), COAP_OPT_LENGTH(query), &rt);
  }

  if (h.length) {   /* client has specified a node name */
    memcpy(loc + loc_size, h.s, min(h.length, LOCSIZE - loc_size - 1));
    loc_size += min(h.length, LOCSIZE - loc_size - 1);

    if (ins.length && loc_size > 1) {
      loc[loc_size++] = '-';
      memcpy((char *)(loc + loc_size),
      ins.s, min(ins.length, LOCSIZE - loc_size - 1));
      loc_size += min(ins.length, LOCSIZE - loc_size - 1);
    }

  } else {      /* generate node identifier */
    loc_size +=
      snprintf((char *)(loc + loc_size), LOCSIZE - loc_size - 1,
               "%x", request->hdr->id);

    if (loc_size > 1) {
      if (ins.length) {
        loc[loc_size++] = '-';
        memcpy((char *)(loc + loc_size),
                ins.s,
                min(ins.length, LOCSIZE - loc_size - 1));
        loc_size += min(ins.length, LOCSIZE - loc_size - 1);
      } else {
        coap_tick_t now;
        coap_ticks(&now);

        loc_size += snprintf((char *)(loc + loc_size),
                             LOCSIZE - loc_size - 1,
                             "-%x",
                             (unsigned int)(now & (unsigned int)-1));
      }
    }
  }

  /* TODO:
   *   - use lt to check expiration
   */

  r = coap_resource_init(loc, loc_size, COAP_RESOURCE_FLAGS_RELEASE_URI);
  coap_register_handler(r, COAP_REQUEST_GET, hnd_get_resource);
  coap_register_handler(r, COAP_REQUEST_PUT, hnd_put_resource);
  coap_register_handler(r, COAP_REQUEST_DELETE, hnd_delete_resource);

  if (ins.s) {
    buf = (unsigned char *)coap_malloc(ins.length + 2);
    if (buf) {
      /* add missing quotes */
      buf[0] = '"';
      memcpy(buf + 1, ins.s, ins.length);
      buf[ins.length + 1] = '"';
      coap_add_attr(r,
                    (unsigned char *)"ins",
                    3,
                    buf,
                    ins.length + 2,
                    COAP_ATTR_FLAGS_RELEASE_VALUE);
    }
  }

  if (rt.s) {
    buf = (unsigned char *)coap_malloc(rt.length + 2);
    if (buf) {
      /* add missing quotes */
      buf[0] = '"';
      memcpy(buf + 1, rt.s, rt.length);
      buf[rt.length + 1] = '"';
      coap_add_attr(r,
                    (unsigned char *)"rt",
                    2,
                    buf,
                    rt.length + 2,COAP_ATTR_FLAGS_RELEASE_VALUE);
    }
  }

  add_source_address(r, peer);

  {
    rd_t *rd;
    rd = make_rd(peer, request);
    if (rd) {
      coap_hash_path(loc, loc_size, rd->key);
      HASH_ADD(hh, resources, key, sizeof(coap_key_t), rd);
    } else {
      /* FIXME: send error response and delete r */
    }
  }

  coap_add_resource(ctx, r);


  /* create response */

  response->hdr->code = COAP_RESPONSE_CODE(201);

  { /* split path into segments and add Location-Path options */
    unsigned char _b[LOCSIZE];
    unsigned char *b = _b;
    size_t buflen = sizeof(_b);
    int nseg;

    nseg = coap_split_path(loc, loc_size, b, &buflen);
    while (nseg--) {
      coap_add_option(response,
                      COAP_OPTION_LOCATION_PATH,
                      COAP_OPT_LENGTH(b),
                      COAP_OPT_VALUE(b));
      b += COAP_OPT_SIZE(b);
    }
  }
}

static void
init_resources(coap_context_t *ctx) {
  coap_resource_t *r;

  r = coap_resource_init(RD_ROOT_STR, RD_ROOT_SIZE, 0);
  coap_register_handler(r, COAP_REQUEST_GET, hnd_get_rd);
  coap_register_handler(r, COAP_REQUEST_POST, hnd_post_rd);

  coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"40", 2, 0);
  coap_add_attr(r, (unsigned char *)"rt", 2, (unsigned char *)"\"core.rd\"", 9, 0);
  coap_add_attr(r, (unsigned char *)"ins", 2, (unsigned char *)"\"default\"", 9, 0);

  coap_add_resource(ctx, r);

}

static void
usage( const char *program, const char *version) {
  const char *p;

  p = strrchr( program, '/' );
  if ( p )
    program = ++p;

  fprintf( stderr, "%s v%s -- CoRE Resource Directory implementation\n"
     "(c) 2011-2012 Olaf Bergmann <bergmann@tzi.org>\n\n"
     "usage: %s [-A address] [-p port]\n\n"
     "\t-A address\tinterface address to bind to\n"
     "\t-p port\t\tlisten on specified port\n"
     "\t-v num\t\tverbosity level (default: 3)\n",
     program, version, program );
}

static coap_context_t *
get_context(const char *node, const char *port) {
  coap_context_t *ctx = NULL;
  int s;
  struct addrinfo hints;
  struct addrinfo *result, *rp;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM; /* Coap uses UDP */
  hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

  s = getaddrinfo(node, port, &hints, &result);
  if ( s != 0 ) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    return NULL;
  }

  /* iterate through results until success */
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    coap_address_t addr;

    if (rp->ai_addrlen <= sizeof(addr.addr)) {
      coap_address_init(&addr);
      addr.size = rp->ai_addrlen;
      memcpy(&addr.addr, rp->ai_addr, rp->ai_addrlen);

      ctx = coap_new_context(&addr);
      if (ctx) {
        /* TODO: output address:port for successful binding */
        goto finish;
      }
    }
  }

  fprintf(stderr, "no context available for interface '%s'\n", node);

 finish:
  freeaddrinfo(result);
  return ctx;
}

static int
join(coap_context_t *ctx, char *group_name) {
  struct ipv6_mreq mreq;
  struct addrinfo   *reslocal = NULL, *resmulti = NULL, hints, *ainfo;
  int result = -1;

  /* we have to resolve the link-local interface to get the interface id */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_DGRAM;

  result = getaddrinfo("::", NULL, &hints, &reslocal);
  if ( result < 0 ) {
    perror("join: cannot resolve link-local interface");
    goto finish;
  }

  /* get the first suitable interface identifier */
  for (ainfo = reslocal; ainfo != NULL; ainfo = ainfo->ai_next) {
    if ( ainfo->ai_family == AF_INET6 ) {
      mreq.ipv6mr_interface =
              ((struct sockaddr_in6 *)ainfo->ai_addr)->sin6_scope_id;
      break;
    }
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_DGRAM;

  /* resolve the multicast group address */
  result = getaddrinfo(group_name, NULL, &hints, &resmulti);

  if ( result < 0 ) {
    perror("join: cannot resolve multicast address");
    goto finish;
  }

  for (ainfo = resmulti; ainfo != NULL; ainfo = ainfo->ai_next) {
    if ( ainfo->ai_family == AF_INET6 ) {
      mreq.ipv6mr_multiaddr =
        ((struct sockaddr_in6 *)ainfo->ai_addr)->sin6_addr;
      break;
    }
  }

  result = setsockopt(ctx->sockfd,
                      IPPROTO_IPV6, IPV6_JOIN_GROUP,
                      (char *)&mreq, sizeof(mreq) );
  if ( result < 0 )
    perror("join: setsockopt");

 finish:
  freeaddrinfo(resmulti);
  freeaddrinfo(reslocal);

  return result;
}

int
main(int argc, char **argv) {
  coap_context_t  *ctx;
  fd_set readfds;
  struct timeval tv, *timeout;
  int result;
  coap_tick_t now;
  coap_queue_t *nextpdu;
  char addr_str[NI_MAXHOST] = "::";
  char port_str[NI_MAXSERV] = "5683";
  char *group = NULL;
  int opt;
  coap_log_t log_level = LOG_WARNING;

  while ((opt = getopt(argc, argv, "A:g:p:v:")) != -1) {
    switch (opt) {
    case 'A' :
      strncpy(addr_str, optarg, NI_MAXHOST-1);
      addr_str[NI_MAXHOST - 1] = '\0';
      break;
    case 'g' :
      group = optarg;
      break;
    case 'p' :
      strncpy(port_str, optarg, NI_MAXSERV-1);
      port_str[NI_MAXSERV - 1] = '\0';
      break;
    case 'v' :
      log_level = strtol(optarg, NULL, 10);
      break;
    default:
      usage( argv[0], PACKAGE_VERSION );
      exit( 1 );
    }
  }

  coap_set_log_level(log_level);

  ctx = get_context(addr_str, port_str);
  if (!ctx)
    return -1;

  if (group)
    join(ctx, group);

  init_resources(ctx);

  signal(SIGINT, handle_sigint);

  while ( !quit ) {
    FD_ZERO(&readfds);
    FD_SET( ctx->sockfd, &readfds );

    nextpdu = coap_peek_next( ctx );

    coap_ticks(&now);
    while ( nextpdu && nextpdu->t <= now ) {
      coap_retransmit( ctx, coap_pop_next( ctx ) );
      nextpdu = coap_peek_next( ctx );
    }

    if ( nextpdu && nextpdu->t <= now + COAP_RESOURCE_CHECK_TIME ) {
      /* set timeout if there is a pdu to send before our automatic
         timeout occurs */
      tv.tv_usec = ((nextpdu->t - now) % COAP_TICKS_PER_SECOND) * 1000000 / COAP_TICKS_PER_SECOND;
      tv.tv_sec = (nextpdu->t - now) / COAP_TICKS_PER_SECOND;
      timeout = &tv;
    } else {
      tv.tv_usec = 0;
      tv.tv_sec = COAP_RESOURCE_CHECK_TIME;
      timeout = &tv;
    }
    result = select( FD_SETSIZE, &readfds, 0, 0, timeout );

    if ( result < 0 ) {     /* error */
      if (errno != EINTR)
        perror("select");
      } else if ( result > 0 ) {  /* read from socket */
        if ( FD_ISSET( ctx->sockfd, &readfds ) ) {
          coap_read( ctx ); /* read received data */
          /* coap_dispatch( ctx );  /\* and dispatch PDUs from receivequeue *\/ */
        }
      } else {            /* timeout */
        /* coap_check_resource_list( ctx ); */
    }
  }

  coap_free_context( ctx );

  return 0;
}
