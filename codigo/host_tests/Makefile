# Tests de host para el TP5 (Etapas 1 y 2).
# Compila las funciones del protocolo en la PC (sin FreeRTOS ni hardware)
# para verificar que generan las tramas correctas antes de flashear la placa.
#
# Uso:
#   make run      compila y corre los dos tests
#   make clean    borra los ejecutables
#
# Se usa '>' como prefijo de receta (en vez de TAB) para evitar problemas
# de copiado entre Windows y Linux.
.RECIPEPREFIX := >

CC     = gcc
CFLAGS = -Wall -Wextra -I../firmware/protocol -I../firmware/config
SRC    = ../firmware/protocol/protocol.c ../firmware/protocol/parser.c

all: test_protocol test_parser

test_protocol: test_protocol.c $(SRC)
> $(CC) $(CFLAGS) $^ -o $@

test_parser: test_parser.c $(SRC)
> $(CC) $(CFLAGS) $^ -o $@

run: all
> @echo "===== Etapa 1: protocol (encode / checksum / decode) ====="
> ./test_protocol
> @echo ""
> @echo "===== Etapa 2: parser (FSM byte a byte) ====="
> ./test_parser

clean:
> rm -f test_protocol test_parser
