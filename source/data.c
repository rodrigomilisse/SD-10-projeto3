/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/

#define _GNU_SOURCE
#include "data.h"
#include <stdlib.h>
#include <string.h>

struct data_t *data_create(int ano, float preco, enum marca_t marca,
                           const char *modelo, enum combustivel_t combustivel)
{
	struct data_t *data = malloc(sizeof(struct data_t));
	if (!data) { return NULL; }

	data->ano = ano;
	data->preco = preco;
	data->marca = marca;
	data->modelo = modelo ? strdup(modelo) : '\0';
	if (modelo && !data->modelo)
	{
		free(data);
		return NULL;
	}
	data->combustivel = combustivel;

	return data;
}

int data_destroy(struct data_t *data)
{
	if (!data) { return -1; }

	free(data->modelo); // liberta modelo dupl
	free(data);
	return 0;
}

struct data_t *data_dup(struct data_t *data)
{
	if (!data) { return NULL; }
	return data_create(data->ano, data->preco, data->marca, data->modelo,
	                   data->combustivel);
}

int data_replace(struct data_t *data, int ano, float preco, enum marca_t marca,
                 const char *modelo, enum combustivel_t combustivel)
{
	if (!data) { return -1; }

	data->ano = ano;
	data->preco = preco;
	data->marca = marca;
	free(data->modelo);
	data->modelo = modelo ? strdup(modelo) : '\0';
	if (modelo && !data->modelo)
	{
		data->modelo = NULL;
		return -1;
	}
	data->combustivel = combustivel;

	return 0;
}