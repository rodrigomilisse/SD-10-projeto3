/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/

#define _POSIX_C_SOURCE 200809L
#include "network_server.h"
#include "list.h"
#include "sdmessage.pb-c.h"

#include "list_skel.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_CLIENTES_ATIVOS 5 // TODO
int num_clientes_ativos = 0;  // TODO
pthread_mutex_t num_clientes_ativos_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t server_log_mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER; TODO

static int read_all_full(int fd, void *buf, size_t len)
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
static int write_all_full(int fd, const void *buf, size_t len)
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

static MessageT *recv_message(int fd)
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

static int send_message(int fd, MessageT *msg)
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

static volatile sig_atomic_t g_shutdown = 0;

int network_server_init(short port)
{
	int sockfd;
	struct sockaddr_in server_address;

	signal(SIGPIPE, SIG_IGN);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return -1;
	}

	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr *)&server_address,
	         sizeof(server_address)) < 0)
	{
		perror("bind");
		close(sockfd);
		return -1;
	}

	if (listen(sockfd, 8) < 0)
	{
		perror("listen");
		close(sockfd);
		return -1;
	}

	return sockfd;
}

int network_main_loop(int listening_socket, struct list_t *list)
{
	if (listening_socket < 0 || list == NULL) return -1;

	while (!g_shutdown)
	{
		struct sockaddr_in client;
		socklen_t size_client = sizeof(client);

		int connsockfd =
		    accept(listening_socket, (struct sockaddr *)&client, &size_client);

		if (connsockfd < 0)
		{
			if (errno == EINTR && g_shutdown) break;
			perror("accept");
			continue;
		}
		pthread_mutex_lock(&num_clientes_ativos_mutex);
		if (num_clientes_ativos < MAX_CLIENTES_ATIVOS)
		{
			pthread_mutex_unlock(&num_clientes_ativos_mutex);

			// cria uma nova thread passando-lhe connsockfd

			pthread_mutex_lock(&server_log_mutex);
			// regista no log do servidor a ligação criada
			pthread_mutex_unlock(&server_log_mutex);

			pthread_mutex_lock(&num_clientes_ativos_mutex);
			num_clientes_ativos++;
			pthread_mutex_unlock(&num_clientes_ativos_mutex);
		}
		else
		{
			pthread_mutex_unlock(&num_clientes_ativos_mutex);
			close(connsockfd);
			continue; // TODO
		}

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
		// termina a thread secundária
	}
	return g_shutdown ? 0 : -1;
}

MessageT *network_receive(int client_socket)
{
	return recv_message(client_socket);
}

int network_send(int client_socket, MessageT *msg)
{
	return send_message(client_socket, msg);
}

int network_server_close(int socket_fd)
{
	if (socket_fd < 0) return -1;
	return close(socket_fd) == 0 ? 0 : -1;
}

void network_server_request_shutdown(void) { g_shutdown = 1; }
