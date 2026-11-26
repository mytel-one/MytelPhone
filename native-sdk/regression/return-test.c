#include <belle-sip/belle-sip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static belle_sip_request_t *pending;
static belle_sip_server_transaction_t *pending_transaction;
static int delay_ticks;

static void incoming(void *ctx, const belle_sip_request_event_t *event) {
	belle_sip_provider_t *provider = (belle_sip_provider_t *)ctx;
	belle_sip_request_t *request = belle_sip_request_event_get_request(event);
	const char *method = belle_sip_request_get_method(request);
	if (strcmp(method, "OPTIONS") == 0 && strcmp(belle_sip_uri_get_user(belle_sip_request_get_uri(request)), "later") == 0) {
		pending = (belle_sip_request_t *)belle_sip_object_ref(request);
		delay_ticks = 25;
	} else if (strcmp(method, "OPTIONS") == 0) {
		belle_sip_provider_send_response(provider, belle_sip_response_create_from_request(request, 200));
	} else if (strcmp(method, "INVITE") == 0) {
		belle_sip_server_transaction_t *transaction = belle_sip_provider_create_server_transaction(provider, request);
		const char *user = belle_sip_uri_get_user(belle_sip_request_get_uri(request));
		belle_sip_server_transaction_send_response(transaction, belle_sip_response_create_from_request(request, 180));
		if (strcmp(user, "later-invite") == 0) {
			pending_transaction = (belle_sip_server_transaction_t *)belle_sip_object_ref(transaction);
			delay_ticks = 25;
			return;
		}
		belle_sip_response_t *final = belle_sip_response_create_from_request(request, strcmp(user, "accept") == 0 ? 200 : 486);
		if (belle_sip_response_get_status_code(final) == 200) {
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(final), BELLE_SIP_HEADER(belle_sip_header_contact_parse("Contact: <sip:accept@127.0.0.1>")));
		}
		belle_sip_server_transaction_send_response(transaction, final);
	}
}

int main(int argc, char **argv) {
	if (argc != 4) return 2;
	const char *transport = argv[1];
	int peer_port = atoi(argv[2]);
	belle_sip_stack_t *stack = belle_sip_stack_new(NULL);
	belle_sip_listening_point_t *lp = belle_sip_stack_create_listening_point(stack, "127.0.0.1", -1, transport);
	if (!lp) return 3;
	if (strcmp(transport, "TLS") == 0)
		belle_tls_crypto_config_set_root_ca(belle_sip_tls_listening_point_get_crypto_config(BELLE_SIP_TLS_LISTENING_POINT(lp)), argv[3]);
	belle_sip_provider_t *provider = belle_sip_stack_create_provider(stack, lp);
	belle_sip_listener_callbacks_t events = {0};
	events.process_request_event = incoming;
	belle_sip_listener_t *listener = belle_sip_listener_create_from_callbacks(&events, provider);
	belle_sip_provider_add_sip_listener(provider, listener);
	printf("READY %d\n", belle_sip_listening_point_get_port(lp)); fflush(stdout);
	if (strcmp(transport,"UDP") != 0) {
		for (int i = 0; i < 2; i++) {
			char msg[1024];
			snprintf(msg,sizeof(msg),"OPTIONS sip:peer@127.0.0.1:%d;transport=%s SIP/2.0\r\nVia: SIP/2.0/%s 127.0.0.1;branch=z9hG4bK-test%d\r\nFrom: <sip:user%d@localhost>;tag=f%d\r\nTo: <sip:peer@127.0.0.1>\r\nCall-ID: opts%d@localhost\r\nCSeq: 1 OPTIONS\r\nMax-Forwards: 70\r\nContent-Length: 0\r\n\r\n",peer_port,transport,transport,i,i,i,i);
			belle_sip_message_t *request = belle_sip_message_parse(msg);
			if (!request) return 4;
			belle_sip_message_set_channel_bank_identifier(request, i == 0 ? "A" : "B");
			belle_sip_provider_send_request(provider, BELLE_SIP_REQUEST(request));
		}
	}
	for (int tick = 0; tick < 1000; ++tick) {
		belle_sip_stack_sleep(stack, 20);
		if (pending_transaction && --delay_ticks == 0) {
			belle_sip_request_t *req = belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(pending_transaction));
			belle_sip_server_transaction_send_response(pending_transaction, belle_sip_response_create_from_request(req, 486));
			belle_sip_object_unref(pending_transaction);
			pending_transaction = NULL;
		}
		if (pending && --delay_ticks == 0) {
			belle_sip_provider_send_response(provider, belle_sip_response_create_from_request(pending, 200));
			belle_sip_object_unref(pending);
			pending = NULL;
		}
	}
	return 0;
}