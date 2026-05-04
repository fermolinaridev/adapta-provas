# Makefile do AdaptaProvas
#
# Compilacao com gcc/clang (C99 estrito). Sem dependencias externas alem
# da biblioteca padrao. Funciona em Linux, macOS e Windows (MinGW/MSYS2).
#
# Comandos:
#   make          -> compila o binario adapta_provas
#   make run      -> compila e executa
#   make clean    -> remove objetos e binario
#   make exemplo  -> compila e roda gerando saidas adaptadas de exemplo

CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -O2 -Iinclude
LDFLAGS :=

SRC_DIR := src
OBJ_DIR := build
BIN     := adapta_provas

SRCS    := $(SRC_DIR)/main.c $(SRC_DIR)/prova.c $(SRC_DIR)/perfis.c $(SRC_DIR)/utils.c
OBJS    := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Adiciona .exe automaticamente em Windows.
ifeq ($(OS),Windows_NT)
	BIN := $(BIN).exe
endif

.PHONY: all run clean exemplo

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(OBJ_DIR) $(BIN) adapta_provas adapta_provas.exe

# Atalho: compila e gera as duas saidas adaptadas a partir do exemplo.
# Usa redirecionamento via "echo" para automatizar o menu.
exemplo: $(BIN)
	@printf "1\nexemplos/prova_exemplo.txt\n2\n1\n5\nexemplos/saida_tdah.txt\n2\n2\n5\nexemplos/saida_tea.txt\n0\n" \
	  | ./$(BIN) >/dev/null
	@echo "Geradas: exemplos/saida_tdah.txt e exemplos/saida_tea.txt"
