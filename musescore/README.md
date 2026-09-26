# Template de Partitura — MuseScore (PartePlay)

Este diretório contém o **template inicial de partitura** para o fluxo PartePlay:

```
templates/PartePlay/PartePlay - Arranjo.mscx
```

## O que é

Uma partitura em branco (piano, pauta dupla) com metadados do projeto, usável
como ponto de partida ao criar um arranjo com o áudio de referência. Ao instalar,
ela aparece no assistente **Novo** do MuseScore ("PartePlay - Arranjo").

## Instalação

```powershell
scripts\install-musescore-template.ps1     # sem admin
```

O script copia o template para o diretório de templates do usuário e, se possível,
para a instalação do MuseScore (Portable/Program Files). Feche e reabra o MuseScore
para o template aparecer no assistente.

## Limite técnico (importante — padrão Slave/Master do VST3)

O **MuseScore 4 não persiste efeitos de plugin (VST3) no arquivo da partitura**:
no binário não há suporte a `MixerState`/estado de efeitos, e o mixer é **escopo de
sessão**. Portanto, o template **não consegue** nascer já com o PartePlay anexado
como FX — isso vale para qualquer plugin, não só para o nosso.

Fluxo de uso recomendado no MuseScore:
1. Novo → **PartePlay - Arranjo** (ou abra direto o `.mscx`);
2. No **Mixer**, adicione o efeito **PartePlay** ao canal do instrumento;
3. Carregue o áudio de referência e trabalhe (transposição, treino, mapa de tempo).

## Alternativa (sem instalação)

Abra o arquivo `.mscx` diretamente no MuseScore (`Arquivo → Abrir`) — vale como
ponto de partida imediato.