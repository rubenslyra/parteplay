#pragma once

#include <JuceHeader.h>

#include <memory>
#include <vector>

// Painel de diagnostico da janela (Ctrl+D). Definido aqui, e nao no .cpp, porque o
// cabecalho e quem precisa da condicao para declarar os membros: se a deteccao
// vivesse no .cpp, ela chegaria depois deste include e o #if abaixo cairia
// sempre pelo lado falso, compilando so a definicao e nenhuma declaracao.
//
// Em Release o codigo nao e compilado: o binario publicado nao carrega o overlay
// nem a tecla.
#if ! defined(PARTEPLAY_DEBUG_OVERLAY)
 #if JUCE_DEBUG || defined(_DEBUG)
  #define PARTEPLAY_DEBUG_OVERLAY 1
 #else
  #define PARTEPLAY_DEBUG_OVERLAY 0
 #endif
#endif

class PlayScoreProcessor;
class TrackPanel;
class TransportPanel;
class SongSheetPanel;
class TuningPanel;
class MeterPanel;
class LoadingOverlay;
class InfoButton;
class CreditsOverlay;

// Painel da interface: desenha o próprio chrome ("vidro fosco") e participa dos
// dois ciclos de atualização — textos (troca de idioma) e estado (timer).
class EditorPanel : public juce::Component
{
public:
    ~EditorPanel() override;

    explicit EditorPanel (PlayScoreProcessor& owner);

    // Reaplica textos traduzidos.
    virtual void refreshTexts() = 0;

    // Reaplica o estado vindo do núcleo.
    virtual void refreshState() {}

protected:
    static constexpr int padding     = 16;
    static constexpr int titleHeight = 20;

    // Chrome: painel + micro-rótulo da seção. Retorna a área útil restante.
    juce::Rectangle<int> paintChrome (juce::Graphics& g, const juce::String& sectionTitle);
    juce::Rectangle<int> contentArea() const;

    PlayScoreProcessor& processor;
};

// Interface 5:4 (1000x800, mínimo 800x640), redimensionável com proporção fixa.
class PlayScoreEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit PlayScoreEditor (PlayScoreProcessor& processor);
    ~PlayScoreEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    PlayScoreProcessor& getProcessor() noexcept { return processor; }

    // Ações pedidas pelos painéis.
    void requestLoadAudio();
    void requestExportMap();
    void languageChanged();

    juce::String exportMessage() const { return lastExportMessage; }

private:
    void timerCallback() override;
    bool keyPressed (const juce::KeyPress& key) override;
    void refreshAllTexts();
    void refreshAllState();
    void refreshLoadingOverlay();
    juce::Rectangle<int> headerArea() const;
    juce::Rectangle<int> bodyArea() const;
    juce::Rectangle<int> footerArea() const;

#if PARTEPLAY_DEBUG_OVERLAY
    void paintDebugOverlay (juce::Graphics& g);
#endif

    PlayScoreProcessor& processor;

    std::unique_ptr<juce::LookAndFeel> lookAndFeel;

    std::unique_ptr<TrackPanel>      trackPanel;
    std::unique_ptr<TransportPanel>  transportPanel;
    std::unique_ptr<SongSheetPanel>  songSheetPanel;
    std::unique_ptr<TuningPanel>     tuningPanel;
    std::unique_ptr<MeterPanel>      meterPanel;

    std::vector<EditorPanel*> panels;

    juce::Label cultureLabel;
    juce::ComboBox cultureSelector;

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::FileChooser> exportChooser;
    juce::String lastExportMessage;

    // Overlay modal de carga: cobre a janela inteira, bloqueia os cliques e fica
    // acima de todos os paineis enquanto o audio carrega em segundo plano.
    std::unique_ptr<LoadingOverlay> loadingOverlay;

    // Botao de informacoes (ao lado da bandeira) e o overlay modal que ele abre.
    std::unique_ptr<InfoButton>     creditsButton;
    std::unique_ptr<CreditsOverlay> creditsOverlay;

    static constexpr int margin       = 20;
    static constexpr int gap          = 14;
    static constexpr int headerHeight = 62;
    static constexpr int footerHeight = 32;

#if PARTEPLAY_DEBUG_OVERLAY
    bool debugOverlayVisible = false;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayScoreEditor)
};
