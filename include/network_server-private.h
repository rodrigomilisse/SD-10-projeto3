/**
 * Projeto: Sistemas Distribuídos 2025/2026
 * Grupo 10
 * Nuno Nobre 61823
 * Márcio Milisse 61799
 * Gonçalo Costa 61803
 */
#ifndef _NETWORK_SERVER_PRIVATE_H
#define _NETWORK_SERVER_PRIVATE_H

#include "list.h"
#include "sdmessage.pb-c.h"
#include <pthread.h>
#include <stddef.h>

#define MAX_CLIENTES_ATIVOS 5

/* Estrutura para passar argumentos para a thread de atendimento ao cliente */
struct client_args
{
	int connsockfd;
	struct list_t *list;
};

/* Função executada pela thread de atendimento ao cliente */
void *client_loop(void *arg);

/* Funções auxiliares de leitura/escrita de mensagens */
int read_all_full(int fd, void *buf, size_t len);
int write_all_full(int fd, const void *buf, size_t len);
MessageT *recv_message(int fd);
int send_message(int fd, MessageT *msg);

#endif

