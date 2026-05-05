/* perfis.c
 *
 * Implementacao das regras de adaptacao por perfil de acessibilidade.
 *
 * O fluxo e sempre o mesmo: adaptar_prova() recebe uma prova original e
 * um perfil, cria uma copia profunda e aplica em cada enunciado uma
 * pipeline de transformacoes condicionais. Cada transformacao consulta
 * apenas as bandeiras do perfil; assim, alterar o comportamento de um
 * perfil reduz-se a editar a tabela PERFIS abaixo - nao precisamos
 * tocar nas funcoes de transformacao.
 *
 * Considermos importante deixar registrado AQUI o porque de cada regra,
 * para que esta logica sirva tambem como documentacao pedagogica:
 *
 *  - TDAH: a literatura aponta dificuldade em sustentar atencao em
 *          textos longos. Reduzir distracoes, destacar a acao pedida,
 *          dividir frases e sugerir tempo objetivo ajudam o aluno a
 *          se manter na tarefa.
 *
 *  - TEA: linguagem literal e estrutura previsivel sao recomendacoes
 *         classicas. Por isso desligamos a reordenacao por dificuldade
 *         (mantendo a ordem original = previsibilidade) e desligamos
 *         dicas (que podem confundir) mas ligamos o dicionario literal.
 *
 *  - Sindrome de Down: vocabulario simples, frases muito curtas e
 *                      tempo extra mais generoso (fator 2,0). Liga
 *                      reordenacao para comecar pelas mais simples.
 *
 *  - DI leve: similar ao Down, mas com frases um pouco mais longas e
 *             menos tempo extra; mantemos dicas e exemplos concretos.
 *
 *  - Generico: combinacao moderada para casos nao especificados.
 */

#include "perfis.h"
#include "prova.h"
#include "utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Tabela de perfis. Os valores aqui representam o GRAU PADRAO (3 -
 * moderado). A funcao perfis_compor() ajusta esses valores de acordo
 * com o grau real escolhido pelo usuario (1 a 5).
 * Adicionar um novo perfil = nova linha aqui + novo enum em perfis.h.
 * ============================================================ */
static const Perfil PERFIS[PERFIL_TOTAL] = {
    {
        .tipo                    = PERFIL_TDAH,
        .nome                    = "TDAH",
        .descricao               = "Transtorno do Deficit de Atencao e Hiperatividade",
        .simplificar_linguagem   = 1,
        .linguagem_literal       = 0,
        .dividir_frases_longas   = 1,
        .destacar_verbos_acao    = 1,
        .sugerir_tempo           = 1,
        .ordenar_por_dificuldade = 1,
        .uma_questao_por_tela    = 1,
        .reduzir_distracoes      = 1,
        .adicionar_dicas         = 1,
        .max_palavras_frase      = 18,
        .tempo_base_minutos      = 4,
        .fator_tempo_extra       = 1.50,
        .grau                    = GRAU_PADRAO,
    },
    {
        .tipo                    = PERFIL_TEA,
        .nome                    = "TEA (Autismo)",
        .descricao               = "Transtorno do Espectro Autista - linguagem literal e estrutura previsivel",
        .simplificar_linguagem   = 1,
        .linguagem_literal       = 1,
        .dividir_frases_longas   = 1,
        .destacar_verbos_acao    = 1,
        .sugerir_tempo           = 1,
        .ordenar_por_dificuldade = 0, /* mantem ordem original = previsivel */
        .uma_questao_por_tela    = 0,
        .reduzir_distracoes      = 1,
        .adicionar_dicas         = 0, /* dicas extras podem confundir       */
        .max_palavras_frase      = 22,
        .tempo_base_minutos      = 5,
        .fator_tempo_extra       = 1.50,
        .grau                    = GRAU_PADRAO,
    },
    {
        .tipo                    = PERFIL_DOWN,
        .nome                    = "Sindrome de Down",
        .descricao               = "Vocabulario simples, frases muito curtas e tempo generoso",
        .simplificar_linguagem   = 1,
        .linguagem_literal       = 1,
        .dividir_frases_longas   = 1,
        .destacar_verbos_acao    = 1,
        .sugerir_tempo           = 1,
        .ordenar_por_dificuldade = 1,
        .uma_questao_por_tela    = 1,
        .reduzir_distracoes      = 1,
        .adicionar_dicas         = 1,
        .max_palavras_frase      = 12,
        .tempo_base_minutos      = 6,
        .fator_tempo_extra       = 2.00,
        .grau                    = GRAU_PADRAO,
    },
    {
        .tipo                    = PERFIL_DI_LEVE,
        .nome                    = "Deficiencia Intelectual Leve",
        .descricao               = "Linguagem simples com apoio visual e dicas de leitura",
        .simplificar_linguagem   = 1,
        .linguagem_literal       = 1,
        .dividir_frases_longas   = 1,
        .destacar_verbos_acao    = 1,
        .sugerir_tempo           = 1,
        .ordenar_por_dificuldade = 1,
        .uma_questao_por_tela    = 0,
        .reduzir_distracoes      = 1,
        .adicionar_dicas         = 1,
        .max_palavras_frase      = 14,
        .tempo_base_minutos      = 5,
        .fator_tempo_extra       = 1.75,
        .grau                    = GRAU_PADRAO,
    },
    {
        .tipo                    = PERFIL_DISLEXIA,
        .nome                    = "Dislexia",
        .descricao               = "Frases curtas, vocabulario simples e MUITO mais tempo de leitura",
        .simplificar_linguagem   = 1,
        .linguagem_literal       = 0,
        .dividir_frases_longas   = 1,
        .destacar_verbos_acao    = 1, /* CAIXA-ALTA destaca o comando        */
        .sugerir_tempo           = 1,
        .ordenar_por_dificuldade = 0, /* manter ordem reduz reorientacao     */
        .uma_questao_por_tela    = 0,
        .reduzir_distracoes      = 1,
        .adicionar_dicas         = 1,
        .max_palavras_frase      = 15,
        .tempo_base_minutos      = 5,
        .fator_tempo_extra       = 1.75, /* tempo extra e fundamental       */
        .grau                    = GRAU_PADRAO,
    },
    {
        .tipo                    = PERFIL_GENERICO,
        .nome                    = "Generico (customizavel)",
        .descricao               = "Adaptacao moderada para casos nao especificados",
        .simplificar_linguagem   = 1,
        .linguagem_literal       = 0,
        .dividir_frases_longas   = 1,
        .destacar_verbos_acao    = 1,
        .sugerir_tempo           = 1,
        .ordenar_por_dificuldade = 0,
        .uma_questao_por_tela    = 0,
        .reduzir_distracoes      = 1,
        .adicionar_dicas         = 1,
        .max_palavras_frase      = 20,
        .tempo_base_minutos      = 4,
        .fator_tempo_extra       = 1.25,
        .grau                    = GRAU_PADRAO,
    },
};

/* ============================================================
 * Dicionarios de substituicao
 * ============================================================ */

typedef struct {
    const char *de;
    const char *para;
} Substituicao;

/* Vocabulario "academiques" -> palavras simples.
 * Mantemos tudo em ASCII (sem acentos) para evitar problemas de encoding
 * entre o console do Windows (CP-1252) e arquivos UTF-8. */
static const Substituicao DICIONARIO_SIMPLIFICA[] = {
    {"considerando o texto apresentado", "olhando o texto"},
    {"considerando", "pensando em"},
    {"posteriormente", "depois"},
    {"anteriormente", "antes"},
    {"de maneira mais adequada", "do jeito certo"},
    {"de maneira adequada", "do jeito certo"},
    {"de maneira detalhada", "com detalhes"},
    {"de maneira", "de forma"},
    {"adequadamente", "do jeito certo"},
    {"identifique", "encontre"},
    {"verifique", "veja"},
    {"determine", "descubra"},
    {"explicite", "diga"},
    {"discorra sobre", "fale sobre"},
    {"disserte sobre", "escreva sobre"},
    {"disserte", "escreva"},
    {"argumente", "explique"},
    {"todavia", "mas"},
    {"entretanto", "mas"},
    {"contudo", "porem"},
    {"outrossim", "tambem"},
    {"mediante", "por meio de"},
    {"a respeito de", "sobre"},
    {"acerca de", "sobre"},
    {"a fim de que", "para que"},
    {"a fim de", "para"},
    {"com o intuito de", "para"},
    {"com o objetivo de", "para"},
    {"de modo a", "para"},
    {"em virtude de", "por causa de"},
    {"no que tange", "sobre"},
    {"no que se refere a", "sobre"},
    {"diversos", "varios"},
    {"diversas", "varias"},
    {NULL, NULL}};

/* Idiomatismos / metaforas trocados por equivalentes literais.
 * Especialmente uteis para o perfil TEA. */
static const Substituicao DICIONARIO_LITERAL[] = {
    {"de maos dadas com", "junto com"},
    {"pe no chao", "realismo"},
    {"matar a charada", "descobrir"},
    {"de cabeca", "sem usar papel"},
    {"de uma so vez", "junto"},
    {"a luz de", "com base em"},
    {"a partir de", "usando"},
    {"em outras palavras", "ou seja"},
    {"em ultima analise", "no fim"},
    {"levando em conta", "pensando em"},
    {"levando em consideracao", "pensando em"},
    {"tendo em vista", "pensando em"},
    {NULL, NULL}};

/* Verbos de acao tipicos em comandos de questao.
 * Quando aparecem capitalizados (inicio de frase), trocamos por CAIXA-ALTA. */
static const char *VERBOS_ACAO[] = {
    "Explique", "Descreva", "Identifique", "Calcule", "Resolva",
    "Marque",   "Assinale", "Escolha",     "Indique", "Encontre",
    "Compare",  "Liste",    "Cite",        "Diga",    "Escreva",
    "Responda", "Complete", "Justifique",  "Analise", "Interprete",
    "Discorra", "Argumente","Demonstre",   "Veja",    "Descubra",
    "Conte",    "Some",     "Subtraia",    "Multiplique", "Divida",
    "Defina",   "Aponte",   "Selecione",   "Verifique", "Mostre",
    NULL};

/* Frases de "enchimento" que costumam aparecer em provas e nao agregam
 * informacao ao comando da questao. Sao removidas quando reduzir_distracoes
 * esta ativo. */
static const char *FRASES_DISTRATORAS[] = {
    "Considerando o texto apresentado anteriormente, ",
    "Considerando o texto apresentado, ",
    "Levando em consideracao tudo o que foi exposto, ",
    "Levando em conta o que foi visto, ",
    "Tendo em vista o conteudo discutido em sala, ",
    "Com base nos estudos realizados ate o momento, ",
    "Com base no que foi estudado, ",
    NULL};

/* ============================================================
 * Transformacoes individuais (cada uma aloca nova string)
 * ============================================================ */

/* Aplica um dicionario de substituicoes em duas passagens: minuscula
 * e capitalizada (primeira letra em maiuscula). Ignora casos no meio
 * de palavras pois usamos strstr; isso e pragmatico - falsos positivos
 * sao raros para o vocabulario alvo. */
static char *aplicar_dicionario(const char *texto, const Substituicao *dic)
{
    if (texto == NULL) {
        return NULL;
    }
    char *atual = str_dup(texto);
    if (atual == NULL) {
        return NULL;
    }
    for (int i = 0; dic[i].de != NULL; i++) {
        char *novo = str_substituir(atual, dic[i].de, dic[i].para);
        if (novo != NULL) {
            free(atual);
            atual = novo;
        }
        /* Versao com primeira letra capitalizada. */
        char *de_cap = str_dup(dic[i].de);
        if (de_cap != NULL && de_cap[0] != '\0') {
            de_cap[0]      = (char)toupper((unsigned char)de_cap[0]);
            char *para_cap = str_dup(dic[i].para);
            if (para_cap != NULL && para_cap[0] != '\0') {
                para_cap[0] = (char)toupper((unsigned char)para_cap[0]);
                novo        = str_substituir(atual, de_cap, para_cap);
                if (novo != NULL) {
                    free(atual);
                    atual = novo;
                }
            }
            free(para_cap);
        }
        free(de_cap);
    }
    return atual;
}

/* Substitui ocorrencias dos verbos da lista por sua versao em CAIXA-ALTA.
 * So substitui a forma capitalizada (Explique, Marque, ...) que e o uso
 * tipico em inicio de comando e evita acertar palavras quaisquer. */
static char *destacar_verbos(const char *texto)
{
    if (texto == NULL) {
        return NULL;
    }
    char *atual = str_dup(texto);
    if (atual == NULL) {
        return NULL;
    }
    char alvo_upper[64];
    for (int i = 0; VERBOS_ACAO[i] != NULL; i++) {
        const char *v = VERBOS_ACAO[i];
        size_t      n = strlen(v);
        if (n >= sizeof(alvo_upper)) {
            continue;
        }
        for (size_t k = 0; k < n; k++) {
            alvo_upper[k] = (char)toupper((unsigned char)v[k]);
        }
        alvo_upper[n] = '\0';
        char *novo    = str_substituir(atual, v, alvo_upper);
        if (novo != NULL) {
            free(atual);
            atual = novo;
        }
    }
    return atual;
}

/* Remove as frases distratoras do inicio do enunciado. */
static char *remover_distracoes(const char *texto)
{
    if (texto == NULL) {
        return NULL;
    }
    char *atual = str_dup(texto);
    if (atual == NULL) {
        return NULL;
    }
    for (int i = 0; FRASES_DISTRATORAS[i] != NULL; i++) {
        char *novo = str_substituir(atual, FRASES_DISTRATORAS[i], "");
        if (novo != NULL) {
            free(atual);
            atual = novo;
        }
    }
    /* Apos remover prefixos, o texto pode comecar com letra minuscula -
     * capitalizamos a primeira letra do enunciado para manter ortografia. */
    if (atual[0] != '\0' && islower((unsigned char)atual[0])) {
        atual[0] = (char)toupper((unsigned char)atual[0]);
    }
    return atual;
}

/* Quebra frases longas: a cada virgula ou ponto-e-virgula encontrado depois
 * de 'max_palavras' palavras desde o ultimo respiro, insere uma quebra de
 * linha. Pontos finais zeram o contador. Resultado: paragrafos respiraveis. */
static char *dividir_frases_longas(const char *texto, int max_palavras)
{
    if (texto == NULL) {
        return NULL;
    }
    if (max_palavras <= 0) {
        return str_dup(texto);
    }
    size_t total = strlen(texto);
    /* Pior caso: cada caractere vira "x\n", logo 2x espaco. */
    char *saida = (char *)malloc(total * 2 + 4);
    if (saida == NULL) {
        return NULL;
    }
    size_t j        = 0;
    int    palavras = 0;
    int    em_palavra = 0;
    for (size_t i = 0; i < total; i++) {
        char c = texto[i];
        saida[j++] = c;
        if (isspace((unsigned char)c)) {
            if (em_palavra) {
                palavras++;
                em_palavra = 0;
            }
        } else {
            em_palavra = 1;
        }
        if (palavras >= max_palavras && (c == ',' || c == ';')) {
            saida[j++] = '\n';
            palavras   = 0;
            em_palavra = 0;
            /* pula brancos seguintes para evitar espacamento no comeco da linha */
            while (i + 1 < total && (texto[i + 1] == ' ' || texto[i + 1] == '\t')) {
                i++;
            }
        }
        if (c == '.' || c == '?' || c == '!') {
            palavras   = 0;
            em_palavra = 0;
        }
    }
    saida[j] = '\0';
    return saida;
}

/* ============================================================
 * Calculo de tempo sugerido por questao
 * ============================================================ */

static int calcular_tempo_questao(const Questao *q, const Perfil *perfil)
{
    int base = perfil->tempo_base_minutos;
    if (q->tipo == QUESTAO_DISCURSIVA) {
        base *= 2;
    }
    if (q->dificuldade > 0) {
        base += (q->dificuldade - 1);
    }
    double t = (double)base * perfil->fator_tempo_extra;
    int    arredondado = (int)(t + 0.5);
    if (arredondado < 1) {
        arredondado = 1;
    }
    return arredondado;
}

/* ============================================================
 * Ordenacao
 * ============================================================ */

static int comparar_questoes(const void *a, const void *b)
{
    const Questao *qa = (const Questao *)a;
    const Questao *qb = (const Questao *)b;
    int da = qa->dificuldade > 0 ? qa->dificuldade : 99;
    int db = qb->dificuldade > 0 ? qb->dificuldade : 99;
    if (da != db) {
        return da - db;
    }
    /* Empate em dificuldade: mantem ordem original pelo numero. */
    return qa->numero - qb->numero;
}

/* ============================================================
 * API publica
 * ============================================================ */

const Perfil *perfis_obter(TipoPerfil tipo)
{
    if ((int)tipo < 0 || (int)tipo >= PERFIL_TOTAL) {
        return NULL;
    }
    return &PERFIS[tipo];
}

void perfis_listar(FILE *out)
{
    if (out == NULL) {
        return;
    }
    fprintf(out, "\nPerfis de acessibilidade disponiveis:\n");
    for (int i = 0; i < PERFIL_TOTAL; i++) {
        fprintf(out, "  %d) %-30s - %s\n", i + 1, PERFIS[i].nome, PERFIS[i].descricao);
    }
}

const char *perfis_descricao_grau(int grau)
{
    switch (grau) {
        case 1:  return "leve";
        case 2:  return "leve a moderado";
        case 3:  return "moderado";
        case 4:  return "moderado a severo";
        case 5:  return "extremo / severo";
        default: return "indefinido";
    }
}

/* Ajusta um perfil base de acordo com o grau (1-5).
 * O grau 3 e neutro (mantem os valores da tabela). Graus mais baixos
 * suavizam adaptacoes (mais proximo da prova original); graus mais
 * altos as intensificam (frases mais curtas, mais tempo, mais dicas).
 *
 * Esta funcao nao modifica a tabela estatica PERFIS; ela copia o
 * conteudo para uma struct local e devolve por valor. */
Perfil perfis_compor(const Perfil *base, int grau)
{
    Perfil out;
    if (base == NULL) {
        memset(&out, 0, sizeof(out));
        out.grau = GRAU_PADRAO;
        return out;
    }

    /* Copia integral e em seguida ajusta. */
    out = *base;

    /* Clamp do grau em [1, 5]. */
    if (grau < GRAU_MIN) grau = GRAU_MIN;
    if (grau > GRAU_MAX) grau = GRAU_MAX;
    out.grau = grau;

    switch (grau) {
        case 1:
            /* Adaptacao bem leve: so simplificar e destacar verbos.
             * Mantemos o restante desligado para nao alterar demais
             * a prova original. */
            out.dividir_frases_longas   = 0;
            out.uma_questao_por_tela    = 0;
            out.ordenar_por_dificuldade = 0;
            out.linguagem_literal       = 0;
            out.adicionar_dicas         = 0;
            out.max_palavras_frase += 10; /* praticamente nao quebra */
            out.fator_tempo_extra -= 0.30;
            break;

        case 2:
            /* Adaptacao moderada-baixa. */
            out.uma_questao_por_tela = 0;
            out.max_palavras_frase += 5;
            out.fator_tempo_extra -= 0.15;
            break;

        case 3:
            /* Sem alteracao - valores da tabela. */
            break;

        case 4:
            /* Adaptacao mais agressiva. */
            out.max_palavras_frase -= 3;
            out.fator_tempo_extra += 0.25;
            out.adicionar_dicas = 1;
            break;

        case 5:
            /* Extremo: tudo ligado, frases minimas, tempo bem maior. */
            out.simplificar_linguagem  = 1;
            out.linguagem_literal      = 1;
            out.dividir_frases_longas  = 1;
            out.destacar_verbos_acao   = 1;
            out.uma_questao_por_tela   = 1;
            out.reduzir_distracoes     = 1;
            out.adicionar_dicas        = 1;
            out.max_palavras_frase -= 6;
            out.fator_tempo_extra += 0.50;
            break;
    }

    /* Limites sensatos. */
    if (out.max_palavras_frase < 6) out.max_palavras_frase = 6;
    if (out.fator_tempo_extra < 1.00) out.fator_tempo_extra = 1.00;

    return out;
}

Prova *adaptar_prova(const Prova *original, const Perfil *perfil)
{
    if (original == NULL || perfil == NULL) {
        return NULL;
    }

    Prova *nova = prova_criar();
    if (nova == NULL) {
        return NULL;
    }

    /* Cabecalho da prova adaptada. */
    {
        const char *base_titulo = original->titulo != NULL ? original->titulo : "Prova";
        size_t      tam         = strlen(base_titulo) + strlen(perfil->nome) + 96;
        char       *titulo      = (char *)malloc(tam);
        if (titulo != NULL) {
            snprintf(titulo, tam, "%s [adaptada para %s - grau %d]", base_titulo,
                     perfil->nome, perfil->grau);
            nova->titulo = titulo;
        }
    }
    if (original->disciplina != NULL) {
        nova->disciplina = str_dup(original->disciplina);
    }

    /* Copia profunda das questoes, aplicando a pipeline de transformacoes. */
    for (int i = 0; i < original->num_questoes; i++) {
        const Questao *q  = &original->questoes[i];
        Questao       *qn = prova_adicionar_questao(nova);
        if (qn == NULL) {
            break;
        }
        qn->numero      = q->numero;
        qn->tipo        = q->tipo;
        qn->dificuldade = q->dificuldade;

        /* Pipeline: enunciado em camadas. Cada camada aloca string nova. */
        char *t = str_dup(q->enunciado != NULL ? q->enunciado : "");
        if (t == NULL) {
            continue;
        }
        if (perfil->reduzir_distracoes) {
            char *novo = remover_distracoes(t);
            if (novo != NULL) { free(t); t = novo; }
        }
        if (perfil->simplificar_linguagem) {
            char *novo = aplicar_dicionario(t, DICIONARIO_SIMPLIFICA);
            if (novo != NULL) { free(t); t = novo; }
        }
        if (perfil->linguagem_literal) {
            char *novo = aplicar_dicionario(t, DICIONARIO_LITERAL);
            if (novo != NULL) { free(t); t = novo; }
        }
        if (perfil->destacar_verbos_acao) {
            char *novo = destacar_verbos(t);
            if (novo != NULL) { free(t); t = novo; }
        }
        if (perfil->dividir_frases_longas) {
            char *novo = dividir_frases_longas(t, perfil->max_palavras_frase);
            if (novo != NULL) { free(t); t = novo; }
        }
        qn->enunciado = t;

        /* Pipeline mais leve para alternativas: simplificar e literal. */
        for (int j = 0; j < q->num_alternativas; j++) {
            char *at = str_dup(q->alternativas[j].texto != NULL ? q->alternativas[j].texto : "");
            if (at == NULL) {
                continue;
            }
            if (perfil->simplificar_linguagem) {
                char *novo = aplicar_dicionario(at, DICIONARIO_SIMPLIFICA);
                if (novo != NULL) { free(at); at = novo; }
            }
            if (perfil->linguagem_literal) {
                char *novo = aplicar_dicionario(at, DICIONARIO_LITERAL);
                if (novo != NULL) { free(at); at = novo; }
            }
            questao_adicionar_alternativa(qn, q->alternativas[j].letra, at);
            free(at);
        }
    }

    if (perfil->ordenar_por_dificuldade && nova->num_questoes > 1) {
        qsort(nova->questoes, (size_t)nova->num_questoes, sizeof(Questao), comparar_questoes);
    }

    return nova;
}

void prova_exibir_adaptada(const Prova *p, const Perfil *perfil, FILE *out)
{
    if (p == NULL || perfil == NULL || out == NULL) {
        return;
    }

    /* Cabecalho. */
    imprimir_separador(out, '=', 70);
    fprintf(out, "TITULO: %s\n", p->titulo != NULL ? p->titulo : "(sem titulo)");
    if (p->disciplina != NULL) {
        fprintf(out, "DISCIPLINA: %s\n", p->disciplina);
    }
    fprintf(out, "PERFIL DE ACESSIBILIDADE: %s (grau %d - %s)\n", perfil->nome,
            perfil->grau, perfis_descricao_grau(perfil->grau));
    fprintf(out, "  -> %s\n", perfil->descricao);

    if (perfil->sugerir_tempo) {
        int total = 0;
        for (int i = 0; i < p->num_questoes; i++) {
            total += calcular_tempo_questao(&p->questoes[i], perfil);
        }
        fprintf(out, "TEMPO TOTAL SUGERIDO: cerca de %d minutos\n", total);
    }
    if (perfil->ordenar_por_dificuldade) {
        fprintf(out, "ORDEM: das questoes mais simples para as mais complexas.\n");
    }
    imprimir_separador(out, '=', 70);
    fputc('\n', out);

    /* Dicas globais antes da primeira questao. */
    if (perfil->adicionar_dicas) {
        fprintf(out, "DICAS GERAIS DE LEITURA:\n");
        fprintf(out, "  - Leia cada questao com calma. Releia se precisar.\n");
        fprintf(out, "  - Foque no que esta em CAIXA-ALTA: e o que deve ser feito.\n");
        fprintf(out, "  - Voce pode pular uma questao e voltar nela depois.\n\n");
    }

    /* Questoes. */
    for (int i = 0; i < p->num_questoes; i++) {
        const Questao *q = &p->questoes[i];

        if (perfil->uma_questao_por_tela) {
            imprimir_separador(out, '-', 70);
        }
        fprintf(out, "QUESTAO %d", q->numero);
        fprintf(out, " [%s]", q->tipo == QUESTAO_DISCURSIVA ? "DISCURSIVA" : "OBJETIVA");
        if (q->dificuldade > 0) {
            fprintf(out, " - dificuldade %d/5", q->dificuldade);
        }
        fputc('\n', out);

        if (perfil->sugerir_tempo) {
            fprintf(out, "Tempo sugerido: %d minutos\n", calcular_tempo_questao(q, perfil));
        }
        fputc('\n', out);

        fprintf(out, "%s\n\n", q->enunciado != NULL ? q->enunciado : "");

        for (int j = 0; j < q->num_alternativas; j++) {
            fprintf(out, "  ( ) %c) %s\n", q->alternativas[j].letra,
                    q->alternativas[j].texto != NULL ? q->alternativas[j].texto : "");
        }

        if (q->tipo == QUESTAO_DISCURSIVA) {
            fprintf(out, "\nResposta:\n");
            for (int k = 0; k < 4; k++) {
                fprintf(out, "  ____________________________________________________\n");
            }
        }

        if (perfil->adicionar_dicas) {
            fprintf(out, "\n  >> DICA: identifique primeiro O QUE a questao pede ");
            fprintf(out, "(palavra em CAIXA-ALTA), depois responda.\n");
        }
        if (perfil->uma_questao_por_tela) {
            fprintf(out, "\n  --- FIM DA QUESTAO. RESPIRE. SIGA QUANDO ESTIVER PRONTO. ---\n");
        }
        fputc('\n', out);
    }

    imprimir_separador(out, '=', 70);
    fprintf(out, "FIM DA PROVA. ENTREGUE QUANDO TERMINAR. BOA SORTE!\n");
    imprimir_separador(out, '=', 70);
}
