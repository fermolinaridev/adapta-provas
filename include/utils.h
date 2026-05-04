/* utils.h
 *
 * Funcoes auxiliares de manipulacao de strings e console usadas por
 * todo o programa. Mantemos tudo em C99 puro, sem dependencias alem
 * da biblioteca padrao, para facilitar a portabilidade entre sistemas.
 */
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdio.h>

/* Aloca uma copia da string (substituto portatil para strdup, que e POSIX). */
char *str_dup(const char *s);

/* Remove espacos no inicio e fim da string IN-PLACE e devolve o ponteiro. */
char *str_trim(char *s);

/* Remove caracteres '\n' e '\r' do final da string IN-PLACE. */
void  remover_quebra_linha(char *s);

/* Conta palavras separadas por espacos em branco. */
int   contar_palavras(const char *s);

/* Substitui TODAS as ocorrencias de 'antiga' por 'nova' em 'texto'.
 * Retorna nova string alocada (caller chama free). */
char *str_substituir(const char *texto, const char *antiga, const char *nova);

/* Imprime n vezes o caractere c seguido de quebra de linha em 'out'. */
void  imprimir_separador(FILE *out, char c, int n);

#endif /* UTILS_H */
