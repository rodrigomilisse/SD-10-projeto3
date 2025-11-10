/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/

#define _GNU_SOURCE

#include "data.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "serialization-private.h"
#include <stddef.h>
#include <string.h>

size_t sizeof_car(struct data_t *car)
{
	return sizeof(car->ano) + sizeof(car->preco) + sizeof(car->marca) +
	       sizeof(car->combustivel) +
	       (car->modelo ? strlen(car->modelo) + 1 : 1);
}

size_t serialize_car(struct data_t *car, char *ptr)
{
	char *cursor = ptr;

	memcpy(cursor, &car->ano, sizeof(car->ano));
	cursor += sizeof(car->ano);

	memcpy(cursor, &car->preco, sizeof(car->preco));
	cursor += sizeof(car->preco);

	memcpy(cursor, &car->marca, sizeof(car->marca));
	cursor += sizeof(car->marca);

	memcpy(cursor, &car->combustivel, sizeof(car->combustivel));
	cursor += sizeof(car->combustivel);

	if (car->modelo) { strcpy(cursor, car->modelo); }
	else { *cursor = '\0'; }

	return sizeof_car(car);
}

size_t deserialize_car(struct data_t *car, char *ptr)
{
	char *cursor = ptr;
	memcpy(&car->ano, cursor, sizeof(car->ano));
	cursor += sizeof(car->ano);

	memcpy(&car->preco, cursor, sizeof(car->preco));
	cursor += sizeof(car->preco);

	memcpy(&car->marca, cursor, sizeof(car->marca));
	cursor += sizeof(car->marca);

	memcpy(&car->combustivel, cursor, sizeof(car->combustivel));
	cursor += sizeof(car->combustivel);

	car->modelo = strdup(cursor);

	return sizeof_car(car);
}