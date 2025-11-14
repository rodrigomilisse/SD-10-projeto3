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
#include <sys/time.h>

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

	char ip[INET_ADDRSTRLEN] = "0.0.0.0";
	unsigned port = 0;
	{
		struct sockaddr_in peer;
		socklen_t plen = sizeof(peer);
		if (getpeername(connsockfd, (struct sockaddr *)&peer, &plen) == 0) {
			inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
			port = (unsigned)ntohs(peer.sin_port);
		}
	}

	struct list_t *list = args->list;

	// Thread secundária de atendimento do cliente
	MessageT *message;
	while ((message = network_receive(connsockfd)))
	{
		if (g_shutdown) break;

		MessageT *req = message;
		if (!req) break;

		struct timeval tv;
		gettimeofday(&tv, NULL);
		
		switch (req->opcode)
		{
		case MESSAGE_T__OPCODE__OP_SIZE:		//tamanho
			server_log("%ld %s:%u REQUEST SIZE\n", tv.tv_sec, ip, port);
			break;

		case MESSAGE_T__OPCODE__OP_ADD:		//adicionar	
			if (req->data && req->data->modelo)
				server_log("%ld %s:%u REQUEST ADD DATA \"%s\"\n", tv.tv_sec, ip,
				           port, req->data->modelo);
			else
				server_log("%ld %s:%u REQUEST ADD DATA\n", tv.tv_sec, ip, port);
			break;

		case MESSAGE_T__OPCODE__OP_GET:		
			if (req->c_type == MESSAGE_T__C_TYPE__CT_YEAR)
				server_log("%ld %s:%u REQUEST GET YEAR %d\n", tv.tv_sec, ip,
				           port, req->result);
			else if (req->c_type == MESSAGE_T__C_TYPE__CT_MARCA)
				server_log("%ld %s:%u REQUEST GET MARCA %d\n", tv.tv_sec, ip,
				           port, req->result); 
			else
				server_log("%ld %s:%u REQUEST GET\n", tv.tv_sec, ip, port);
			break;

		case MESSAGE_T__OPCODE__OP_DEL:		//apagar
		{
			const char *model =
			    (req->n_models > 0 && req->models && req->models[0])
			        ? req->models[0]
			        : (req->data ? req->data->modelo : NULL);
			if (model)
				server_log("%ld %s:%u REQUEST DEL MODEL \"%s\"\n", tv.tv_sec,
				           ip, port, model);
			else
				server_log("%ld %s:%u REQUEST DEL MODEL\n", tv.tv_sec, ip,
				           port);
			break;
		}

		case MESSAGE_T__OPCODE__OP_GETMODELS:		// modelos
			server_log("%ld %s:%u REQUEST GETMODELS\n", tv.tv_sec, ip, port);
			break;

		case MESSAGE_T__OPCODE__OP_GETLISTBYTEAR:	// lista p/ ano
			server_log("%ld %s:%u REQUEST GETLISTBYTEAR\n", tv.tv_sec, ip,
			           port);
			break;

		case MESSAGE_T__OPCODE__OP_ORDER:			
			server_log("%ld %s:%u REQUEST ORDER\n", tv.tv_sec, ip, port);
			break;
			
		default:
			server_log("%ld %s:%u REQUEST OP_%d\n", tv.tv_sec, ip, port,
			           (int)req->opcode);
			break;
		}



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
	{
		struct timeval tv2; gettimeofday(&tv2, NULL);
		server_log("%ld %s:%u DISCONNECT\n", tv2.tv_sec, ip, port);
	}
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
