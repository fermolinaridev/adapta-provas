# adapta-provas

**AdaptaProvas** é um aplicativo CLI escrito em **C (C99)** que adapta provas
educacionais para estudantes com diferentes perfis de acessibilidade — TDAH,
TEA, Síndrome de Down, deficiência intelectual leve, **dislexia** e um perfil
genérico customizável. Cada perfil pode ainda ser escalonado em **5 graus**
(1 = leve, 5 = extremo), o que ajusta automaticamente a intensidade das
adaptações.

A prova pode ser fornecida de **três formas diferentes** (vide menu):
1. **Carregar de arquivo `.txt`** já no formato INI do programa.
2. **Digitar interativamente**, passo a passo (título → questão por questão).
3. **Colar texto livre** (copiado de PDF/Word) — o programa detecta
   automaticamente os números de questão (`1.`, `2)`, `Questao 3:`, `Q4)`)
   e as alternativas (`a)`, `(b)`, `C.`).

> Projeto educacional, sem dependências externas além da biblioteca padrão de
> C. Compila com `gcc` (Linux, macOS, Windows/MinGW).

### 🌐 Front-end web (porte JS)

Além da CLI em C, o projeto também conta com um **front-end web** em vanilla
JavaScript que roda 100% no navegador (`web/index.html`) — basta abrir o
arquivo: nada é enviado para servidores, toda a adaptação acontece localmente.
A lógica de perfis, parser e pipeline de transformação foi portada fielmente
de `src/perfis.c` para `web/js/`. Tem tema claro/escuro, dropzone para upload
de `.txt`, comparação lado a lado e exportação direta.

```
web/
├── index.html
├── styles.css
└── js/
    ├── perfis.js     ← porte de perfis.c (definição + composição por grau)
    ├── adaptador.js  ← pipeline de transformações + renderização
    ├── parser.js     ← detector de questões em texto livre + INI
    ├── exemplos.js   ← provas de exemplo embutidas
    └── app.js        ← UI / orquestração
```

### ⚡ Início rápido no Windows (zero configuração)

**Duplo clique em `start.bat`** — o script `setup.ps1` cuida de tudo:

1. Detecta se o `gcc` está instalado
2. Se não estiver, baixa e instala o **MinGW-W64** via `winget` automaticamente
3. Compila o projeto (só na primeira vez, ou quando os fontes mudarem)
4. Abre o aplicativo

Não precisa instalar nada manualmente. Basta ter Windows 10/11 com `winget`
(já incluso por padrão no Windows 11 e em atualizações recentes do Windows 10).

---

## Sumário

- [Início rápido (Windows)](#-início-rápido-no-windows-zero-configuração)
- [Como compilar](#como-compilar)
- [Como usar](#como-usar)
- [Formato do arquivo de entrada](#formato-do-arquivo-de-entrada)
- [Arquitetura do sistema](#arquitetura-do-sistema)
- [Regras de adaptação por perfil](#regras-de-adaptação-por-perfil)
- [Sistema de graus (1–5)](#sistema-de-graus-1-5)
- [Exemplos](#exemplos)
- [Como adicionar um novo perfil](#como-adicionar-um-novo-perfil)
- [Limitações conhecidas](#limitações-conhecidas)

---

## Como compilar

Pré-requisito: `gcc` (ou `clang`) e `make`.

```bash
make            # compila -> ./adapta_provas (ou adapta_provas.exe no Windows)
make run        # compila e executa
make exemplo    # gera as 4 saídas de exemplo automaticamente
make clean      # remove binário e objetos
```

Compilação manual (sem `make`):

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic -O2 -Iinclude \
    src/main.c src/prova.c src/perfis.c src/utils.c \
    -o adapta_provas
```

> **Nota Windows**: instale o `gcc` via MinGW. Caminho mais rápido (testado
> neste projeto):
> ```powershell
> winget install --id BrechtSanders.WinLibs.POSIX.UCRT -e
> ```
> Após instalar, abra um novo terminal e o `gcc 16.x` estará disponível
> globalmente. Também é possível usar MSYS2 (`pacman -S mingw-w64-x86_64-gcc`).

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
 --- entrada ---
 1) Carregar prova de arquivo .txt
 2) Digitar uma nova prova (interativo, passo a passo)
 3) Colar prova de texto livre (de PDF/Word)
 --- adaptacao ---
 4) Escolher perfil de acessibilidade + grau
 5) Visualizar prova original
 6) Visualizar prova adaptada
 --- saida ---
 7) Exportar prova adaptada para .txt
 8) Salvar prova original em .txt (formato editavel)
 9) Listar perfis disponiveis
 0) Sair
Opcao:
```

### Fluxos típicos

**(A) Você tem um arquivo `.txt` no formato INI** — opção 1, indique o caminho.

**(B) Você quer digitar a prova do zero, no terminal** — opção 2. O programa
te pergunta título, disciplina e cada questão (tipo, enunciado, dificuldade,
alternativas). Ao final, você pode salvar em `.txt` para reutilizar depois.

**(C) Você tem a prova em PDF ou Word** — opção 3. Copie o texto da prova
no Acrobat/Word com Ctrl+A, Ctrl+C e cole no terminal. Termine com `FIM`
em uma linha sozinha. O programa detecta automaticamente questões e
alternativas. Veja a [seção sobre PDF/Word](#provas-em-pdf-ou-word) abaixo.

Depois de carregar a prova:
- **Opção 4** → escolha o perfil (TDAH, TEA, Down, DI Leve, Dislexia,
  Genérico) e em seguida o **grau** (1 a 5; ENTER usa o padrão = 3).
- **Opção 6** → visualize a prova adaptada na tela.
- **Opção 7** → exporte para um `.txt` para imprimir/enviar ao aluno.

---

## Provas em PDF ou Word

O programa em si **não lê PDF ou .docx diretamente** — fazer isso em C
puro exigiria bibliotecas externas (Poppler/MuPDF para PDF, parser de
ZIP+XML para .docx). Em vez disso, o fluxo recomendado é:

### Para Word (.docx)

1. Abra o arquivo no Microsoft Word ou LibreOffice.
2. Ctrl+A para selecionar tudo, Ctrl+C para copiar.
3. No AdaptaProvas, escolha **opção 3 (Colar prova de texto livre)**.
4. Cole no terminal (Ctrl+Shift+V no Linux/macOS, botão direito → colar
   no Windows).
5. Digite `FIM` em uma linha sozinha e pressione Enter.

### Para PDF

Se o PDF tem texto selecionável (não é só imagem):

1. Abra no Adobe Acrobat / navegador.
2. Selecione o texto, Ctrl+C, e cole no AdaptaProvas (opção 3).

Se preferir extrair o texto antes:

```bash
# Linux/macOS (poppler-utils):
pdftotext minha_prova.pdf minha_prova.txt
cat minha_prova.txt | ./adapta_provas    # ou copie/cole opção 3
```

```powershell
# Windows: instale o poppler via winget
winget install oschwartz10612.Poppler
pdftotext minha_prova.pdf minha_prova.txt
```

### O que o parser entende

| Padrão da entrada                        | Interpretação                |
|------------------------------------------|------------------------------|
| `1.` `2)` `3-` `4:` (no início da linha) | Número da questão            |
| `Q1)` `Quest 2.` `Questao 3:`            | Número da questão (com prefixo) |
| `Pergunta 4-` `Pergunta 5:`              | Número da questão            |
| `a)` `b)` `c)` `d)` (após `<espaço>`)    | Alternativa                  |
| `(a)` `(b)` `(c)`                        | Alternativa                  |
| `A.` `B.` `C.`                           | Alternativa                  |
| Linhas que não casam com nada acima      | Continuação do enunciado/alternativa anterior |

Após colar, o programa pergunta se você quer atribuir dificuldade
(1–5) a cada questão — **isto melhora a adaptação** porque é o que
permite reordenação e cálculo de tempo proporcional.

Questões sem alternativas são automaticamente marcadas como
**discursivas**.

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
│   ├── prova_exemplo.txt        # entrada de exemplo
│   ├── saida_tdah.txt           # saida (TDAH grau 3)
│   ├── saida_tea.txt            # saida (TEA grau 3)
│   ├── saida_dislexia.txt       # saida (Dislexia grau 3)
│   └── saida_tdah_grau5.txt     # saida (TDAH grau 5 - extremo)
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

Os valores abaixo correspondem ao **grau 3 (moderado)**. Outros graus
ajustam estes números automaticamente — ver seção seguinte.

| Bandeira / Parâmetro    | TDAH | TEA  | Down | DI Leve | Dislexia | Genérico |
|-------------------------|:----:|:----:|:----:|:-------:|:--------:|:--------:|
| Simplificar linguagem   | ✓    | ✓    | ✓    | ✓       | ✓        | ✓        |
| Linguagem literal       |      | ✓    | ✓    | ✓       |          |          |
| Dividir frases longas   | ✓    | ✓    | ✓    | ✓       | ✓        | ✓        |
| Destacar verbos         | ✓    | ✓    | ✓    | ✓       | ✓        | ✓        |
| Sugerir tempo           | ✓    | ✓    | ✓    | ✓       | ✓        | ✓        |
| Reordenar por dificuld. | ✓    |      | ✓    | ✓       |          |          |
| Uma questão por tela    | ✓    |      | ✓    |         |          |          |
| Reduzir distrações      | ✓    | ✓    | ✓    | ✓       | ✓        | ✓        |
| Adicionar dicas         | ✓    |      | ✓    | ✓       | ✓        | ✓        |
| Máx. palavras p/ quebra | 18   | 22   | 12   | 14      | 15       | 20       |
| Tempo base (min)        | 4    | 5    | 6    | 5       | 5        | 4        |
| Fator de tempo extra    | 1.5× | 1.5× | 2.0× | 1.75×   | 1.75×    | 1.25×    |

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
- **Dislexia** — *o tempo extra é o ajuste mais importante* (decodificação
  de palavras é mais lenta). Frases curtas, vocabulário simples e dicas
  visuais ajudam. Mantemos a ordem original para evitar reorientação extra.
  Linguagem literal fica desligada pois normalmente não há prejuízo de
  compreensão semântica.
- **Genérico** — combinação moderada para casos não especificados.

---

## Sistema de graus (1–5)

Cada perfil pode ser aplicado em 5 graus de intensidade. O grau ajusta
automaticamente os parâmetros sem alterar a tabela base:

| Grau | Nome              | Efeito sobre o perfil base                                 |
|:----:|-------------------|------------------------------------------------------------|
| 1    | leve              | Desliga divisão de frases, "uma por tela", reordenação e dicas; +10 palavras no limite; -0,30 no fator de tempo. |
| 2    | leve a moderado   | Desliga "uma por tela"; +5 palavras no limite; -0,15 no fator de tempo. |
| 3    | moderado          | **Padrão** — usa exatamente os valores da tabela acima.    |
| 4    | moderado a severo | -3 palavras no limite; +0,25 no fator de tempo; força dicas. |
| 5    | extremo           | Liga **todas** as bandeiras; -6 palavras no limite (mín. 6); +0,50 no fator de tempo. |

> O fator de tempo nunca cai abaixo de 1,00× e o limite de palavras nunca
> cai abaixo de 6 — ambos clampados em `perfis_compor()`.

### Exemplo prático: TDAH em grau 3 vs grau 5

Para o mesmo aluno, em momentos diferentes:

| Métrica                    | TDAH grau 3 | TDAH grau 5         |
|----------------------------|:-----------:|:-------------------:|
| Tempo total sugerido       | 58 min      | **76 min** (+31%)   |
| Limite de palavras p/quebra| 18          | **12**              |
| Linguagem literal          | desligada   | **ligada**          |
| Q3 e Q5 quebradas em linhas| não         | **sim**             |

Veja [exemplos/saida_tdah.txt](exemplos/saida_tdah.txt) e
[exemplos/saida_tdah_grau5.txt](exemplos/saida_tdah_grau5.txt) para
comparar lado a lado.

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

### Quatro saídas geradas pelo `make exemplo`

Cada arquivo abaixo é gerado *pelo binário compilado*, a partir do mesmo
[`prova_exemplo.txt`](exemplos/prova_exemplo.txt):

| Arquivo                                                          | Perfil    | Grau            | Tempo total |
|------------------------------------------------------------------|-----------|-----------------|:-----------:|
| [`saida_tdah.txt`](exemplos/saida_tdah.txt)                      | TDAH      | 3 — moderado    | 58 min      |
| [`saida_tea.txt`](exemplos/saida_tea.txt)                        | TEA       | 3 — moderado    | 69 min      |
| [`saida_dislexia.txt`](exemplos/saida_dislexia.txt)              | Dislexia  | 3 — moderado    | 80 min      |
| [`saida_tdah_grau5.txt`](exemplos/saida_tdah_grau5.txt)          | TDAH      | 5 — **extremo** | 76 min      |

#### Saída — perfil **TDAH** (grau 3)

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

> Removeu "Considerando o texto apresentado anteriormente,", trocou
> "identifique" por **ENCONTRE** (verbo destacado), trocou "de maneira mais
> adequada" por "do jeito certo", adicionou tempo + dica.

#### Saída — perfil **Dislexia** (grau 3)

```
QUESTAO 3 [DISCURSIVA] - dificuldade 4/5
Tempo sugerido: 23 minutos

ESCREVA, com detalhes e levando em consideracao os fatores ambientais
que podem influenciar a sua eficiencia,
sobre o processo de fotossintese realizado pelas plantas verdes.
```

> Tempo MUITO maior (23 min para uma única questão discursiva), e a frase
> longa foi automaticamente quebrada em duas linhas em "eficiencia,".

#### Saída — perfil **TDAH em grau 5 (extremo)**

```
QUESTAO 5 [DISCURSIVA] - dificuldade 5/5
Tempo sugerido: 24 minutos

EXPLIQUE de forma completa e detalhada as diferencas entre celulas
eucariontes e celulas procariontes,
citando exemplos concretos de cada tipo.
```

> No grau 3 essa questão NÃO quebrava (max=18). Em grau 5 (max=12), o
> programa força a divisão em "procariontes," e o tempo sobe de 18 para
> 24 min. Linguagem literal também é forçada, o que mudou Q3 de "levando em
> consideração" para "pensando em".

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
       PERFIL_DISLEXIA,
       PERFIL_GENERICO,
       PERFIL_TDI,        // <-- novo (ex: transtorno de processamento auditivo)
       PERFIL_TOTAL
   } TipoPerfil;
   ```
2. Em [src/perfis.c](src/perfis.c), adicione uma entrada na tabela
   `PERFIS[]` configurando as bandeiras desejadas (não esqueça
   `.grau = GRAU_PADRAO`).
3. Recompile. Pronto. O novo perfil aparece automaticamente no menu e em
   `perfis_listar()`, e já recebe o sistema de graus (1–5) sem nenhuma
   mudança em `main.c`, no parser, no adaptador ou no exibidor.

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
