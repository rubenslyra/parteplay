#pragma once

#include <JuceHeader.h>

#include <initializer_list>

// Camada de textos (i18n) do projeto.
//
// Padrão: toda string de interface/usuário é criada por Text::t("chave"), onde a
// chave é o próprio literal padrão (pt-BR). A tabela de traduções (PT/EN/ES) é
// centralizada em Source/Text.cpp; sem entrada na tabela, retorna a própria chave
// decodificada como UTF-8.
//
// UTF-8 é o fundamento: os fontes são compilados com /utf-8 e salvos com BOM, e
// toda literal entra no runtime por Text::from -> juce::String::fromUTF8.
//
// PROIBIDO na interface:
//   - juce::String(const char*) — o JUCE 9 decodifica como 8-bit ASCII (CharPointer_ASCII),
//     transformando "Afinação" em "AfinaÃ§Ã£o";
//   - juce::String::formatted — internamente faz String(const char*) do formato (mesma
//     quebra de acentos) e, no Windows, usa _vsnwprintf, em que %s exige wchar_t*.
//     Para interpolar, usar Text::format com marcadores {0}, {1}, ...
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

    // Decodifica uma literal UTF-8 do fonte. Usar em vez de juce::String ("...").
    juce::String from (const char* utf8);

    // Traduz a chave para a cultura ativa; sem entrada na tabela, retorna a chave.
    juce::String t (const char* utf8);
    juce::String t (const juce::String& key);

    // Substitui os marcadores {0}, {1}, ... do modelo pelos valores informados.
    // Substituto seguro de juce::String::formatted para texto de interface.
    juce::String format (const juce::String& model,
                         const std::initializer_list<juce::String>& values);

    // Rótulos de tempo ("mm:ss" e "mm:ss.cc"), sempre ASCII.
    juce::String time (double seconds);
    juce::String timePrecise (double seconds);
}
