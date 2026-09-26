# 📑 REVALIDAÇÃO E ESTRUTURA COMPLETA DO PROJETO
**Projeto:** PartePlay / PlayScore
**Data:** 25/09/2026 | **Status:** Em desenvolvimento — núcleo de sincronia implementado, retomada
**Ambiente:** MuseScore 4 (VST3) + DAWs compatíveis
**Pilha:** C++ / JUCE 8.x / Visual Studio 2022

---

## 🎯 1. REVISÃO DA IDEIA E PROPÓSITO

### O que resolve
> Músico/arranjador escreve a parte de um instrumento transpositor na partitura, carrega o áudio de referência e **o que se ouve bate exatamente com o que está escrito — sem atraso, sem descompasso, na tonalidade correta para o instrumento**.

### Fluxo do usuário
```
1. Abre MuseScore 4 → carrega o plugin
2. Carrega áudio de referência (WAV/FLAC/MP3)
3. Seleciona instrumento: Piano(C), Trompete(Si♭), Sax Tenor(Si♭), Sax Alto(Mi♭), Trompa(Fá), etc.
4. Ajusta afinação de referência (padrão 440Hz — 432–445Hz)
5. Dá Play → partitura toca SINCRONIZADA + áudio transposto automaticamente
```

### Diferencial de mercado
- ✅ Sincronia real via relógio do hospedeiro (não relógio próprio) → ZERO descompasso
- ✅ Transposição por instrumento + afinação de referência em um só lugar
- ✅ Feito **para educação e arranjo** — não é efeito genérico
- ✅ Brasileiro, multilíngue, acessível

---

## ⚖️ 2. TRADEOFFS PREVISTOS

| Decisão | Ganho | Custo / Risco | Mitigação |
|---|---|---|---|
| **JUCE + GPLv3** | Compatibilidade total VST3/MuseScore, comunidade grande, código aberto | Se vender → precisa licença comercial (~US$ 400/ano) ou manter código aberto | Modelo híbrido: versão livre GPL + licença comercial fechada para recursos avançados |
| **Pitch-shifting em tempo real** | Transposição instantânea sem interromper reprodução | Uso de CPU; artefatos sonoros em notas longas | Usar algoritmo Rubber Band (integrável ao JUCE); permitir qualidade/buffer ajustável |
| **Sincronia via tempo do hospedeiro** | Perfeita, funciona com avanço/retrocesso do MuseScore | Depende da precisão do relógio do hospedeiro | Validar contra múltiplas versões do MuseScore; fallback de tolerância |
| **Suporte a MP3** | Ampla adoção | Decodificação em tempo real = processamento extra | Decodificar para buffer na memória no carregamento, não em tempo real |
| **Doação voluntária → modelo comercial** | Lançamento rápido, validação de demanda | Risco de confusão entre "gratuito" e "pago" | Nomes claros: **PlayScore Free** vs **PlayScore Pro**; licenças distintas |

---

## 🔄 3. MELHORIAS CONTÍNUAS — ROTEIRO DE EVOLUÇÃO

### Fase 1 ✅ Concluído — Base
- [x] Ambiente: VS2022 + JUCE + MuseScore 4
- [x] Projeto VST3 criado e compilado
- [x] Sincronia com tempo do hospedeiro funcional
- [x] Carregamento WAV/FLAC/MP3
- [x] Interface mínima + carregador de arquivo

### Fase 2 ⏳ Em andamento — Transposição Inteligente
- [ ] Pitch-shifting com time-stretching independente
- [ ] Tabela de instrumentos e semitons de transposição
- [ ] Controle de afinação de referência (432–445Hz)
- [ ] Teste de validação: tom de referência × partitura × áudio
- [ ] Tratamento de borda: fim de arquivo, parada/retorno, loop

### Fase 3 — Interface e Usabilidade
- [ ] Visualização de forma de onda
- [ ] Seleção de instrumento em menu amigável
- [ ] Controles deslizantes de afinação
- [ ] Indicador de sincronia e status
- [ ] Suporte multilíngue (PT-BR / EN / ES)

### Fase 4 — Recursos Avançados (candidatos à versão Pro)
- [ ] Mapeamento de afinações regionais (viola caipira, cordas características)
- [ ] Salva/recupera presets por arranjo/partitura
- [ ] Processamento em lote — transpor arquivos sem abrir o MuseScore
- [ ] Modo treino: repetição de trecho, ajuste de velocidade sem alterar tom
- [ ] Análise de tonalidade automática do áudio

### Fase 5 — Ecossistema
- [ ] Extensão para DAWs (Além do MuseScore: Reaper, Cubase, Studio One)
- [ ] Versão LV2 (Linux/Ardour)
- [ ] Biblioteca de timbres brasileiros como conteúdo complementar

---

## 💰 4. PROJETO COMO NEGÓCIO — MODELO MODULAR

### Estrutura de Produto
| Camada | Nome | Licença | Código | Público |
|---|---|---|---|---|
| 🆓 **Free** | PlayScore Free | GPLv3 | Aberto | Estudantes, professores, amadores |
| 💎 **Pro** | PlayScore Pro | Comercial / Fechado | Código proprietário | Arranjadores profissionais, estúdios, escolas |
| 🏫 **Empresarial** | PlayScore Edu | Licença anual | Suporte dedicado | Redes de ensino, conservatórios, projetos sociais |

### O que diferencia a versão Pro
- ✅ Afinações personalizadas e presets salvos
- ✅ Processamento em lote e modo independente
- ✅ Sem obrigação de liberação de código
- ✅ Atualizações prioritárias
- ✅ Suporte técnico direto
- ✅ Instalação em múltiplas máquinas

### Fluxo de Receita
```
1. Usuário → baixa Free → usa → sente necessidade de recursos avançados
2. Site → comparação Free × Pro → adquire licença anual/perpétua
3. Receita → reinvestimento em melhorias + sustentação do projeto
4. Parte do valor → fundo de versões educacionais subsidiadas
```

### Preços sugeridos (estudo inicial)
- **PlayScore Free**: Gratuito + doação voluntária US$ 3–5
- **PlayScore Pro**: US$ 29/perpétua ou US$ 12/ano
- **PlayScore Edu**: US$ 5/máquina/ano (lotes mínimos)
- **Empresarial**: sob consulta

### Plataforma de Venda
- Página própria + **Gumroad** ou **Paddle** → emite licença, entrega download, cuida de impostos
- Recebimento → Conta Global Banco Inter (USD)
- Ko-fi mantido para doações voluntárias ao projeto livre

### Estratégia de Código
- **Free**: repositório público GitHub (GPLv3) — contribuições bem-vindas
- **Pro**: desenvolvimento em repositório privado Azure DevOps — funcionalidades isoladas
- **Regra**: tudo que é GPL permanece GPL; Pro agrega módulos separados com licença comercial

---

## 📌 5. PLANO DE RETOMADA — PRÓXIMOS PASSOS

### Imediato (1–2 semanas)
1. [ ] Confirmar resultado dos seus testes locais — funcionou? onde travou?
2. [ ] Implementar transposição com Rubber Band Library
3. [ ] Adicionar tabela de instrumentos e controle de afinação
4. [ ] Documentar tudo no Wiki do Azure DevOps

### Curto prazo (1 mês)
1. [ ] Interface polida e usabilidade
2. [ ] Versão beta fechada — convidar 5–10 músicos/arranjadores para testar
3. [ ] Preparar página de lançamento
4. [ ] Definir licença comercial e fluxo de entrega

### Médio prazo (3 meses)
1. [ ] Lançamento oficial — Free + Pro
2. [ ] Gravar tutoriais em PT-BR e EN
3. [ ] Contato com escolas e projetos de educação musical
4. [ ] Primeiro ciclo de receita → reinvestimento

---

## ✅ RESUMÃO
- ✅ O **coração do problema** está resolvido na base: sincronia perfeita
- ⏳ Falta implementar **transposição inteligente** — próximo passo técnico
- 💰 Modelo de negócio definido: **Free aberto + Pro fechado** — respeita GPL e gera sustentabilidade
- 📂 Repositórios separados: Azure (privado/Pro) + GitHub (público/Free)
- 🚀 Próxima ação: **seu retorno com o feedback dos testes** → implementação da Fase 2

---

Ficou tudo alinhado com o que você imagina? Quer que eu já entre direto no código da **transposição com Rubber Band**, ou prefere primeiro conversarmos sobre o modelo de negócio e licença? 🎵💻
