# Grupo 10 
# Nuno Nobre 61823
# Márcio Milisse 61799
# Gonçalo Costa 61803

CC			:= gcc
CFLAGS		:= -std=c11 -Wall -Wextra -Werror -g -Iinclude -pthread
LDFLAGS		:= -lm -lprotobuf-c -pthread

SRC_DIR   	:= source
INC_DIR		:= include
OBJ_DIR		:= object
BIN_DIR		:= binary
LIB_DIR		:= lib

# Proto (se existir, gera automaticamente; senão usa os .c/.h já no include)
PROTO     := sdmessage.proto
PB_C      := $(SRC_DIR)/sdmessage.pb-c.c
PB_H      := $(INC_DIR)/sdmessage.pb-c.h
PB_OBJ    := $(OBJ_DIR)/sdmessage.pb-c.o

# === Ficheiros da biblioteca estática liblist.a ===
LIB_SRCS  := $(SRC_DIR)/data.c $(SRC_DIR)/list.c
LIB_OBJS  := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(LIB_SRCS))
LIB_A     := $(LIB_DIR)/liblist.a

# === Cliente ===
CLIENT_SRCS := $(SRC_DIR)/list_client.c $(SRC_DIR)/client_stub.c $(SRC_DIR)/network_client.c
CLIENT_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(CLIENT_SRCS)) $(PB_OBJ)
CLIENT_BIN  := $(BIN_DIR)/list_client

# === Servidor ===
SERVER_SRCS := $(SRC_DIR)/list_server.c $(SRC_DIR)/list_skel.c $(SRC_DIR)/network_server.c \
               $(SRC_DIR)/network_server-private.c $(SRC_DIR)/message-private.c $(SRC_DIR)/serialization.c $(SRC_DIR)/serialization-private.c
SERVER_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SERVER_SRCS)) $(PB_OBJ)
SERVER_BIN  := $(BIN_DIR)/list_server

.PHONY: all
all: protobuf liblist list_client list_server 

.PHONY: protobuf
protobuf: $(PB_C) $(PB_H)

.PHONY: liblist
liblist: dirs $(LIB_A)
	@echo "Biblioteca criada: $(LIB_A)"

$(LIB_A): $(LIB_OBJS) | dirs
	@ar -rcs $@ $^

libobjs: dirs $(LIB_OBJS)
	@echo "Biblioteca objects criados"

# === list_client ===
.PHONY: list_client protobuf
list_client: dirs protobuf $(CLIENT_BIN) 
	@echo "Cliente compilado: $(CLIENT_BIN)"

$(CLIENT_BIN): $(CLIENT_OBJS) $(LIB_A) | dirs
	@$(CC) $(CLIENT_OBJS) -L$(LIB_DIR) -llist $(LDFLAGS) -o $@

# === list_server ===
.PHONY: list_server protobuf
list_server: dirs protobuf $(SERVER_BIN)
	@echo "Servidor compilado: $(SERVER_BIN)"

$(SERVER_BIN): $(SERVER_OBJS) $(LIB_A) | dirs
	@$(CC) $(SERVER_OBJS) -L$(LIB_DIR) -llist $(LDFLAGS) -o $@

# Regra geral de compilação (.c -> .o) 
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Compilar o ficheiro gerado pelo protobuf-c
$(OBJ_DIR)/sdmessage.pb-c.o: $(PB_C) $(PB_H) | dirs
	@$(CC) $(CFLAGS) -MMD -MP -c $(PB_C) -o $@

# === Geração do protobuf (se existir o .proto) ===
# Se sdmessage.proto existir, gera os ficheiros no source/ e depois copia o .h para include/
$(PB_C) $(PB_H): $(PROTO)
	protoc-c --c_out=$(SRC_DIR) $(PROTO)
	@mv $(SRC_DIR)/sdmessage.pb-c.h $(INC_DIR)/sdmessage.pb-c.h
	@echo "Gerados: $(PB_C) e $(PB_H)"

.PHONY: dirs
dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)

.PHONY: clean
clean:
	@$(RM) -r $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)
	@$(RM) -f $(PB_C) $(PB_H)
	@echo "Limpo"

# Dependências automáticas 
DEPS := $(wildcard $(OBJ_DIR)/*.d)
-include $(DEPS)
