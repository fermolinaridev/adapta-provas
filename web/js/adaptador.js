/* adaptador.js
 *
 * Porte JavaScript do pipeline de adaptacao de src/perfis.c.
 * Cada transformacao recebe e devolve string. A funcao adaptarProva()
 * orquestra a pipeline de acordo com as bandeiras do perfil.
 */

/* =================== DICIONARIOS =================== */

/* Vocabulario formal -> simples. */
const DICIONARIO_SIMPLIFICA = [
    ["considerando o texto apresentado", "olhando o texto"],
    ["considerando", "pensando em"],
    ["posteriormente", "depois"],
    ["anteriormente", "antes"],
    ["de maneira mais adequada", "do jeito certo"],
    ["de maneira adequada", "do jeito certo"],
    ["de maneira detalhada", "com detalhes"],
    ["de maneira", "de forma"],
    ["adequadamente", "do jeito certo"],
    ["identifique", "encontre"],
    ["verifique", "veja"],
    ["determine", "descubra"],
    ["explicite", "diga"],
    ["discorra sobre", "fale sobre"],
    ["disserte sobre", "escreva sobre"],
    ["disserte", "escreva"],
    ["argumente", "explique"],
    ["todavia", "mas"],
    ["entretanto", "mas"],
    ["contudo", "porém"],
    ["outrossim", "também"],
    ["mediante", "por meio de"],
    ["a respeito de", "sobre"],
    ["acerca de", "sobre"],
    ["a fim de que", "para que"],
    ["a fim de", "para"],
    ["com o intuito de", "para"],
    ["com o objetivo de", "para"],
    ["de modo a", "para"],
    ["em virtude de", "por causa de"],
    ["no que tange", "sobre"],
    ["no que se refere a", "sobre"],
    ["diversos", "vários"],
    ["diversas", "várias"],
];

/* Idiomatismos / metaforas -> equivalentes literais. */
const DICIONARIO_LITERAL = [
    ["de mãos dadas com", "junto com"],
    ["de maos dadas com", "junto com"],
    ["pé no chão", "realismo"],
    ["pe no chao", "realismo"],
    ["matar a charada", "descobrir"],
    ["de cabeça", "sem usar papel"],
    ["de cabeca", "sem usar papel"],
    ["de uma só vez", "junto"],
    ["de uma so vez", "junto"],
    ["à luz de", "com base em"],
    ["a luz de", "com base em"],
    ["a partir de", "usando"],
    ["em outras palavras", "ou seja"],
    ["em última análise", "no fim"],
    ["em ultima analise", "no fim"],
    ["levando em conta", "pensando em"],
    ["levando em consideração", "pensando em"],
    ["levando em consideracao", "pensando em"],
    ["tendo em vista", "pensando em"],
];

/* Verbos de acao tipicos em comandos de questao. */
const VERBOS_ACAO = [
    "Explique", "Descreva", "Identifique", "Calcule", "Resolva",
    "Marque", "Assinale", "Escolha", "Indique", "Encontre",
    "Compare", "Liste", "Cite", "Diga", "Escreva",
    "Responda", "Complete", "Justifique", "Analise", "Interprete",
    "Discorra", "Argumente", "Demonstre", "Veja", "Descubra",
    "Conte", "Some", "Subtraia", "Multiplique", "Divida",
    "Defina", "Aponte", "Selecione", "Verifique", "Mostre",
];

/* Frases distratoras a remover quando reduzir_distracoes=1. */
const FRASES_DISTRATORAS = [
    "Considerando o texto apresentado anteriormente, ",
    "Considerando o texto apresentado, ",
    "Levando em consideração tudo o que foi exposto, ",
    "Levando em consideracao tudo o que foi exposto, ",
    "Levando em conta o que foi visto, ",
    "Tendo em vista o conteúdo discutido em sala, ",
    "Tendo em vista o conteudo discutido em sala, ",
    "Com base nos estudos realizados até o momento, ",
    "Com base nos estudos realizados ate o momento, ",
    "Com base no que foi estudado, ",
];

/* =================== HELPERS =================== */

function escapeRegex(s) {
    return s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

function capitalizar(s) {
    if (!s) return s;
    return s.charAt(0).toUpperCase() + s.slice(1);
}

/* Substitui ocorrencias de 'de' por 'para' (case-insensitive parcial:
 * troca a forma minuscula e a forma capitalizada). */
function substituirCaseSensitive(texto, de, para) {
    if (!de) return texto;
    let resultado = texto.split(de).join(para);
    const deCap = capitalizar(de);
    const paraCap = capitalizar(para);
    if (deCap !== de) {
        resultado = resultado.split(deCap).join(paraCap);
    }
    return resultado;
}

/* =================== TRANSFORMACOES =================== */

function aplicarDicionario(texto, dic) {
    if (!texto) return "";
    let atual = texto;
    for (const [de, para] of dic) {
        atual = substituirCaseSensitive(atual, de, para);
    }
    return atual;
}

function destacarVerbos(texto) {
    if (!texto) return "";
    let atual = texto;
    for (const v of VERBOS_ACAO) {
        atual = atual.split(v).join(v.toUpperCase());
    }
    return atual;
}

function removerDistracoes(texto) {
    if (!texto) return "";
    let atual = texto;
    for (const frase of FRASES_DISTRATORAS) {
        atual = atual.split(frase).join("");
    }
    /* Capitaliza primeira letra se ficou minuscula. */
    if (atual.length > 0 && /[a-záéíóúâêôãõç]/.test(atual.charAt(0))) {
        atual = atual.charAt(0).toUpperCase() + atual.slice(1);
    }
    return atual;
}

/* Quebra frases longas em '\n' apos virgulas/ponto-e-virgula depois de
 * 'maxPalavras' palavras desde o ultimo respiro. */
function dividirFrasesLongas(texto, maxPalavras) {
    if (!texto) return "";
    if (maxPalavras <= 0) return texto;

    const out = [];
    let palavras = 0;
    let emPalavra = false;
    let i = 0;
    while (i < texto.length) {
        const c = texto[i];
        out.push(c);

        if (/\s/.test(c)) {
            if (emPalavra) {
                palavras++;
                emPalavra = false;
            }
        } else {
            emPalavra = true;
        }

        if (palavras >= maxPalavras && (c === ',' || c === ';')) {
            out.push('\n');
            palavras = 0;
            emPalavra = false;
            /* pula brancos seguintes */
            while (i + 1 < texto.length && (texto[i + 1] === ' ' || texto[i + 1] === '\t')) {
                i++;
            }
        }

        if (c === '.' || c === '?' || c === '!') {
            palavras = 0;
            emPalavra = false;
        }

        i++;
    }
    return out.join('');
}

/* =================== TEMPO =================== */

function calcularTempoQuestao(q, perfil) {
    let base = perfil.tempo_base_minutos;
    if (q.tipo === "discursiva") base *= 2;
    if (q.dificuldade > 0) base += (q.dificuldade - 1);
    const t = base * perfil.fator_tempo_extra;
    return Math.max(1, Math.round(t));
}

/* =================== ADAPTACAO =================== */

function adaptarProva(prova, perfil) {
    const nova = {
        titulo: `${prova.titulo || "Prova"} [adaptada para ${perfil.nome} - grau ${perfil.grau}]`,
        disciplina: prova.disciplina || "",
        questoes: [],
    };

    for (const q of prova.questoes) {
        const qn = {
            numero: q.numero,
            tipo: q.tipo,
            dificuldade: q.dificuldade,
            enunciado: q.enunciado || "",
            alternativas: [],
        };

        let t = qn.enunciado;
        if (perfil.reduzir_distracoes) t = removerDistracoes(t);
        if (perfil.simplificar_linguagem) t = aplicarDicionario(t, DICIONARIO_SIMPLIFICA);
        if (perfil.linguagem_literal) t = aplicarDicionario(t, DICIONARIO_LITERAL);
        if (perfil.destacar_verbos_acao) t = destacarVerbos(t);
        if (perfil.dividir_frases_longas) t = dividirFrasesLongas(t, perfil.max_palavras_frase);
        qn.enunciado = t;

        for (const alt of (q.alternativas || [])) {
            let at = alt.texto || "";
            if (perfil.simplificar_linguagem) at = aplicarDicionario(at, DICIONARIO_SIMPLIFICA);
            if (perfil.linguagem_literal) at = aplicarDicionario(at, DICIONARIO_LITERAL);
            qn.alternativas.push({ letra: alt.letra, texto: at });
        }

        nova.questoes.push(qn);
    }

    if (perfil.ordenar_por_dificuldade && nova.questoes.length > 1) {
        nova.questoes.sort((a, b) => {
            const da = a.dificuldade > 0 ? a.dificuldade : 99;
            const db = b.dificuldade > 0 ? b.dificuldade : 99;
            if (da !== db) return da - db;
            return a.numero - b.numero;
        });
    }

    return nova;
}

/* =================== EXIBICAO =================== */

function separador(c, n) {
    return c.repeat(n);
}

/* Renderiza a prova ADAPTADA como texto plano (mesmo formato da CLI). */
function renderizarProvaAdaptada(prova, perfil) {
    const linhas = [];

    linhas.push(separador('=', 70));
    linhas.push(`TÍTULO: ${prova.titulo || "(sem título)"}`);
    if (prova.disciplina) linhas.push(`DISCIPLINA: ${prova.disciplina}`);
    linhas.push(`PERFIL DE ACESSIBILIDADE: ${perfil.nome} (grau ${perfil.grau} - ${perfilDescricaoGrau(perfil.grau)})`);
    linhas.push(`  -> ${perfil.descricao}`);

    if (perfil.sugerir_tempo) {
        let total = 0;
        for (const q of prova.questoes) total += calcularTempoQuestao(q, perfil);
        linhas.push(`TEMPO TOTAL SUGERIDO: cerca de ${total} minutos`);
    }
    if (perfil.ordenar_por_dificuldade) {
        linhas.push(`ORDEM: das questões mais simples para as mais complexas.`);
    }
    linhas.push(separador('=', 70));
    linhas.push('');

    if (perfil.adicionar_dicas) {
        linhas.push(`DICAS GERAIS DE LEITURA:`);
        linhas.push(`  - Leia cada questão com calma. Releia se precisar.`);
        linhas.push(`  - Foque no que está em CAIXA-ALTA: é o que deve ser feito.`);
        linhas.push(`  - Você pode pular uma questão e voltar nela depois.`);
        linhas.push('');
    }

    for (const q of prova.questoes) {
        if (perfil.uma_questao_por_tela) {
            linhas.push(separador('-', 70));
        }
        let cab = `QUESTÃO ${q.numero} [${q.tipo === "discursiva" ? "DISCURSIVA" : "OBJETIVA"}]`;
        if (q.dificuldade > 0) cab += ` - dificuldade ${q.dificuldade}/5`;
        linhas.push(cab);

        if (perfil.sugerir_tempo) {
            linhas.push(`Tempo sugerido: ${calcularTempoQuestao(q, perfil)} minutos`);
        }
        linhas.push('');
        linhas.push(q.enunciado || "");
        linhas.push('');

        for (const alt of (q.alternativas || [])) {
            linhas.push(`  ( ) ${alt.letra}) ${alt.texto || ""}`);
        }

        if (q.tipo === "discursiva") {
            linhas.push('');
            linhas.push(`Resposta:`);
            for (let k = 0; k < 4; k++) {
                linhas.push(`  ____________________________________________________`);
            }
        }

        if (perfil.adicionar_dicas) {
            linhas.push('');
            linhas.push(`  >> DICA: identifique primeiro O QUE a questão pede (palavra em CAIXA-ALTA), depois responda.`);
        }
        if (perfil.uma_questao_por_tela) {
            linhas.push('');
            linhas.push(`  --- FIM DA QUESTÃO. RESPIRE. SIGA QUANDO ESTIVER PRONTO. ---`);
        }
        linhas.push('');
    }

    linhas.push(separador('=', 70));
    linhas.push(`FIM DA PROVA. ENTREGUE QUANDO TERMINAR. BOA SORTE!`);
    linhas.push(separador('=', 70));

    return linhas.join('\n');
}

/* Renderiza a prova ORIGINAL (sem adaptacao) para comparacao. */
function renderizarProvaOriginal(prova) {
    const linhas = [];
    linhas.push(separador('=', 70));
    linhas.push(`TÍTULO: ${prova.titulo || "(sem título)"}`);
    if (prova.disciplina) linhas.push(`DISCIPLINA: ${prova.disciplina}`);
    linhas.push(`Total de questões: ${prova.questoes.length}`);
    linhas.push(separador('=', 70));
    linhas.push('');

    for (const q of prova.questoes) {
        let cab = `QUESTÃO ${q.numero} [${q.tipo === "discursiva" ? "DISCURSIVA" : "OBJETIVA"}]`;
        if (q.dificuldade > 0) cab += ` - dificuldade ${q.dificuldade}/5`;
        linhas.push(cab);
        linhas.push('');
        linhas.push(q.enunciado || "");
        linhas.push('');
        for (const alt of (q.alternativas || [])) {
            linhas.push(`  ${alt.letra}) ${alt.texto || ""}`);
        }
        linhas.push('');
    }
    return linhas.join('\n');
}
