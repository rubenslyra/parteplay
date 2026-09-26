# 🎵 PARTE 1 — CÓDIGO: TRANSPOSIÇÃO INTELIGENTE + RUBBER BAND

Vamos integrar a biblioteca **Rubber Band** — padrão mundial de qualidade para pitch-shifting com time-stretching independente. O som muda de altura **sem acelerar nem desacelerar**.

## 📦 Passo 1 — Adicionar Rubber Band ao Projeto

1. Baixe a biblioteca: **github.com/breakfastquay/rubberband** → versão mais recente
2. Extraia para: `C:\Dev\Libraries\RubberBand`
3. No **Projucer**:
   - Abra o projeto → **Configurações do Projeto**
   - **Header Search Paths** → adicione: `C:\Dev\Libraries\RubberBand\include`
   - **Library Search Paths** → adicione: `C:\Dev\Libraries\RubberBand\lib`
   - **External Libraries** → adicione: `rubberband`

## 📂 Passo 2 — Atualizar `PluginProcessor.h`

```cpp
#pragma once

#include <JuceHeader.h>
#include <rubberband/RubberBandStretcher.h>

//==============================================================================
enum class Instrument
{
    PIANO_C = 0,        // 0 semitons — Dó
    TRUMPET_Bb = -2,    // -2 semitons → Si♭
    TROMBONE_C = 0,     // 0 — Dó
    SAX_TENOR_Bb = -2,  // -2 — Si♭
    SAX_ALTO_Eb = 3,    // +3 — Mi♭
    FRENCH_HORN_F = -5, // -5 — Fá
    CUSTOM = 99         // Ajuste manual
};

//==============================================================================
class PlayScoreProcessor  : public juce::AudioProcessor
{
public:
    PlayScoreProcessor();
    ~PlayScoreProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override;

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ===== MÉTODOS PÚBLICOS =====
    void loadAudioFile (const juce::File& file);
    void setInstrument (Instrument instr);
    void setReferencePitch (double hz);
    void setTransposeSemitones (int semitons);

    Instrument getInstrument() const { return currentInstrument; }
    double getReferencePitch() const { return referencePitchHz; }
    int getTransposeSemitones() const { return semitonesToTranspose; }

private:
    // ===== ÁUDIO E SINCRONIA =====
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioBuffer<float>> originalAudio;
    double fileSampleRate = 0.0;

    // ===== TRANSPOSIÇÃO =====
    std::unique_ptr<RubberBand::RubberBandStretcher> stretcher;
    Instrument currentInstrument = Instrument::PIANO_C;
    int semitonesToTranspose = 0;
    double referencePitchHz = 440.0;
    double baseFilePitchHz = 440.0;

    void rebuildStretcher();
    double calculatePitchRatio() const;

    // ===== CONTROLE DE FLUXO =====
    int64_t lastProcessedSample = -1;
    bool isFirstRunAfterSeek = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayScoreProcessor)
};
```

## 📂 Passo 3 — Atualizar `PluginProcessor.cpp`

```cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"

PlayScoreProcessor::PlayScoreProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats();
}

PlayScoreProcessor::~PlayScoreProcessor() { releaseResources(); }

const juce::String PlayScoreProcessor::getName() const { return "PlayScore | PartePlay"; }
bool PlayScoreProcessor::acceptsMidi() const { return false; }
bool PlayScoreProcessor::producesMidi() const { return false; }
bool PlayScoreProcessor::isMidiEffect() const { return false; }
double PlayScoreProcessor::getTailLengthSeconds() const { return 0.0; }

int PlayScoreProcessor::getNumPrograms() { return 1; }
int PlayScoreProcessor::getCurrentProgram() { return 0; }
void PlayScoreProcessor::setCurrentProgram (int) {}
const juce::String PlayScoreProcessor::getProgramName (int) { return "Default"; }
void PlayScoreProcessor::changeProgramName (int, const juce::String&) {}

bool PlayScoreProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void PlayScoreProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    rebuildStretcher();
}

void PlayScoreProcessor::releaseResources()
{
    stretcher.reset();
    originalAudio.reset();
}

// ⭐ RECALCULA RELAÇÃO DE AFINAÇÃO
double PlayScoreProcessor::calculatePitchRatio() const
{
    // Relação de transposição por semitom: 2^(n/12)
    double semitoneFactor = std::pow (2.0, semitonesToTranspose / 12.0);
    // Ajuste de afinação de referência
    double pitchAdjust = baseFilePitchHz / referencePitchHz;
    return semitoneFactor * pitchAdjust;
}

// ⭐ RECRIA O PROCESSADOR DE TRANSPOSIÇÃO
void PlayScoreProcessor::rebuildStretcher()
{
    if (getSampleRate() <= 0) return;

    stretcher = std::make_unique<RubberBand::RubberBandStretcher>(
        getSampleRate(),
        2, // canais estéreo
        RubberBand::RubberBandStretcher::OptionProcessRealTime |
        RubberBand::RubberBandStretcher::OptionPitchHighQuality |
        RubberBand::RubberBandStretcher::OptionWindowStandard
    );

    stretcher->setPitchScale (calculatePitchRatio());
    stretcher->reset();
}

// ⭐ TROCAR INSTRUMENTO → RECALCULA SEMITONS
void PlayScoreProcessor::setInstrument (Instrument instr)
{
    currentInstrument = instr;

    if (instr != Instrument::CUSTOM)
    {
        semitonesToTranspose = static_cast<int> (instr);
        if (stretcher)
            stretcher->setPitchScale (calculatePitchRatio());
    }

    lastProcessedSample = -1; // força reinício
    isFirstRunAfterSeek = true;
}

// ⭐ AJUSTAR AFINAÇÃO DE REFERÊNCIA
void PlayScoreProcessor::setReferencePitch (double hz)
{
    referencePitchHz = juce::jlimit (432.0, 445.0, hz);
    if (stretcher)
        stretcher->setPitchScale (calculatePitchRatio());
}

// ⭐ AJUSTE MANUAL DE SEMITONS
void PlayScoreProcessor::setTransposeSemitones (int semitons)
{
    semitonesToTranspose = juce::jlimit (-12, 12, semitons);
    currentInstrument = Instrument::CUSTOM;
    if (stretcher)
        stretcher->setPitchScale (calculatePitchRatio());
}

// ⭐ CARREGAR ARQUIVO DE ÁUDIO
void PlayScoreProcessor::loadAudioFile (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (!reader) return;

    originalAudio = std::make_unique<juce::AudioBuffer<float>>(
        static_cast<int> (reader->numChannels),
        static_cast<int> (reader->lengthInSamples)
    );

    reader->read (originalAudio.get(), 0, static_cast<int> (reader->lengthInSamples), 0, true, true);
    fileSampleRate = reader->sampleRate;
    baseFilePitchHz = 440.0; // assumimos arquivo em 440Hz — ajustável depois

    lastProcessedSample = -1;
    isFirstRunAfterSeek = true;
    rebuildStretcher();

    DBG ("Arquivo carregado: " << file.getFileName()
         << " | Semitons: " << semitonesToTranspose
         << " | Afinação: " << referencePitchHz << "Hz");
}

// ⭐ PROCESSAMENTO PRINCIPAL — SINCRONIA + TRANSPOSIÇÃO
void PlayScoreProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    buffer.clear();
    if (!originalAudio || !stretcher || fileSampleRate <= 0) return;

    auto playHead = getPlayHead();
    if (!playHead) return;
    auto posInfo = playHead->getCurrentPosition();
    if (!posInfo) return;

    if (!posInfo->isPlaying)
    {
        lastProcessedSample = -1;
        isFirstRunAfterSeek = true;
        stretcher->reset();
        return;
    }

    const int64_t currentSample = posInfo->timeInSamples;
    const int numSamples = buffer.getNumSamples();
    const double sampleRatio = fileSampleRate / getSampleRate();
    const int64_t startInFile = static_cast<int64_t> (currentSample * sampleRatio);
    const int64_t endInFile = startInFile + static_cast<int64_t> (numSamples * sampleRatio);

    // Detecção de busca/retorno no tempo
    if (lastProcessedSample >= 0 && std::abs (currentSample - lastProcessedSample) > numSamples * 2)
    {
        stretcher->reset();
        isFirstRunAfterSeek = true;
    }
    lastProcessedSample = currentSample;

    // Fim do arquivo
    if (startInFile >= originalAudio->getNumSamples()) return;

    // Preparar dados para o Rubber Band
    const int startOffset = static_cast<int> (startInFile);
    const int samplesAvailable = originalAudio->getNumSamples() - startOffset;
    const int samplesToProcess = juce::jmin (numSamples, samplesAvailable);

    if (samplesToProcess <= 0) return;

    const float* inL = originalAudio->getReadPointer (0, startOffset);
    const float* inR = originalAudio->getNumChannels() > 1 ? originalAudio->getReadPointer (1, startOffset) : nullptr;
    float* outL = buffer.getWritePointer (0);
    float* outR = buffer.getWritePointer (1);

    const float* inPtrs[2] = { inL, inR ? inR : inL };
    float* outPtrs[2] = { outL, outR };

    // Alimentar processador
    stretcher->push (inPtrs, samplesToProcess, isFirstRunAfterSeek);
    isFirstRunAfterSeek = false;

    // Retirar áudio processado
    if (stretcher->available() >= numSamples)
        stretcher->pull (outPtrs, numSamples);
    else
        buffer.clear();
}

void PlayScoreProcessor::getStateInformation (juce::MemoryBlock& data)
{
    juce::MemoryOutputStream stream (data, false);
    stream.writeInt (semitonesToTranspose);
    stream.writeDouble (referencePitchHz);
    stream.writeInt (static_cast<int> (currentInstrument));
}

void PlayScoreProcessor::setStateInformation (const void* data, int size)
{
    if (size < 16) return;
    juce::MemoryInputStream stream (data, static_cast<size_t> (size), false);
    semitonesToTranspose = stream.readInt();
    referencePitchHz = stream.readDouble();
    currentInstrument = static_cast<Instrument> (stream.readInt());
    rebuildStretcher();
}

juce::AudioProcessorEditor* PlayScoreProcessor::createEditor() { return new PlayScoreEditor (*this); }
bool PlayScoreProcessor::hasEditor() const { return true; }
```

---

# 🎨 PARTE 2 — INTERFACE ATUALIZADA (`PluginEditor.cpp`)

```cpp
#include "PluginEditor.h"
#include "PluginProcessor.h"

PlayScoreEditor::PlayScoreEditor (PlayScoreProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (480, 320);

    // Carregar arquivo
    loadButton.setButtonText ("📂 Carregar Áudio");
    loadButton.onClick = [this]() {
        auto chooser = std::make_unique<juce::FileChooser>("Escolha o áudio",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.wav;*.flac;*.mp3");
        chooser->launchAsync(juce::FileBrowserComponent::openMode,
            [this](const juce::FileChooser& fc) {
                if (fc.getResult() != juce::File())
                    processor.loadAudioFile(fc.getResult());
            });
    };
    addAndMakeVisible(loadButton);

    // Seletor de Instrumento
    instrumentLabel.setText("Instrumento:", juce::dontSendNotification);
    addAndMakeVisible(instrumentLabel);
    instrumentSelector.addItem("Piano / Trombone (C)", 1);
    instrumentSelector.addItem("Trompete / Sax Tenor (Bb)", 2);
    instrumentSelector.addItem("Sax Alto (Eb)", 3);
    instrumentSelector.addItem("Trompa (F)", 4);
    instrumentSelector.addItem("Ajuste Manual", 5);
    instrumentSelector.setSelectedId(1);
    instrumentSelector.onChange = [this]() {
        switch(instrumentSelector.getSelectedId()) {
            case 1: processor.setInstrument(Instrument::PIANO_C); break;
            case 2: processor.setInstrument(Instrument::TRUMPET_Bb); break;
            case 3: processor.setInstrument(Instrument::SAX_ALTO_Eb); break;
            case 4: processor.setInstrument(Instrument::FRENCH_HORN_F); break;
            case 5: processor.setInstrument(Instrument::CUSTOM); break;
        }
    };
    addAndMakeVisible(instrumentSelector);

    // Afinação de referência
    pitchLabel.setText("Afinação (Hz):", juce::dontSendNotification);
    addAndMakeVisible(pitchLabel);
    pitchSlider.setRange(432.0, 445.0, 0.5);
    pitchSlider.setValue(440.0);
    pitchSlider.onValueChange = [this]() {
        processor.setReferencePitch(pitchSlider.getValue());
    };
    addAndMakeVisible(pitchSlider);

    // Semitons manual
    transposeLabel.setText("Transposição (semitons):", juce::dontSendNotification);
    addAndMakeVisible(transposeLabel);
    transposeSlider.setRange(-12, 12, 1);
    transposeSlider.setValue(0);
    transposeSlider.onValueChange = [this]() {
        if (processor.getInstrument() == Instrument::CUSTOM)
            processor.setTransposeSemitones(static_cast<int>(transposeSlider.getValue()));
    };
    addAndMakeVisible(transposeSlider);

    startTimerHz(10);
}

PlayScoreEditor::~PlayScoreEditor() {}

void PlayScoreEditor::timerCallback()
{
    pitchSlider.setValue(processor.getReferencePitch(), juce::dontSendNotification);
    transposeSlider.setValue(processor.getTransposeSemitones(), juce::dontSendNotification);
}

void PlayScoreEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour(30, 30, 35));
    g.setColour(juce::Colours::white);
    g.setFont(20.f);
    g.drawText("PlayScore | PartePlay", 20, 15, 440, 30, juce::Justification::centredTop);
    g.setFont(13.f);
    g.setColour(juce::Colours::lightgrey);
    g.drawText("Sincronia + Transposição Inteligente", 20, 50, 440, 20, juce::Justification::centredTop);
}

void PlayScoreEditor::resized()
{
    loadButton.setBounds(140, 85, 200, 45);
    instrumentLabel.setBounds(30, 150, 130, 25);
    instrumentSelector.setBounds(170, 150, 260, 25);
    pitchLabel.setBounds(30, 195, 130, 25);
    pitchSlider.setBounds(170, 195, 260, 25);
    transposeLabel.setBounds(30, 240, 130, 25);
    transposeSlider.setBounds(170, 240, 260, 25);
}
```

**E adicione os membros no `PluginEditor.h`:**

```cpp
private:
    PlayScoreProcessor& processor;
    juce::TextButton loadButton;
    juce::Label instrumentLabel, pitchLabel, transposeLabel;
    juce::ComboBox instrumentSelector;
    juce::Slider pitchSlider, transposeSlider;
    void timerCallback() override;
```

---

# 💼 PARTE 3 — MODELO DE NEGÓCIO E ESTRATÉGIA DE LICENÇA

## 📋 Divisão de Código e Repositórios

| Camada          | Repositório            | Licença   | O que tem                                                                                        |
| --------------- | ---------------------- | --------- | ------------------------------------------------------------------------------------------------ |
| **Núcleo Free** | GitHub (Público)       | GPLv3     | Sincronia, carregamento, transposição básica, interface                                          |
| **Módulos Pro** | Azure DevOps (Privado) | Comercial | Presets, processamento lote, modos de treino, afinações regionais, sem obrigação de abrir código |

> ⚖️ **Regra legal**: Código GPL modificado/permanece GPL. Novos arquivos/módulos separados = licença própria.

## 💰 Tabela de Produto e Preços

| Versão   | Preço                        | Para quem                 | Recursos                                                    |
| -------- | ---------------------------- | ------------------------- | ----------------------------------------------------------- |
| **Free** | Gratuito + doação US$3–5     | Estudantes, amadores      | Sincronia, 6 instrumentos, afinação 432–445Hz, WAV/FLAC/MP3 |
| **Pro**  | US$ 29 perpétua / US$ 12/ano | Arranjadores, professores | + Presets, modo treino, afinações personalizadas, sem GPL   |
| **Edu**  | US$ 5/máquina/ano            | Escolas, redes            | + instalação em lote, suporte dedicado                      |

## 🔄 Fluxo Completo

```
Usuário → baixa Free (GitHub) → usa → precisa de mais → site → compra Pro
  ↓
Pagamento → Gumroad/Paddle → licença automática → download do instalador Pro
  ↓
Receita → Conta Global Inter (USD) → reinvestimento + fundo Edu subsidiado
```

## 📁 Documentação para Wiki do Azure

```
📌 PROJETO: PlayScore / PartePlay — ESTRUTURA COMPLETA
Atualizado: 25/09/2026

---
✅ FASE 1 — CONCLUÍDA
- Ambiente configurado
- Sincronia com tempo do hospedeiro funcional
- Carregamento de áudio implementado

⏳ FASE 2 — EM EXECUÇÃO (CÓDIGO ENTREGUE)
- Integração Rubber Band → pitch-shifting em tempo real
- Tabela de instrumentos transpositores
- Controle de afinação de referência
- Detecção de busca/retorno no tempo

📐 MODELO DE NEGÓCIO — DEFINIDO
- Free (GPLv3, público) ↔ Pro (comercial, privado)
- Preços: US$29 perpétua / US$12/ano / US$5/Edu
- Venda: Gumroad/Paddle → entrega automática
- Recebimento: Conta Global Banco Inter (USD)

⚠️ RISCOS E MITIGAÇÃO
- GPL → módulos Pro em arquivos separados
- Performance CPU → qualidade Rubber Band ajustável em versões futuras
- Compatibilidade MuseScore → validar versões periodicamente
```

---

## ✅ RESUMO — O QUE FAZER AGORA

1. **Colar os códigos** no projeto, adicionar a Rubber Band no Projucer
2. **Compilar** → testar no MuseScore 4 com um arquivo de áudio
3. **Selecionar instrumento** → confirmar que o tom muda sem acelerar
4. **Testar afinação** → ajustar para 432Hz e verificar diferença sutil
5. **Feedback** → me diz o que funcionou, o que travou, o que sentiu

Tudo pronto aí! Quer que eu detalhe a compilação da Rubber Band ou a configuração da página de venda? 🎵💻🚀
