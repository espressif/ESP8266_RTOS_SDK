/* tiny -- tiny sender
 *
 * Copyright (C) 2010,2011 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use. 
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "../coap.h"

static coap_tid_t id;

coap_pdu_t *
make_pdu( unsigned int value ) {
  coap_pdu_t *pdu;
  unsigned char enc;
  static unsigned char buf[20];
  int len, ls;

  if ( ! ( pdu = coap_new_pdu() ) )
    return NULL;

  pdu->hdr->type = COAP_MESSAGE_NON;
  pdu->hdr->code = COAP_REQUEST_POST;
  pdu->hdr->id = htons(id++);

  enc = COAP_PSEUDOFP_ENCODE_8_4_DOWN(value,ls);
  coap_add_data( pdu, 1, &enc);

  len = sprintf((char *)buf, "%u", COAP_PSEUDOFP_DECODE_8_4(enc));
  if ( len > 0 ) {
    coap_add_data( pdu, len, buf );
  }

  return pdu;
}

void
usage( const char *program ) {
  const char *p;

  p = strrchr( program, '/' );
  if ( p )
    program = ++p;

  fprintf( stderr, "%s -- tiny fake sensor\n"
	   "(c) 2010 Olaf Bergmann <bergmann@tzi.org>\n\n"
	   "usage: %s [group address]\n"
	   "\n\nSends some fake sensor values to specified multicast group\n",
	   program, program );
}

coap_context_t *
get_context(const char *node, const char *port) {
  coap_context_t *ctx = NULL;  
  int s;
  struct addrinfo hints;
  struct addrinfo *result, *rp;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM; /* Coap uses UDP */
  hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV | AI_ALL;
  
  s = getaddrinfo(node, port, &hints, &result);
  if ( s != 0 ) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    return NULL;
  } 

  /* iterate through results until success */
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    ctx = coap_new_context(rp->ai_addr, rp->ai_addrlen);
    if (ctx) {
      /* TODO: output address:port for successful binding */
      goto finish;
    }
  }
  
  fprintf(stderr, "no context available for interface '%s'\n", node);

 finish:
  freeaddrinfo(result);
  return ctx;
}

int
main(int argc, char **argv) {
  coap_context_t  *ctx;
  struct timeval tv;
  coap_pdu_t  *pdu;
  struct sockaddr_in6 dst;
  int hops = 16;

  if ( argc > 1 && strncmp(argv[1], "-h", 2) == 0 ) {
    usage( argv[0] );
    exit( 1 );
  }

  ctx = get_context("::", NULL);
  if ( !ctx )
    return -1;

  id = rand() & INT_MAX;

  memset(&dst, 0, sizeof(struct sockaddr_in6 ));
  dst.sin6_family = AF_INET6;
  inet_pton( AF_INET6, argc > 1 ? argv[1] : "::1", &dst.sin6_addr );
  dst.sin6_port = htons( COAP_DEFAULT_PORT );

  if ( IN6_IS_ADDR_MULTICAST(&dst.sin6_addr) ) {
    /* set socket options for multicast */

    if ( setsockopt( ctx->sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
		     (char *)&hops, sizeof(hops) ) < 0 )
      perror("setsockopt: IPV6_MULTICAST_HOPS");

  }

  while ( 1 ) {

    if (! (pdu = make_pdu( rand() & 0xfff ) ) )
      return -1;

    coap_send( ctx, (struct sockaddr *)&dst, sizeof(dst), pdu );
    coap_delete_pdu(pdu);

    tv.tv_sec = 5; tv.tv_usec = 0;

    select( 0, 0, 0, 0, &tv );

  }

  coap_free_context( ctx );

  return 0;
}
