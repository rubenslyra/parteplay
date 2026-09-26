#pragma once

#include <JuceHeader.h>

// Construção de juce::String a partir de literal UTF-8.
//
// Contexto: os fontes são compilados com /utf-8 (literais em UTF-8), mas o
// construtor juce::String(const char*) do JUCE 9 interpreta a entrada como
// 8-bit ASCII (CharPointer_ASCII): bytes acima de 127 são convertidos 1:1 para
// U+0000-U+00FF, causando mojibake em texto pt-BR (ex.: "Afinação" -> "AfinaÃ§Ã£o").
// Usar Text::from() direciona toda literal para juce::String::fromUTF8 (UTF-8).
//
// Padrão do projeto: TODA string de interface/usuário é criada via Text::from(),
// inclusive as que hoje são somente ASCII — textos futuros podem ganhar acentos
// sem quebrar silenciosamente.
namespace Text
{
    inline juce::String from (const char* utf8)
    {
        return juce::String::fromUTF8 (utf8);
    }
}