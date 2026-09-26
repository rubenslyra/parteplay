#pragma once

#include <JuceHeader.h>

// Camada de textos (i18n) do projeto.
//
// Padrão: toda string de interface/usuário é criada via Text::t("chave"), onde a
// chave é o próprio literal padrão (pt-BR). A tabela de traduções (PT/EN/ES)
// é centralizada em Source/Text.cpp; quando a chave não está na tabela, retorna a
// própria chave decodificada como UTF-8.
//
// UTF-8 é o fundamento: os fontes são compilados com /utf-8 e Text::t() decodifica
// a literal via juce::String::fromUTF8 — nunca pelo construtor String(const char*)
// do JUCE 9 (que interpreta como 8-bit ASCII e quebraria o texto pt-BR).
namespace Text
{
    enum class Culture
    {
        PortugueseBR,
        English,
        Spanish
    };

    void setCulture (Culture culture);
    Culture getCulture() noexcept;
    juce::String cultureCode() noexcept;
    juce::String cultureName() noexcept;

    // Traduz a chave para a cultura ativa; sem entrada na tabela, retorna a chave.
    juce::String t (const char* utf8);
    juce::String t (const juce::String& key);
}