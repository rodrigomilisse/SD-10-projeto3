/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/

#define _GNU_SOURCE

#include "list.h"
#include "data.h"
#include "list-private.h"
#include <stdlib.h>
#include <string.h>

struct list_t *list_create()
{
	struct list_t *carList = malloc(sizeof(struct list_t));
	if (carList == NULL)
	{ // caso malloc falhe
		return NULL;
	}
	carList->head = NULL;
	carList->size = 0;
	return carList;
}

int list_destroy(struct list_t *list)
{

	if (!list) { return -1; }
	for (struct car_t *car = list->head, *next = NULL; car; car = next)
	{
		next = car->next;
		if (car->data) { data_destroy(car->data); }
		free(car);
	}
	free(list);
	return 0;
}

int list_add(struct list_t *list, struct data_t *car)
{
	if (!list) { return -1; }
	struct car_t *carData =
	    malloc(sizeof(struct car_t)); // passar de data_t para car_t
	if (!carData) { return -1; }
	carData->data = car;
	carData->next = NULL;

	if (list->head == NULL)
	{ // caso a lista esteja vazia
		list->head = carData;
		list->size = 1;
	}
	else
	{
		struct car_t *car;
		for (car = list->head; car->next; car = car->next) {}
		car->next = carData;
		list->size++;
	}
	return 0;
}

int list_remove_by_model(struct list_t *list, const char *modelo)
{
	if (!list || !list->head || !modelo) { return -1; }
	else
	{
		struct car_t *previous = NULL;
		struct car_t *car = list->head;
		for (car = list->head; car; previous = car, car = car->next)
		{
			if (!(car->data && car->data->modelo &&
			      strcmp(car->data->modelo, modelo) == 0))
			{
				continue;
			}
			if (previous) { previous->next = car->next; }
			else { list->head = car->next; }
			list->size--;
			if (car->data) { data_destroy(car->data); }
			free(car);
			return 0;
		}
		return 1;
	}
}

struct data_t *list_get_by_marca(struct list_t *list, enum marca_t marca)
{
	if (!list || !list->head) { return NULL; }
	else
	{
		struct car_t *car = list->head;
		for (car = list->head; car; car = car->next)
		{
			if (car->data && car->data->marca == marca) { return car->data; }
		}
		return NULL;
	}
}

struct data_t **list_get_by_year(struct list_t *list, int ano)
{
	if (!list || !list->head) { return NULL; }
	else
	{
		int count = 0;
		for (struct car_t *car = list->head; car; car = car->next)
		{
			if (car->data && car->data->ano == ano) { count++; }
		}
		struct data_t **arrayList =
		    malloc(sizeof(struct data_t *) * (count + 1));
		if (!arrayList) { return NULL; }
		int i = 0;
		for (struct car_t *car = list->head; car; car = car->next)
		{
			if (car->data && car->data->ano == ano)
			{
				arrayList[i++] = car->data;
			}
		}
		arrayList[i] = NULL;
		return arrayList;
	}
}

int list_order_by_year(struct list_t *list)
{
	if (list == NULL) { return -1; }

	int swapped;
	do {
		swapped = 0;
		struct car_t *cur = list->head;
		while (cur != NULL && cur->next != NULL)
		{
			if (cur->data && cur->next->data &&
			    cur->data->ano > cur->next->data->ano)
			{
				struct data_t *tmp = cur->data;
				cur->data = cur->next->data;
				cur->next->data = tmp;
				swapped = 1;
			}
			cur = cur->next;
		}
	} while (swapped);
	return 0;
}

int list_size(struct list_t *list) { return list ? list->size : -1; }

char **list_get_model_list(struct list_t *list)
{
	if (!list) { return NULL; }
	int count = list->size;
	char **models = malloc(sizeof(char *) * (count + 1));
	if (!models) { return NULL; }
	int i = 0;
	for (struct car_t *car = list->head; car; car = car->next)
	{
		if (car->data && car->data->modelo)
		{
			models[i] = strdup(car->data->modelo);
			if (!models[i])
			{
				for (int j = 0; j < i; j++) { free(models[j]); }
				free(models);
				return NULL;
			}
			i++;
		}
	}
	models[i] = NULL;
	return models;
}

int list_free_model_list(char **models)
{
	if (!models) { return -1; }
	for (int i = 0; models[i]; i++) { free(models[i]); }
	free(models);
	return 0;
}
