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

# Atalho: compila e gera saidas adaptadas para varios perfis e graus a
# partir do mesmo arquivo de exemplo. Cada bloco de linhas equivale a:
#   2  -> escolher perfil
#   N  -> indice do perfil (1=TDAH, 2=TEA, 3=Down, 4=DI, 5=Dislexia, 6=Generico)
#   G  -> grau (1..5)
#   5  -> exportar
#   <caminho>
exemplo: $(BIN)
	@printf "1\nexemplos/prova_exemplo.txt\n\
2\n1\n3\n5\nexemplos/saida_tdah.txt\n\
2\n2\n3\n5\nexemplos/saida_tea.txt\n\
2\n5\n3\n5\nexemplos/saida_dislexia.txt\n\
2\n1\n5\n5\nexemplos/saida_tdah_grau5.txt\n\
0\n" | ./$(BIN) >/dev/null
	@echo "Geradas em exemplos/:"
	@echo "  - saida_tdah.txt        (TDAH grau 3)"
	@echo "  - saida_tea.txt         (TEA grau 3)"
	@echo "  - saida_dislexia.txt    (Dislexia grau 3)"
	@echo "  - saida_tdah_grau5.txt  (TDAH grau 5 - extremo)"
