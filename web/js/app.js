/* app.js
 *
 * Orquestracao da UI: liga eventos, chama parser/adaptador, renderiza saidas.
 */

(function () {
    'use strict';

    /* =================== ESTADO =================== */

    const estado = {
        provaOriginal: null,
        provaAdaptada: null,
        perfilSelecionado: null,
        grau: 3,
        textoAdaptado: "",
        textoOriginal: "",
    };

    /* =================== HELPERS =================== */

    const $ = (sel) => document.querySelector(sel);
    const $$ = (sel) => document.querySelectorAll(sel);

    function toast(msg, tipo = "") {
        const t = $('#toast');
        t.textContent = msg;
        t.className = `toast visivel ${tipo}`;
        clearTimeout(toast._timer);
        toast._timer = setTimeout(() => {
            t.classList.remove('visivel');
        }, 2800);
    }

    /* =================== TEMA =================== */

    function aplicarTema(tema) {
        document.documentElement.setAttribute('data-tema', tema);
        try { localStorage.setItem('adaptaprovas-tema', tema); } catch {}
    }
    function alternarTema() {
        const atual = document.documentElement.getAttribute('data-tema') || 'light';
        aplicarTema(atual === 'light' ? 'dark' : 'light');
    }
    /* Inicializa tema com preferencia do sistema OU localStorage. */
    (function initTema() {
        let tema;
        try { tema = localStorage.getItem('adaptaprovas-tema'); } catch {}
        if (!tema) {
            tema = window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
        }
        aplicarTema(tema);
    })();

    /* =================== ABAS DE ENTRADA =================== */

    function setupAbas() {
        $$('.aba').forEach(aba => {
            aba.addEventListener('click', () => {
                const id = aba.dataset.aba;
                $$('.aba').forEach(a => {
                    a.classList.remove('ativa');
                    a.setAttribute('aria-selected', 'false');
                });
                aba.classList.add('ativa');
                aba.setAttribute('aria-selected', 'true');
                $$('.aba-painel').forEach(p => p.classList.remove('ativa'));
                $(`.aba-painel[data-painel="${id}"]`).classList.add('ativa');
            });
        });
    }

    /* =================== PERFIS UI =================== */

    function renderizarPerfis() {
        const grid = $('#perfis-grid');
        grid.innerHTML = '';
        for (let i = 0; i < PERFIS.length; i++) {
            const p = PERFIS[i];
            const card = document.createElement('button');
            card.className = 'perfil-card';
            card.dataset.tipo = p.tipo;
            card.innerHTML = `
                <span class="emoji">${p.emoji}</span>
                <strong>${p.nome}</strong>
                <small>${p.tldr}</small>
            `;
            card.addEventListener('click', () => selecionarPerfil(p.tipo));
            grid.appendChild(card);
        }
    }

    function selecionarPerfil(tipo) {
        estado.perfilSelecionado = tipo;
        $$('.perfil-card').forEach(c => c.classList.toggle('selecionado', parseInt(c.dataset.tipo) === tipo));
        atualizarBotaoAdaptar();
    }

    /* =================== GRAU =================== */

    function setupSlider() {
        const slider = $('#grau-slider');
        slider.addEventListener('input', () => {
            estado.grau = parseInt(slider.value, 10);
            $('#grau-numero').textContent = estado.grau;
            $('#grau-label').textContent = perfilDescricaoGrau(estado.grau);
            /* Atualiza preenchimento visual da barra. */
            const pct = ((estado.grau - 1) / 4) * 100;
            slider.style.background = `linear-gradient(to right, var(--primaria) 0%, var(--primaria) ${pct}%, var(--borda) ${pct}%, var(--borda) 100%)`;
        });
        slider.dispatchEvent(new Event('input'));
    }

    /* =================== ENTRADA: COLAR =================== */

    function processarColado() {
        const texto = $('#campo-prova').value;
        const titulo = $('#campo-titulo').value;
        const disciplina = $('#campo-disciplina').value;

        if (!texto.trim()) {
            toast('Cole o texto da prova primeiro!', 'erro');
            return;
        }

        const prova = parseProva(texto, titulo, disciplina);
        if (!prova.questoes || prova.questoes.length === 0) {
            toast('Não consegui identificar nenhuma questão. Confira o formato (1. … a) … b) …)', 'erro');
            return;
        }

        estado.provaOriginal = prova;
        toast(`✓ ${prova.questoes.length} questão(ões) detectada(s)`, 'sucesso');
        atualizarBotaoAdaptar();
        rolarSuave('#painel-perfil');
    }

    /* =================== ENTRADA: ARQUIVO =================== */

    function setupDropzone() {
        const dz = $('#dropzone');
        const input = $('#campo-arquivo');

        dz.addEventListener('click', () => input.click());
        input.addEventListener('change', () => {
            if (input.files && input.files[0]) lerArquivo(input.files[0]);
        });

        ['dragenter', 'dragover'].forEach(e => {
            dz.addEventListener(e, ev => { ev.preventDefault(); dz.classList.add('ativo'); });
        });
        ['dragleave', 'drop'].forEach(e => {
            dz.addEventListener(e, ev => { ev.preventDefault(); dz.classList.remove('ativo'); });
        });
        dz.addEventListener('drop', ev => {
            if (ev.dataTransfer.files && ev.dataTransfer.files[0]) {
                lerArquivo(ev.dataTransfer.files[0]);
            }
        });
    }

    function lerArquivo(arquivo) {
        if (!/\.(txt|ini)$/i.test(arquivo.name)) {
            toast('Apenas arquivos .txt são aceitos.', 'erro');
            return;
        }
        const reader = new FileReader();
        reader.onload = () => {
            const texto = reader.result;
            const prova = parseProva(texto);
            if (!prova.questoes || prova.questoes.length === 0) {
                toast('Arquivo sem questões reconhecíveis.', 'erro');
                return;
            }
            estado.provaOriginal = prova;
            toast(`✓ Carregado "${arquivo.name}" — ${prova.questoes.length} questão(ões)`, 'sucesso');
            atualizarBotaoAdaptar();
            rolarSuave('#painel-perfil');
        };
        reader.onerror = () => toast('Erro ao ler arquivo.', 'erro');
        reader.readAsText(arquivo, 'utf-8');
    }

    /* =================== ENTRADA: EXEMPLO =================== */

    function setupExemplos() {
        $$('.exemplo-card').forEach(card => {
            card.addEventListener('click', () => {
                const id = card.dataset.exemplo;
                const ex = EXEMPLOS[id];
                if (!ex) return;
                const prova = parseProva(ex.texto, ex.titulo, ex.disciplina);
                estado.provaOriginal = prova;
                /* Mostra na aba "colar" para facilitar edicao. */
                $('#campo-titulo').value = ex.titulo;
                $('#campo-disciplina').value = ex.disciplina;
                $('#campo-prova').value = ex.texto;
                toast(`✓ Exemplo carregado: "${ex.titulo}"`, 'sucesso');
                atualizarBotaoAdaptar();
                rolarSuave('#painel-perfil');
            });
        });
    }

    /* =================== ADAPTAR =================== */

    function atualizarBotaoAdaptar() {
        const btn = $('#btn-adaptar');
        const ok = estado.provaOriginal && estado.perfilSelecionado !== null;
        btn.disabled = !ok;
    }

    async function adaptar() {
        if (!estado.provaOriginal || estado.perfilSelecionado === null) {
            toast('Carregue uma prova e selecione um perfil.', 'erro');
            return;
        }
        const base = PERFIS[estado.perfilSelecionado];
        const perfil = perfilCompor(base, estado.grau);

        /* Etapa 1: adaptacao heuristica (rapida, estrutura). */
        let adaptada = adaptarProva(estado.provaOriginal, perfil);

        /* Etapa 2 (opcional): reescrita semantica com IA. */
        const cfgIA = obterConfigIA();
        if (cfgIA.ativo) {
            const provedor = PROVEDORES[cfgIA.provedor];
            if (provedor && provedor.precisaChave && !cfgIA.chave) {
                toast(`O provedor ${provedor.nome} precisa de uma chave. Configure ou desligue a IA.`, 'erro');
                return;
            }

            const btn = $('#btn-adaptar');
            btn.disabled = true;
            const textoBtn = btn.innerHTML;
            btn.innerHTML = '<span style="display:inline-block;width:18px;height:18px;border:2px solid #fff5;border-top-color:#fff;border-radius:50%;animation:girar .8s linear infinite;"></span> Reescrevendo com IA...';

            $('#ia-progresso').classList.remove('oculto');
            $('#ia-progresso-fill').style.width = '0%';

            try {
                adaptada = await adaptarProvaComIA(adaptada, perfil, cfgIA, (feitas, total, msg) => {
                    const pct = total > 0 ? Math.round((feitas / total) * 100) : 0;
                    $('#ia-progresso-fill').style.width = pct + '%';
                    $('#ia-progresso-num').textContent = `${feitas}/${total}`;
                    $('#ia-progresso-texto').textContent = msg;
                });
                toast('✓ IA concluiu a reescrita das questões', 'sucesso');
            } catch (e) {
                console.error('Erro na IA:', e);
                toast(`IA falhou: ${e.message}. Mantendo adaptação heurística.`, 'erro');
            } finally {
                $('#ia-progresso').classList.add('oculto');
                btn.disabled = false;
                btn.innerHTML = textoBtn;
            }
        }

        estado.provaAdaptada = adaptada;
        estado.textoAdaptado = renderizarProvaAdaptada(adaptada, perfil);
        estado.textoOriginal = renderizarProvaOriginal(estado.provaOriginal);

        $('#saida-adaptada').textContent = estado.textoAdaptado;
        $('#saida-original').textContent = estado.textoOriginal;
        $('#saida-adaptada-2').textContent = estado.textoAdaptado;
        $('#saida-original-2').textContent = estado.textoOriginal;

        $('#painel-saida').classList.remove('oculto');
        toast(`✓ Prova adaptada para ${perfil.nome} (grau ${perfil.grau})`, 'sucesso');
        rolarSuave('#painel-saida');
    }

    /* =================== ABAS DE SAIDA =================== */

    function setupAbasSaida() {
        $$('.aba-saida').forEach(aba => {
            aba.addEventListener('click', () => {
                $$('.aba-saida').forEach(a => a.classList.remove('ativa'));
                aba.classList.add('ativa');
                const tipo = aba.dataset.saida;
                $('#saida-adaptada').classList.toggle('ativa', tipo === 'adaptada');
                $('#saida-original').classList.toggle('ativa', tipo === 'original');
                $('#saida-lado-lado').classList.toggle('ativa', tipo === 'lado-lado');
            });
        });
    }

    /* =================== EXPORTAR =================== */

    function baixarTxt() {
        if (!estado.textoAdaptado) return;
        const nome = (estado.provaAdaptada && estado.provaAdaptada.titulo)
            ? estado.provaAdaptada.titulo.replace(/[\\\/:*?"<>|]/g, '').slice(0, 60)
            : 'prova-adaptada';
        const blob = new Blob([estado.textoAdaptado], { type: 'text/plain;charset=utf-8' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `${nome}.txt`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
        toast('✓ Arquivo baixado', 'sucesso');
    }

    function copiar() {
        if (!estado.textoAdaptado) return;
        navigator.clipboard.writeText(estado.textoAdaptado).then(
            () => toast('✓ Copiado para a área de transferência', 'sucesso'),
            () => toast('Falha ao copiar.', 'erro')
        );
    }

    function imprimir() {
        if (!estado.textoAdaptado) return;
        window.print();
    }

    /* =================== UTIL =================== */

    function rolarSuave(seletor) {
        const el = $(seletor);
        if (el) el.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }

    function limpar() {
        $('#campo-titulo').value = '';
        $('#campo-disciplina').value = '';
        $('#campo-prova').value = '';
        estado.provaOriginal = null;
        estado.provaAdaptada = null;
        $('#painel-saida').classList.add('oculto');
        atualizarBotaoAdaptar();
        toast('Campos limpos');
    }

    /* =================== PAINEL DE IA =================== */

    function setupIA() {
        const cfg = obterConfigIA();
        const toggle = $('#ia-toggle');
        const provedorSelect = $('#ia-provedor');
        const chaveInput = $('#ia-chave');
        const modeloInput = $('#ia-modelo');
        const configBtn = $('#ia-config-btn');
        const configBox = $('#ia-config');
        const chaveGrupo = $('.ia-chave-grupo');
        const modeloGrupo = $('.ia-modelo-grupo');

        /* Restaurar estado salvo. */
        toggle.checked = cfg.ativo;
        provedorSelect.value = cfg.provedor || 'pollinations';
        chaveInput.value = cfg.chave || '';
        modeloInput.value = cfg.modelo || '';

        function atualizarVisibilidade() {
            const aberto = toggle.checked;
            configBox.classList.toggle('oculto', !aberto);

            const provedorAtual = PROVEDORES[provedorSelect.value];
            const precisaChave = provedorAtual && provedorAtual.precisaChave;
            chaveGrupo.classList.toggle('oculto', !precisaChave);
            modeloGrupo.classList.toggle('oculto', !precisaChave);
        }

        function salvar() {
            salvarConfigIA({
                ativo: toggle.checked,
                provedor: provedorSelect.value,
                chave: chaveInput.value.trim(),
                modelo: modeloInput.value.trim(),
            });
        }

        toggle.addEventListener('change', () => {
            atualizarVisibilidade();
            salvar();
            if (toggle.checked) {
                const p = PROVEDORES[provedorSelect.value];
                toast(`✓ IA ativada: ${p.nome}`, 'sucesso');
            }
        });
        provedorSelect.addEventListener('change', () => {
            atualizarVisibilidade();
            salvar();
        });
        chaveInput.addEventListener('input', salvar);
        modeloInput.addEventListener('input', salvar);
        configBtn.addEventListener('click', () => {
            toggle.checked = !toggle.checked;
            toggle.dispatchEvent(new Event('change'));
        });

        /* Botao "Testar conexao". */
        const testarBtn = $('#ia-testar-btn');
        const testarResultado = $('#ia-teste-resultado');
        if (testarBtn) {
            testarBtn.addEventListener('click', async () => {
                salvar();
                const cfgAtual = obterConfigIA();
                testarBtn.disabled = true;
                const textoOriginal = testarBtn.innerHTML;
                testarBtn.innerHTML = '<span style="display:inline-block;width:16px;height:16px;border:2px solid currentColor;border-top-color:transparent;border-radius:50%;animation:girar .8s linear infinite;"></span> Testando...';
                testarResultado.textContent = '';
                testarResultado.className = 'ia-teste-resultado';

                try {
                    const r = await testarConexaoIA(cfgAtual);
                    if (r.ok) {
                        testarResultado.textContent = `✓ ${r.mensagem} → "${r.resposta}"`;
                        testarResultado.classList.add('sucesso');
                        toast(`✓ ${PROVEDORES[cfgAtual.provedor].nome} funcionando!`, 'sucesso');
                    } else {
                        testarResultado.textContent = `✗ ${r.mensagem}`;
                        testarResultado.classList.add('erro');
                        toast(`Falha no teste: ${r.mensagem}`, 'erro');
                    }
                } catch (e) {
                    testarResultado.textContent = `✗ ${e.message}`;
                    testarResultado.classList.add('erro');
                    toast(`Erro: ${e.message}`, 'erro');
                } finally {
                    testarBtn.disabled = false;
                    testarBtn.innerHTML = textoOriginal;
                }
            });
        }

        atualizarVisibilidade();
    }

    /* =================== INIT =================== */

    document.addEventListener('DOMContentLoaded', () => {
        setupAbas();
        renderizarPerfis();
        setupSlider();
        setupDropzone();
        setupExemplos();
        setupAbasSaida();
        setupIA();

        $('#btn-tema').addEventListener('click', alternarTema);
        $('#btn-processar-colar').addEventListener('click', processarColado);
        $('#btn-limpar').addEventListener('click', limpar);
        $('#btn-adaptar').addEventListener('click', adaptar);
        $('#btn-baixar').addEventListener('click', baixarTxt);
        $('#btn-copiar').addEventListener('click', copiar);
        $('#btn-imprimir').addEventListener('click', imprimir);

        /* Atalho: Ctrl+Enter no textarea processa. */
        $('#campo-prova').addEventListener('keydown', (e) => {
            if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
                e.preventDefault();
                processarColado();
            }
        });
    });

})();
