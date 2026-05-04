/* main.c
 *
 * Ponto de entrada do AdaptaProvas. Implementa um menu interativo de
 * terminal (CLI) que orquestra os modulos prova.[ch] e perfis.[ch].
 *
 * Fluxo tipico de uso:
 *   1. Carregar prova .txt
 *   2. Escolher perfil de acessibilidade
 *   3. Visualizar prova adaptada na tela
 *   4. (opcional) Exportar a versao adaptada para outro arquivo .txt
 */

#include "perfis.h"
#include "prova.h"
#include "utils.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_TAM 1024

static void imprimir_cabecalho(void)
{
    printf("\n");
    imprimir_separador(stdout, '=', 70);
    printf("       AdaptaProvas - Adaptador de Provas Acessivel\n");
    printf("       Linguagem C / CLI - Versao 1.0\n");
    imprimir_separador(stdout, '=', 70);
}

static void imprimir_menu(const Prova *prova, const Perfil *perfil)
{
    printf("\n");
    imprimir_separador(stdout, '-', 70);
    printf("PROVA  : %s\n", prova != NULL && prova->titulo != NULL ? prova->titulo
                                                                  : "(nenhuma carregada)");
    printf("PERFIL : %s\n", perfil != NULL ? perfil->nome : "(nenhum selecionado)");
    imprimir_separador(stdout, '-', 70);
    printf(" 1) Carregar prova de arquivo .txt\n");
    printf(" 2) Escolher perfil de acessibilidade\n");
    printf(" 3) Visualizar prova original\n");
    printf(" 4) Visualizar prova adaptada\n");
    printf(" 5) Exportar prova adaptada para arquivo .txt\n");
    printf(" 6) Listar perfis disponiveis\n");
    printf(" 0) Sair\n");
    printf("Opcao: ");
    fflush(stdout);
}

/* Le uma linha de stdin para 'buf'. Retorna 1 em sucesso, 0 em EOF/erro. */
static int ler_linha(char *buf, size_t tam)
{
    if (fgets(buf, (int)tam, stdin) == NULL) {
        return 0;
    }
    remover_quebra_linha(buf);
    str_trim(buf);
    return 1;
}

static void acao_carregar(Prova **prova_atual)
{
    char caminho[BUF_TAM];
    printf("Caminho do arquivo (ex.: exemplos/prova_exemplo.txt): ");
    fflush(stdout);
    if (!ler_linha(caminho, sizeof(caminho)) || caminho[0] == '\0') {
        printf("[ERRO] Caminho vazio.\n");
        return;
    }

    Prova *novo = prova_criar();
    if (novo == NULL) {
        printf("[ERRO] Memoria insuficiente.\n");
        return;
    }
    if (!prova_carregar_arquivo(novo, caminho)) {
        printf("[ERRO] Nao foi possivel carregar a prova de '%s'.\n", caminho);
        prova_destruir(novo);
        return;
    }
    if (*prova_atual != NULL) {
        prova_destruir(*prova_atual);
    }
    *prova_atual = novo;
    printf("[OK] Prova carregada com %d questao(oes).\n", novo->num_questoes);
}

static void acao_escolher_perfil(const Perfil **perfil_atual)
{
    perfis_listar(stdout);
    printf("Numero do perfil: ");
    fflush(stdout);
    char buf[64];
    if (!ler_linha(buf, sizeof(buf)) || buf[0] == '\0') {
        printf("[ERRO] Entrada vazia.\n");
        return;
    }
    int idx = atoi(buf) - 1;
    if (idx < 0 || idx >= (int)PERFIL_TOTAL) {
        printf("[ERRO] Perfil invalido.\n");
        return;
    }
    *perfil_atual = perfis_obter((TipoPerfil)idx);
    printf("[OK] Perfil selecionado: %s\n", (*perfil_atual)->nome);
}

static void acao_visualizar_original(const Prova *prova)
{
    if (prova == NULL) {
        printf("[ERRO] Carregue uma prova primeiro (opcao 1).\n");
        return;
    }
    putchar('\n');
    prova_exibir(prova, stdout);
}

static void acao_visualizar_adaptada(const Prova *prova, const Perfil *perfil)
{
    if (prova == NULL) {
        printf("[ERRO] Carregue uma prova primeiro (opcao 1).\n");
        return;
    }
    if (perfil == NULL) {
        printf("[ERRO] Escolha um perfil primeiro (opcao 2).\n");
        return;
    }
    Prova *adaptada = adaptar_prova(prova, perfil);
    if (adaptada == NULL) {
        printf("[ERRO] Falha ao adaptar a prova.\n");
        return;
    }
    putchar('\n');
    prova_exibir_adaptada(adaptada, perfil, stdout);
    prova_destruir(adaptada);
}

static void acao_exportar(const Prova *prova, const Perfil *perfil)
{
    if (prova == NULL) {
        printf("[ERRO] Carregue uma prova primeiro (opcao 1).\n");
        return;
    }
    if (perfil == NULL) {
        printf("[ERRO] Escolha um perfil primeiro (opcao 2).\n");
        return;
    }
    char caminho[BUF_TAM];
    printf("Caminho do arquivo de saida (ex.: prova_adaptada.txt): ");
    fflush(stdout);
    if (!ler_linha(caminho, sizeof(caminho)) || caminho[0] == '\0') {
        printf("[ERRO] Caminho vazio.\n");
        return;
    }
    Prova *adaptada = adaptar_prova(prova, perfil);
    if (adaptada == NULL) {
        printf("[ERRO] Falha ao adaptar a prova.\n");
        return;
    }
    FILE *f = fopen(caminho, "w");
    if (f == NULL) {
        printf("[ERRO] Nao foi possivel criar '%s'.\n", caminho);
        prova_destruir(adaptada);
        return;
    }
    prova_exibir_adaptada(adaptada, perfil, f);
    fclose(f);
    prova_destruir(adaptada);
    printf("[OK] Prova adaptada salva em '%s'.\n", caminho);
}

int main(void)
{
    /* setlocale ajuda na exibicao de caracteres acentuados em consoles
     * que respeitam a configuracao do sistema. Mantemos os textos do
     * proprio programa em ASCII por seguranca multiplataforma. */
    setlocale(LC_ALL, "");

    Prova        *prova  = NULL;
    const Perfil *perfil = NULL;
    char          buf[BUF_TAM];

    imprimir_cabecalho();

    while (1) {
        imprimir_menu(prova, perfil);
        if (!ler_linha(buf, sizeof(buf))) {
            break; /* EOF */
        }
        if (buf[0] == '\0') {
            continue;
        }
        int opcao = atoi(buf);
        switch (opcao) {
            case 1:
                acao_carregar(&prova);
                break;
            case 2:
                acao_escolher_perfil(&perfil);
                break;
            case 3:
                acao_visualizar_original(prova);
                break;
            case 4:
                acao_visualizar_adaptada(prova, perfil);
                break;
            case 5:
                acao_exportar(prova, perfil);
                break;
            case 6:
                perfis_listar(stdout);
                break;
            case 0:
                if (prova != NULL) {
                    prova_destruir(prova);
                }
                printf("\nAte logo!\n");
                return 0;
            default:
                printf("[ERRO] Opcao invalida.\n");
        }
    }

    if (prova != NULL) {
        prova_destruir(prova);
    }
    return 0;
}
