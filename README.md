/*
    Grupo 10
    Nuno Nobre 61823
    Márcio Milisse 61799
    Gonçalo Costa 61803
*/

# SD-10-projeto2

## Build Instructions

Run `make` to build both the client and server.

## Usage

### Server
```
./binary/list_server <port>
```

### Client
```
./binary/list_client <server>:<port>
```

Example:
```
./binary/list_client 127.0.0.1:8080
```

## Commands

- `add <ano> <preco> <marca> <modelo> <combustivel>`
- `remove <modelo>`
- `get_by_year <ano>`
- `get_by_marca <marca>`
- `get_list_by_year <ano>`
- `get_model_list`
- `get_list_ordered_by_year`
- `size`
- `quit`

## Clean

Run `make clean` to remove compiled files.