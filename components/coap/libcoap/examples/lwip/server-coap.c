#include "coap_config.h"
#include <coap.h>

coap_context_t *main_coap_context;

void server_coap_init(void)
{
	coap_address_t listenaddress;

	coap_address_init(&listenaddress);

	/* looks like a server address, but is used as end point for clients too */
	listenaddress.addr = *(IP_ANY_TYPE);
	listenaddress.port = COAP_DEFAULT_PORT;

	main_coap_context = coap_new_context(&listenaddress);

	LWIP_ASSERT("Failed to initialize context", main_coap_context != NULL);
}

void server_coap_poll(void)
{
	coap_check_notify(main_coap_context);
}
