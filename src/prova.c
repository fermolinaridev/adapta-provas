/* prova.c
 *
 * Implementacao do parser e das estruturas de prova. O formato de
 * arquivo aceito e um INI simplificado, com duas secoes: [PROVA] para
 * metadados e [QUESTAO] (uma por questao) com chaves como numero, tipo,
 * dificuldade, enunciado e A=, B=, C=... para alternativas. Linhas em
 * branco e comecadas por '#' sao tratadas como comentarios.
 *
 * Por que esse formato? E suficientemente expressivo para o problema,
 * trivial de digitar a mao em qualquer editor de texto e nao requer
 * bibliotecas externas para parsear.
 */

#include "prova.h"
#include "utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINHA_MAX 4096

/* ---------- helpers internos ---------- */

static void questao_destruir_interno(Questao *q)
{
    if (q == NULL) {
        return;
    }
    free(q->enunciado);
    q->enunciado = NULL;
    for (int i = 0; i < q->num_alternativas; i++) {
        free(q->alternativas[i].texto);
    }
    free(q->alternativas);
    q->alternativas     = NULL;
    q->num_alternativas = 0;
}

/* Divide "chave=valor" em duas strings apontando para o proprio buffer.
 * Retorna 1 em sucesso, 0 se nao houver '='. */
static int dividir_chave_valor(char *linha, char **chave, char **valor)
{
    char *eq = strchr(linha, '=');
    if (eq == NULL) {
        return 0;
    }
    *eq    = '\0';
    *chave = str_trim(linha);
    *valor = str_trim(eq + 1);
    return 1;
}

/* ---------- API publica ---------- */

Prova *prova_criar(void)
{
    Prova *p = (Prova *)calloc(1, sizeof(Prova));
    if (p == NULL) {
        return NULL;
    }
    p->capacidade = 8;
    p->questoes   = (Questao *)calloc((size_t)p->capacidade, sizeof(Questao));
    if (p->questoes == NULL) {
        free(p);
        return NULL;
    }
    return p;
}

void prova_destruir(Prova *p)
{
    if (p == NULL) {
        return;
    }
    free(p->titulo);
    free(p->disciplina);
    for (int i = 0; i < p->num_questoes; i++) {
        questao_destruir_interno(&p->questoes[i]);
    }
    free(p->questoes);
    free(p);
}

Questao *prova_adicionar_questao(Prova *p)
{
    if (p == NULL) {
        return NULL;
    }
    /* Cresce o vetor em potencias de 2 quando atinge a capacidade. */
    if (p->num_questoes >= p->capacidade) {
        int      nova_cap = p->capacidade * 2;
        Questao *nv       = (Questao *)realloc(p->questoes, (size_t)nova_cap * sizeof(Questao));
        if (nv == NULL) {
            return NULL;
        }
        memset(nv + p->capacidade, 0, (size_t)(nova_cap - p->capacidade) * sizeof(Questao));
        p->questoes   = nv;
        p->capacidade = nova_cap;
    }
    Questao *q = &p->questoes[p->num_questoes++];
    memset(q, 0, sizeof(*q));
    q->numero      = p->num_questoes; /* default: posicao de insercao. */
    q->tipo        = QUESTAO_OBJETIVA;
    q->dificuldade = 0;
    return q;
}

void questao_adicionar_alternativa(Questao *q, char letra, const char *texto)
{
    if (q == NULL) {
        return;
    }
    Alternativa *na =
        (Alternativa *)realloc(q->alternativas, (size_t)(q->num_alternativas + 1) * sizeof(Alternativa));
    if (na == NULL) {
        return;
    }
    q->alternativas                              = na;
    q->alternativas[q->num_alternativas].letra   = (char)toupper((unsigned char)letra);
    q->alternativas[q->num_alternativas].texto   = str_dup(texto != NULL ? texto : "");
    q->num_alternativas++;
}

int prova_carregar_arquivo(Prova *p, const char *caminho)
{
    if (p == NULL || caminho == NULL) {
        return 0;
    }
    FILE *f = fopen(caminho, "r");
    if (f == NULL) {
        return 0;
    }

    enum { SEC_NENHUMA, SEC_PROVA, SEC_QUESTAO } secao = SEC_NENHUMA;
    char     linha[LINHA_MAX];
    Questao *q_atual = NULL;

    while (fgets(linha, sizeof(linha), f) != NULL) {
        remover_quebra_linha(linha);
        char *lt = str_trim(linha);

        /* ignora linhas vazias e comentarios */
        if (*lt == '\0' || *lt == '#') {
            continue;
        }

        /* identifica secao [PROVA] / [QUESTAO] */
        size_t comp = strlen(lt);
        if (lt[0] == '[' && lt[comp - 1] == ']') {
            if (strcmp(lt, "[PROVA]") == 0) {
                secao   = SEC_PROVA;
                q_atual = NULL;
            } else if (strcmp(lt, "[QUESTAO]") == 0) {
                secao   = SEC_QUESTAO;
                q_atual = prova_adicionar_questao(p);
            } else {
                /* secao desconhecida: ignoramos silenciosamente. */
                secao   = SEC_NENHUMA;
                q_atual = NULL;
            }
            continue;
        }

        /* dentro de uma secao: linhas chave=valor */
        char *chave = NULL;
        char *valor = NULL;
        if (!dividir_chave_valor(lt, &chave, &valor)) {
            continue;
        }

        if (secao == SEC_PROVA) {
            if (strcmp(chave, "titulo") == 0) {
                free(p->titulo);
                p->titulo = str_dup(valor);
            } else if (strcmp(chave, "disciplina") == 0) {
                free(p->disciplina);
                p->disciplina = str_dup(valor);
            }
            /* Outras chaves de prova podem ser adicionadas aqui no futuro. */
        } else if (secao == SEC_QUESTAO && q_atual != NULL) {
            if (strcmp(chave, "numero") == 0) {
                q_atual->numero = atoi(valor);
            } else if (strcmp(chave, "tipo") == 0) {
                if (strcmp(valor, "discursiva") == 0) {
                    q_atual->tipo = QUESTAO_DISCURSIVA;
                } else {
                    q_atual->tipo = QUESTAO_OBJETIVA;
                }
            } else if (strcmp(chave, "dificuldade") == 0) {
                q_atual->dificuldade = atoi(valor);
            } else if (strcmp(chave, "enunciado") == 0) {
                free(q_atual->enunciado);
                q_atual->enunciado = str_dup(valor);
            } else if (strlen(chave) == 1 && isalpha((unsigned char)chave[0])) {
                /* Linhas A=, B=, C=... viram alternativas. */
                questao_adicionar_alternativa(q_atual, chave[0], valor);
            }
        }
    }

    fclose(f);
    /* Consideramos sucesso se conseguimos pelo menos um titulo OU uma questao. */
    return (p->num_questoes > 0 || p->titulo != NULL) ? 1 : 0;
}

void prova_exibir(const Prova *p, FILE *out)
{
    if (p == NULL || out == NULL) {
        return;
    }
    imprimir_separador(out, '=', 70);
    fprintf(out, "TITULO: %s\n", p->titulo != NULL ? p->titulo : "(sem titulo)");
    if (p->disciplina != NULL) {
        fprintf(out, "DISCIPLINA: %s\n", p->disciplina);
    }
    fprintf(out, "QUESTOES: %d\n", p->num_questoes);
    imprimir_separador(out, '=', 70);
    fputc('\n', out);

    for (int i = 0; i < p->num_questoes; i++) {
        const Questao *q = &p->questoes[i];
        fprintf(out, "Questao %d", q->numero);
        if (q->dificuldade > 0) {
            fprintf(out, " (dificuldade %d/5)", q->dificuldade);
        }
        fprintf(out, " [%s]\n", q->tipo == QUESTAO_DISCURSIVA ? "Discursiva" : "Objetiva");
        fprintf(out, "%s\n", q->enunciado != NULL ? q->enunciado : "");
        for (int j = 0; j < q->num_alternativas; j++) {
            fprintf(out, "  %c) %s\n", q->alternativas[j].letra,
                    q->alternativas[j].texto != NULL ? q->alternativas[j].texto : "");
        }
        fputc('\n', out);
    }
}
