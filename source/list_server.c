/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "list_skel.h"
#include "network_server.h"

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "Uso: %s <porto>\n", argv[0]);
		return 1;
	}

	int port = atoi(argv[1]);
	if (port <= 0 || port > 65535)
	{
		fprintf(stderr, "Porto inválido: %s\n", argv[1]);
		return 1;
	}

	struct list_t *list = list_skel_init();
	if (list == NULL)
	{
		fprintf(stderr, "Erro a criar lista.\n");
		return 1;
	}

	int lsock = network_server_init((short)port);
	if (lsock < 0)
	{
		fprintf(stderr, "Erro em network_server_init().\n");
		list_skel_destroy(list);
		list_destroy(list);
		return 1;
	}

	int rc = network_main_loop(lsock, list);

	// limpeza
	network_server_close(lsock);
	list_skel_destroy(list);
	list_destroy(list);

	return (rc == 0) ? 0 : 1;
}
