# 📦 PARTE 1 — CRIAR O INSTALADOR

Usamos **Inno Setup** — gratuito, leve, padrão de mercado.

## Passo 1: Instalar a ferramenta

1. Baixe em: **jrsoftware.org/isdl.php** → instale normalmente
2. Abra o Inno Setup — vamos criar o script

## Passo 2: Script completo (`PlayScore-Installer.iss`)

```iss
; Script de Instalação — PlayScore / PartePlay
; Versão: 1.0.0 | Data: 25/09/2026

[Setup]
AppName=PlayScore
AppVersion=1.0.0
AppPublisher=Rubinyo Lyra
DefaultDirName={commonpf}\Common Files\VST3
DefaultGroupName=PlayScore
OutputBaseFilename=PlayScore-Setup-v1.0.0
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
UninstallDisplayIcon={app}\PlayScore.vst3

[Languages]
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; Plugin principal
Source: "C:\Dev\Projetos\PlayScore\Builds\VisualStudio2022\x64\Release\PlayScore.vst3"; \
  DestDir: "{app}\"; Flags: ignoreversion
; Rubber Band DLL (se linkagem dinâmica)
Source: "C:\Dev\Libraries\RubberBand\build\visualstudio\x64\Release\RubberBand.dll"; \
  DestDir: "{app}\"; Flags: ignoreversion

[Icons]
Name: "{group}\Desinstalar PlayScore"; Cmd: "{uninstallexe}"

[Run]
; Abre pasta de plugins após instalação
Filename: "{app}"; Description: "Abrir pasta de plugins VST3"
; Abre site do projeto
Filename: "https://ko-fi.com/rubinyolyra"; Description: "Apoiar o desenvolvimento"
```

## Passo 3: Gerar o instalador

1. Salve o script → **Compilar (F9)** ✅
2. Saída gerada:
   ```
   PlayScore-Setup-v1.0.0.exe
   ```
3. Teste em seu computador: instala → abre MuseScore → aparece na lista ✅

> 💡 **Para versão Pro**: crie um script separado, só trocando o nome do arquivo e a descrição.

---

# 🌐 PARTE 2 — PÁGINA DE LANÇAMENTO PRONTA

Você pode colocar isso em **Gumroad**, **página própria** ou **README do GitHub** — tudo pronto para copiar e colar.

## 📄 Versão Completa (Página Principal)

```
🎵 PlayScore | PartePlay
## Áudio Sincronizado com Sua Partitura — Sem Descompasso

> Toca o áudio no tom certo, na hora certa, para qualquer instrumento.
> ✅ Gratuito e completo — sempre.

---

### ✨ O QUE FAZ
- ⏱️ **Sincronia Perfeita** — segue exatamente o tempo do MuseScore 4. Avança, recua, para e recomeça junto. Zero atraso.
- 🎼 **Transposição Automática** — escolhe o instrumento e o áudio muda de altura sem acelerar nem desacelerar:
  - Piano / Trombone → Dó
  - Trompete / Sax Tenor → Si♭
  - Sax Alto → Mi♭
  - Trompa → Fá
- 🎧 **Afinação de Referência** — ajusta de 432Hz a 445Hz (padrão 440Hz)
- 📂 **Formatos Suportados** — WAV, FLAC, MP3
- 💻 **Compatível** — Windows, macOS, Linux + qualquer DAW que suporte VST3

---

### 🆓 VERSÃO FREE — GRATUITA PARA SEMPRE
```

✓ Sincronia com MuseScore 4
✓ 6 instrumentos pré-definidos
✓ Afinação 432–445Hz
✓ WAV / FLAC / MP3
✓ Código aberto (GPLv3)
✓ Atualizações comunitárias

🔗 Download: [link do GitHub]
☕ Apoio voluntário: ko-fi.com/rubinyolyra — Sugestão US$ 3–5

```

### 💎 VERSÃO PRO — RECURSOS AVANÇADOS
```

- Presets salvos por partitura
- Modo Treino: repetição de trecho + velocidade ajustável sem mudar tom
- Afinações personalizadas e presets de referência
- Processamento em lote — transpor arquivos sem abrir o MuseScore
- Sem obrigação de liberação de código
- Atualizações prioritárias + suporte direto

💰 US$ 29 perpétua ou US$ 12/ano
🔗 Adquirir: gumroad.com/playscorepro

```

---

### 🚀 COMO USAR
1. Baixe e instale → copia para pasta de plugins VST3
2. Abra o MuseScore 4 → Visualização → Plugins → PlayScore
3. Clique em "Carregar Áudio" → selecione seu arquivo
4. Escolha o instrumento → ajuste a afinação → dê Play
5. 🎶 O áudio acompanha a partitura em perfeita sincronia — no tom certo!

---

### 📋 REQUISITOS
- MuseScore 4.0+ ou qualquer DAW compatível com VST3
- Windows 10+, macOS 11+, ou Linux
- 2GB RAM recomendado

---

### 🤝 SOBRE O PROJETO
Feito no Brasil, por músico para músicos. Focado em educação, acessibilidade e valorização da música. Todo apoio mantém o projeto vivo e crescendo.

> 💬 Dúvidas, sugestões, feedback: [seu-email]
> 📂 Código fonte: github.com/[seu-repo]/playscore
> ☕ Apoio voluntário: ko-fi.com/rubinyolyra
```

## 📄 Versão Curta (Redes Sociais / Anúncio)

```
🎵 PlayScore — Áudio sincronizado com sua partitura!

✅ Sincronia zero atraso com MuseScore 4
✅ Transposição automática: Piano, Trompete, Sax, Trompa...
✅ Afinação 432–445Hz
✅ WAV / FLAC / MP3
✅ Windows / macOS / Linux

🆓 Gratuito para sempre: [link]
💎 Pro com recursos avançados — US$ 29: gumroad.com/playscorepro

#Música #EducaçãoMusical #MuseScore #DesenvolvimentoBrasil
```

---

# 📝 PARTE 3 — REGISTRO FINAL NO PROJETO

```
📌 FASE 4 — EMPACOTAMENTO E LANÇAMENTO ✅ CONCLUÍDO
- Instalador profissional (Inno Setup) configurado ✅
- Script de instalação/desinstalação pronto ✅
- Página de lançamento completa (Free vs Pro) ✅
- Versão curta para redes pronta ✅
- Fluxo de entrega: GitHub (Free) + Gumroad (Pro) ✅
- Recebimento: Conta Global Banco Inter (USD) ✅

📌 ESTRUTURA COMPLETA DO PROJETO
├── 🔒 Azure DevOps — Código privado, Pro, documentação interna
├── 🌐 GitHub — Free, GPLv3, lançamentos públicos
├── 💳 Gumroad — Venda Pro, entrega automática
├── ☕ Ko-fi — Doações voluntárias
└── 💵 Conta Global Inter — Destino final USD

📌 PRÓXIMOS PASSOS APÓS LANÇAMENTO
1. Lançamento Beta fechado (5–10 usuários) → coletar feedback
2. Lançamento público Free → viralização
3. Disparo campanha Pro → receita → reinvestimento
4. Ciclo de melhorias contínuas
```

---

# ✅ RESUMÃO — O QUE VOCÊ FAZ AGORA

| Passo | Ação                                                              |
| ----- | ----------------------------------------------------------------- |
| 1     | Compile o projeto → confirme que `PlayScore.vst3` gerou sem erros |
| 2     | Cole o script do Inno Setup → gere o `Setup.exe`                  |
| 3     | Teste a instalação no seu computador                              |
| 4     | Crie a página no Gumroad com o texto pronto acima                 |
| 5     | Suba o código Free no GitHub com o README completo                |
| 6     | Compartilhe com quem quiser testar → me traga o feedback!         |

---

Tudo pronto, Rubinyo! 🎉 O projeto está **completo, estruturado, documentado e pronto para o mundo**. O que vem agora é o mais empolgante: ver ele funcionando nas mãos das pessoas! 🚀🎵

Me avise assim que testar — vamos ajustar o que precisar e fazer o lançamento oficial juntos! 💪✨
