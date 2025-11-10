/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/

#define _POSIX_C_SOURCE 200809L

#include "list-private.h"
#include "list.h"
#include "message-private.h"
#include "network_server.h"
#include "sdmessage.pb-c.h"
#include "serialization.h"
#include <stdlib.h>
#include <string.h>


struct list_t *list_skel_init() { return list_create(); }


int list_skel_destroy(struct list_t *list)
{
	return list ? list_destroy(list) : -1;
}


int invoke(MessageT *msg, struct list_t *list)
{
	if (msg == NULL || list == NULL) { return -1; }
	// para msg q podem ser pedidos do cliente
	switch (msg->opcode)
	{
	case MESSAGE_T__OPCODE__OP_SIZE:
	{
		int s = list_size(list);
		if (s < 0)
		{
			msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
			msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
			return -1;
		}
		msg->opcode = (MessageT__Opcode)(MESSAGE_T__OPCODE__OP_SIZE + 1);
		msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
		msg->result = s;
		return 0;
	}

	case MESSAGE_T__OPCODE__OP_ADD:
	{
		if (msg->data == NULL)
		{
			msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
			msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
			return -1;
		}

		struct data_t *car = data_create(
		    msg->data->ano, msg->data->preco, (enum marca_t)msg->data->marca,
		    msg->data->modelo, (enum combustivel_t)msg->data->combustivel);

		data__free_unpacked(msg->data, NULL);
		msg->data = NULL;

		if (car == NULL)
		{
			msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
			msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
			return -1;
		}

		if (list_add(list, car) != 0)
		{
			data_destroy(car);
			msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
			msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
			return -1;
		}
		else
		{
			msg->opcode = (MessageT__Opcode)(MESSAGE_T__OPCODE__OP_ADD + 1);
			msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
			return 0;
		}
	}

	case MESSAGE_T__OPCODE__OP_GET:
	{
		if (msg->c_type == MESSAGE_T__C_TYPE__CT_YEAR)
		{
			int year = msg->result;
			struct data_t **arr = list_get_by_year(list, year);
			if (!arr)
			{
				msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
				msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
				return -1;
			}

			size_t n = 0;
			while (arr[n]) n++;
			msg->cars = (Data **)malloc(sizeof(Data *) * n);
			if (!msg->cars)
			{
				free(arr);
				msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
				msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
				return -1;
			}
			for (size_t i = 0; i < n; i++)
			{
				Data *pd = (Data *)malloc(sizeof(Data));
				if (!pd)
				{
					for (size_t j = 0; j < i; j++)
					{
						if (msg->cars[j])
						{
							if (msg->cars[j]->modelo)
								free(msg->cars[j]->modelo);
							free(msg->cars[j]);
						}
					}
					free(msg->cars);
					free(arr);
					msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
					msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
					return -1;
				}
				data__init(pd);
				pd->ano = arr[i]->ano;
				pd->preco = arr[i]->preco;
				pd->marca = (Marca)arr[i]->marca;
				pd->modelo =
				    arr[i]->modelo ? strdup(arr[i]->modelo) : strdup("");
				pd->combustivel = (Combustivel)arr[i]->combustivel;
				msg->cars[i] = pd;
			}
			msg->n_cars = n;
			free(arr);
			msg->opcode = (MessageT__Opcode)(MESSAGE_T__OPCODE__OP_GET + 1);
			msg->c_type = MESSAGE_T__C_TYPE__CT_LIST;
			return 0;
		}

		if (msg->c_type != MESSAGE_T__C_TYPE__CT_MARCA)
		{
			if (msg->data)
			{
				data__free_unpacked(msg->data, NULL);
				msg->data = NULL;
			}
			msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
			msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
			return -1;
		}

		enum marca_t marca = (enum marca_t)msg->result;

		struct data_t *res = list_get_by_marca(list, marca);
		if (!res)
		{
			msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
			msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
			return -1;
		}

		Data *pd = (Data *)malloc(sizeof(Data));
		if (!pd)
		{
			msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
			msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
			return -1;
		}
		data__init(pd);
		pd->ano = res->ano;
		pd->preco = res->preco;
		pd->marca = (Marca)res->marca;
		pd->modelo = res->modelo ? strdup(res->modelo) : strdup("");
		pd->combustivel = (Combustivel)res->combustivel;

		msg->data = pd;
		msg->opcode = (MessageT__Opcode)(MESSAGE_T__OPCODE__OP_GET + 1);
		msg->c_type = MESSAGE_T__C_TYPE__CT_DATA;
		return 0;
	}

	case MESSAGE_T__OPCODE__OP_DEL:
	{
		const char *model = NULL;
		if (msg->n_models > 0 && msg->models && msg->models[0])
		{
			model = msg->models[0];
		}
		else if (msg->data && msg->data->modelo) { model = msg->data->modelo; }
		else
		{
			msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
			msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
			return -1;
		}

		int r = list_remove_by_model(list, model);

		if (msg->data)
		{
			data__free_unpacked(msg->data, NULL);
			msg->data = NULL;
		}
		if (msg->models)
		{
			for (size_t i = 0; i < msg->n_models; i++)
			{
				if (msg->models[i]) free(msg->models[i]);
			}
			free(msg->models);
			msg->models = NULL;
			msg->n_models = 0;
		}

		if (r == -1)
		{
			msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
			msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
			return -1;
		}

		msg->opcode = (MessageT__Opcode)(MESSAGE_T__OPCODE__OP_DEL + 1);
		msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
		return 0;
	}

	case MESSAGE_T__OPCODE__OP_GETMODELS:
	{
		char **models = list_get_model_list(list);
		if (!models)
		{
			msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
			msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
			return -1;
		}
		size_t n = 0;
		while (models[n]) n++;
		msg->models = (char **)malloc(sizeof(char *) * n);
		if (!msg->models)
		{
			list_free_model_list(models);
			msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
			msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
			return -1;
		}
		for (size_t i = 0; i < n; i++) { msg->models[i] = strdup(models[i]); }
		msg->n_models = n;
		list_free_model_list(models);
		msg->opcode = (MessageT__Opcode)(MESSAGE_T__OPCODE__OP_GETMODELS + 1);
		msg->c_type = MESSAGE_T__C_TYPE__CT_MODEL;
		return 0;
	}

	case MESSAGE_T__OPCODE__OP_GETLISTBYTEAR:
	{
		if (list_order_by_year(list) != 0)
		{
			msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
			msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
			return -1;
		}
		int count = 0;
		for (struct car_t *c = list->head; c; c = c->next) count++;
		msg->cars = (Data **)malloc(sizeof(Data *) * count);
		if (!msg->cars)
		{
			msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
			msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
			return -1;
		}
		int i = 0;
		for (struct car_t *node = list->head; node; node = node->next, i++)
		{
			struct data_t *d = node->data;
			Data *pd = (Data *)malloc(sizeof(Data));
			if (!pd)
			{
				for (int j = 0; j < i; j++)
				{
					if (msg->cars[j])
					{
						if (msg->cars[j]->modelo) free(msg->cars[j]->modelo);
						free(msg->cars[j]);
					}
				}
				free(msg->cars);
				msg->cars = NULL;
				msg->n_cars = 0;
				msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
				msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
				return -1;
			}
			data__init(pd);
			pd->ano = d->ano;
			pd->preco = d->preco;
			pd->marca = (Marca)d->marca;
			pd->modelo = d->modelo ? strdup(d->modelo) : strdup("");
			pd->combustivel = (Combustivel)d->combustivel;
			msg->cars[i] = pd;
		}
		msg->n_cars = (size_t)count;
		msg->opcode =
		    (MessageT__Opcode)(MESSAGE_T__OPCODE__OP_GETLISTBYTEAR + 1);
		msg->c_type = MESSAGE_T__C_TYPE__CT_LIST;
		return 0;
	}

	case MESSAGE_T__OPCODE__OP_ORDER:
	{
		int r = list_order_by_year(list);
		if (r != 0)
		{
			msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
			msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
			return -1;
		}
		msg->opcode = (MessageT__Opcode)(MESSAGE_T__OPCODE__OP_ORDER + 1);
		msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
		msg->result = 0;
		return 0;
	}

	default:
		msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
		msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
		return -1;
	}
}
