/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/

#ifndef _SERIALIZATION_PRIVATE_H
#define _SERIALIZATION_PRIVATE_H

#include "data.h"
#include <stddef.h>

/*
 * Calcula o tamanho do buffer necessário para armazenar um carro.
 * Retorna o tamanho do buffer.
 */
size_t sizeof_car(struct data_t *car);

/*
 * Serializa um carro (data_t) para um buffer.
 * Retorna o tamanho do buffer ou -1 em caso de erro.
 * Retorna o número de bytes consumidos/escritos.
 */
size_t serialize_car(struct data_t *car, char *ptr);

/*
 * Desserializa um buffer para um carro (data_t).
 * Retorna o tamanho do buffer ou -1 em caso de erro.
 * Retorna o número de bytes consumidos/escritos.
 */
size_t deserialize_car(struct data_t *car, char *ptr);

#endif