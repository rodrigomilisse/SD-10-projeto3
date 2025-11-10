/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/

#define _POSIX_C_SOURCE 200809L

#include "client_stub.h"
#include "client_stub-private.h"
#include "data.h"
#include "list.h"
#include "network_client.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sdmessage.pb-c.h"

static void msg_create(MessageT *m, MessageT__Opcode op, MessageT__CType ct)
{
	message_t__init(m);
	m->opcode = op;
	m->c_type = ct;
}

static int ok_response(const MessageT *ans, const MessageT *req)
{
	if (!ans) return 0;
	if (ans->opcode == MESSAGE_T__OPCODE__OP_ERROR) return 0;
	return ans->opcode == (req->opcode + 1);
}

struct rlist_t *rlist_connect(char *address_port)
{
	if (!address_port) return NULL;

	struct rlist_t *r = (struct rlist_t *)malloc(sizeof(*r));
	if (!r) return NULL;

	char *addr_copy = strdup(address_port);
	if (!addr_copy)
	{
		free(r);
		return NULL;
	}

	char *addr = strtok(addr_copy, ":");
	char *port = strtok(NULL, ":");
	if (!addr || !port)
	{
		free(addr_copy);
		free(r);
		return NULL;
	}

	r->server_address = strdup(addr);
	if (!r->server_address)
	{
		free(addr_copy);
		free(r);
		return NULL;
	}

	r->server_port = atoi(port);
	r->sockfd = -1;
	free(addr_copy);

	if (network_connect(r) != 0)
	{
		free(r->server_address);
		free(r);
		return NULL;
	}
	return r;
}

int rlist_disconnect(struct rlist_t *rlist)
{
	if (!rlist) return -1;
	network_close(rlist);
	if (rlist->server_address) free(rlist->server_address);
	free(rlist);
	return 0;
}

int rlist_add(struct rlist_t *rlist, struct data_t *car)
{
	if (!rlist || !car) return -1;

	MessageT msg;
	msg_create(&msg, MESSAGE_T__OPCODE__OP_ADD, MESSAGE_T__C_TYPE__CT_DATA);

	Data d;
	data__init(&d);
	d.ano = car->ano;
	d.preco = car->preco;
	d.marca = (Marca)car->marca;
	d.combustivel = (Combustivel)car->combustivel;
	d.modelo = car->modelo ? strdup(car->modelo) : NULL;
	msg.data = &d;

	MessageT *ans = network_send_receive(rlist, &msg);

	int ret = -1;
	if (ok_response(ans, &msg))
	{
		ret = (ans->c_type == MESSAGE_T__C_TYPE__CT_RESULT)
		          ? (ans->result == 0 ? 0 : -1)
		          : 0;
	}

	if (ans) message_t__free_unpacked(ans, NULL);
	if (d.modelo) free(d.modelo);
	return ret;
}

int rlist_remove_by_model(struct rlist_t *rlist, const char *modelo)
{
	if (!rlist || !modelo) return -1;

	MessageT msg;
	msg_create(&msg, MESSAGE_T__OPCODE__OP_DEL, MESSAGE_T__C_TYPE__CT_MODEL);

	char **models = (char **)malloc(sizeof(char *));
	if (!models) return -1;
	models[0] = strdup(modelo);
	if (!models[0])
	{
		free(models);
		return -1;
	}

	msg.n_models = 1;
	msg.models = models;

	MessageT *ans = network_send_receive(rlist, &msg);

	int ret = -1;
	if (ok_response(ans, &msg))
	{
		ret = (ans->c_type == MESSAGE_T__C_TYPE__CT_RESULT)
		          ? (ans->result == 0 ? 0 : -1)
		          : 0;
	}

	if (ans) message_t__free_unpacked(ans, NULL);
	free(models[0]);
	free(models);
	return ret;
}

struct data_t *rlist_get_by_marca(struct rlist_t *rlist, enum marca_t marca)
{
	if (!rlist) return NULL;

	MessageT msg;
	msg_create(&msg, MESSAGE_T__OPCODE__OP_GET, MESSAGE_T__C_TYPE__CT_MARCA);
	msg.result = (int32_t)marca;

	MessageT *ans = network_send_receive(rlist, &msg);
	if (!ans) return NULL;

	if (ans->opcode != MESSAGE_T__OPCODE__OP_GET + 1 ||
	    ans->c_type != MESSAGE_T__C_TYPE__CT_DATA || ans->data == NULL)
	{
		message_t__free_unpacked(ans, NULL);
		return NULL;
	}

	struct data_t *d = malloc(sizeof(*d));
	if (!d)
	{
		message_t__free_unpacked(ans, NULL);
		return NULL;
	}
	d->ano = ans->data->ano;
	d->preco = ans->data->preco;
	d->marca = (enum marca_t)ans->data->marca;
	d->combustivel = (enum combustivel_t)ans->data->combustivel;
	d->modelo = strdup(ans->data->modelo ? ans->data->modelo : "");

	message_t__free_unpacked(ans, NULL);
	return d;
}

struct data_t **rlist_get_by_year(struct rlist_t *rlist, int ano)
{
	if (!rlist) return NULL;

	MessageT msg;
	msg_create(&msg, MESSAGE_T__OPCODE__OP_GET, MESSAGE_T__C_TYPE__CT_YEAR);
	msg.result = ano;

	MessageT *ans = network_send_receive(rlist, &msg);
	if (!ans) return NULL;

	if (ans->opcode != MESSAGE_T__OPCODE__OP_GET + 1 ||
	    ans->c_type != MESSAGE_T__C_TYPE__CT_LIST)
	{
		message_t__free_unpacked(ans, NULL);
		return NULL;
	}

	size_t n = ans->n_cars;
	struct data_t **out = malloc((n + 1) * sizeof(*out));
	if (!out)
	{
		message_t__free_unpacked(ans, NULL);
		return NULL;
	}

	for (size_t i = 0; i < n; i++)
	{
		Data *src = ans->cars[i];
		struct data_t *d = malloc(sizeof(*d));
		if (!d)
		{
			for (size_t j = 0; j < i; j++)
			{
				if (out[j]->modelo) { free(out[j]->modelo); }
				free(out[j]);
			}
			free(out);
			message_t__free_unpacked(ans, NULL);
			return NULL;
		}
		d->ano = src->ano;
		d->preco = src->preco;
		d->marca = (enum marca_t)src->marca;
		d->combustivel = (enum combustivel_t)src->combustivel;
		d->modelo = strdup(src->modelo ? src->modelo : "");
		out[i] = d;
	}
	out[n] = NULL;

	message_t__free_unpacked(ans, NULL);
	return out;
}

int rlist_order_by_year(struct rlist_t *rlist)
{
	if (!rlist) return -1;

	MessageT msg;
	msg_create(&msg, MESSAGE_T__OPCODE__OP_GETLISTBYTEAR,
	           MESSAGE_T__C_TYPE__CT_NONE);

	MessageT *ans = network_send_receive(rlist, &msg);
	if (!ans) return -1;

	int ret = ok_response(ans, &msg) ? 0 : -1;

	message_t__free_unpacked(ans, NULL);
	return ret;
}

int rlist_size(struct rlist_t *rlist)
{
	if (!rlist) return -1;

	MessageT msg;
	msg_create(&msg, MESSAGE_T__OPCODE__OP_SIZE, MESSAGE_T__C_TYPE__CT_NONE);

	MessageT *ans = network_send_receive(rlist, &msg);
	if (!ans) return -1;

	int size = -1;
	if (ok_response(ans, &msg) && ans->c_type == MESSAGE_T__C_TYPE__CT_RESULT)
		size = ans->result;

	message_t__free_unpacked(ans, NULL);
	return size;
}

char **rlist_get_model_list(struct rlist_t *rlist)
{
	if (!rlist) return NULL;

	MessageT msg;
	msg_create(&msg, MESSAGE_T__OPCODE__OP_GETMODELS,
	           MESSAGE_T__C_TYPE__CT_NONE);

	MessageT *ans = network_send_receive(rlist, &msg);
	if (!ans) return NULL;

	if (ans->opcode != MESSAGE_T__OPCODE__OP_GETMODELS + 1 ||
	    ans->c_type != MESSAGE_T__C_TYPE__CT_MODEL)
	{
		message_t__free_unpacked(ans, NULL);
		return NULL;
	}

	size_t n = ans->n_models;
	char **out = malloc((n + 1) * sizeof(char *));
	if (!out)
	{
		message_t__free_unpacked(ans, NULL);
		return NULL;
	}

	for (size_t i = 0; i < n; i++)
		out[i] = strdup(ans->models[i] ? ans->models[i] : "");
	out[n] = NULL;

	message_t__free_unpacked(ans, NULL);
	return out;
}

int rlist_free_model_list(char **models)
{
	if (!models) return -1;
	for (size_t i = 0; models[i] != NULL; i++) free(models[i]);
	free(models);
	return 0;
}
