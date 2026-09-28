#pragma once

#include <JuceHeader.h>

#include <initializer_list>

// Camada de textos (i18n) do projeto.
//
// Padrão: toda string de interface/usuário é criada por Text::t("chave"), onde a
// chave é o próprio literal padrão (pt-BR). A tabela de traduções vive em
// Source/Text.cpp com colunas PT | en-GB | en-US | es-ES; sem entrada na tabela,
// retorna a própria chave decodificada como UTF-8.
//
// Culturas usam códigos de idioma BCP 47 completos, porque o inglês tem variante:
//   pt-BR, en-GB (Reino Unido — códigos ISO 3166, "UK" não é um código válido),
//   en-US e es-ES. A grafia difere entre en-GB e en-US ("analysing/cancelled" vs
//   "analyzing/canceled"), por isso não existe um único "English" nem um
//   "Spanish": o es-ES tem acentuação própria (válida, vacía, aún).
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
        PortugueseBR,                 // pt-BR
        EnglishUK,                    // en-GB  (o antigo "English" da tabela)
        EnglishUS,                    // en-US
        SpanishES                     // es-ES
    };

    void setCulture (Culture culture);
    Culture getCulture() noexcept;

    // Código BCP 47 completo ("pt-BR", "en-GB", "en-US", "es-ES") e nome nativo
    // da cultura ("Português (Brasil)", "English (UK)", ...). Os overloads com
    // Culture são para listar todas as opções (combo de idioma) sem depender da
    // cultura ativa; os sem argumento usam a cultura ativa.
    juce::String cultureCode (Culture culture) noexcept;
    juce::String cultureName (Culture culture) noexcept;
    Culture cultureFromCode (const juce::String& code) noexcept;
    juce::String cultureCode() noexcept;
    juce::String cultureName() noexcept;

    // Decodifica uma literal UTF-8 do fonte. Usar em vez de juce::String ("...").
    juce::String from (const char* utf8);

    // Traduz a chave para a cultura ativa; sem entrada na tabela, retorna a chave.
    juce::String t (const char* utf8);
    juce::String t (const juce::String& key);

    // Diz se a chave tem entrada na tabela, independente de a tradução coincidir
    // com a chave.
    //
    // Existe porque comparar t(chave) com a chave não detecta falta quando a
    // tradução correta é idêntica ao original: "cancelada" é a mesma palavra em
    // pt-BR e es-ES, e "BPM original" é igual nos quatro idiomas. Um teste que se
    // apoiasse só na diferença daria atestado falso nesses casos.
    bool has (const juce::String& key);
    bool has (const char* utf8);

    // Substitui os marcadores {0}, {1}, ... do modelo pelos valores informados.
    // Substituto seguro de juce::String::formatted para texto de interface.
    juce::String format (const juce::String& model,
                         const std::initializer_list<juce::String>& values);

    // Número com o separador decimal da cultura ativa. en-US e en-GB usam ponto
    // ("440.5"); pt-BR e es-ES usam vírgula ("128,5") — formato padrão de quem
    // escreve afinação e BPM nesses países. Sem agrupamento de milhar: os valores
    // da interface (Hz, BPM, cents, MB) ficam abaixo de 1000, e o agrupamento
    // rouba largura numa coluna de ~100 px. Usar em TODO número com decimal visível.
    juce::String number (double value, int decimalPlaces);
    juce::String number (int value);

    // Rótulos de tempo ("mm:ss" e "mm:ss.cc"), sempre ASCII.
    juce::String time (double seconds);
    juce::String timePrecise (double seconds);
}