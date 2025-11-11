/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/

#define _POSIX_C_SOURCE 200809L
#include "network_server.h"
#include "network_server-private.h"
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

/* Variáveis globais */
pthread_t client_threads[MAX_CLIENTES_ATIVOS];
int num_clientes_ativos = 0;
pthread_mutex_t num_clientes_ativos_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t server_log_mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER; TODO

volatile sig_atomic_t g_shutdown = 0;

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
			struct client_args *args = malloc(sizeof(struct client_args));
			if (args == NULL)
			{
				close(connsockfd);
				continue;
			}
			args->connsockfd = connsockfd;
			args->list = list;

			pthread_t thread_id;
			if (pthread_create(&thread_id, NULL, client_loop, args) != 0)
			{
				free(args);
				close(connsockfd);
				continue;
			}
			pthread_detach(thread_id);

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
		}
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
