/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/
#include "serialization.h"
#include "data.h"
#include "list-private.h"
#include "list.h"
#include "serialization-private.h"
#include "stdbool.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int car_to_buffer(struct data_t *car, char **car_buf)
{
	if (!car || !car_buf) { return -1; }

	// Assumindo que se o modelo for null ou vazio o tamnho é 1
	size_t buffer_size = sizeof_car(car);

	*car_buf = malloc(buffer_size);
	if (!*car_buf) { return -1; }

	serialize_car(car, *car_buf);

	return (int)buffer_size;
}

struct data_t *buffer_to_car(char *car_buf)
{
	if (!car_buf) { return NULL; }

	struct data_t *car = malloc(sizeof(struct data_t));
	if (!car) { return NULL; }

	deserialize_car(car, car_buf);

	return car;
}

int car_list_to_buffer(struct list_t *list, char **list_buf)
{
	if (!list || !list_buf) { return -1; }

	int count = list->size;
	size_t total_size = sizeof(count);
	for (struct car_t *car = list->head; car; car = car->next)
	{
		total_size += sizeof_car(car->data);
	}

	*list_buf = malloc(total_size);
	if (!*list_buf) return -1;

	char *cursor = *list_buf;
	memcpy(cursor, &count, sizeof(count));
	cursor += sizeof(count);

	for (struct car_t *car = list->head; car; car = car->next)
	{
		cursor += serialize_car(car->data, cursor);
	}
	return (int)(cursor - *list_buf);
}

struct list_t *buffer_to_car_list(char *list_buf)
{
	if (!list_buf) { return NULL; }

	char *cursor = list_buf;
	int count = 0;
	memcpy(&count, cursor, sizeof(count));
	cursor += sizeof(count);

	struct list_t *list = list_create();
	if (!list) { return NULL; }

	for (int i = 0; i < count; i++)
	{
		struct data_t *car = buffer_to_car(cursor);
		if (!car)
		{
			list_destroy(list);
			return NULL;
		}
		list_add(list, car);
		cursor += sizeof_car(car);
	}

	return list;
}
