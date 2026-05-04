/* utils.c
 *
 * Implementacoes das rotinas auxiliares de string declaradas em utils.h.
 * Sao deliberadamente simples e dependem apenas da biblioteca padrao
 * para que o programa compile com qualquer toolchain C99 (gcc, clang,
 * MSVC com /std:c99 ou mais recente).
 */

#include "utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *str_dup(const char *s)
{
    if (s == NULL) {
        return NULL;
    }
    size_t n = strlen(s) + 1;
    char  *r = (char *)malloc(n);
    if (r == NULL) {
        return NULL;
    }
    memcpy(r, s, n);
    return r;
}

char *str_trim(char *s)
{
    if (s == NULL) {
        return s;
    }
    /* Remove brancos do inicio movendo o conteudo para a esquerda. */
    char *ini = s;
    while (*ini != '\0' && isspace((unsigned char)*ini)) {
        ini++;
    }
    if (ini != s) {
        memmove(s, ini, strlen(ini) + 1);
    }
    /* Remove brancos do final colocando '\0' no lugar. */
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) {
        s[n - 1] = '\0';
        n--;
    }
    return s;
}

void remover_quebra_linha(char *s)
{
    if (s == NULL) {
        return;
    }
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[n - 1] = '\0';
        n--;
    }
}

int contar_palavras(const char *s)
{
    if (s == NULL) {
        return 0;
    }
    int total = 0;
    int dentro_palavra = 0;
    while (*s != '\0') {
        if (isspace((unsigned char)*s)) {
            if (dentro_palavra) {
                total++;
                dentro_palavra = 0;
            }
        } else {
            dentro_palavra = 1;
        }
        s++;
    }
    if (dentro_palavra) {
        total++;
    }
    return total;
}

char *str_substituir(const char *texto, const char *antiga, const char *nova)
{
    /* Casos triviais: retorna copia simples para que o caller sempre possa free. */
    if (texto == NULL) {
        return NULL;
    }
    if (antiga == NULL || *antiga == '\0' || nova == NULL) {
        return str_dup(texto);
    }

    size_t la = strlen(antiga);
    size_t ln = strlen(nova);
    size_t lt = strlen(texto);

    /* Conta ocorrencias para alocar exatamente o necessario. */
    size_t      ocorrencias = 0;
    const char *p           = texto;
    while ((p = strstr(p, antiga)) != NULL) {
        ocorrencias++;
        p += la;
    }

    /* Calculo do tamanho final evitando underflow quando ln < la. */
    size_t novo_tam;
    if (ln >= la) {
        novo_tam = lt + ocorrencias * (ln - la) + 1;
    } else {
        novo_tam = lt + 1; /* nunca cresce */
    }

    char *resultado = (char *)malloc(novo_tam);
    if (resultado == NULL) {
        return NULL;
    }

    char *dst = resultado;
    p         = texto;
    while (*p != '\0') {
        /* strncmp para com '\0' em qualquer dos lados, entao e seguro
         * mesmo perto do fim de 'p'. Se p tem menos que 'la' chars
         * restantes, strncmp retorna nao-zero corretamente. */
        if (strncmp(p, antiga, la) == 0) {
            memcpy(dst, nova, ln);
            dst += ln;
            p += la;
        } else {
            *dst++ = *p++;
        }
    }
    *dst = '\0';
    return resultado;
}

void imprimir_separador(FILE *out, char c, int n)
{
    for (int i = 0; i < n; i++) {
        fputc(c, out);
    }
    fputc('\n', out);
}
