/* prova.h
 *
 * Estruturas de dados e operacoes para representar uma prova carregada
 * a partir de um arquivo de texto. As provas sao formadas por questoes
 * objetivas (com alternativas) ou discursivas (sem alternativas).
 *
 * Decisao de acessibilidade: separamos rigorosamente "estrutura" da
 * "apresentacao" da prova. As funcoes deste modulo nao tomam decisoes
 * sobre como adaptar a prova; isso e responsabilidade de perfis.[ch].
 * Assim, podemos adicionar novos perfis no futuro sem mexer no parser.
 */
#ifndef PROVA_H
#define PROVA_H

#include <stdio.h>

typedef enum {
    QUESTAO_OBJETIVA = 0,
    QUESTAO_DISCURSIVA
} TipoQuestao;

typedef struct {
    char  letra;  /* 'A', 'B', 'C'... mantemos sempre maiuscula. */
    char *texto;  /* Texto da alternativa (alocado dinamicamente). */
} Alternativa;

typedef struct {
    int          numero;            /* Numero original da questao na prova.    */
    TipoQuestao  tipo;              /* Objetiva ou discursiva.                 */
    char        *enunciado;         /* Texto do enunciado (alocado).           */
    Alternativa *alternativas;      /* Vetor de alternativas (apenas objetivas).*/
    int          num_alternativas;  /* Quantidade efetiva no vetor.            */
    int          dificuldade;       /* 1 a 5; 0 indica "nao informada".        */
} Questao;

typedef struct {
    char    *titulo;       /* Titulo livre, ex: "Prova de Ciencias - 7o ano". */
    char    *disciplina;   /* Nome da disciplina.                              */
    Questao *questoes;     /* Vetor dinamico.                                  */
    int      num_questoes; /* Tamanho usado.                                   */
    int      capacidade;   /* Capacidade alocada (cresce em potencias de 2).   */
} Prova;

/* Aloca uma prova vazia com vetor inicial pre-dimensionado. */
Prova *prova_criar(void);

/* Libera memoria da prova e de todos os campos internos. Aceita NULL. */
void prova_destruir(Prova *p);

/* Adiciona uma nova questao em branco no fim da prova e devolve ponteiro
 * para ela (ja com numero default = posicao). Retorna NULL em erro de alocacao. */
Questao *prova_adicionar_questao(Prova *p);

/* Adiciona uma alternativa ao final da questao. */
void questao_adicionar_alternativa(Questao *q, char letra, const char *texto);

/* Le uma prova a partir de um arquivo texto no formato INI simplificado.
 * Retorna 1 em sucesso (carregou pelo menos titulo OU questao) e 0 caso contrario. */
int prova_carregar_arquivo(Prova *p, const char *caminho);

/* Imprime a prova em 'out' em formato leve (sem decoracoes de perfil). */
void prova_exibir(const Prova *p, FILE *out);

/* Serializa a prova de volta para o formato INI (mesmo formato que
 * prova_carregar_arquivo aceita). Util para o usuario salvar uma prova
 * digitada interativamente e poder recarrega-la depois.
 * Retorna 1 em sucesso, 0 em erro. */
int prova_salvar_arquivo_ini(const Prova *p, const char *caminho);

#endif /* PROVA_H */
