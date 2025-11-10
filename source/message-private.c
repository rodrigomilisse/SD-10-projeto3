/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/
#include "message-private.h"
#include <unistd.h>
#include <errno.h>

int write_all(int sock, void *buf, int len) {
    if (buf == NULL || len <= 0) return 0;
    char *p = (char*)buf;
    int remaining = len;
    while (remaining > 0) {
        ssize_t n = write(sock, p, (size_t)remaining);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return 0;   //fechado
        p += n;
        remaining -= (int)n;
    }
    return len;
}

int read_all(int sock, void *buf, int len) {
    if (buf == NULL || len <= 0) return 0;
    char *p = (char*)buf;
    int remaining = len;
    while (remaining > 0) {
        ssize_t n = read(sock, p, (size_t)remaining);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return 0;       //peer fecha
        p += n;
        remaining -= (int)n;
    }
    return len;
}
