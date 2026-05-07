/* ia.js
 *
 * Camada de Inteligencia Artificial para reescrita SEMANTICA das questoes.
 * O adaptador heuristico (adaptador.js) ainda cuida da estrutura (tempo,
 * dicas, ordem, separadores). A IA assume a reescrita do TEXTO de cada
 * questao e alternativa, interpretando-as e recriando-as no nivel correto.
 *
 * Provedores suportados:
 *   - "pollinations": gratuito, sem chave (https://pollinations.ai)
 *   - "openai":       compativel com OpenAI (chave do usuario)
 *   - "groq":         API Groq (chave do usuario, gratuita)
 *   - "gemini":       Google Gemini (chave do usuario, gratuita)
 *   - "openrouter":   OpenRouter (varios modelos, chave do usuario)
 */

const PROVEDORES = {
    pollinations: {
        nome: "Pollinations (grátis, sem chave)",
        precisaChave: false,
        endpoint: "https://text.pollinations.ai/",
        modeloPadrao: "openai",
    },
    openai: {
        nome: "OpenAI (gpt-4o-mini)",
        precisaChave: true,
        endpoint: "https://api.openai.com/v1/chat/completions",
        modeloPadrao: "gpt-4o-mini",
    },
    groq: {
        nome: "Groq (llama-3.3-70b — grátis)",
        precisaChave: true,
        endpoint: "https://api.groq.com/openai/v1/chat/completions",
        modeloPadrao: "llama-3.3-70b-versatile",
    },
    gemini: {
        nome: "Google Gemini (grátis)",
        precisaChave: true,
        endpoint: "https://generativelanguage.googleapis.com/v1beta/models",
        modeloPadrao: "gemini-2.0-flash",
    },
    openrouter: {
        nome: "OpenRouter (modelo a escolher)",
        precisaChave: true,
        endpoint: "https://openrouter.ai/api/v1/chat/completions",
        modeloPadrao: "meta-llama/llama-3.3-70b-instruct:free",
    },
};

/* =================== PROMPTS POR PERFIL =================== */

const INSTRUCOES_PERFIL = {
    "TDAH": `O ALUNO TEM TDAH (Transtorno do Déficit de Atenção e Hiperatividade).
PRINCÍPIOS:
- Atenção curta: vá direto ao ponto, sem rodeios
- Use frases curtas e diretas (10-15 palavras cada)
- Comece sempre pelo verbo de ação em CAIXA-ALTA (ex: "MARQUE", "CALCULE", "EXPLIQUE")
- Elimine TODAS as frases introdutórias desnecessárias
- Use números, marcadores e estrutura visual clara
- Vocabulário cotidiano e familiar`,

    "TEA (Autismo)": `O ALUNO TEM TEA (Transtorno do Espectro Autista).
PRINCÍPIOS:
- Linguagem 100% LITERAL — proibido metáforas, ironias, ditos populares
- Estrutura previsível: sempre na mesma ordem (comando → contexto → o que responder)
- Frases diretas, sem ambiguidade
- Evite duplo sentido, expressões idiomáticas ou linguagem figurada
- Termos técnicos podem ser mantidos se forem precisos
- Não use perguntas retóricas`,

    "Síndrome de Down": `O ALUNO TEM SÍNDROME DE DOWN.
PRINCÍPIOS:
- Vocabulário MUITO simples, palavras do dia a dia
- Frases bem CURTAS (máximo 8-10 palavras)
- Uma ideia por frase
- Use exemplos concretos e familiares
- Evite palavras abstratas — use sinônimos simples
- Linguagem afetuosa e encorajadora
- Quebre a questão em passos pequenos quando possível`,

    "Deficiência Intelectual Leve": `O ALUNO TEM DEFICIÊNCIA INTELECTUAL LEVE.
PRINCÍPIOS:
- Linguagem simples mas não infantilizada
- Frases curtas e claras (máximo 12-14 palavras)
- Use exemplos do cotidiano sempre que possível
- Defina termos abstratos com palavras simples
- Estrutura clara: o que ler primeiro, o que responder depois
- Evite construções complexas (subordinações longas, voz passiva)`,

    "Dislexia": `O ALUNO TEM DISLEXIA.
PRINCÍPIOS:
- Frases CURTAS (máximo 12 palavras)
- Vocabulário simples, evite palavras longas e complexas
- Verbos de comando em CAIXA-ALTA no início (ex: "ESCREVA", "ESCOLHA")
- NUNCA use palavras com letras facilmente confundidas se houver alternativa simples
- Quebre informações longas em itens (use traços ou números)
- Espaçamento generoso entre ideias`,

    "Genérico": `O ALUNO TEM ALGUMA NECESSIDADE EDUCATIVA ESPECIAL não específica.
PRINCÍPIOS:
- Linguagem clara e acessível
- Frases moderadamente curtas (12-18 palavras)
- Vocabulário adaptado ao nível escolar
- Estrutura organizada e direta
- Comando inicial destacado em CAIXA-ALTA`,
};

const INSTRUCOES_GRAU = {
    1: "GRAU 1 (LEVE): Adaptação suave. Mantenha a maior parte do texto original, apenas simplificando o vocabulário mais difícil. Tamanho similar ao original.",
    2: "GRAU 2 (LEVE-MODERADO): Simplifique vocabulário, encurte frases muito longas, mantenha a estrutura geral.",
    3: "GRAU 3 (MODERADO): Reescreva ativamente. Simplifique frases, troque vocabulário formal, mantenha o conteúdo conceitual intacto.",
    4: "GRAU 4 (SEVERO): Reescrita agressiva. Frases bem curtas, vocabulário mínimo, divida em etapas se possível, mantenha apenas o essencial.",
    5: "GRAU 5 (EXTREMO): Reescrita máxima. Frases curtíssimas, palavras básicas, máxima clareza. Pode quebrar a questão em sub-perguntas se ajudar a compreensão. Apenas o conteúdo essencial.",
};

/* =================== CONFIGURACAO (localStorage) =================== */

const CONFIG_KEY = "adaptaprovas-ia-config";

function obterConfigIA() {
    try {
        const raw = localStorage.getItem(CONFIG_KEY);
        if (raw) return JSON.parse(raw);
    } catch {}
    return {
        ativo: false,
        provedor: "pollinations",
        chave: "",
        modelo: "",
    };
}

function salvarConfigIA(config) {
    try { localStorage.setItem(CONFIG_KEY, JSON.stringify(config)); } catch {}
}

/* =================== CHAMADAS A APIS =================== */

/* Constroi o prompt do sistema com base no perfil + grau. */
function construirPromptSistema(perfil) {
    const instrucoesPerfil = INSTRUCOES_PERFIL[perfil.nome] || INSTRUCOES_PERFIL["Genérico"];
    const instrucoesGrau = INSTRUCOES_GRAU[perfil.grau] || INSTRUCOES_GRAU[3];

    return `Você é um especialista em educação inclusiva, especializado em adaptar enunciados de provas para alunos com necessidades educacionais especiais.

${instrucoesPerfil}

${instrucoesGrau}

REGRAS CRÍTICAS:
1. PRESERVE 100% do conteúdo conceitual e da resposta correta — você está mudando APENAS o jeito de PERGUNTAR, não o que está sendo perguntado
2. NÃO dê dicas que entreguem a resposta
3. NÃO mude o tipo de questão (objetiva continua objetiva)
4. NÃO mude o número da questão
5. Responda APENAS com o texto adaptado, sem comentários, sem explicações, sem prefixos como "Resposta adaptada:" — só a questão reescrita
6. Mantenha o português brasileiro

Sua tarefa: receber um enunciado de questão escolar e devolvê-lo adaptado seguindo todos os princípios acima.`;
}

/* Chamada para Pollinations.ai (gratis, sem chave).
 * Usa o endpoint OpenAI-compatible (/openai) que retorna o formato padrao
 * com choices[0].message.content. O endpoint legado / esta deprecando. */
async function chamarPollinations(promptSistema, textoUsuario) {
    const body = {
        model: "openai",
        messages: [
            { role: "system", content: promptSistema },
            { role: "user", content: textoUsuario },
        ],
        temperature: 0.5,
    };
    console.log("[Pollinations] Enviando:", body);
    let resp;
    try {
        resp = await fetch("https://text.pollinations.ai/openai", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(body),
        });
    } catch (e) {
        throw new Error(`Falha de rede com Pollinations: ${e.message}. Verifique sua conexão.`);
    }
    console.log("[Pollinations] Status:", resp.status);
    if (!resp.ok) {
        const errBody = await resp.text().catch(() => "");
        throw new Error(`Pollinations ${resp.status}: ${errBody.slice(0, 300)}`);
    }
    const data = await resp.json();
    console.log("[Pollinations] Modelo:", data.model, "| Tier:", data.user_tier);
    const conteudo = data.choices?.[0]?.message?.content;
    if (!conteudo) {
        throw new Error(`Pollinations retornou resposta vazia: ${JSON.stringify(data).slice(0, 200)}`);
    }

    /* Detecta se a resposta e na verdade um aviso de deprecacao do servico. */
    const lower = conteudo.toLowerCase();
    if (lower.includes("legacy text api is being deprecat") ||
        lower.includes("please migrate to our new service")) {
        throw new Error(
            "Pollinations retornou aviso de deprecação em vez de gerar conteúdo. " +
            "Recomendamos usar Gemini (gratuito): pegue uma chave em https://aistudio.google.com/app/apikey"
        );
    }

    console.log("[Pollinations] Resposta:", conteudo.substring(0, 200));
    return conteudo.trim();
}

/* Chamada compativel com OpenAI / Groq / OpenRouter. */
async function chamarOpenAICompat(provedor, chave, modelo, promptSistema, textoUsuario) {
    const cfg = PROVEDORES[provedor];
    const body = {
        model: modelo || cfg.modeloPadrao,
        messages: [
            { role: "system", content: promptSistema },
            { role: "user", content: textoUsuario },
        ],
        temperature: 0.4,
        max_tokens: 800,
    };
    const headers = {
        "Content-Type": "application/json",
        "Authorization": `Bearer ${chave}`,
    };
    if (provedor === "openrouter") {
        headers["HTTP-Referer"] = "https://github.com/fernandomunhozmolinari/adapta-provas";
        headers["X-Title"] = "AdaptaProvas";
    }
    const resp = await fetch(cfg.endpoint, {
        method: "POST",
        headers,
        body: JSON.stringify(body),
    });
    if (!resp.ok) {
        const errBody = await resp.text();
        throw new Error(`${provedor} ${resp.status}: ${errBody.slice(0, 200)}`);
    }
    const data = await resp.json();
    return (data.choices?.[0]?.message?.content || "").trim();
}

/* Chamada para Google Gemini. */
async function chamarGemini(chave, modelo, promptSistema, textoUsuario) {
    const m = modelo || PROVEDORES.gemini.modeloPadrao;
    const url = `${PROVEDORES.gemini.endpoint}/${m}:generateContent?key=${chave}`;
    const body = {
        systemInstruction: { parts: [{ text: promptSistema }] },
        contents: [{ role: "user", parts: [{ text: textoUsuario }] }],
        generationConfig: { temperature: 0.4, maxOutputTokens: 800 },
    };
    const resp = await fetch(url, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body),
    });
    if (!resp.ok) {
        const errBody = await resp.text();
        throw new Error(`Gemini ${resp.status}: ${errBody.slice(0, 200)}`);
    }
    const data = await resp.json();
    const texto = data.candidates?.[0]?.content?.parts?.[0]?.text || "";
    return texto.trim();
}

/* Roteador: chama o provedor correto. */
async function reescreverComIA(promptSistema, texto, config) {
    const p = config.provedor || "pollinations";
    if (p === "pollinations") return await chamarPollinations(promptSistema, texto);
    if (p === "gemini") return await chamarGemini(config.chave, config.modelo, promptSistema, texto);
    return await chamarOpenAICompat(p, config.chave, config.modelo, promptSistema, texto);
}

/* =================== TESTE DE CONEXAO =================== */

/* Faz uma chamada simples para validar que o provedor responde corretamente.
 * Retorna { ok: bool, mensagem: string, resposta?: string }. */
async function testarConexaoIA(config) {
    try {
        const promptSistema = "Você é um assistente. Responda APENAS o que for pedido, em uma palavra.";
        const textoUsuario = "Diga a palavra: FUNCIONA";
        console.log("[Teste IA] Provedor:", config.provedor);
        const resposta = await reescreverComIA(promptSistema, textoUsuario, config);
        console.log("[Teste IA] Resposta:", resposta);

        const r = (resposta || "").toLowerCase();
        if (r.includes("deprecat") || r.includes("legacy") || r.includes("migrat")) {
            return {
                ok: false,
                mensagem: "Pollinations retornou aviso de deprecação. Tente Gemini (gratuito).",
                resposta: resposta,
            };
        }
        if (!resposta || resposta.length < 1) {
            return { ok: false, mensagem: "Resposta vazia do provedor.", resposta: "" };
        }
        return {
            ok: true,
            mensagem: `IA respondeu corretamente`,
            resposta: resposta.substring(0, 100),
        };
    } catch (e) {
        return {
            ok: false,
            mensagem: e.message,
            resposta: "",
        };
    }
}

/* =================== ADAPTACAO COMPLETA COM IA =================== */

/* Recebe a prova ja adaptada heuristicamente + perfil. Reescreve cada
 * enunciado e cada alternativa via IA. Aceita callback de progresso.
 * Lanca erro se a PRIMEIRA chamada falhar (assim o usuario sabe que algo deu errado). */
async function adaptarProvaComIA(prova, perfil, config, onProgress) {
    const promptSistema = construirPromptSistema(perfil);
    console.log("[IA] Configuração:", config);
    console.log("[IA] Provedor:", PROVEDORES[config.provedor]);
    console.log("[IA] Prompt do sistema:", promptSistema.substring(0, 200) + "...");

    let totalChamadas = 0;
    for (const q of prova.questoes) {
        totalChamadas += 1;
        totalChamadas += (q.alternativas || []).length;
    }
    let feitas = 0;
    let sucessos = 0;
    let primeiroErro = null;

    function reportar(msg) {
        if (onProgress) onProgress(feitas, totalChamadas, msg);
    }

    for (let i = 0; i < prova.questoes.length; i++) {
        const q = prova.questoes[i];

        /* Reescrever enunciado. */
        try {
            reportar(`Reescrevendo questão ${q.numero}...`);
            console.log(`[IA] Pedindo para reescrever questão ${q.numero}...`);
            const novoEnunciado = await reescreverComIA(
                promptSistema,
                `Enunciado original: """${q.enunciado}"""`,
                config
            );
            console.log(`[IA] Resposta da questão ${q.numero}:`, novoEnunciado);
            if (novoEnunciado && novoEnunciado.length > 5) {
                q.enunciado = novoEnunciado;
                sucessos++;
            } else {
                console.warn(`[IA] Resposta vazia ou muito curta na questão ${q.numero}`);
            }
        } catch (e) {
            console.error(`[IA] FALHA na questão ${q.numero}:`, e);
            if (!primeiroErro) primeiroErro = e;
            /* Se a primeira chamada ja falhou, aborta antes de gastar mais tempo. */
            if (sucessos === 0 && i === 0) {
                throw new Error(`Falha na primeira chamada à IA: ${e.message}`);
            }
        }
        feitas++;

        /* Reescrever alternativas. */
        for (let j = 0; j < (q.alternativas || []).length; j++) {
            const alt = q.alternativas[j];
            try {
                reportar(`Reescrevendo alternativa ${alt.letra} da questão ${q.numero}...`);
                const novoTexto = await reescreverComIA(
                    promptSistema,
                    `Alternativa de prova: """${alt.texto}"""\nReescreva esta alternativa (curta, mesma resposta).`,
                    config
                );
                if (novoTexto && novoTexto.length > 0 && novoTexto.length < alt.texto.length * 4) {
                    alt.texto = novoTexto;
                    sucessos++;
                }
            } catch (e) {
                console.warn(`[IA] Falha na alt ${alt.letra} da questão ${q.numero}:`, e.message);
                if (!primeiroErro) primeiroErro = e;
            }
            feitas++;
        }
    }

    console.log(`[IA] Sucessos: ${sucessos} de ${totalChamadas}`);

    if (sucessos === 0) {
        throw new Error(`Nenhuma reescrita foi bem-sucedida. Primeiro erro: ${primeiroErro ? primeiroErro.message : 'desconhecido'}`);
    }

    reportar(`Concluído! ${sucessos}/${totalChamadas} reescritas com sucesso.`);
    return prova;
}
