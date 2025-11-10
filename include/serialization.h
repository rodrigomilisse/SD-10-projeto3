/**
 * @file serialization.h
 * @brief Interface para serialização de carros e listas de carros.
 *
 * Projeto: Sistemas Distribuídos 2025/2026
 * Autor: José Cecílio
 * Data: 13/09/2025
 */
#ifndef _SERIALIZATION_H
#define _SERIALIZATION_H

#include "data.h"
#include "list.h"

/* Serializa um carro (data_t) para um buffer.
 * Retorna o tamanho do buffer ou -1 em caso de erro.
 */
int car_to_buffer(struct data_t *car, char **car_buf);

/* Desserializa um buffer para um carro (data_t).
 * Retorna o ponteiro para data_t ou NULL em caso de erro.
 */
struct data_t *buffer_to_car(char *car_buf);

/* Serializa uma lista de carros para um buffer.
 * Retorna o tamanho do buffer ou -1 em caso de erro.
 */
int car_list_to_buffer(struct list_t *list, char **list_buf);

/* Desserializa um buffer para uma lista de carros.
 * Retorna ponteiro para list_t ou NULL em caso de erro.
 */
struct list_t *buffer_to_car_list(char *list_buf);

#endif
