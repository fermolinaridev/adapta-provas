/* perfis.js
 *
 * Porte JavaScript de src/perfis.c (apenas a parte de definicao de perfis e
 * composicao por grau). A logica de adaptacao em si esta em adaptador.js.
 */

const GRAU_MIN = 1;
const GRAU_MAX = 5;
const GRAU_PADRAO = 3;

const TipoPerfil = {
    TDAH: 0,
    TEA: 1,
    DOWN: 2,
    DI_LEVE: 3,
    DISLEXIA: 4,
    GENERICO: 5,
};

/* Tabela de perfis BASE (grau padrao = 3). */
const PERFIS = [
    {
        tipo: TipoPerfil.TDAH,
        nome: "TDAH",
        emoji: "⚡",
        descricao: "Transtorno do Déficit de Atenção e Hiperatividade",
        tldr: "Frases curtas, foco no comando, tempo objetivo, ordem por dificuldade.",
        simplificar_linguagem: 1,
        linguagem_literal: 0,
        dividir_frases_longas: 1,
        destacar_verbos_acao: 1,
        sugerir_tempo: 1,
        ordenar_por_dificuldade: 1,
        uma_questao_por_tela: 1,
        reduzir_distracoes: 1,
        adicionar_dicas: 1,
        max_palavras_frase: 18,
        tempo_base_minutos: 4,
        fator_tempo_extra: 1.50,
    },
    {
        tipo: TipoPerfil.TEA,
        nome: "TEA (Autismo)",
        emoji: "🧩",
        descricao: "Transtorno do Espectro Autista — linguagem literal e estrutura previsível",
        tldr: "Linguagem literal, sem metáforas, mantém ordem original (previsibilidade).",
        simplificar_linguagem: 1,
        linguagem_literal: 1,
        dividir_frases_longas: 1,
        destacar_verbos_acao: 1,
        sugerir_tempo: 1,
        ordenar_por_dificuldade: 0,
        uma_questao_por_tela: 0,
        reduzir_distracoes: 1,
        adicionar_dicas: 0,
        max_palavras_frase: 22,
        tempo_base_minutos: 5,
        fator_tempo_extra: 1.50,
    },
    {
        tipo: TipoPerfil.DOWN,
        nome: "Síndrome de Down",
        emoji: "🌟",
        descricao: "Vocabulário simples, frases muito curtas e tempo generoso",
        tldr: "Frases bem curtas, vocabulário simples, dobro do tempo, dicas extras.",
        simplificar_linguagem: 1,
        linguagem_literal: 1,
        dividir_frases_longas: 1,
        destacar_verbos_acao: 1,
        sugerir_tempo: 1,
        ordenar_por_dificuldade: 1,
        uma_questao_por_tela: 1,
        reduzir_distracoes: 1,
        adicionar_dicas: 1,
        max_palavras_frase: 12,
        tempo_base_minutos: 6,
        fator_tempo_extra: 2.00,
    },
    {
        tipo: TipoPerfil.DI_LEVE,
        nome: "Deficiência Intelectual Leve",
        emoji: "🌱",
        descricao: "Linguagem simples com apoio visual e dicas de leitura",
        tldr: "Linguagem simples, dicas em cada questão, ordenação por dificuldade.",
        simplificar_linguagem: 1,
        linguagem_literal: 1,
        dividir_frases_longas: 1,
        destacar_verbos_acao: 1,
        sugerir_tempo: 1,
        ordenar_por_dificuldade: 1,
        uma_questao_por_tela: 0,
        reduzir_distracoes: 1,
        adicionar_dicas: 1,
        max_palavras_frase: 14,
        tempo_base_minutos: 5,
        fator_tempo_extra: 1.75,
    },
    {
        tipo: TipoPerfil.DISLEXIA,
        nome: "Dislexia",
        emoji: "📖",
        descricao: "Frases curtas, vocabulário simples e MUITO mais tempo de leitura",
        tldr: "Comando em CAIXA-ALTA, ordem original, tempo bem maior, sem distrações.",
        simplificar_linguagem: 1,
        linguagem_literal: 0,
        dividir_frases_longas: 1,
        destacar_verbos_acao: 1,
        sugerir_tempo: 1,
        ordenar_por_dificuldade: 0,
        uma_questao_por_tela: 0,
        reduzir_distracoes: 1,
        adicionar_dicas: 1,
        max_palavras_frase: 15,
        tempo_base_minutos: 5,
        fator_tempo_extra: 1.75,
    },
    {
        tipo: TipoPerfil.GENERICO,
        nome: "Genérico",
        emoji: "🎯",
        descricao: "Adaptação moderada para casos não especificados",
        tldr: "Configuração balanceada, útil quando o perfil específico não está listado.",
        simplificar_linguagem: 1,
        linguagem_literal: 0,
        dividir_frases_longas: 1,
        destacar_verbos_acao: 1,
        sugerir_tempo: 1,
        ordenar_por_dificuldade: 0,
        uma_questao_por_tela: 0,
        reduzir_distracoes: 1,
        adicionar_dicas: 1,
        max_palavras_frase: 20,
        tempo_base_minutos: 4,
        fator_tempo_extra: 1.25,
    },
];

function perfilDescricaoGrau(grau) {
    switch (grau) {
        case 1: return "leve";
        case 2: return "leve a moderado";
        case 3: return "moderado";
        case 4: return "moderado a severo";
        case 5: return "extremo";
        default: return "indefinido";
    }
}

/* Compor um perfil base com um grau (1-5). Devolve uma copia ajustada. */
function perfilCompor(base, grau) {
    if (grau < GRAU_MIN) grau = GRAU_MIN;
    if (grau > GRAU_MAX) grau = GRAU_MAX;

    const out = { ...base, grau: grau };

    switch (grau) {
        case 1:
            out.dividir_frases_longas = 0;
            out.uma_questao_por_tela = 0;
            out.ordenar_por_dificuldade = 0;
            out.linguagem_literal = 0;
            out.adicionar_dicas = 0;
            out.max_palavras_frase += 10;
            out.fator_tempo_extra -= 0.30;
            break;
        case 2:
            out.uma_questao_por_tela = 0;
            out.max_palavras_frase += 5;
            out.fator_tempo_extra -= 0.15;
            break;
        case 3:
            /* sem alteracoes - valores da tabela */
            break;
        case 4:
            out.max_palavras_frase -= 3;
            out.fator_tempo_extra += 0.25;
            out.adicionar_dicas = 1;
            break;
        case 5:
            out.simplificar_linguagem = 1;
            out.linguagem_literal = 1;
            out.dividir_frases_longas = 1;
            out.destacar_verbos_acao = 1;
            out.uma_questao_por_tela = 1;
            out.reduzir_distracoes = 1;
            out.adicionar_dicas = 1;
            out.max_palavras_frase -= 6;
            out.fator_tempo_extra += 0.50;
            break;
    }

    if (out.max_palavras_frase < 6) out.max_palavras_frase = 6;
    if (out.fator_tempo_extra < 1.00) out.fator_tempo_extra = 1.00;

    return out;
}
