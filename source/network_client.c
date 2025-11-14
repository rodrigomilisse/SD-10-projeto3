/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/
#define _POSIX_C_SOURCE 200809L

#include "network_client.h"
#include "client_stub-private.h"
#include "sdmessage.pb-c.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int send_all(int fd, const void *buf, size_t len)
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

static int recv_all(int fd, void *buf, size_t len)
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

static MessageT *recv_message(int fd)
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

int network_connect(struct rlist_t *rlist)
{
	if (!rlist || !rlist->server_address || rlist->server_port <= 0) return -1;

	signal(SIGPIPE, SIG_IGN);

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("socket");
		return -1;
	}

	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons((uint16_t)rlist->server_port);

	if (inet_pton(AF_INET, rlist->server_address, &server_address.sin_addr) <=
	    0)
	{
		perror("inet_pton");
		close(sockfd);
		return -1;
	}

	if (connect(sockfd, (struct sockaddr *)&server_address,
	            sizeof(server_address)) < 0)
	{
		perror("connect");
		close(sockfd);
		return -1;
	}

	// Receber mensagem de estado do servidor (OP_READY ou OP_BUSY)
	MessageT *status_msg = recv_message(sockfd);
	if (!status_msg)
	{
		close(sockfd);
		return -1;
	}

	// Processar mensagem de estado
	if (status_msg->opcode == MESSAGE_T__OPCODE__OP_BUSY)
	{
		fprintf(stderr, "Server busy. Try again later.\n");
		message_t__free_unpacked(status_msg, NULL);
		close(sockfd);
		return -1;
	}

	if (status_msg->opcode != MESSAGE_T__OPCODE__OP_READY)
	{
		message_t__free_unpacked(status_msg, NULL);
		close(sockfd);
		return -1;
	}

	message_t__free_unpacked(status_msg, NULL);
	rlist->sockfd = sockfd;
	return 0;
}

MessageT *network_send_receive(struct rlist_t *rlist, MessageT *msg)
{
	if (!rlist || rlist->sockfd < 0 || !msg) return NULL;

	size_t req_sz = message_t__get_packed_size(msg);
	uint8_t *req_buf = (uint8_t *)malloc(req_sz);
	if (!req_buf) return NULL;
	message_t__pack(msg, req_buf);

	uint32_t net_len = htonl((uint32_t)req_sz);
	if (send_all(rlist->sockfd, &net_len, sizeof(net_len)) < 0 ||
	    send_all(rlist->sockfd, req_buf, req_sz) < 0)
	{
		free(req_buf);
		return NULL;
	}
	free(req_buf);

	uint32_t net_ans_len = 0;
	if (recv_all(rlist->sockfd, &net_ans_len, sizeof(net_ans_len)) < 0)
	{
		return NULL;
	}
	uint32_t ans_len = ntohl(net_ans_len);
	if (ans_len == 0 || ans_len > (32u * 1024u * 1024u)) { return NULL; }

	uint8_t *ans_buf = (uint8_t *)malloc(ans_len);
	if (!ans_buf) return NULL;
	if (recv_all(rlist->sockfd, ans_buf, ans_len) < 0)
	{
		free(ans_buf);
		return NULL;
	}

	MessageT *ans = message_t__unpack(NULL, ans_len, ans_buf);
	free(ans_buf);
	return ans;
}

int network_close(struct rlist_t *rlist)
{
	if (!rlist || rlist->sockfd < 0) return -1;
	if (close(rlist->sockfd) < 0) return -1;
	rlist->sockfd = -1;
	return 0;
}
