/* perfis.h
 *
 * Perfis de acessibilidade e regras de adaptacao. Cada perfil e um
 * conjunto de "switches" booleanos e parametros numericos que dizem ao
 * adaptador o que ligar ou desligar (simplificar linguagem, dividir
 * frases longas, sugerir tempo, etc).
 *
 * Decisao de design: as regras NAO estao codificadas via if-else
 * espalhados pelo codigo. Cada bandeira do struct Perfil habilita uma
 * transformacao independente em adaptar_prova(). Para criar um novo
 * perfil basta acrescentar uma entrada na tabela PERFIS em perfis.c -
 * sem tocar no main, no parser ou no exibidor.
 */
#ifndef PERFIS_H
#define PERFIS_H

#include "prova.h"

#include <stdio.h>

typedef enum {
    PERFIL_TDAH = 0,
    PERFIL_TEA,
    PERFIL_DOWN,
    PERFIL_DI_LEVE,
    PERFIL_DISLEXIA,
    PERFIL_GENERICO,
    PERFIL_TOTAL  /* sentinela: numero de perfis */
} TipoPerfil;

/* Grau de adaptacao aplicado sobre um perfil base.
 *   1 = leve     (ajuste minimo, mais proximo da prova original)
 *   2 = leve a moderado
 *   3 = moderado (valor padrao da tabela)
 *   4 = moderado a severo
 *   5 = extremo  (todas as adaptacoes ligadas, frases minimas, tempo dobrado)
 */
#define GRAU_MIN 1
#define GRAU_MAX 5
#define GRAU_PADRAO 3

typedef struct {
    TipoPerfil  tipo;
    const char *nome;
    const char *descricao;

    /* Bandeiras (1 = ativa, 0 = desliga). */
    int simplificar_linguagem;   /* substituir vocabulario dificil          */
    int linguagem_literal;       /* trocar metaforas/idiomatismos por literal*/
    int dividir_frases_longas;   /* quebrar em '\n' apos virgulas em frases longas */
    int destacar_verbos_acao;    /* deixar EXPLIQUE/MARQUE/etc em CAIXA-ALTA */
    int sugerir_tempo;           /* imprimir tempo sugerido por questao     */
    int ordenar_por_dificuldade; /* exibir das mais simples para as complexas*/
    int uma_questao_por_tela;    /* separadores fortes entre questoes        */
    int reduzir_distracoes;      /* remover frases de enchimento             */
    int adicionar_dicas;         /* incluir dicas de leitura                 */

    /* Parametros numericos. */
    int    max_palavras_frase;  /* limite que dispara a quebra em virgulas  */
    int    tempo_base_minutos;  /* base de tempo por questao objetiva       */
    double fator_tempo_extra;   /* multiplicador final do tempo sugerido    */

    /* Grau efetivo (1-5). Os valores acima representam o GRAU_PADRAO; o
     * grau e ajustado em runtime por perfis_compor(). */
    int grau;
} Perfil;

/* Devolve ponteiro para o perfil BASE de um dado tipo (grau padrao = 3).
 * Retorna NULL se 'tipo' for invalido. */
const Perfil *perfis_obter(TipoPerfil tipo);

/* Lista todos os perfis disponiveis em 'out', um por linha, numerados. */
void perfis_listar(FILE *out);

/* Devolve uma descricao curta do grau (ex.: "leve", "moderado", "extremo"). */
const char *perfis_descricao_grau(int grau);

/* Compoe um perfil base com um grau (1-5) e devolve uma struct Perfil
 * AJUSTADA. Em graus baixos algumas adaptacoes sao desligadas; em graus
 * altos os parametros se intensificam (frases mais curtas, mais tempo,
 * uma questao por tela, etc). Grau fora de [1,5] e clampado. */
Perfil perfis_compor(const Perfil *base, int grau);

/* Cria uma NOVA prova adaptada a partir da original e do perfil
 * (ja composto com grau). O caller chama prova_destruir() no resultado. */
Prova *adaptar_prova(const Prova *original, const Perfil *perfil);

/* Imprime a prova ja adaptada com decoracoes apropriadas ao perfil
 * (cabecalho destacado, tempo sugerido, separadores, dicas). */
void prova_exibir_adaptada(const Prova *p, const Perfil *perfil, FILE *out);

#endif /* PERFIS_H */
