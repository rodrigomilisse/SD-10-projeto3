/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/

#define _POSIX_C_SOURCE 200809L
#include "network_server-private.h"
#include "list_skel.h"
#include "network_server.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Variáveis globais privadas */
extern pthread_t client_threads[MAX_CLIENTES_ATIVOS];
extern int num_clientes_ativos;
extern pthread_mutex_t num_clientes_ativos_mutex;
extern pthread_mutex_t server_log_mutex;
extern volatile sig_atomic_t g_shutdown;

int read_all_full(int fd, void *buf, size_t len)
{
	unsigned char *p = (unsigned char *)buf;
	size_t got = 0;
	while (got < len)
	{
		ssize_t n = read(fd, p + got, len - got);
		if (n <= 0) return -1;
		got += (size_t)n;
	}
	return (int)got;
}

int write_all_full(int fd, const void *buf, size_t len)
{
	const unsigned char *p = (const unsigned char *)buf;
	size_t sent = 0;
	while (sent < len)
	{
		ssize_t n = write(fd, p + sent, len - sent);
		if (n <= 0) return -1;
		sent += (size_t)n;
	}
	return (int)sent;
}

MessageT *recv_message(int fd)
{
	uint32_t net_len = 0;
	if (read_all_full(fd, &net_len, sizeof(net_len)) != (int)sizeof(net_len))
		return NULL;
	uint32_t len = ntohl(net_len);
	if (len == 0 || len > (32u * 1024u * 1024u)) return NULL;

	uint8_t *buf = (uint8_t *)malloc(len);
	if (!buf) return NULL;
	if (read_all_full(fd, buf, len) != (int)len)
	{
		free(buf);
		return NULL;
	}

	MessageT *msg = message_t__unpack(NULL, len, buf);
	free(buf);
	return msg;
}

int send_message(int fd, MessageT *msg)
{
	size_t sz = message_t__get_packed_size(msg);
	uint8_t *buf = (uint8_t *)malloc(sz);
	if (!buf) return -1;
	message_t__pack(msg, buf);

	uint32_t net_len = htonl((uint32_t)sz);
	if (write_all_full(fd, &net_len, sizeof(net_len)) != (int)sizeof(net_len) ||
	    write_all_full(fd, buf, sz) != (int)sz)
	{
		free(buf);
		return -1;
	}
	free(buf);
	return 0;
}

void *client_loop(void *arg)
{
	struct client_args *args = (struct client_args *)arg;
	int connsockfd = args->connsockfd;
	struct list_t *list = args->list;

	// Thread secundária de atendimento do cliente
	MessageT *message;
	while ((message = network_receive(connsockfd)))
	{
		if (g_shutdown) break;

		MessageT *req = message;
		if (!req) break;

		if (invoke(req, list) != 0)
		{
			req->opcode = MESSAGE_T__OPCODE__OP_ERROR;
			req->c_type = MESSAGE_T__C_TYPE__CT_NONE;
		}

		if (network_send(connsockfd, req) != 0)
		{
			message_t__free_unpacked(req, NULL);
			break;
		}
		message_t__free_unpacked(req, NULL);
	}
	close(connsockfd);
	pthread_mutex_lock(&server_log_mutex);
	// regista no log do servidor o fecho da ligação com o cliente
	pthread_mutex_unlock(&server_log_mutex);
	pthread_mutex_lock(&num_clientes_ativos_mutex);
	num_clientes_ativos--;
	pthread_mutex_unlock(&num_clientes_ativos_mutex);

	free(args);

	// termina a thread secundária
	return NULL;
}
