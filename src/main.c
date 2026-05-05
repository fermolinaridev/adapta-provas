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

#include <ctype.h>
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
    const char *rotulo_prova;
    if (prova == NULL) {
        rotulo_prova = "(nenhuma carregada)";
    } else if (prova->titulo != NULL) {
        rotulo_prova = prova->titulo;
    } else {
        rotulo_prova = "(prova sem titulo)";
    }
    printf("\n");
    imprimir_separador(stdout, '-', 70);
    printf("PROVA  : %s\n", rotulo_prova);
    if (perfil != NULL) {
        printf("PERFIL : %s (grau %d - %s)\n", perfil->nome, perfil->grau,
               perfis_descricao_grau(perfil->grau));
    } else {
        printf("PERFIL : (nenhum selecionado)\n");
    }
    imprimir_separador(stdout, '-', 70);
    printf(" --- entrada ---\n");
    printf(" 1) Carregar prova de arquivo .txt\n");
    printf(" 2) Digitar uma nova prova (interativo, passo a passo)\n");
    printf(" 3) Colar prova de texto livre (de PDF/Word)\n");
    printf(" --- adaptacao ---\n");
    printf(" 4) Escolher perfil de acessibilidade + grau\n");
    printf(" 5) Visualizar prova original\n");
    printf(" 6) Visualizar prova adaptada\n");
    printf(" --- saida ---\n");
    printf(" 7) Exportar prova adaptada para .txt\n");
    printf(" 8) Salvar prova original em .txt (formato editavel)\n");
    printf(" 9) Listar perfis disponiveis\n");
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

/* ========================================================================
 * Parser heuristico para texto livre colado de PDF/Word.
 * Detecta inicio de questao ("1.", "2)", "Questao 3", "Q4)", "Pergunta 5:")
 * e inicio de alternativa ("a)", "(a)", "A.", "B)").
 * ====================================================================== */

/* Concatena 'extra' ao final de 'base' (com um espaco no meio).
 * Realloca 'base' e devolve o novo ponteiro. Se base eh NULL, devolve copia. */
static char *concat_com_espaco(char *base, const char *extra)
{
    if (extra == NULL || *extra == '\0') {
        return base;
    }
    if (base == NULL) {
        return str_dup(extra);
    }
    size_t lb   = strlen(base);
    size_t le   = strlen(extra);
    char  *novo = (char *)realloc(base, lb + le + 2);
    if (novo == NULL) {
        return base;
    }
    novo[lb]                  = ' ';
    memcpy(novo + lb + 1, extra, le + 1);
    return novo;
}

/* Tenta detectar inicio de questao na linha 'l'. Aceita formatos:
 *   "1." | "1)" | "1-" | "1:"          (com ate 3 digitos)
 *   "Q1)" | "Questao 1." | "Pergunta 2:"
 * Em sucesso, devolve 1 e preenche *numero_out e *resto_out (apontando
 * para o conteudo apos o separador). */
static int detectar_inicio_questao(const char *l, int *numero_out, const char **resto_out)
{
    while (*l != '\0' && isspace((unsigned char)*l)) {
        l++;
    }
    const char *p = l;

    /* Pula prefixo opcional "Q", "Questao", "Quest", "Pergunta". */
    if (toupper((unsigned char)p[0]) == 'Q' || toupper((unsigned char)p[0]) == 'P') {
        const char *q = p;
        while (*q != '\0' && isalpha((unsigned char)*q)) {
            q++;
        }
        /* Aceita o prefixo se a palavra tem entre 1 e 9 letras. */
        size_t len = (size_t)(q - p);
        if (len >= 1 && len <= 9) {
            p = q;
            while (*p != '\0' && isspace((unsigned char)*p)) {
                p++;
            }
        }
    }

    if (!isdigit((unsigned char)*p)) {
        return 0;
    }
    int n      = 0;
    int digits = 0;
    while (isdigit((unsigned char)*p) && digits < 3) {
        n = n * 10 + (*p - '0');
        p++;
        digits++;
    }
    if (n < 1) {
        return 0;
    }

    /* Pula brancos antes do separador. */
    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }
    if (*p != '.' && *p != ')' && *p != '-' && *p != ':') {
        return 0;
    }
    p++;
    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }
    *numero_out = n;
    *resto_out  = p;
    return 1;
}

/* Tenta detectar inicio de alternativa. Aceita "a)", "(a)", "a.", "A)" etc.
 * Letras validas: A a J (raramente uma prova passa disso). */
static int detectar_alternativa(const char *l, char *letra_out, const char **resto_out)
{
    while (*l != '\0' && isspace((unsigned char)*l)) {
        l++;
    }
    const char *p     = l;
    char        letra = 0;

    if (*p == '(' && isalpha((unsigned char)p[1]) && p[2] == ')') {
        letra = (char)toupper((unsigned char)p[1]);
        p += 3;
    } else if (isalpha((unsigned char)*p) && (p[1] == ')' || p[1] == '.')) {
        letra = (char)toupper((unsigned char)*p);
        p += 2;
    } else {
        return 0;
    }
    if (letra < 'A' || letra > 'J') {
        return 0;
    }
    /* Exigir um espaco apos o separador para evitar confundir com palavras
     * como "a." sendo abreviacao em meio a frase. */
    if (*p != ' ' && *p != '\t') {
        return 0;
    }
    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }
    *letra_out = letra;
    *resto_out = p;
    return 1;
}

/* Modo "colar prova": le linhas ate o usuario digitar "FIM" e tenta
 * estruturar automaticamente. Funciona bem para texto copiado de PDF/Word
 * com numeracao convencional. */
static void acao_colar_prova(Prova **prova_atual)
{
    char buf[BUF_TAM];

    Prova *novo = prova_criar();
    if (novo == NULL) {
        printf("[ERRO] Memoria insuficiente.\n");
        return;
    }

    printf("\n=== Colar prova de texto livre (PDF/Word convertido) ===\n");
    printf("Titulo da prova (opcional): ");
    fflush(stdout);
    if (ler_linha(buf, sizeof(buf)) && buf[0] != '\0') {
        free(novo->titulo);
        novo->titulo = str_dup(buf);
    }
    printf("Disciplina (opcional): ");
    fflush(stdout);
    if (ler_linha(buf, sizeof(buf)) && buf[0] != '\0') {
        free(novo->disciplina);
        novo->disciplina = str_dup(buf);
    }

    printf("\nAgora cole o texto da prova abaixo. Suporta os formatos:\n");
    printf("  Questoes:    '1.' '2)' 'Q3)' 'Questao 4:' 'Pergunta 5-'\n");
    printf("  Alternativas: 'a)' '(a)' 'A.' 'B)'\n");
    printf("Digite 'FIM' (em linha unica) para encerrar.\n\n");

    Questao *q          = NULL;
    char    *enun_buf   = NULL;
    char    *alt_buf    = NULL;
    char     letra_alt  = 0;
    enum { EST_NADA, EST_ENUN, EST_ALT } estado = EST_NADA;

    while (ler_linha(buf, sizeof(buf))) {
        if (strcmp(buf, "FIM") == 0 || strcmp(buf, "###") == 0
            || strcmp(buf, "fim") == 0) {
            break;
        }
        if (buf[0] == '\0') {
            continue;
        }

        int         num   = 0;
        const char *resto = NULL;

        /* tenta inicio de questao primeiro */
        if (detectar_inicio_questao(buf, &num, &resto)) {
            /* fecha alternativa pendente */
            if (estado == EST_ALT && q != NULL && alt_buf != NULL) {
                questao_adicionar_alternativa(q, letra_alt, alt_buf);
            }
            free(alt_buf);
            alt_buf   = NULL;
            letra_alt = 0;

            /* fecha enunciado pendente */
            if (q != NULL && enun_buf != NULL) {
                free(q->enunciado);
                q->enunciado = enun_buf;
                enun_buf     = NULL;
            }

            q = prova_adicionar_questao(novo);
            if (q == NULL) {
                break;
            }
            q->numero = num;
            q->tipo   = QUESTAO_OBJETIVA;
            enun_buf  = str_dup(resto);
            estado    = EST_ENUN;
            continue;
        }

        /* tenta alternativa (so se ja temos questao) */
        char letra = 0;
        if (q != NULL && detectar_alternativa(buf, &letra, &resto)) {
            /* fecha alternativa anterior */
            if (estado == EST_ALT && alt_buf != NULL) {
                questao_adicionar_alternativa(q, letra_alt, alt_buf);
                free(alt_buf);
                alt_buf = NULL;
            }
            /* fecha enunciado se ainda estava acumulando */
            if (estado == EST_ENUN && enun_buf != NULL) {
                free(q->enunciado);
                q->enunciado = enun_buf;
                enun_buf     = NULL;
            }
            letra_alt = letra;
            alt_buf   = str_dup(resto);
            estado    = EST_ALT;
            continue;
        }

        /* linha de continuacao - acumula no que estiver aberto */
        if (estado == EST_ENUN) {
            enun_buf = concat_com_espaco(enun_buf, buf);
        } else if (estado == EST_ALT) {
            alt_buf = concat_com_espaco(alt_buf, buf);
        }
        /* se EST_NADA, ignoramos (texto antes da primeira questao) */
    }

    /* finaliza pendencias */
    if (estado == EST_ALT && q != NULL && alt_buf != NULL) {
        questao_adicionar_alternativa(q, letra_alt, alt_buf);
    }
    free(alt_buf);
    if (q != NULL && enun_buf != NULL) {
        free(q->enunciado);
        q->enunciado = enun_buf;
    } else {
        free(enun_buf);
    }

    /* questoes sem alternativas ficam como discursivas. */
    for (int i = 0; i < novo->num_questoes; i++) {
        if (novo->questoes[i].num_alternativas == 0) {
            novo->questoes[i].tipo = QUESTAO_DISCURSIVA;
        }
    }

    if (novo->num_questoes == 0) {
        printf("[INFO] Nenhuma questao detectada. Prova descartada.\n");
        prova_destruir(novo);
        return;
    }

    printf("\n[OK] Detectadas %d questao(oes):\n", novo->num_questoes);
    for (int i = 0; i < novo->num_questoes; i++) {
        const Questao *qi = &novo->questoes[i];
        printf("  - Q%d (%s, %d alternativa(s))\n", qi->numero,
               qi->tipo == QUESTAO_DISCURSIVA ? "discursiva" : "objetiva",
               qi->num_alternativas);
    }

    printf("\nDefinir dificuldade (1-5) de cada questao? (s/N): ");
    fflush(stdout);
    if (ler_linha(buf, sizeof(buf)) && (buf[0] == 's' || buf[0] == 'S')) {
        for (int i = 0; i < novo->num_questoes; i++) {
            printf("  Questao %d - dificuldade [1-5, ENTER ignora]: ",
                   novo->questoes[i].numero);
            fflush(stdout);
            if (ler_linha(buf, sizeof(buf)) && buf[0] != '\0') {
                int d = atoi(buf);
                if (d >= 1 && d <= 5) {
                    novo->questoes[i].dificuldade = d;
                }
            }
        }
    }

    if (*prova_atual != NULL) {
        prova_destruir(*prova_atual);
    }
    *prova_atual = novo;
    printf("\n[OK] Prova carregada.\n");

    printf("Salvar em arquivo .txt para reutilizar? (caminho ou ENTER para nao): ");
    fflush(stdout);
    if (ler_linha(buf, sizeof(buf)) && buf[0] != '\0') {
        if (prova_salvar_arquivo_ini(novo, buf)) {
            printf("[OK] Prova salva em '%s'.\n", buf);
        } else {
            printf("[ERRO] Nao foi possivel salvar.\n");
        }
    }
}

/* Cria uma prova nova guiando o usuario passo a passo no terminal.
 * Permite ao usuario digitar a propria prova sem precisar montar um .txt
 * no formato INI antes. No final, oferece salvar em arquivo. */
static void acao_criar_prova_interativa(Prova **prova_atual)
{
    char buf[BUF_TAM];

    Prova *novo = prova_criar();
    if (novo == NULL) {
        printf("[ERRO] Memoria insuficiente.\n");
        return;
    }

    printf("\n=== Criando nova prova passo a passo ===\n");
    printf("(Pressione ENTER em branco em qualquer pergunta para encerrar a etapa atual)\n\n");

    printf("Titulo da prova: ");
    fflush(stdout);
    if (ler_linha(buf, sizeof(buf)) && buf[0] != '\0') {
        free(novo->titulo);
        novo->titulo = str_dup(buf);
    }

    printf("Disciplina (opcional): ");
    fflush(stdout);
    if (ler_linha(buf, sizeof(buf)) && buf[0] != '\0') {
        free(novo->disciplina);
        novo->disciplina = str_dup(buf);
    }

    int numero = 1;
    while (1) {
        printf("\n--- Questao %d ---\n", numero);
        printf("Tipo (o=objetiva, d=discursiva, ENTER em branco encerra): ");
        fflush(stdout);
        if (!ler_linha(buf, sizeof(buf)) || buf[0] == '\0') {
            break;
        }

        Questao *q = prova_adicionar_questao(novo);
        if (q == NULL) {
            printf("[ERRO] Falha ao alocar nova questao.\n");
            break;
        }
        q->numero = numero;
        q->tipo   = (buf[0] == 'd' || buf[0] == 'D') ? QUESTAO_DISCURSIVA : QUESTAO_OBJETIVA;

        printf("Enunciado: ");
        fflush(stdout);
        if (ler_linha(buf, sizeof(buf)) && buf[0] != '\0') {
            free(q->enunciado);
            q->enunciado = str_dup(buf);
        }

        printf("Dificuldade (1=facil ... 5=dificil, ENTER ignora): ");
        fflush(stdout);
        if (ler_linha(buf, sizeof(buf)) && buf[0] != '\0') {
            int d = atoi(buf);
            if (d >= 1 && d <= 5) {
                q->dificuldade = d;
            }
        }

        if (q->tipo == QUESTAO_OBJETIVA) {
            printf("Alternativas (texto da alternativa; ENTER em branco encerra):\n");
            char letra = 'A';
            while (letra <= 'Z') {
                printf("  %c) ", letra);
                fflush(stdout);
                if (!ler_linha(buf, sizeof(buf)) || buf[0] == '\0') {
                    break;
                }
                questao_adicionar_alternativa(q, letra, buf);
                letra++;
            }
            if (q->num_alternativas == 0) {
                printf("  (nenhuma alternativa cadastrada - questao mantida assim mesmo)\n");
            }
        }
        numero++;
    }

    if (novo->num_questoes == 0 && novo->titulo == NULL && novo->disciplina == NULL) {
        printf("[INFO] Nenhum dado fornecido. Prova descartada.\n");
        prova_destruir(novo);
        return;
    }

    if (*prova_atual != NULL) {
        prova_destruir(*prova_atual);
    }
    *prova_atual = novo;
    printf("\n[OK] Prova criada com %d questao(oes).\n", novo->num_questoes);

    /* Oferta de salvar em arquivo .txt para reutilizacao futura. */
    printf("Salvar a prova em arquivo .txt? (caminho ou ENTER para nao salvar): ");
    fflush(stdout);
    if (ler_linha(buf, sizeof(buf)) && buf[0] != '\0') {
        if (prova_salvar_arquivo_ini(novo, buf)) {
            printf("[OK] Prova salva em '%s'.\n", buf);
        } else {
            printf("[ERRO] Nao foi possivel salvar em '%s'.\n", buf);
        }
    }
}

/* Salva a prova ORIGINAL atualmente carregada em formato INI editavel,
 * permitindo ao usuario versao-la, editar manualmente, etc. */
static void acao_salvar_original(const Prova *prova)
{
    if (prova == NULL) {
        printf("[ERRO] Carregue ou digite uma prova primeiro (opcao 1 ou 2).\n");
        return;
    }
    char caminho[BUF_TAM];
    printf("Caminho de saida (.txt) para a prova ORIGINAL: ");
    fflush(stdout);
    if (!ler_linha(caminho, sizeof(caminho)) || caminho[0] == '\0') {
        printf("[ERRO] Caminho vazio.\n");
        return;
    }
    if (prova_salvar_arquivo_ini(prova, caminho)) {
        printf("[OK] Prova original salva em '%s'.\n", caminho);
    } else {
        printf("[ERRO] Nao foi possivel salvar em '%s'.\n", caminho);
    }
}

/* Pergunta ao usuario o grau (1-5) de adaptacao a aplicar.
 * Retorna o grau ja "clampado" em [GRAU_MIN, GRAU_MAX]; se a entrada for
 * vazia ou invalida, devolve GRAU_PADRAO. */
static int perguntar_grau(void)
{
    printf("\nGrau de adaptacao (1=leve ... 5=extremo) [padrao=%d]: ", GRAU_PADRAO);
    fflush(stdout);
    char buf[32];
    if (!ler_linha(buf, sizeof(buf)) || buf[0] == '\0') {
        return GRAU_PADRAO;
    }
    int g = atoi(buf);
    if (g < GRAU_MIN) g = GRAU_MIN;
    if (g > GRAU_MAX) g = GRAU_MAX;
    return g;
}

/* Escolhe o perfil base + grau e armazena o resultado composto em
 * '*perfil_aplicado', sinalizando 'definido = 1' em caso de sucesso. */
static void acao_escolher_perfil(Perfil *perfil_aplicado, int *definido)
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
    const Perfil *base = perfis_obter((TipoPerfil)idx);
    int grau = perguntar_grau();
    *perfil_aplicado = perfis_compor(base, grau);
    *definido = 1;
    printf("[OK] Perfil selecionado: %s (grau %d - %s)\n", perfil_aplicado->nome,
           perfil_aplicado->grau, perfis_descricao_grau(perfil_aplicado->grau));
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

    Prova *prova            = NULL;
    Perfil perfil_aplicado;            /* perfil base + grau ja aplicado     */
    int    perfil_definido  = 0;       /* 1 quando o usuario escolheu perfil */
    char   buf[BUF_TAM];

    imprimir_cabecalho();

    while (1) {
        imprimir_menu(prova, perfil_definido ? &perfil_aplicado : NULL);
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
                acao_criar_prova_interativa(&prova);
                break;
            case 3:
                acao_colar_prova(&prova);
                break;
            case 4:
                acao_escolher_perfil(&perfil_aplicado, &perfil_definido);
                break;
            case 5:
                acao_visualizar_original(prova);
                break;
            case 6:
                acao_visualizar_adaptada(prova,
                                         perfil_definido ? &perfil_aplicado : NULL);
                break;
            case 7:
                acao_exportar(prova,
                              perfil_definido ? &perfil_aplicado : NULL);
                break;
            case 8:
                acao_salvar_original(prova);
                break;
            case 9:
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
