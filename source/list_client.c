/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "data.h"
#include "client_stub.h"
#include "sdmessage.pb-c.h"
#include "network_client.h"


static void to_upper(char *s) {
    for (; s && *s; ++s)
        *s = (char)toupper((unsigned char)*s);
}


static struct data_t **rlist_get_all(struct rlist_t *rlist) {
    if (rlist == NULL)
        return NULL;

    MessageT msg;
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_GETLISTBYTEAR;  
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *ans = network_send_receive(rlist, &msg);
    if (ans == NULL)
        return NULL;

    if (ans->opcode != MESSAGE_T__OPCODE__OP_GETLISTBYTEAR + 1 ||
        ans->c_type  != MESSAGE_T__C_TYPE__CT_LIST) {
        message_t__free_unpacked(ans, NULL);
        return NULL;
    }

    size_t n = ans->n_cars;
    struct data_t **cars = malloc((n + 1) * sizeof(struct data_t *));
    if (!cars) {
        message_t__free_unpacked(ans, NULL);
        return NULL;
    }

    for (size_t i = 0; i < n; i++) {
        Data *src = ans->cars[i];
        struct data_t *d = malloc(sizeof(struct data_t));
        if (!d) {
            for (size_t j = 0; j < i; j++) {
                free(cars[j]->modelo);
                free(cars[j]);
            }
            free(cars);
            message_t__free_unpacked(ans, NULL);
            return NULL;
        }

        d->ano         = src->ano;
        d->preco       = src->preco;
        d->marca       = (enum marca_t)src->marca;
        d->combustivel = (enum combustivel_t)src->combustivel;
        d->modelo      = src->modelo ? strdup(src->modelo) : NULL;
        cars[i] = d;
    }
    cars[n] = NULL;

    message_t__free_unpacked(ans, NULL);
    return cars;
}

static int parse_marca(const char *s, enum marca_t *out) {
    if (!s || !out) return -1;

    char buf[64];
    snprintf(buf, sizeof(buf), "%s", s);
    to_upper(buf);

    if (strcmp(buf, "TOYOTA") == 0)
        *out = MARCA_TOYOTA;
    else if (strcmp(buf, "BMW") == 0)
        *out = MARCA_BMW;
    else if (strcmp(buf, "RENAULT") == 0)
        *out = MARCA_RENAULT;
    else if (strcmp(buf, "AUDI") == 0)
        *out = MARCA_AUDI;
    else if (strcmp(buf, "MERCEDES") == 0)
        *out = MARCA_MERCEDES;
    else
        return -1;

    return 0;
}

static int parse_comb(const char *s, enum combustivel_t *out) {
    if (!s || !out) return -1;

    char buf[64];
    snprintf(buf, sizeof(buf), "%s", s);
    to_upper(buf);

    if (strcmp(buf, "GASOLINA") == 0)
        *out = COMBUSTIVEL_GASOLINA;
    else if (strcmp(buf, "GASOLEO") == 0 || strcmp(buf, "DIESEL") == 0)
        *out = COMBUSTIVEL_GASOLEO;
    else if (strcmp(buf, "ELETRICO") == 0 || strcmp(buf, "ELECTRICO") == 0)
        *out = COMBUSTIVEL_ELETRICO;
    else if (strcmp(buf, "HIBRIDO") == 0 || strcmp(buf, "HÍBRIDO") == 0)
        *out = COMBUSTIVEL_HIBRIDO;
    else
        return -1;

    return 0;
}

static const char *m2s(enum marca_t m){
    switch(m){
      case MARCA_TOYOTA: return "Toyota";
      case MARCA_BMW: return "BMW";
      case MARCA_RENAULT: return "Renault";
      case MARCA_AUDI: return "Audi";
      case MARCA_MERCEDES: return "Mercedes";
      default: return "Desconhecida";
    }
}
static const char *c2s(enum combustivel_t c){
    switch(c){
      case COMBUSTIVEL_GASOLINA: return "Gasolina";
      case COMBUSTIVEL_GASOLEO:  return "Gasoleo";
      case COMBUSTIVEL_ELETRICO: return "Eletrico";
      case COMBUSTIVEL_HIBRIDO:  return "Hibrido";
      default: return "Desconhecido";
    }
}

static void print_car(const struct data_t *car) {
    if (!car) { puts("(null)"); return; }
     printf("%s %s %d %s %.2f\n",
        car->modelo, m2s(car->marca), car->ano, c2s(car->combustivel), car->preco);
}


static void show_help(void) {
    puts("Comandos:");
    puts("  add <ano> <preco> <marca> <modelo> <combustivel>");
    puts("  remove <modelo>");
    puts("  get_by_year <ano>");
    puts("  get_list_by_year <ano>   (alias de get_by_year)");
    puts("  get_by_marca <marca>");
    puts("  get_model_list");
    puts("  get_list_ordered_by_year");
    puts("  size");
    puts("  quit");
}


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <server>:<port>\n", argv[0]);
        return 1;
    }

    struct rlist_t *r = rlist_connect(argv[1]);
    if (!r) {
        fprintf(stderr, "Erro: rlist_connect falhou para '%s'\n", argv[1]);
        return 1;
    }

    puts("Ligado. Escreva 'help' para ajuda.");

    char line[1024];
    while(1) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\r\n")] = '\0';
        if (!*line) break;

        char *cmd = strtok(line, " ");
        if (!cmd) break;

        char cmdup[64]; 
        snprintf(cmdup, sizeof(cmdup), "%s", cmd);
        to_upper(cmdup);

        if (!strcmp(cmdup, "QUIT")) {
            break;
        }
        else if (!strcmp(cmdup, "HELP")) {
            show_help();
        }
        else if (!strcmp(cmdup, "SIZE")) {
            int s = rlist_size(r);
            if (s < 0) puts("ERRO");
            else       printf("%d\n", s);
        }
        else if (strcmp(cmdup, "ADD") == 0) {
            char *sano = strtok(NULL, " ");
            char *sprec = strtok(NULL, " ");
            char *smarca = strtok(NULL, " ");
            char *smodelo = strtok(NULL, " ");
            char *scomb = strtok(NULL, " ");

            if (!sano || !sprec || !smarca || !smodelo || !scomb) {
                puts("uso: add <ano> <preco> <marca> <modelo> <combustivel>");
                continue;
            }

            enum marca_t marca;
            enum combustivel_t comb;

            if (parse_marca(smarca, &marca) != 0 || parse_comb(scomb, &comb) != 0) {
                puts("ERRO: marca/combustivel inválidos");
                continue;
            }

            int ano = atoi(sano);
            float preco = (float)atof(sprec);

            struct data_t *car = data_create(ano, preco, marca, smodelo, comb);
            if (!car) { puts("ERRO: data_create"); continue; }

            int rc = rlist_add(r, car);
            data_destroy(car);
            puts(rc == 0 ? "OK" : "ERRO");
        }
        else if (!strcmp(cmdup, "REMOVE")) {
            char *modelo = strtok(NULL, "");
            if (!modelo) { puts("uso: remove <modelo>"); continue; }
            while (*modelo == ' ') modelo++;
            int rc = rlist_remove_by_model(r, modelo);
            if (rc == 0) puts("OK");
            else if (rc == 1) puts("NAO ENCONTRADO");
            else puts("ERRO");
        }
        else if (!strcmp(cmdup, "GET_BY_MARCA")) {
            char *smarca = strtok(NULL, " ");
            if (!smarca) { puts("uso: get_by_marca <marca>"); continue; }
            enum marca_t marca;
            if (parse_marca(smarca, &marca) != 0) { puts("ERRO: marca inválida"); continue; }
            struct data_t *c = rlist_get_by_marca(r, marca);
            if (!c) puts("ERRO ou NAO ENCONTRADO");
            else {
                print_car(c);
                free(c->modelo);
                free(c);
            }
        }
        else if (!strcmp(cmdup, "GET_BY_YEAR") || !strcmp(cmdup, "GET_LIST_BY_YEAR")) {
            char *sano = strtok(NULL, " ");
            if (!sano) { puts("uso: get_by_year <ano>"); continue; }
            int ano = atoi(sano);
            struct data_t **arr = rlist_get_by_year(r, ano);
            if (!arr) { puts("ERRO"); continue; }
            size_t n = 0;
            while (arr[n]) {
                print_car(arr[n]);
                free(arr[n]->modelo);
                free(arr[n]);
                n++;
            }
            free(arr);
            if (n == 0) puts("(sem resultados)");
        }
        else if (!strcmp(cmdup, "GET_MODEL_LIST")) {
            char **models = rlist_get_model_list(r);
            if (!models) { puts("ERRO"); continue; }
            for (size_t i = 0; models[i] != NULL; i++) {
                puts(models[i]);
                free(models[i]);
            }
            free(models);
        }
        else if (!strcmp(cmdup, "GET_LIST_ORDERED_BY_YEAR")) {
            int rc = rlist_order_by_year(r);
            if (rc != 0) { puts("ERRO"); continue; }

            struct data_t **carList = rlist_get_all(r);
            if (!carList) { puts("ERRO"); continue; }

            for (size_t i = 0; carList[i] != NULL; i++) {
                print_car(carList[i]);
            }

            puts("OK");

            for (size_t i = 0; carList[i] != NULL; i++) {
                free(carList[i]->modelo);
                free(carList[i]);
            }
            free(carList);
        }

        else {
            puts("Comando desconhecido. Escreva 'help'.");
        }
    }

    rlist_disconnect(r);
    return 0;
}
