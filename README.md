# adapta-provas

**AdaptaProvas** é um aplicativo CLI escrito em **C (C99)** que adapta provas
educacionais para estudantes com diferentes perfis de acessibilidade — TDAH,
TEA, Síndrome de Down, deficiência intelectual leve e um perfil genérico
customizável.

> Projeto educacional, sem dependências externas além da biblioteca padrão de
> C. Compila com `gcc` (Linux, macOS, Windows/MinGW).

---

## Sumário

- [Como compilar](#como-compilar)
- [Como usar](#como-usar)
- [Formato do arquivo de entrada](#formato-do-arquivo-de-entrada)
- [Arquitetura do sistema](#arquitetura-do-sistema)
- [Regras de adaptação por perfil](#regras-de-adaptação-por-perfil)
- [Exemplos](#exemplos)
- [Como adicionar um novo perfil](#como-adicionar-um-novo-perfil)
- [Limitações conhecidas](#limitações-conhecidas)

---

## Como compilar

Pré-requisito: `gcc` (ou `clang`) e `make`.

```bash
make            # compila -> ./adapta_provas (ou adapta_provas.exe no Windows)
make run        # compila e executa
make clean      # remove binário e objetos
```

Compilação manual (sem `make`):

```bash
gcc -std=c99 -Wall -Wextra -O2 -Iinclude \
    src/main.c src/prova.c src/perfis.c src/utils.c \
    -o adapta_provas
```

> **Nota Windows**: instale o `gcc` via MSYS2/MinGW. No PowerShell sem
> compilador instalado, o build falha. Recomendado usar `Git Bash` + MinGW.

---

## Como usar

```text
$ ./adapta_provas

======================================================================
       AdaptaProvas - Adaptador de Provas Acessivel
       Linguagem C / CLI - Versao 1.0
======================================================================
PROVA  : (nenhuma carregada)
PERFIL : (nenhum selecionado)
----------------------------------------------------------------------
 1) Carregar prova de arquivo .txt
 2) Escolher perfil de acessibilidade
 3) Visualizar prova original
 4) Visualizar prova adaptada
 5) Exportar prova adaptada para arquivo .txt
 6) Listar perfis disponiveis
 0) Sair
Opcao:
```

Fluxo típico:

1. **Opção 1** → digite `exemplos/prova_exemplo.txt`.
2. **Opção 2** → escolha um perfil (ex.: `1` para TDAH).
3. **Opção 4** → veja a prova adaptada na tela.
4. **Opção 5** → exporte para um novo `.txt`.

---

## Formato do arquivo de entrada

Formato **INI simplificado**, fácil de digitar à mão. Linhas iniciadas por `#`
são comentários.

```ini
[PROVA]
titulo=Prova de Ciencias - Capitulo 4
disciplina=Biologia

[QUESTAO]
numero=1
tipo=objetiva
dificuldade=2
enunciado=Considerando o texto apresentado anteriormente, identifique...
A=Oxigenio
B=Gas carbonico
C=Nitrogenio
D=Hidrogenio

[QUESTAO]
numero=2
tipo=discursiva
dificuldade=4
enunciado=Disserte sobre o processo de fotossintese...
```

Campos suportados:

| Seção       | Chave         | Valores                                      |
|-------------|---------------|----------------------------------------------|
| `[PROVA]`   | `titulo`      | texto livre                                  |
| `[PROVA]`   | `disciplina` | texto livre                                  |
| `[QUESTAO]` | `numero`      | inteiro                                      |
| `[QUESTAO]` | `tipo`        | `objetiva` ou `discursiva`                   |
| `[QUESTAO]` | `dificuldade` | 1 a 5                                        |
| `[QUESTAO]` | `enunciado`   | texto livre (uma linha)                      |
| `[QUESTAO]` | `A`..`Z`      | texto da alternativa correspondente          |

Veja o arquivo completo em [exemplos/prova_exemplo.txt](exemplos/prova_exemplo.txt).

---

## Arquitetura do sistema

```
adapta-provas/
├── include/
│   ├── prova.h        # struct Prova / Questao + parser API
│   ├── perfis.h       # struct Perfil + adaptar_prova()
│   └── utils.h        # str_dup, str_trim, str_substituir, etc.
├── src/
│   ├── main.c         # menu interativo CLI
│   ├── prova.c        # parser INI, exibicao
│   ├── perfis.c       # tabela de perfis + pipeline de adaptacao
│   └── utils.c        # funcoes auxiliares de string
├── exemplos/
│   ├── prova_exemplo.txt   # entrada de exemplo
│   ├── saida_tdah.txt      # saida adaptada (perfil TDAH)
│   └── saida_tea.txt       # saida adaptada (perfil TEA)
├── Makefile
└── README.md
```

### Princípios de design

1. **Separação rigorosa estrutura ↔ apresentação.** O módulo `prova.[ch]`
   nada sabe sobre acessibilidade. Toda lógica de adaptação vive em
   `perfis.[ch]`. Isso permite trocar o adaptador sem mexer no parser.

2. **Perfis declarativos.** Cada perfil é uma struct preenchida na tabela
   `PERFIS[]` em [src/perfis.c](src/perfis.c). Não há `if (tipo == TDAH) ...`
   espalhado pelo código — o pipeline consulta apenas as bandeiras do struct.

3. **Pipeline de transformações independentes.** O enunciado passa por uma
   sequência de funções que cada uma aplica UMA transformação:
   ```
   enunciado_original
     → remover_distracoes
     → aplicar_dicionario(SIMPLIFICA)
     → aplicar_dicionario(LITERAL)        // se o perfil pede
     → destacar_verbos
     → dividir_frases_longas
     → enunciado_adaptado
   ```
   Cada etapa é controlada por uma bandeira do perfil. Adicionar uma nova
   transformação = nova função + nova bandeira.

4. **Sem dependências externas.** Apenas `<stdio.h>`, `<stdlib.h>`,
   `<string.h>`, `<ctype.h>`, `<locale.h>`. Compila em qualquer toolchain C99.

5. **Strings em ASCII no código.** Para evitar problemas de encoding entre
   o console do Windows (CP-1252) e arquivos UTF-8, todos os textos do
   programa, dicionários e exemplos estão em ASCII puro (sem acentos).

### Estruturas principais

```c
typedef struct {
    int          numero;
    TipoQuestao  tipo;            // OBJETIVA ou DISCURSIVA
    char        *enunciado;
    Alternativa *alternativas;
    int          num_alternativas;
    int          dificuldade;     // 1-5
} Questao;

typedef struct {
    char    *titulo;
    char    *disciplina;
    Questao *questoes;
    int      num_questoes;
    int      capacidade;
} Prova;

typedef struct {
    TipoPerfil  tipo;
    const char *nome;
    const char *descricao;
    int simplificar_linguagem;
    int linguagem_literal;
    int dividir_frases_longas;
    int destacar_verbos_acao;
    int sugerir_tempo;
    int ordenar_por_dificuldade;
    int uma_questao_por_tela;
    int reduzir_distracoes;
    int adicionar_dicas;
    int    max_palavras_frase;
    int    tempo_base_minutos;
    double fator_tempo_extra;
} Perfil;
```

---

## Regras de adaptação por perfil

| Bandeira / Parâmetro    | TDAH | TEA | Down | DI Leve | Genérico |
|-------------------------|:----:|:---:|:----:|:-------:|:--------:|
| Simplificar linguagem   | ✓    | ✓   | ✓    | ✓       | ✓        |
| Linguagem literal       |      | ✓   | ✓    | ✓       |          |
| Dividir frases longas   | ✓    | ✓   | ✓    | ✓       | ✓        |
| Destacar verbos         | ✓    | ✓   | ✓    | ✓       | ✓        |
| Sugerir tempo           | ✓    | ✓   | ✓    | ✓       | ✓        |
| Reordenar por dificuld. | ✓    |     | ✓    | ✓       |          |
| Uma questão por tela    | ✓    |     | ✓    |         |          |
| Reduzir distrações      | ✓    | ✓   | ✓    | ✓       | ✓        |
| Adicionar dicas         | ✓    |     | ✓    | ✓       | ✓        |
| Máx. palavras p/ quebra | 18   | 22  | 12   | 14      | 20       |
| Tempo base (min)        | 4    | 5   | 6    | 5       | 4        |
| Fator de tempo extra    | 1.5× | 1.5×| 2.0× | 1.75×   | 1.25×    |

### Justificativa das escolhas

- **TDAH** — literatura aponta dificuldade em sustentar atenção em textos
  longos. **Reduzir distrações**, **destacar a ação pedida** em CAIXA-ALTA,
  **dividir frases** e **sugerir tempo objetivo** ajudam o aluno a se manter
  na tarefa. Reordenamos por dificuldade para gerar pequenas vitórias rápidas
  no início.
- **TEA** — recomendações clássicas: **linguagem literal** (sem metáforas) e
  **estrutura previsível**. Por isso *desligamos* a reordenação por
  dificuldade — a previsibilidade da ordem original importa mais — e
  *desligamos* dicas extras, que podem ser interpretadas literalmente e
  confundir.
- **Síndrome de Down** — vocabulário simples, **frases muito curtas**
  (máx. 12 palavras) e tempo extra generoso (fator 2,0). Liga reordenação
  para começar pelas mais simples.
- **DI leve** — similar ao Down, mas com frases um pouco mais longas e
  menos tempo extra; mantemos dicas e exemplos concretos.
- **Genérico** — combinação moderada para casos não especificados.

---

## Exemplos

### Entrada (trecho)

```ini
[QUESTAO]
numero=1
tipo=objetiva
dificuldade=2
enunciado=Considerando o texto apresentado anteriormente, identifique qual
das alternativas a seguir representa, de maneira mais adequada, o principal
gas utilizado pelas plantas no processo de fotossintese.
```

### Saída — perfil **TDAH** (extraída de [exemplos/saida_tdah.txt](exemplos/saida_tdah.txt))

```
QUESTAO 1 [OBJETIVA] - dificuldade 2/5
Tempo sugerido: 8 minutos

ENCONTRE qual das alternativas a seguir representa, do jeito certo,
o principal gas utilizado pelas plantas no processo de fotossintese.

  ( ) A) Oxigenio
  ( ) B) Gas carbonico
  ( ) C) Nitrogenio
  ( ) D) Hidrogenio

  >> DICA: identifique primeiro O QUE a questao pede (palavra em CAIXA-ALTA)
```

> Note: removeu o enchimento "Considerando o texto apresentado anteriormente,",
> trocou "identifique" por "ENCONTRE" (verbo destacado), trocou
> "de maneira mais adequada" por "do jeito certo", e adicionou tempo + dica.

### Saída — perfil **TEA** (extraída de [exemplos/saida_tea.txt](exemplos/saida_tea.txt))

```
QUESTAO 1 [OBJETIVA] - dificuldade 2/5
Tempo sugerido: 9 minutos

ENCONTRE qual das alternativas a seguir representa, do jeito certo,
o principal gas utilizado pelas plantas no processo de fotossintese.

  ( ) A) Oxigenio
  ( ) B) Gas carbonico
  ( ) C) Nitrogenio
  ( ) D) Hidrogenio
```

> Note: mesma simplificação de vocabulário, mas SEM dicas extras (que poderiam
> confundir um aluno com TEA) e mantendo a ordem original das questões para
> preservar previsibilidade. Idiomatismos como "tendo em vista" são trocados
> por "pensando em" (dicionário literal), o que se vê melhor na questão 3.

Veja os arquivos completos em [exemplos/saida_tdah.txt](exemplos/saida_tdah.txt) e [exemplos/saida_tea.txt](exemplos/saida_tea.txt).

---

## Como adicionar um novo perfil

Três passos:

1. Em [include/perfis.h](include/perfis.h), adicione um valor ao enum
   `TipoPerfil` *antes* de `PERFIL_TOTAL`:
   ```c
   typedef enum {
       PERFIL_TDAH = 0,
       PERFIL_TEA,
       PERFIL_DOWN,
       PERFIL_DI_LEVE,
       PERFIL_GENERICO,
       PERFIL_DISLEXIA,   // <-- novo
       PERFIL_TOTAL
   } TipoPerfil;
   ```
2. Em [src/perfis.c](src/perfis.c), adicione uma entrada na tabela
   `PERFIS[]` configurando as bandeiras desejadas.
3. Recompile. Pronto. O novo perfil aparece automaticamente no menu e em
   `perfis_listar()` — não há mudança em `main.c`, no parser ou no exibidor.

Se a sua adaptação requer uma transformação inédita (ex.: substituir números
por palavras), basta adicionar uma função no pipeline de `adaptar_prova()`
condicional a uma nova bandeira do struct `Perfil`.

---

## Limitações conhecidas

- **Substituições são por strings literais.** O dicionário não entende
  conjugação verbal nem concordância. Trocar "levando em consideração" por
  "pensando em" pode gerar pequenas estranheza gramatical
  (ex.: "pensando em os fatores" em vez do mais natural "pensando nos
  fatores"). Isso pode ser melhorado adicionando entradas mais específicas
  no dicionário.
- **Acentos não suportados.** O parser e os dicionários trabalham em ASCII.
  Arquivos `.txt` com acentos serão lidos corretamente, mas as palavras com
  acentos não casam com as entradas do dicionário (que são sem acento).
  Recomenda-se manter as provas em ASCII ou expandir os dicionários.
- **Enunciado em uma linha.** Cada `enunciado=...` deve caber em uma linha
  (até 4096 caracteres). Quebras de parágrafo dentro de uma questão não
  são suportadas no formato atual.
- **Sem chave de respostas.** O programa não armazena nem corrige
  alternativas corretas — apenas adapta a apresentação.

---

## Licença

Projeto educacional. Use, modifique e redistribua livremente.
