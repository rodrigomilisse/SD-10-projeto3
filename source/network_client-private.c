/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/
#define _POSIX_C_SOURCE 200809L

#include "network_client-private.h"
#include "sdmessage.pb-c.h"

#include <arpa/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int send_all(int fd, const void *buf, size_t len)
{
	const uint8_t *p = (const uint8_t *)buf;
	size_t sent = 0;
	while (sent < len)
	{
		ssize_t n = send(fd, p + sent, len - sent, 0);
		if (n <= 0) return -1;
		sent += (size_t)n;
	}
	return 0;
}

int recv_all(int fd, void *buf, size_t len)
{
	uint8_t *p = (uint8_t *)buf;
	size_t recvd = 0;
	while (recvd < len)
	{
		ssize_t n = recv(fd, p + recvd, len - recvd, 0);
		if (n <= 0) return -1;
		recvd += (size_t)n;
	}
	return 0;
}

MessageT *recv_message(int fd)
{
	uint32_t net_len = 0;
	if (recv_all(fd, &net_len, sizeof(net_len)) < 0) return NULL;
	uint32_t len = ntohl(net_len);
	if (len == 0 || len > (32u * 1024u * 1024u)) return NULL;

	uint8_t *buf = (uint8_t *)malloc(len);
	if (!buf) return NULL;
	if (recv_all(fd, buf, len) < 0)
	{
		free(buf);
		return NULL;
	}

	MessageT *msg = message_t__unpack(NULL, len, buf);
	free(buf);
	return msg;
}

