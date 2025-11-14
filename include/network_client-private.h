/**
 * Projeto: Sistemas Distribuídos 2025/2026
 * Grupo 10
 * Nuno Nobre 61823
 * Márcio Milisse 61799
 * Gonçalo Costa 61803
 */
#ifndef _NETWORK_CLIENT_PRIVATE_H
#define _NETWORK_CLIENT_PRIVATE_H

#include "sdmessage.pb-c.h"
#include <stddef.h>

int send_all(int fd, const void *buf, size_t len);

int recv_all(int fd, void *buf, size_t len);

MessageT *recv_message(int fd);

#endif

