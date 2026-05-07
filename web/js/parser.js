/* parser.js
 *
 * Parser heuristico para detectar questoes e alternativas em texto livre
 * (saida de copiar/colar de PDF ou Word).
 *
 * Formatos suportados:
 *   Questoes:    "1." "2)" "Q3)" "Questão 4:" "Pergunta 5-"
 *   Alternativas: "a)" "(a)" "A." "B)"
 *   E tambem o formato INI do AdaptaProvas (com [PROVA] e [QUESTAO]).
 */

/* Detecta se uma linha e o INICIO de uma questao.
 * Retorna { numero } ou null. */
function detectarInicioQuestao(linha) {
    const l = linha.trim();
    if (!l) return null;

    /* "Questão 4:" ou "Questao 4:" ou "Pergunta 5-" */
    let m = l.match(/^(?:Quest(?:ã|a)o|Pergunta)\s*(\d+)\s*[:\-\.\)]?\s*(.*)$/i);
    if (m) return { numero: parseInt(m[1], 10), resto: m[2] };

    /* "Q3)" ou "Q3." */
    m = l.match(/^Q\s*(\d+)\s*[\)\.\-:]\s*(.*)$/i);
    if (m) return { numero: parseInt(m[1], 10), resto: m[2] };

    /* "1." ou "2)" ou "3-" no inicio */
    m = l.match(/^(\d+)\s*[\)\.\-]\s+(.*)$/);
    if (m) return { numero: parseInt(m[1], 10), resto: m[2] };

    return null;
}

/* Detecta se uma linha e uma alternativa (a) ... ).
 * Retorna { letra, texto } ou null. */
function detectarAlternativa(linha) {
    const l = linha.trim();
    if (!l) return null;

    /* "a)" "(a)" "A." "B)" "[a]" */
    let m = l.match(/^[\(\[]?\s*([a-eA-E])\s*[\)\]\.\-]\s+(.*)$/);
    if (m) return { letra: m[1].toUpperCase(), texto: m[2].trim() };

    return null;
}

/* Detecta se a linha e uma label "Aluno:" "Nome:" "Data:" "Turma:" etc.
 * Essas linhas sao ignoradas pelo parser. */
function ehLabelMetadata(linha) {
    const l = linha.trim().toLowerCase();
    return /^(aluno|nome|turma|data|professor|professora|escola|colegio|colégio|disciplina|materia|matéria|ano|serie|série|nota)\s*[:\-]/.test(l);
}

/* Heuristica para detectar tipo de questao.
 * Se tem alternativas, e objetiva. Caso contrario, discursiva. */
function detectarTipo(temAlternativas) {
    return temAlternativas ? "objetiva" : "discursiva";
}

/* =================== PARSER PRINCIPAL =================== */

function parseTextoLivre(texto, tituloPadrao = "", disciplinaPadrao = "") {
    const prova = {
        titulo: tituloPadrao || "",
        disciplina: disciplinaPadrao || "",
        questoes: [],
    };

    if (!texto) return prova;

    /* Normaliza quebras e remove BOM. */
    texto = texto.replace(/^﻿/, '');
    const linhas = texto.split(/\r?\n/);

    let questaoAtual = null;
    let acumuladorEnunciado = [];

    function fecharQuestao() {
        if (questaoAtual === null) return;
        questaoAtual.enunciado = acumuladorEnunciado.join(' ').trim();
        questaoAtual.tipo = detectarTipo(questaoAtual.alternativas.length > 0);
        prova.questoes.push(questaoAtual);
        questaoAtual = null;
        acumuladorEnunciado = [];
    }

    for (const linhaRaw of linhas) {
        const linha = linhaRaw.replace(/\s+$/, '');
        const trim = linha.trim();

        /* Linha vazia: nao fecha questao por si so (alunos pulam linha). */
        if (!trim) continue;

        /* Ignorar linhas de metadata. */
        if (ehLabelMetadata(trim)) continue;

        /* Tentar detectar inicio de questao. */
        const inicio = detectarInicioQuestao(trim);
        if (inicio) {
            fecharQuestao();
            questaoAtual = {
                numero: inicio.numero,
                tipo: "objetiva",
                enunciado: "",
                dificuldade: 0,
                alternativas: [],
            };
            if (inicio.resto) acumuladorEnunciado.push(inicio.resto);
            continue;
        }

        /* Sem questao em curso: pular linha. */
        if (questaoAtual === null) continue;

        /* Detectar alternativa. */
        const alt = detectarAlternativa(trim);
        if (alt) {
            questaoAtual.alternativas.push(alt);
            continue;
        }

        /* Caso contrario: linha de continuacao do enunciado.
         * Se ja temos alternativas, ignora (provavelmente lixo entre questoes). */
        if (questaoAtual.alternativas.length === 0) {
            acumuladorEnunciado.push(trim);
        }
    }

    fecharQuestao();

    /* Renumerar caso os numeros estejam fora de ordem ou faltando. */
    let n = 1;
    for (const q of prova.questoes) {
        if (!q.numero || q.numero <= 0) q.numero = n;
        n++;
    }

    return prova;
}

/* =================== PARSER INI =================== */

/* Parser do formato INI usado pelo binario C.
 * Retorna prova ou null se nao parecer ser INI. */
function parseINI(texto) {
    if (!texto) return null;
    if (!/^\s*\[PROVA\]/m.test(texto) && !/^\s*\[QUESTAO\]/m.test(texto)) {
        return null;
    }

    const prova = { titulo: "", disciplina: "", questoes: [] };
    let secao = null;
    let qAtual = null;

    const linhas = texto.split(/\r?\n/);
    for (const linhaRaw of linhas) {
        let linha = linhaRaw.trim();
        if (!linha || linha.startsWith('#') || linha.startsWith(';')) continue;

        const secaoMatch = linha.match(/^\[(\w+)\]$/);
        if (secaoMatch) {
            if (qAtual) prova.questoes.push(qAtual);
            qAtual = null;
            secao = secaoMatch[1].toUpperCase();
            if (secao === "QUESTAO") {
                qAtual = {
                    numero: prova.questoes.length + 1,
                    tipo: "objetiva",
                    enunciado: "",
                    dificuldade: 0,
                    alternativas: [],
                };
            }
            continue;
        }

        const eq = linha.indexOf('=');
        if (eq < 0) continue;
        const chave = linha.substring(0, eq).trim().toLowerCase();
        const valor = linha.substring(eq + 1).trim();

        if (secao === "PROVA") {
            if (chave === "titulo") prova.titulo = valor;
            else if (chave === "disciplina") prova.disciplina = valor;
        } else if (secao === "QUESTAO" && qAtual) {
            if (chave === "numero") qAtual.numero = parseInt(valor, 10) || qAtual.numero;
            else if (chave === "tipo") qAtual.tipo = valor.toLowerCase() === "discursiva" ? "discursiva" : "objetiva";
            else if (chave === "dificuldade") qAtual.dificuldade = parseInt(valor, 10) || 0;
            else if (chave === "enunciado") qAtual.enunciado = valor;
            else if (/^[a-e]$/.test(chave)) {
                qAtual.alternativas.push({ letra: chave.toUpperCase(), texto: valor });
            }
        }
    }
    if (qAtual) prova.questoes.push(qAtual);
    return prova;
}

/* Parser unificado: detecta o formato e roteia. */
function parseProva(texto, tituloPadrao = "", disciplinaPadrao = "") {
    const ini = parseINI(texto);
    if (ini && ini.questoes.length > 0) {
        if (tituloPadrao && !ini.titulo) ini.titulo = tituloPadrao;
        if (disciplinaPadrao && !ini.disciplina) ini.disciplina = disciplinaPadrao;
        return ini;
    }
    return parseTextoLivre(texto, tituloPadrao, disciplinaPadrao);
}
