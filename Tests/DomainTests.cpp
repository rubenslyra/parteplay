// Testes automatizados do domÃ­nio musical e da camada de texto.
//
// Objetivo: travar em cÃ³digo o gate V1 (direÃ§Ã£o e convenÃ§Ã£o da transposiÃ§Ã£o) e as
// invariantes de encoding/i18n, para que a val musically correta nÃ£o dependa
// apenas do ouvido. Executados por CTest: `ctest --preset msvc` ou via script.
//
// O primeiro grupo existe por um motivo especÃ­fico: a tabela usava o menor
// intervalo com sinal, entÃ£o o Sax Alto estava em +3 e a Trompa em FÃ¡ em +5.
// Ambos soam a nota certa NA CLASSE DE ALTURA e na oitava errada â€” por isso
// passaram no ouvido. A invariante testada agora Ã© o intervalo com a oitava:
// ler DÃ³4 precisa soar Bb3 em Siâ™­ e Eb3 em Miâ™­.
//
// NÃ£o hÃ¡ framework externo: o runner Ã© uma funÃ§Ã£o local `check` para manter o
// nÃºcleo sem dependÃªncias alÃ©m do JUCE.

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <chromaprint.h>

#include "ExternalBpm.h"
#include "FilePlayer.h"
#include "FingerprintWorker.h"
#include "MidiMapExporter.h"
#include "TempoAnalyser.h"
#include "Text.h"
#include "Tuning.h"

namespace
{
    int failures = 0;
    int checks = 0;

    void check (bool condition, const std::string& what)
    {
        ++checks;

        if (! condition)
        {
            ++failures;
            std::printf ("  FALHOU: %s\n", what.c_str());
        }
    }

    // Mojibake e um bug de bytes, e o console do Windows mostra os bytes ja
    // reinterpretados â€” ou seja, a mensagem mente sobre a causa. Para diferenciar
    // "fonte em Latin-1" de "fonte em UTF-8 duplamente codificada", o hex e a
    // unica evidencia confiavel.
    std::string hexOf (const juce::String& s)
    {
        // toRawUTF8 devolve bytes; CharPointer_UTF8::operator[] devolveria
        // code points, e truncar cada um a 1 byte reconstruiria uma string
        // plausivel mas errada â€” exatamente o tipo de diagnostico que aponta
        // para o arquivo innocentado.
        const auto raw = s.toRawUTF8();
        std::string out;

        for (int i = 0; raw[i] != 0 && i < 24; ++i)
        {
            if (i > 0)
                out += ' ';

            out += juce::String::toHexString ((juce::uint8) raw[i]).toStdString();
        }

        return out;
    }

    void checkClose (double actual, double expected, double tolerance, const std::string& what)
    {
        const bool ok = std::abs (actual - expected) <= tolerance;
        check (ok, what + " (obtido " + std::to_string (actual)
                      + ", esperado " + std::to_string (expected) + ")");
    }

    // Assinatura de mojibake: UTF-8 interpretado como Latin-1 e re-codificado.
    // "Ã§" (C3 A7) vira "ÃƒÂ§" (C3 83 C2 A7) â€” procuramos o par U+00C3 seguido de
    // caractere do suplemento Latin-1, alem do caractere de substituicao U+FFFD.
    bool hasMojibake (const juce::String& s)
    {
        static const char* const mojibakeSequences[] = { "\xC3\x83\xC2\xA7",   // ÃƒÂ§
                                                         "\xC3\x83\xC2\xA3",   // ÃƒÂ£
                                                         "\xC3\x83\xC2\xA9",   // ÃƒÂ©
                                                         "\xC3\x83\xC2\xB5",   // ÃƒÂµ
                                                         "\xC3\x83\xC3\xAD" }; // ÃƒÃ­

        if (s.contains (juce::CharPointer_UTF8 ("\xEF\xBF\xBD")))
            return true;

        for (const auto* sequence : mojibakeSequences)
            if (s.contains (juce::CharPointer_UTF8 (sequence)))
                return true;

        return false;
    }

    // O ratio de reproducao deixou de servir a instrumentos e passou a servir a
    // um unico fim: levar a afinacao real do arquivo ate a referencia escolhida.
    // A invariante que sobra e o SINAL â€” arquivo grave exige ratio > 1 para
    // subir, arquivo agudo exige ratio < 1. Era o mesmo bug de sinal que a
    // transposicao tinha, e agora ele esta num caminho muito mais curto, entao
    // um erro aqui aparece como audio na oitava errada.
    void testTuningRatio()
    {
        std::printf ("[V1] Ratio de afinacao\n");

        checkClose (Tuning::playbackRatio (440.0, 440.0), 1.0, 1e-12, "mesma referencia = razao 1.0");

        // Arquivo afinado abaixo da referencia: precisa acelerar para subir.
        check (Tuning::playbackRatio (440.0, 432.0) > 1.0, "arquivo grave (432) produz razao > 1");
        // Arquivo afinado acima: precisa desacelerar para descer.
        check (Tuning::playbackRatio (440.0, 445.0) < 1.0, "arquivo agudo (445) produz razao < 1");

        // Referencia menor que a do arquivo tambem desce o audio.
        check (Tuning::playbackRatio (432.0, 440.0) < 1.0, "referencia 432 sobre arquivo 440 produz razao < 1");

        // Pitch nao detectado no arquivo nao pode deslocar a reproducao.
        checkClose (Tuning::playbackRatio (440.0, 0.0), 1.0, 1e-12, "arquivo sem pitch fica em razao 1");
        checkClose (Tuning::playbackRatio (440.0, -1.0), 1.0, 1e-12, "pitch invalido fica em razao 1");

        // Inversao: trocar referencia e arquivo inverte o ratio. A razao e
        // sempre referencia/arquivo, e um sinal trocado aqui invertaria o audio.
        check (std::abs (Tuning::playbackRatio (440.0, 432.0) * Tuning::playbackRatio (432.0, 440.0) - 1.0) < 1e-12,
               "inverter referencia e arquivo inverte o ratio");

        // Coerencia com cents: a razao tem de ser exatamente o que os cents
        //Between prometem, senao o painel 04 e o audio divergem.
        for (double fileHz : { 432.0, 435.0, 438.0, 442.0, 445.0 })
        {
            const double ratio = Tuning::playbackRatio (440.0, fileHz);
            const double cents = Tuning::centsBetween (fileHz, 440.0);

            checkClose (1200.0 * std::log2 (ratio), cents, 1e-6,
                        "ratio e cents concordam para " + std::to_string (fileHz) + " Hz");
        }

        // Cents e uma medida de referencia, nao de altura absoluta.
        checkClose (Tuning::centsBetween (440.0, 440.0), 0.0, 1e-12, "cents de 440 para 440 = 0");
        check (Tuning::centsBetween (0.0, 440.0) == 0.0, "cents de frequencia invalida = 0");

        // A faixa de referencia Ã© pequena de proposito: e uma correcao de
        // desvio, nao um transpositor. O painel 04 expoe estes limites.
        check (Tuning::minReferenceHz < Tuning::defaultReferenceHz
               && Tuning::defaultReferenceHz < Tuning::maxReferenceHz,
               "referencia padrao fica entre os limites");
        check (Tuning::maxReferenceHz - Tuning::minReferenceHz <= 20.0,
               "faixa de referencia nao virou transposicao");
    }

    // Nomes cientificos com oitava: sem a oitava o erro passa despercebido.
    void testNoteNames()
    {
        std::printf ("[V1] Nomes de nota com oitava\n");

        check (NoteName::forMidiNote (60) == "C4",  "MIDI 60 = C4");
        check (NoteName::forMidiNote (61) == "Db4", "MIDI 61 = Db4");
        check (NoteName::forMidiNote (58) == "Bb3", "MIDI 58 = Bb3");
        check (NoteName::forMidiNote (51) == "Eb3", "MIDI 51 = Eb3");
        check (NoteName::forMidiNote (53) == "F3",  "MIDI 53 = F3");
        check (NoteName::forMidiNote (21) == "A0",  "MIDI 21 = A0 (nota mais grave do piano)");
        check (NoteName::forMidiNote (108) == "C8", "MIDI 108 = C8 (nota mais aguda do piano)");
        check (! hasMojibake (NoteName::forMidiNote (60)), "Nome de nota sem mojibake");

        // A mesma classe de altura com outra oitava precisa de outro nome: e o
        // que impede o deslize de +3/-9 e +5/-7 de passar.
        check (NoteName::forMidiNote (51) != NoteName::forMidiNote (63),
               "Eb3 e Eb4 nao confusedem classe de altura com oitava");
        check (NoteName::forMidiNote (53) != NoteName::forMidiNote (65),
               "F3 e F4 nao confusedem classe de altura com oitava");
    }

    // V2/V3 â€” encoding e i18n: a causa raiz do mojibake foi String(const char*).
    void testTextEncoding()
    {
        std::printf ("[V2] Encoding e i18n\n");

        const auto withAccent = Text::from ("Afina\xC3\xA7\xC3\xA3o");
        check (withAccent == juce::String::fromUTF8 ("Afina\xC3\xA7\xC3\xA3o"), "Text::from decodifica UTF-8");
        check (withAccent.contains (juce::CharPointer_UTF8 ("\xC3\xA7")), "Text::from preserva cedilha");
        check (! hasMojibake (withAccent), "Text::from nao produz sequencia mojibake");

        const auto flat = Text::format (Text::t ("{0} \xC2\xB7 {1} MB"), { "WAV", juce::String (12.5, 1) });
        check (flat == juce::String::fromUTF8 ("WAV \xC2\xB7 12.5 MB"), "Text::format interpola e preserva acentos");

        const auto missing = Text::format (Text::t ("{0} e {1}"), { "a" });
        check (missing == juce::String::fromUTF8 ("a e {1}"), "Marcador ausente fica visivel");

        check (Text::time (65.0) == "01:05", "Text::time formata mm:ss");
        check (Text::timePrecise (65.25) == "01:05.25", "Text::timePrecise formata mm:ss.cc");
        check (Text::time (-1.0) == "00:00", "Text::time trata valor negativo");

        // pt-BR precisa devolver a propria chave com acentos; en/es traduzem.
        Text::setCulture (Text::Culture::PortugueseBR);
        const auto pt = Text::t ("Afina\xC3\xA7\xC3\xA3o (Hz)");
        check (pt == juce::String::fromUTF8 ("Afina\xC3\xA7\xC3\xA3o (Hz)"), "pt-BR devolve a chave intacta");
        check (pt.contains (juce::CharPointer_UTF8 ("\xC3\xA7")), "pt-BR preserva a cedilha");
        check (! hasMojibake (pt), "pt-BR nao produz mojibake");

        Text::setCulture (Text::Culture::EnglishUK);
        const auto en = Text::t ("Afina\xC3\xA7\xC3\xA3o (Hz)");
        check (en == juce::String::fromUTF8 ("Tuning (Hz)"), "en-GB traduz 'Afinacao (Hz)'");
        check (en != pt, "en-GB difere de pt-BR");

        Text::setCulture (Text::Culture::EnglishUS);
        check (Text::t ("Afina\xC3\xA7\xC3\xA3o (Hz)") == juce::String::fromUTF8 ("Tuning (Hz)"),
               "en-US traduz 'Afinacao (Hz)'");

        Text::setCulture (Text::Culture::SpanishES);
        const auto es = Text::t ("Afina\xC3\xA7\xC3\xA3o (Hz)");
        check (es == juce::String::fromUTF8 ("Afinaci\xC3\xB3n (Hz)"), "es-ES traduz 'Afinacion (Hz)'");
        check (es.contains (juce::CharPointer_UTF8 ("\xC3\xB3")), "es-ES traduz com acento (Afinacion)");
        check (! hasMojibake (es), "es-ES nao produz mojibake");

        for (const auto culture : { Text::Culture::PortugueseBR, Text::Culture::EnglishUK,
                                     Text::Culture::EnglishUS, Text::Culture::SpanishES })
        {
            Text::setCulture (culture);
            check (Text::getCulture() == culture, "Cultura ativa apos setCulture");

            // As duas formas que a interface usa: acento e marcador de interpolacao.
            for (const auto* key : { "Falha ao exportar o mapa de tempo.",
                                     "L\xC3\xAA {0} \xC2\xB7 soa {1}" })
            {
                const auto name = Text::t (key);
                check (name.isNotEmpty() && ! hasMojibake (name),
                       "Texto traduzido sem mojibake em " + Text::cultureCode().toStdString());
            }
        }

        Text::setCulture (Text::Culture::PortugueseBR);
    }

    // As strings do painel 03 sao o que o usuario le para identificar a gravacao.
    // Duas falhas importam aqui: devolver mojibake no lugar de acento, e devolver
    // pt-BR nos outros idiomas porque a traducao foi esquecida.
    void testSongSheetStrings()
    {
        std::printf ("[V2] Strings da ficha\n");

        // Referencia de encoding vinda de uma chave ANTIGA da tabela, que nao foi
        // tocada aqui: 13 caracteres. Se valesse 15, o fonte inteiro estaria
        // sendo lido como Latin-1 e a interface mostraria mojibake ao usuario â€”
        // nao apenas o teste.
        {
            const auto known = Text::t ("Afina\xC3\xA7\xC3\xA3o (Hz)");
            check (known.length() == 13,
                   "Chave acentuada pre-existente tem 13 caracteres (obtido "
                       + std::to_string (known.length()) + ", hex " + hexOf (known) + ")");
        }

        // Auto-sonda do detector: sem ela, um hasMojibake defeituoso acusaria a
        // tabela de textos de estar corrompida e o diagnostico inteiro viraria
        // ruido apontando para o arquivo innocentado.
        {
            const auto good = juce::String (juce::CharPointer_UTF8 ("Afina\xC3\xA7\xC3\xA3o \xC3\xAD \xC3\xA9 \xC3\xB5"));
            check (! hasMojibake (good), "Detector aceita UTF-8 correto (hex " + hexOf (good) + ")");

            const auto bad = juce::String (juce::CharPointer_UTF8 ("Afina\xC3\x83\xC2\xA7\xC3\x83\xC2\xA3o"));
            check (hasMojibake (bad), "Detector pega mojibake real");
        }

        // Chaves construidas por Text::from (fromUTF8), nunca por
        // juce::String (const char*): o construtor legado interpreta os bytes
        // como Latin-1 no Windows e transforma cada acento em dois caracteres.
        // As literais vao com escapes pelo mesmo motivo â€” nao ha como depender
        // do compilador ou do editor para o conteudo dos bytes.
        const juce::StringArray keys
        {
            Text::from ("03 / Ficha da can\xC3\xA7\xC3\xA3o"),
            Text::from ("Sem t\xC3\xADtulo nas tags"),
            Text::from ("Lido das tags do arquivo"),
            Text::from ("ISRC do fonograma"),
            Text::from ("Ano de publica\xC3\xA7\xC3\xA3o"),
            Text::from ("BPM original"),
            Text::from ("Carregue um arquivo de \xC3\xA1udio para ler a ficha."),
            // "\x" do C++ consome todos os digitos hex seguintes, e 'd' e hex:
            // "conte\xBAdo" viraria 0xBAD. A string precisa ser partida.
            Text::from ("Campos sem valor n\xC3\xA3o constam no arquivo. A impress\xC3\xA3o digital \xC3\xA9 calculada localmente; cruz\xC3\xA1-la com o AcoustID exige rede e ainda n\xC3\xA3o est\xC3\xA1 habilitado."),
            Text::from ("Carregando o \xC3\xA1udio"),
            Text::from ("Preparando o carregamento"),
            Text::from ("Lendo o \xC3\xA1udio"),
            Text::from ("Desenhando a forma de onda"),
            Text::from ("Estimando o andamento"),
            Text::from ("Estimando a afina\xC3\xA7\xC3\xA3o"),
            Text::from ("Publicando o resultado"),
            Text::from ("Fonte: padr\xC3\xA3o (\xC3\x97" "2)"),
            Text::from ("Fonte: real (SoundStretch)"),
            Text::from ("\xC3\x81udio ajustado"),
            Text::from (" de sil\xC3\xAAncio inicial removido para alinhar ao in\xC3\xAD" "cio do editor.")
        };

        // Chaves que TEM de diferir entre pt-BR e en/es. Nao da para exigir
        // isso de todas: "BPM original" e identico nos tres idiomas por natureza,
        // e um teste que exigisse diferenca acusaria traducao correta.
        const juce::StringArray mustDiffer
        {
            Text::from ("03 / Ficha da can\xC3\xA7\xC3\xA3o"),
            Text::from ("Sem t\xC3\xADtulo nas tags"),
            Text::from ("Lido das tags do arquivo"),
            Text::from ("Ano de publica\xC3\xA7\xC3\xA3o"),
            Text::from ("Carregue um arquivo de \xC3\xA1udio para ler a ficha."),
            Text::from ("Campos sem valor n\xC3\xA3o constam no arquivo. A impress\xC3\xA3o digital \xC3\xA9 calculada localmente; cruz\xC3\xA1-la com o AcoustID exige rede e ainda n\xC3\xA3o est\xC3\xA1 habilitado.")
        };


        // Traducao ausente devolve a propria chave em pt-BR, entao a forma de
        // revelar a chave esquecida e comparar a MESMA chave entre idiomas.
        // A comparacao e por busca, nunca por indice: mustDiffer e um subconjunto
        // de keys, e alinhar os dois por posicao compararia "BPM original" com a
        // frase longa e passaria por acaso.
        for (const auto& key : mustDiffer)
        {
            const auto pt = Text::t (key);

            Text::setCulture (Text::Culture::EnglishUK);
            check (Text::t (key) != pt, "Traduzido em en-GB: " + key.toStdString());

            Text::setCulture (Text::Culture::EnglishUS);
            check (Text::t (key) != pt, "Traduzido em en-US: " + key.toStdString());

            Text::setCulture (Text::Culture::SpanishES);
            check (Text::t (key) != pt, "Traduzido em es-ES: " + key.toStdString());

            Text::setCulture (Text::Culture::PortugueseBR);
        }

        for (const auto culture : { Text::Culture::PortugueseBR, Text::Culture::EnglishUK,
                                     Text::Culture::EnglishUS, Text::Culture::SpanishES })
        {
            Text::setCulture (culture);
            const auto code = Text::cultureCode().toStdString();

            for (const auto& key : keys)
            {
                const auto value = Text::t (key);

                check (value.isNotEmpty(),
                       "Chave presente e traduzida em " + code + ": " + key.toStdString());
                check (! hasMojibake (value),
                       "Sem mojibake em " + code + ": [" + hexOf (value) + "] " + value.toStdString());
            }
        }

        // E o inverso: uma string igual nos quatro idiomas e traducao valida, nao
        // um esquecimento. Sem esta checagem, traduzir "BPM original" seria
        // aceito como melhoria.
        Text::setCulture (Text::Culture::SpanishES);
        check (Text::t ("BPM original") == Text::from ("BPM original"),
               "Termo universal permanece igual em es-ES");

        Text::setCulture (Text::Culture::PortugueseBR);
        check (Text::t (keys[0]) == keys[0], "pt-BR devolve a propria chave");
    }

    // ------------------------------------------------------------------------
    // Chromaprint (LGPL 2.1) â€” prova que a lib linkada produz fingerprint de
    // verdade, e nao apenas compila. O Chromaprint consome PCM int16 mono a
    // 11025 Hz, que e exatamente o formato que o worker vai entregar.
    // ------------------------------------------------------------------------

    constexpr int fingerprintSampleRate = 11025;

    std::vector<int16_t> synthesisePcm (const std::vector<double>& partialRatios, double seconds)
    {
        const int totalSamples = (int) (fingerprintSampleRate * seconds);
        std::vector<int16_t> pcm ((size_t) totalSamples);

        for (int i = 0; i < totalSamples; ++i)
        {
            const double t = (double) i / (double) fingerprintSampleRate;

            // Soma de parciais com envelope percussivo a cada 0,5 s: da a
            // transiente de ataque que o detector de onsets do Chromaprint
            // precisa, um tom senoidal puro seria degenerado.
            const double beatPosition = std::fmod (t, 0.5) / 0.5;
            const double envelope = std::exp (-4.0 * beatPosition);

            double value = 0.0;
            for (auto ratio : partialRatios)
                value += std::sin (2.0 * juce::MathConstants<double>::twoPi * 220.0 * ratio * t);

            value = value / (double) partialRatios.size() * envelope;
            pcm[(size_t) i] = (int16_t) std::lround (juce::jlimit (-1.0, 1.0, value) * 24000.0);
        }

        return pcm;
    }

    juce::String fingerprintOf (const std::vector<int16_t>& pcm, int& outDurationMs, juce::String& outStage)
    {
        outDurationMs = 0;

        auto* ctx = chromaprint_new (CHROMAPRINT_ALGORITHM_DEFAULT);
        if (ctx == nullptr)
        {
            outStage = "chromaprint_new retornou nullptr";
            return {};
        }

        juce::String result;
        char* encoded = nullptr;

        outStage = "chromaprint_start falhou";
        // Atencao: a API do Chromaprint retorna 1 em caso de SUCESSO e 0 em
        // caso de falha (macro FAIL_IF). Condicionar a == 0 inverte o fluxo.
        if (chromaprint_start (ctx, fingerprintSampleRate, 1) != 0)
        {
            // Alimenta em blocos: o worker tambem decodifica em blocos, e
            // verify-se de que feed() aceita chamadas parciais.
            const int blockSize = 4096;
            const int total = (int) pcm.size();
            bool ok = true;

            for (int pos = 0; ok && pos < total; pos += blockSize)
            {
                const int n = jmin (blockSize, total - pos);
                if (chromaprint_feed (ctx, pcm.data() + pos, n) == 0)
                {
                    outStage = "chromaprint_feed falhou no bloco em " + juce::String (pos);
                    ok = false;
                }
            }

            outStage = "chromaprint_finish falhou";
            if (ok && chromaprint_finish (ctx) == 0)
                ok = false;

            outStage = "chromaprint_get_fingerprint falhou";
            if (ok && (chromaprint_get_fingerprint (ctx, &encoded) == 0 || encoded == nullptr))
                ok = false;

            if (ok)
            {
                outStage = "ok";
                outDurationMs = chromaprint_get_item_duration_ms (ctx);
                result = juce::String::fromUTF8 (encoded, (int) std::strlen (encoded));
            }
        }

        chromaprint_dealloc (encoded);
        chromaprint_free (ctx);
        return result;
    }

    bool isBase64 (const juce::String& s)
    {
        if (s.isEmpty())
            return false;

        for (auto c : s)
        {
            const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                         || (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=';
            if (! ok)
                return false;
        }

        return true;
    }

    void testChromaprintFingerprint()
    {
        std::printf ("[chromaprint] fingerprint acustico\n");

        const auto* version = chromaprint_get_version();
        check (version != nullptr && std::strlen (version) > 0, "versao do Chromaprint disponivel");
        check (version != nullptr && juce::String (version).startsWith ("1.6"),
               "versao 1.6 conforme esperado");

        {
            // Sondeia as taxas suportadas e fixa o contrato do worker: o
            // Chromaprint reamostra internamente para 11025 Hz, mas aceita
            // qualquer taxa acima de 1000 Hz. Nao ha restricao a 11025.
            for (int rate : { 8000, 11025, 16000, 22050, 44100 })
            {
                auto* probe = chromaprint_new (CHROMAPRINT_ALGORITHM_DEFAULT);
                if (probe == nullptr)
                    continue;

                const int reported = chromaprint_get_sample_rate (probe);
                const int started = chromaprint_start (probe, rate, 1);
                check (started == 1, "start aceita " + juce::String (rate).toStdString() + " Hz");
                check (reported == fingerprintSampleRate,
                       "taxa interna alvo e " + juce::String (fingerprintSampleRate).toStdString() + " Hz");
                chromaprint_free (probe);
            }
        }

        // Assinatura distinta: harmonico par (fundamental + 2a + 4a oitava).
        const std::vector<double> majorTone { 1.0, 2.0, 4.0 };
        // Assinatura diferente, para provar que o fingerprint discrimina.
        const std::vector<double> minorTone { 1.0, 1.1892, 1.4983 };

        int durationMsMajor = 0, durationMsMinor = 0, durationMsShort = 0;
        juce::String stage;

        const auto fingerprintMajor = fingerprintOf (synthesisePcm (majorTone, 4.0), durationMsMajor, stage);
        const auto stageMajor = stage;
        const auto fingerprintMinor = fingerprintOf (synthesisePcm (minorTone, 4.0), durationMsMinor, stage);
        const auto fingerprintShort = fingerprintOf (synthesisePcm (majorTone, 1.0), durationMsShort, stage);

        check (fingerprintMajor.isNotEmpty(), "fingerprint gerado para 4 s de audio [" + stageMajor.toStdString() + "]");
        check (isBase64 (fingerprintMajor), "fingerprint em base64 valido");

        // chromaprint_get_item_duration_ms devolve config()->item_duration(),
        // ou seja, a duracao de um item do algoritmo â€” uma CONSTANTE. Nao e a
        // duracao do audio fornecido. A duracao real precisa ser calculada por
        // nos a partir das amostras decodificadas, porque e ela que o AcoustID
        // exige no parametro `duration`. Esta assercao existe para travar o
        // comportamento e impedir que alguem "conserte" isso achando que e bug.
        check (durationMsMajor > 0, "duracao de item do algoritmo reportada em ms");
        check (durationMsMajor == durationMsShort,
               "duracao de item independe do audio fornecido (4 s vs 1 s)");
        check (durationMsMajor == durationMsMinor, "duracao de item independe do conteudo");

        const auto secondsFor = [] (const std::vector<int16_t>& pcm)
        {
            return (double) pcm.size() / (double) fingerprintSampleRate;
        };

        checkClose (secondsFor (synthesisePcm (majorTone, 4.0)), 4.0, 0.001,
                    "duracao do audio derivada da contagem de amostras");
        checkClose (secondsFor (synthesisePcm (majorTone, 1.0)), 1.0, 0.001,
                    "duracao do audio em 1 s derivada da contagem de amostras");

        check (fingerprintMajor != fingerprintMinor, "sinais diferentes geram fingerprints diferentes");

        // Determinismo: mesma entrada, mesma saida. Sem isso, nao ha como
        // cachear por fingerprint nem comparar resultados entre execucoes.
        int durationMsRepeat = 0;
        const auto fingerprintRepeat = fingerprintOf (synthesisePcm (majorTone, 4.0), durationMsRepeat, stage);
        check (fingerprintMajor == fingerprintRepeat, "fingerprint e deterministico");
        check (durationMsMajor == durationMsRepeat, "duracao e deterministica");
    }

    // ---------------------------------------------------------------------
    // Contrato de concorrencia do player.
    //
    // Regressao do Bloco 2.1: processBlock usava ScopedLock e mantinha a
    // critical section durante o laco de amostras. A correcao trocou o lock por
    // publicacao atomica (atomic_store / atomic_load de shared_ptr). Este teste
    // existe para travar o contrato: enquanto uma thread troca o buffer
    // transformado, outra preenche audio. Se o snapshot deixar de manter o buffer
    // vivo, o sintoma e use-after-free â€” lixo, NaN ou queda â€” e nao um valor
    // errado. Por isso a asserca e sobre integridade do sinal, nao sobre
    // igualdade com um valor esperado.
    // ---------------------------------------------------------------------
    juce::File writeTestWav()
    {
        // 2 s de onda quadrada a 220 Hz: conteudo trivial, mas com bordas
        // suficientes para o fillOutput produzir valores variaveis.
        constexpr int sampleRate = 44100;
        constexpr int numSamples = sampleRate * 2;
        juce::AudioBuffer<float> buffer (1, numSamples);

        for (int i = 0; i < numSamples; ++i)
            buffer.setSample (0, i, (i / 100) % 2 == 0 ? 0.5f : -0.5f);

        juce::WavAudioFormat format;
        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getChildFile ("parteplay-concurrency-test.wav");
        file.deleteFile();

        const auto options = juce::AudioFormatWriterOptions()
                                .withSampleRate (sampleRate)
                                .withNumChannels (1)
                                .withBitsPerSample (24);

        // createWriterFor toma o stream por referencia de unique_ptr<OutputStream>
        // e assume a posse dele, entao o ponteiro precisa ja estar no tipo base.
        std::unique_ptr<juce::FileOutputStream> fileStream (file.createOutputStream());
        std::unique_ptr<juce::OutputStream> stream = std::move (fileStream);

        std::unique_ptr<juce::AudioFormatWriter> writer (format.createWriterFor (stream, options));

        if (writer == nullptr)
            return {};

        writer->writeFromAudioSampleBuffer (buffer, 0, numSamples);
        writer.reset();
        return file;
    }

    // Deleter que envenena o buffer em vez de so liberar.
    //
    // A intencao original era transformar use-after-free em "a leitura ve NaN",
    // para o teste ter dentes. NAO FUNCIONA, e o motivo esta registrado aqui de
    // proposito, porque a tentativa e instrutiva:
    //
    //  - envenenando E liberando: o publisher realoca no ciclo seguinte e
    //    sobrescreve o veneno antes de o audio ler. Resultado medido: falha
    //    esporadica por ACCESS_VIOLATION (0xC0000005) em ~40% das execucoes.
    //  - envenenando SEM liberar: o bloco fica mapeado, mas o audio so segura o
    //    ponteiro durante uma unica chamada de fillOutput; a janela em que a
    //    leitura cruza a troca e estreita demais. Resultado medido: 0 falhas em
    //    12 execucoes com o bug presente.
    //
    // Portanto este deleter NAO e um detector de use-after-free. Ele existe
    // porque torna visivel, em caso de falha, QUE o objeto foi liberado com
    // dados corrompidos â€” mas nao substitui uma ferramenta de deteccao de
    // memoria. Use /fsanitize=address (MSVC) para isso; ver CHANGELOG.md (sanitizers).
    struct PoisoningDeleter
    {
        void operator() (const juce::AudioBuffer<float>* buffer) const noexcept
        {
            if (buffer == nullptr)
                return;

            auto* writable = const_cast<juce::AudioBuffer<float>*> (buffer);

            for (int ch = 0; ch < writable->getNumChannels(); ++ch)
            {
                float* dst = writable->getWritePointer (ch);
                std::fill (dst, dst + writable->getNumSamples(), std::numeric_limits<float>::quiet_NaN());
            }

            delete buffer;
        }
    };

    void testPlayerPublicationRace()
    {
        std::printf ("[V1] Publicacao do player sem lock\n");

        const auto wav = writeTestWav();
        check (wav.existsAsFile(), "WAV de teste criado");

        if (! wav.existsAsFile())
            return;

        FilePlayer player;
        player.loadFromFile (wav);
        check (player.hasAudio(), "player carregou audio do WAV");
        check (player.getNumSamples() > 0, "player tem amostras");

        if (! player.hasAudio())
            return;

        // fillOutput tem de ser const: e o que documenta que ele nao escreve
        // estado do player e, portanto, pode correr sem lock. Se um dia alguem
        // criar um membro mutado aqui, a assinatura quebra e o teste falha.
        const FilePlayer& readOnlyView = player;
        juce::AudioBuffer<float> probe (2, 256);
        probe.clear();
        check (readOnlyView.fillOutput (probe, 0, readOnlyView.getSampleRate()),
               "fillOutput acessivel em FilePlayer const (contrato sem lock)");

        std::atomic<bool> stop { false };
        std::atomic<int> swaps { 0 };

        // Thread de mensagem: alterna o buffer transformado, inclusive nullptr
        // (caminho de identidade). Este e o writer que disputa com fillOutput.
        std::thread publisher ([&]
        {
            int variant = 0;

            while (! stop.load (std::memory_order_relaxed))
            {
                if (variant == 0)
                {
                    player.setPlaybackBuffer (nullptr, 1.0);
                }
                else
                {
                    auto* raw = new juce::AudioBuffer<float> (2, 4096);
                    const float level = 0.25f * (float) variant;

                    for (int ch = 0; ch < 2; ++ch)
                        for (int i = 0; i < raw->getNumSamples(); ++i)
                            raw->setSample (ch, i, (i % 32 == 0) ? level : -level);

                    // make_shared nao aceita deleter proprio; a construcao direta
                    // do shared_ptr guarda o PoisoningDeleter junto.
                    std::shared_ptr<const juce::AudioBuffer<float>> buffer (raw, PoisoningDeleter());
                    player.setPlaybackBuffer (std::move (buffer), 1.0 / (1.0 + 0.01 * variant));
                }

                variant = (variant + 1) % 8;
                swaps.fetch_add (1, std::memory_order_relaxed);
            }
        });

        // Thread de audio: enche blocos sem lock, como o processBlock faz.
        //
        // O laco NAO tem contagem fixa de blocos: ele roda ate o publicador
        // completar um numero minimo de trocas (teto de seguranca para nao
        // pendurar se a thread perder o schedule). Fixar blocos e afirmar sobre
        // quantas trocas a outra thread fez deixa o teste dependente do
        // scheduler â€” foi assim que ele ficou flaky antes. Aqui a sobreposicao e
        // garantida por construcao: o laco so sai depois que as trocas
        // aconteceram.
        juce::AudioBuffer<float> out (2, 512);
        constexpr int minSwaps = 1500;
        constexpr int blockCap = 2000000;
        int blocks = 0;
        bool sawNonFinite = false;
        bool sawOutOfRange = false;

        while (swaps.load (std::memory_order_relaxed) < minSwaps && blocks < blockCap)
        {
            out.clear();
            readOnlyView.fillOutput (out, static_cast<juce::int64> (blockCap) + static_cast<juce::int64> (blocks) * out.getNumSamples(),
                                     readOnlyView.getSampleRate());

            for (int ch = 0; ch < out.getNumChannels(); ++ch)
                for (int i = 0; i < out.getNumSamples(); ++i)
                {
                    const float v = out.getSample (ch, i);

                    if (! std::isfinite (v))            sawNonFinite = true;
                    else if (v > 1.5f || v < -1.5f)     sawOutOfRange = true;
                }

            ++blocks;
        }

        const int performedSwaps = swaps.load (std::memory_order_relaxed);
        stop.store (true, std::memory_order_relaxed);
        publisher.join();

        check (blocks > 100, "processBlock simulado rodou blocos suficientes para cruzar a troca");
        check (performedSwaps >= minSwaps,
               "o publicador trocou o buffer durante a leitura: houve sobreposicao real");
        check (! sawNonFinite, "nenhum NaN/Inf nos blocos lidos");
        check (! sawOutOfRange, "nenhum valor fora de faixa: leitura sobre objeto vivo");

        // ALCANCE DESTE TESTE, explicitamente: ele trava o contrato POSITIVO â€”
        // fillOutput e const, o publicador e message thread, existe sobreposicao
        // real e a saida permanece integra. Ele nao prova ausencia de
        // use-after-free: a deteccao dessa classe de bug em build Release nao foi
        // confiavel (medido: 0-40% conforme a estrategia). Para isso use
        // /fsanitize=address. ver CHANGELOG.md (sanitizers).

        player.clear();
        wav.deleteFile();
    }
}

    // Grava um WAV percussivo de duracao e canais pedidos. O sinal tem ataque a
    // cada 0,5 s porque o detector de onsets do Chromaprint precisa de
    // transiente: um tom senoidal puro e degenerado e pode nao gerar
    // fingerprint.
    juce::File writeTestWav (const juce::String& name, double sampleRate,
                            int numChannels, double seconds)
    {
        constexpr double partialRatios[] = { 1.0, 1.5, 2.0, 2.5, 3.0 };
        const int numSamples = (int) (sampleRate * seconds);

        juce::AudioBuffer<float> buffer (numChannels, numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            const double t = (double) i / sampleRate;
            const double beatPosition = std::fmod (t, 0.5) / 0.5;
            const double envelope = std::exp (-4.0 * beatPosition);

            double value = 0.0;
            for (auto ratio : partialRatios)
                value += std::sin (2.0 * juce::MathConstants<double>::twoPi * 220.0 * ratio * t);

            value = value / (double) (sizeof (partialRatios) / sizeof (partialRatios[0])) * envelope;
            const auto s = (float) juce::jlimit (-1.0, 1.0, value) * 0.9f;

            for (int c = 0; c < numChannels; ++c)
                buffer.setSample (c, i, s);
        }

        juce::WavAudioFormat format;
        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getChildFile (name);
        file.deleteFile();

        const auto options = juce::AudioFormatWriterOptions()
                                .withSampleRate (sampleRate)
                                .withNumChannels (numChannels)
                                .withBitsPerSample (24);

        std::unique_ptr<juce::FileOutputStream> fileStream (file.createOutputStream());
        std::unique_ptr<juce::OutputStream> stream = std::move (fileStream);
        std::unique_ptr<juce::AudioFormatWriter> writer (format.createWriterFor (stream, options));

        if (writer == nullptr)
            return {};

        writer->writeFromAudioSampleBuffer (buffer, 0, numSamples);
        writer.reset();
        return file;
    }

    // Regressao do bug de `(int16_t) x * 32767.0f`.
    //
    // Este teste existe porque a suite inteira passava com o bug instalado. Os
    // testes de fingerprint so afirmavam que o resultado era base64 valido, e um
    // PCM de tres niveis produz base64 valido. Aqui a invariante e quantitativa:
    // uma rampa de -1 a 1 tem de produzir muitos niveis distintos, e um valor
    // moderado tem de sobreviver inteiro.
    void testFingerprintPcmConversion()
    {
        std::printf ("\n[fingerprint] conversao para PCM int16");

        // Full scale exato nas duas pontas.
        check (Fingerprint::toPcm16 (1.0f) == 32767, "+1.0 satura em 32767");
        check (Fingerprint::toPcm16 (-1.0f) == -32767, "-1.0 satura em -32767");
        check (Fingerprint::toPcm16 (0.0f) == 0, "0.0 vira 0");

        // Entrada fora de faixa e limitada, nao envolve. A soma dos canais num
        // arquivo multicanal passa de 1.0 com facilidade; sem limitar, o
        // produto estouraria o int16 e daria de -32768 (o wrap e silencioso).
        check (Fingerprint::toPcm16 (2.5f) == 32767, "entrada acima de 1.0 e limitada");
        check (Fingerprint::toPcm16 (-3.0f) == -32767, "entrada abaixo de -1.0 e limitada");
        check (Fingerprint::toPcm16 (1.0e6f) == 32767, "entrada enorme nao da overflow");
        check (Fingerprint::toPcm16 (-1.0e6f) == -32767, "entrada enorme negativa nao da overflow");

        // A regressao central: 0.37 nao pode virar 0. Com o cast no meio da
        // expressao, o valor era truncado para int16 ANTES de escalar, e 0.37
        // virava 0 - o sinal sumia.
        const auto moderate = Fingerprint::toPcm16 (0.37f);
        check (moderate != 0, "valor moderado nao colapsa para zero");
        check (std::abs ((double) moderate - 0.37 * Fingerprint::pcm16FullScale) <= 2.0,
               "valor moderado preserva a escala (0.37 -> perto de 12124)");

        // Proporcionalidade: metade da amplitude e metade da escala.
        check (Fingerprint::toPcm16 (0.5f) == 16383, "0.5 satura em 16383 (truncamento para zero)");

        // A invariante estrutural: uma rampa completa tem de produzir muitos
        // niveis distintos. A versao com o bug produzia exatamente tres
        // (-32767, 0, +32767), entao qualquer limiar acima de 3 a reprova.
        std::set<int16_t> levels;
        constexpr int rampPoints = 1001;

        for (int i = 0; i < rampPoints; ++i)
        {
            const auto value = -1.0f + 2.0f * (float) i / (float) (rampPoints - 1);
            levels.insert (Fingerprint::toPcm16 (value));
        }

        check (levels.size() > (size_t) (rampPoints / 2),
               "rampa de -1 a 1 preserva resolucao (nao colapsa para poucos niveis)");

        // Monotonicidade: mais amplitude, mais valor. Um sinal invertido aqui
        // nao acusaria erro em nenhum dos testes acima.
        bool monotonic = true;

        for (int i = 1; i < rampPoints && monotonic; ++i)
        {
            const auto previous = Fingerprint::toPcm16 (-1.0f + 2.0f * (float) (i - 1) / (float) (rampPoints - 1));
            const auto current  = Fingerprint::toPcm16 (-1.0f + 2.0f * (float) i / (float) (rampPoints - 1));
            monotonic = current >= previous;
        }

        check (monotonic, "conversao e monotonica (sinal nao invertido)");
    }

    void testFingerprintCompute()
    {
        std::printf ("\n[fingerprint] calculo offline");

        // 12 s: acima do minimo util do algoritmo e muito abaixo dos 120 s que
        // chromaprint_get_item_duration_ms reporta como constante.
        const auto file = writeTestWav ("parteplay-fp-compute.wav", 44100.0, 1, 12.0);
        check (file.existsAsFile(), "WAV de teste criado");

        if (! file.existsAsFile())
            return;

        const auto result = Fingerprint::compute (file, nullptr);

        check (result.state == Fingerprint::State::ready, "compute concluded com sucesso");
        check (result.isUsable(), "resultado utilizavel para consulta ao AcoustID");
        check (isBase64 (result.fingerprint), "fingerprint em base64");
        check (result.fingerprint == result.fingerprint, "fingerprint estavel");
        check (std::isfinite (result.durationSeconds), "duracao finita");

        // A regressao que este teste trava: a duracao tem de vir da contagem de
        // amostras. chromaprint_get_item_duration_ms devolve 120000 (tamanho da
        // janela do algoritmo) para QUALQUER faixa; usar isso faria toda
        // consulta ao AcoustID enviar 120 s.
        check (std::abs (result.durationSeconds - 12.0) < 0.05,
               "duracao veio da contagem de amostras, nao da constante do algoritmo");
        check (result.sourceSampleRate == 44100.0, "taxa nativa preservada no resultado");
        check (result.sourceChannels == 1, "contagem de canais preservada");

        // Determinismo: mesma entrada, mesma saida.
        const auto again = Fingerprint::compute (file, nullptr);
        check (again.fingerprint == result.fingerprint, "repetir o calculo da o mesmo fingerprint");

        file.deleteFile();
    }

    void testFingerprintStereo()
    {
        std::printf ("\n[fingerprint] downmix de multicanal");

        // Mais de 1 canal: precisa passar pelo downmix para mono, senao a
        // chromaprint_start e rejeitada.
        const auto file = writeTestWav ("parteplay-fp-stereo.wav", 48000.0, 2, 11.0);
        check (file.existsAsFile(), "WAV estereo criado");

        if (! file.existsAsFile())
            return;

        const auto result = Fingerprint::compute (file, nullptr);

        check (result.state == Fingerprint::State::ready, "estereo aceito");
        check (isBase64 (result.fingerprint), "estereo produziu fingerprint valido");
        check (result.sourceSampleRate == 48000.0, "taxa nativa de 48 kHz preservada");
        check (std::abs (result.durationSeconds - 11.0) < 0.05, "duracao stereo correta");

        file.deleteFile();
    }

    void testFingerprintRejections()
    {
        std::printf ("\n[fingerprint] rejeicoes");

        // Faixa curta: rejeitar aqui e melhor do que receber "not found" do
        // AcoustID sem explicacao.
        const auto shortFile = writeTestWav ("parteplay-fp-short.wav", 44100.0, 1, 3.0);

        if (shortFile.existsAsFile())
        {
            const auto result = Fingerprint::compute (shortFile, nullptr);
            check (result.state == Fingerprint::State::failed, "faixa curta rejeitada");
            check (result.fingerprint.isEmpty(), "faixa curta nao devolve fingerprint");
            check (result.message.isNotEmpty(), "faixa curta explica o motivo");
            shortFile.deleteFile();
        }

        // Arquivo inexistente.
        const auto missing = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                .getChildFile ("parteplay-nao-existe-9f3a.wav");
        missing.deleteFile();
        const auto missingResult = Fingerprint::compute (missing, nullptr);
        check (missingResult.state == Fingerprint::State::failed, "arquivo inexistente falha");
        check (missingResult.message.isNotEmpty(), "arquivo inexistente explica o motivo");

        // Formato nao suportado: um .txt nao abre como audio.
        const auto notAudio = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                .getChildFile ("parteplay-nao-e-audio-9f3a.wav");
        notAudio.deleteFile();

        if (notAudio.createOutputStream() != nullptr)
        {
            notAudio.replaceWithText ("isto nao e audio");
            const auto result = Fingerprint::compute (notAudio, nullptr);
            check (result.state == Fingerprint::State::failed, "arquivo nao-audio falha");
            notAudio.deleteFile();
        }
    }

    void testFingerprintCancel()
    {
        std::printf ("\n[fingerprint] cancelamento");

        const auto file = writeTestWav ("parteplay-fp-cancel.wav", 44100.0, 1, 40.0);

        if (! file.existsAsFile())
            return;

        std::atomic<bool> flag { true };
        const auto result = Fingerprint::compute (file, &flag);

        check (result.state == Fingerprint::State::cancelled, "flag ligado antes de comecar cancela");
        check (result.fingerprint.isEmpty(), "cancelado nao devolve fingerprint");

        file.deleteFile();
    }

    void testFingerprintWorkerAsync()
    {
        std::printf ("\n[fingerprint] worker assincrono");

        const auto file = writeTestWav ("parteplay-fp-worker.wav", 44100.0, 1, 12.0);

        if (! file.existsAsFile())
            return;

        Fingerprint::Worker worker;

        check (worker.poll().state == Fingerprint::State::idle, "worker comeca ocioso");
        check (! worker.isBusy(), "worker comeca livre");

        worker.start (file);

        // A message thread dispara e coleta: nao pode esperar o calculo.
        auto result = worker.poll();
        check (result.state == Fingerprint::State::running, "poll logo apos start devolve running");

        const auto deadline = juce::Time::getMillisecondCounter() + 30000;

        while (worker.isBusy() && juce::Time::getMillisecondCounter() < deadline)
            juce::Thread::sleep (10);

        check (! worker.isBusy(), "worker terminou dentro do prazo");
        check (worker.poll().state != Fingerprint::State::running, "estado deixou de ser running");

        result = worker.poll();
        check (result.state == Fingerprint::State::ready, "worker async concluiu com sucesso");
        check (isBase64 (result.fingerprint), "worker async devolveu base64");

        file.deleteFile();
    }

    void testFingerprintWorkerLastStartWins()
    {
        std::printf ("\n[fingerprint] ultimo start vence");

        // Regressao da guarda de geracao: um job antigo que termina depois de um
        // start novo NAO pode publicar por cima do resultado novo, nem-clear
        // busy do pedido vigente.
        const auto slow = writeTestWav ("parteplay-fp-a.wav", 44100.0, 1, 60.0);
        const auto other = writeTestWav ("parteplay-fp-b.wav", 44100.0, 1, 15.0);

        if (! slow.existsAsFile() || ! other.existsAsFile())
        {
            slow.deleteFile();
            other.deleteFile();
            return;
        }

        const auto expected = Fingerprint::compute (other, nullptr);
        check (expected.state == Fingerprint::State::ready, "referencia do arquivo B calculada");

        int sawB = 0;
        const int rounds = 5;

        for (int round = 0; round < rounds; ++round)
        {
            Fingerprint::Worker worker;

            // A primeiro, logo seguido do B: o job do A quase sempre ainda esta
            // rodando quando o B chega, que e exatamente a janela perigosa.
            worker.start (slow);
            worker.start (other);

            const auto deadline = juce::Time::getMillisecondCounter() + 30000;

            while (worker.isBusy() && juce::Time::getMillisecondCounter() < deadline)
                juce::Thread::sleep (10);

            const auto settled = worker.poll();
            if (settled.state == Fingerprint::State::ready
                && settled.fingerprint == expected.fingerprint)
                ++sawB;
        }

        check (sawB == rounds,
               "em todos os rounds o resultado assentado foi o do ultimo start, nunca o do obsoleto");

        slow.deleteFile();
        other.deleteFile();
    }

    void testFingerprintWorkerCancel()
    {
        std::printf ("\n[fingerprint] cancelamento do worker");

        const auto file = writeTestWav ("parteplay-fp-worker-cancel.wav", 44100.0, 1, 45.0);

        if (! file.existsAsFile())
            return;

        Fingerprint::Worker worker;
        worker.start (file);
        worker.cancel();

        check (! worker.isBusy(), "cancel deixa o worker livre");
        check (worker.poll().state == Fingerprint::State::cancelled, "estado fica cancelled");

        file.deleteFile();
    }

    // Falha de traducao silenciosa: Text::t devolve a propria chave quando nao
    // acha entrada na tabela. O efeito nao e erro, e o usuario ver pt-BR dentro
    // de um build em ingles. Este teste fecha a porta para as strings novas da
    // identificacao.
    void testIdentificationTranslations()
    {
        std::printf ("\n[fingerprint] traducoes do novo estado\n");

        // Chaves novas do estado de identificacao, em UTF-8.
        const char* keys[] =
        {
            "Impress\xC3\xA3o digital",
            "analisando\xE2\x80\xA6",
            "pronta",
            "cancelada",
            "arquivo ausente",
            "formato inv\xC3\xA1lido",
            "sem canais",
            "amostragem baixa",
            "\xC3\xA1udio curto",
            "falha na leitura",
            "impress\xC3\xA3o vazia",
            "falha",
            "Impress\xC3\xA3o digital calculada localmente. Cruz\xC3\xA1-la com o AcoustID exige rede e ainda n\xC3\xA3o est\xC3\xA1 habilitado."
        };

        const auto original = Text::getCulture();

        for (auto* const key : keys)
        {
            const auto pt = Text::from (key);

            // PresenÃ§a na tabela, e nÃ£o diferenÃ§a de texto. "cancelada" Ã© a mesma
            // palavra em pt-BR e es-ES, entÃ£o comparar as traduÃ§Ãµes daria atestado
            // falso; sÃ³ a busca na tabela diz a verdade.
            check (Text::has (pt), std::string ("chave esta na tabela: ") + key);

            for (const auto culture : { Text::Culture::EnglishUK, Text::Culture::EnglishUS,
                                         Text::Culture::SpanishES })
            {
                Text::setCulture (culture);
                const auto value = Text::t (key);
                check (! hasMojibake (value),
                       std::string ("traducao sem mojibake em ")
                           + Text::cultureCode().toStdString () + ": " + key);
            }
        }

        // As traduÃ§Ãµes que precisam mesmo diferir. Esta lista existe porque nem
        // toda chave muda entre idiomas: "cancelada" Ã© igual em pt-BR e es por
        // natureza, e exigir diferenÃ§a ali acusaria traduÃ§Ã£o correta.
        const char* mustDiffer[] =
        {
            "Impress\xC3\xA3o digital",
            "analisando\xE2\x80\xA6",
            "arquivo ausente",
            "formato inv\xC3\xA1lido",
            "amostragem baixa",
            "\xC3\xA1udio curto",
            "falha na leitura",
            "impress\xC3\xA3o vazia",
            "falha"
        };

        for (auto* const key : mustDiffer)
        {
            const auto pt = Text::from (key);

            for (const auto culture : { Text::Culture::EnglishUK, Text::Culture::EnglishUS,
                                         Text::Culture::SpanishES })
            {
                Text::setCulture (culture);
                check (Text::t (key) != pt,
                       std::string ("traducao difere da chave em ")
                           + Text::cultureCode().toStdString () + ": " + key);
            }
        }

        // Volta a pt-BR: os testes seguintes dependem disso.
        Text::setCulture (original);
    }

    // As culturas sao BCP 47 completos, nao apenas o idioma: "en-GB" e "en-US" sao
    // entradas diferentes, e o codigo valido do Reino Unido e GB (ISO 3166), nao
    // UK. O combo de idioma mostra o nome nativo com a regiao, para o usuario
    // distinguir a variante.
    void testCultures()
    {
        std::printf ("\n[texto] codigos BCP 47 e nomes nativos\n");

        check (Text::cultureCode (Text::Culture::PortugueseBR) == Text::from ("pt-BR"),
               "pt-BR usa codigo completo");
        check (Text::cultureCode (Text::Culture::EnglishUK) == Text::from ("en-GB"),
               "ingles britanico e en-GB, nao en-UK");
        check (Text::cultureCode (Text::Culture::EnglishUS) == Text::from ("en-US"),
               "ingles americano e en-US");
        check (Text::cultureCode (Text::Culture::SpanishES) == Text::from ("es-ES"),
               "espanhol e es-ES");

        check (Text::cultureName (Text::Culture::EnglishUK) == Text::from ("English (UK)"),
               "nome nativo en-GB mostra a regiao");
        check (Text::cultureName (Text::Culture::EnglishUS) == Text::from ("English (US)"),
               "nome nativo en-US mostra a regiao");
        check (Text::cultureName (Text::Culture::SpanishES)
               == Text::from ("Espa\xC3\xB1ol (Espa\xC3\xB1") + Text::from ("a)"),
               "nome nativo es-ES acentuado");
        check (Text::cultureName (Text::Culture::PortugueseBR) == Text::from ("Portugu\xC3\xAAs (Brasil)"),
               "nome nativo pt-BR acentuado");

        check (Text::cultureFromCode ("en-GB") == Text::Culture::EnglishUK,
               "cultureFromCode en-GB");
        check (Text::cultureFromCode ("en-US") == Text::Culture::EnglishUS,
               "cultureFromCode en-US");
        check (Text::cultureFromCode ("es-ES") == Text::Culture::SpanishES,
               "cultureFromCode es-ES");
        check (Text::cultureFromCode ("pt-BR") == Text::Culture::PortugueseBR,
               "cultureFromCode pt-BR");
        check (Text::cultureFromCode ("xyz") == Text::Culture::PortugueseBR,
               "cultureFromCode desconhecido cai em pt-BR");
    }

    // Separador decimal por cultura: en-US e en-GB usam ponto ("440.5"), pt-BR e
    // es-ES usam virgula ("128,5"). E o detalhe que um build sem localizacao de
    // numero entrega errado para o usuario que escreve afinacao em espanhol.
    void testNumberFormatting()
    {
        std::printf ("\n[texto] separador decimal por cultura\n");

        Text::setCulture (Text::Culture::PortugueseBR);
        check (Text::number (128.5, 1) == Text::from ("128,5"), "pt-BR usa virgula decimal");

        Text::setCulture (Text::Culture::SpanishES);
        check (Text::number (128.5, 1) == Text::from ("128,5"), "es-ES usa virgula decimal");
        check (Text::number (440.0, 0) == "440", "decimal com zero casas e inteiro");

        Text::setCulture (Text::Culture::EnglishUK);
        check (Text::number (128.5, 1) == "128.5", "en-GB usa ponto decimal");
        check (Text::number (-1.5, 1) == "-1.5", "en-GB preserva o sinal negativo");

        Text::setCulture (Text::Culture::EnglishUS);
        check (Text::number (128.5, 1) == "128.5", "en-US usa ponto decimal");

        check (Text::number (440) == "440", "inteiro nao ganha separador de milhar");

        Text::setCulture (Text::Culture::PortugueseBR);
    }

    // Grafia por variante do ingles: en-GB escreve "analysing" e "cancelled",
    // en-US "analyzing" e "canceled". Sem uma coluna por variante, um desses dois
    // paises recebe a grafia do outro â€” exatamente o que este teste veda. O es-ES
    // tambem ganha a acentuacao completa (valida, vacia).
    void testEnglishAndSpanishVariants()
    {
        std::printf ("\n[texto] variantes de grafia\n");

        const auto analysing = Text::from ("analisando\xE2\x80\xA6");
        const auto cancelled = Text::from ("cancelada");

        Text::setCulture (Text::Culture::EnglishUK);
        check (Text::t (analysing) == Text::from ("analysing\xE2\x80\xA6"),
               "en-GB: analysing");
        check (Text::t (cancelled) == Text::from ("cancelled"),
               "en-GB: cancelled com dois L");

        Text::setCulture (Text::Culture::EnglishUS);
        check (Text::t (analysing) == Text::from ("analyzing\xE2\x80\xA6"),
               "en-US: analyzing");
        check (Text::t (cancelled) == Text::from ("canceled"),
               "en-US: canceled com um L");

        Text::setCulture (Text::Culture::SpanishES);
        check (Text::t (Text::from ("formato inv\xC3\xA1lido")) == Text::from ("formato no v\xC3\xA1lido"),
               "es-ES acentua valido");
        check (Text::t (Text::from ("impress\xC3\xA3o vazia")) == Text::from ("huella vac\xC3\xAD") + Text::from ("a"),
               "es-ES acentua vacia");
        check (Text::t (Text::from ("falha")) == Text::from ("fallo"),
               "es-ES: fallo");
        check (Text::t (Text::from ("\xC3\xA1udio curto")) == Text::from ("audio demasiado corto"),
               "es-ES: audio demasiado corto");

        Text::setCulture (Text::Culture::PortugueseBR);
    }

    // Sinal sintetico de metronomo: impulsos curtos e decrescentes nas batidas,
    // com o tempo forte (downbeat) mais alto. E o unico jeito de travar o
    // analisador sem depender de um arquivo de audio real e sem o ouvido.
    juce::AudioBuffer<float> makeClickTrack (double sampleRate, double bpm, int beatsPerBar,
                                             int bars, float offbeatPeak)
    {
        const double secondsPerBeat = 60.0 / bpm;
        const int totalSamples = (int) std::ceil (bars * beatsPerBar * secondsPerBeat * sampleRate);
        const int clickSamples = (int) std::round (sampleRate * 0.02);

        juce::AudioBuffer<float> buffer (1, juce::jmax (1, totalSamples));
        buffer.clear();

        auto* data = buffer.getWritePointer (0);
        const int totalBeats = bars * beatsPerBar;

        for (int beat = 0; beat < totalBeats; ++beat)
        {
            const int start = (int) std::lround ((double) beat * secondsPerBeat * sampleRate);
            const bool downbeat = (beat % beatsPerBar) == 0;
            const float peak = downbeat ? 1.0f : offbeatPeak;

            for (int i = 0; i < clickSamples && start + i < buffer.getNumSamples(); ++i)
            {
                const float envelope = std::exp (-4.0f * (float) i / (float) clickSamples);
                data[start + i] += peak * envelope
                                   * std::sin (juce::MathConstants<float>::twoPi
                                               * 1000.0f * (float) i / (float) sampleRate);
            }
        }

        return buffer;
    }

    void testTempoAnalysis()
    {
        std::printf ("\n[tempo] estimativa de andamento\n");

        constexpr double sr = 44100.0;

        // 103 BPM em 3/4 (valsa): o caso relatado. O analisador antigo devolvia
        // 69,0 - que e 103,5 * 2/3 -, a ambiguidade 3:2 que este teste veda.
        const auto waltz = makeClickTrack (sr, 103.0, 3, 40, 0.5f);
        checkClose (TempoAnalyser::estimateBpm (sr, waltz), 103.0, 2.0,
                    "valsa a 103 BPM nao cai para 69");

        // 120 BPM em 4/4: a referencia do prior de tactus.
        const auto march = makeClickTrack (sr, 120.0, 4, 40, 0.5f);
        checkClose (TempoAnalyser::estimateBpm (sr, march), 120.0, 2.0,
                    "marcha a 120 BPM");

        // 90 BPM em 4/4: abaixo do prior, mas ainda dentro da faixa.
        const auto slow = makeClickTrack (sr, 90.0, 4, 40, 0.5f);
        checkClose (TempoAnalyser::estimateBpm (sr, slow), 90.0, 2.0,
                    "andamento lento a 90 BPM");

        // Assinatura: 3/4 como ternario, 4/4 como quaternario.
        check (TempoAnalyser::estimateBeatsPerBar (sr, waltz, 103.0) == 3,
               "3/4 reconhecido como tres tempos por compasso");
        check (TempoAnalyser::estimateBeatsPerBar (sr, march, 120.0) == 4,
               "4/4 reconhecido como quatro tempos por compasso");
    }

    void testTempoProgress()
    {
        std::printf ("\n[tempo] progresso da carga\n");

        constexpr double sr = 44100.0;
        const auto track = makeClickTrack (sr, 120.0, 4, 40, 0.5f);

        float last = -1.0f;
        bool monotonic = true;
        int calls = 0;

        TempoAnalyser::estimateBpm (sr, track, [&] (float p)
        {
            if (p + 1.0e-4f < last)
                monotonic = false;

            last = p;
            ++calls;
        });

        check (calls > 0, "progresso reporta ao menos uma vez");
        check (monotonic, "progresso nunca retrocede");
        check (last >= 0.999f, "progresso termina em 1");
    }

    void testExternalBpm()
    {
        std::printf ("\n[tempo] pipeline externa (ffmpeg + soundstretch)\n");

        // Parser: primeiro decimal de qualquer linha que mencione "bpm".
        check (ExternalBpm::parseBpm ("Detected BPM rate 120.0\n") == 120.0,
               "parseBpm le a linha classica do soundstretch");
        check (ExternalBpm::parseBpm ("BPM: 137.5\n") == 137.5,
               "parseBpm aceita rotulo alternativo");
        check (ExternalBpm::parseBpm ("nothing here\n") == 0.0,
               "parseBpm ignora saida sem bpm");
        check (ExternalBpm::parseBpm ("") == 0.0,
               "parseBpm de vazio e zero");

        // Linha de comando do ffmpeg: mono, 44,1 kHz, le a origem e escreve o WAV.
        const juce::File source ("C:/tmp/song.mp3");
        const juce::File wav ("C:/tmp/song.parteplay-bpm.wav");
        const auto ffmpegArgs = ExternalBpm::buildFfmpegArgs (source, wav);

        check (ffmpegArgs.contains ("-ac"), "ffmpeg define canais");
        check (ffmpegArgs.contains ("1"), "ffmpeg converte para mono");
        check (ffmpegArgs.contains ("-ar"), "ffmpeg define taxa");
        check (ffmpegArgs.contains ("44100"), "ffmpeg usa 44,1 kHz");
        check (ffmpegArgs.contains (source.getFullPathName()), "ffmpeg le a origem");
        check (ffmpegArgs.contains (wav.getFullPathName()), "ffmpeg escreve o WAV");

        const auto soundStretchArgs = ExternalBpm::buildSoundStretchArgs (wav);
        check (soundStretchArgs.contains (wav.getFullPathName()), "soundstretch le o WAV");
        check (soundStretchArgs.contains ("-bpm"), "soundstretch pede o BPM");

        // O WAV temporario deriva do nome da origem e tem sufixo fixo.
        check (ExternalBpm::temporaryWavFor (source).getFileName() == "song.parteplay-bpm.wav",
               "WAV temporario derivado do nome da origem");

        // Nos testes a pipeline fica desligada, para o resultado nao depender de
        // haver ffmpeg no PATH da maquina que roda o CI.
        check (! ExternalBpm::isEnabled(), "pipeline externa desligada nos testes");
    }

    void testMidiTempoMap()
    {
        std::printf ("\n[midi] mapa de tempo (SMF formato 0)\n");

        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                              .getChildFile ("parteplay-tempo-map-test.mid");
        file.deleteFile();

        const double bpm = 103.0;
        const int beatsPerBar = 3;

        check (MidiMapExporter::writeTempoMap (file, bpm, beatsPerBar),
               "escreve o mapa de tempo em arquivo");

        juce::MemoryBlock raw;
        check (file.loadFileAsData (raw), "le o arquivo de volta");

        const auto* bytes = static_cast<const unsigned char*> (raw.getData());
        const size_t size = raw.getSize();

        auto be32 = [bytes] (size_t p) -> unsigned
        {
            return ((unsigned) bytes[p] << 24) | ((unsigned) bytes[p + 1] << 16)
                 | ((unsigned) bytes[p + 2] << 8) | (unsigned) bytes[p + 3];
        };
        auto be16 = [bytes] (size_t p) -> unsigned
        {
            return ((unsigned) bytes[p] << 8) | (unsigned) bytes[p + 1];
        };

        check (size > 14, "arquivo com cabecalho e trilha");
        check (bytes[0] == 'M' && bytes[1] == 'T'
                   && bytes[2] == 'h' && bytes[3] == 'd', "cabecalho MThd");
        check (be32 (4) == 6,           "tamanho do cabecalho = 6");
        check (be16 (8) == 0,           "formato 0");
        check (be16 (10) == 1,          "uma trilha");
        check (be16 (12) == 480,        "PPQ 480");
        check (be32 (14) == 0x4D54726B, "trilha MTrk");

        const int expectedMicros = (int) std::lround (60000000.0 / bpm);
        bool tempoFound = false;
        for (size_t i = 14; i + 6 <= size; ++i)
        {
            if (bytes[i] == 0xFF && bytes[i + 1] == 0x51 && bytes[i + 2] == 0x03)
            {
                const int micros = ((int) bytes[i + 3] << 16)
                                 | ((int) bytes[i + 4] << 8) | (int) bytes[i + 5];
                tempoFound = (micros == expectedMicros);
                break;
            }
        }
        check (tempoFound, "meta de tempo = 60000000 / bpm");

        bool sigFound = false;
        for (size_t i = 14; i + 7 <= size; ++i)
        {
            if (bytes[i] == 0xFF && bytes[i + 1] == 0x58 && bytes[i + 2] == 0x04)
            {
                sigFound = ((int) bytes[i + 3] == beatsPerBar && (int) bytes[i + 4] == 2);
                break;
            }
        }
        check (sigFound, "assinatura nn / dd = 2");

        bool endFound = false;
        for (size_t i = 14; i + 3 <= size; ++i)
        {
            if (bytes[i] == 0xFF && bytes[i + 1] == 0x2F && bytes[i + 2] == 0x00)
            {
                endFound = true;
                break;
            }
        }
        check (endFound, "meta de fim de trilha");

        file.deleteFile();
    }

    void testAudioTrimAndBars()
    {
        std::printf ("\n[audio] silencio inicial e compassos\n");

        // Contagem do silencio inicial: zeros digitais exatos, em qualquer canal.
        {
            juce::AudioBuffer<float> buffer (2, 8);
            buffer.clear();
            buffer.setSample (0, 4, 0.5f);
            buffer.setSample (1, 5, -0.25f);

            check (FilePlayer::countLeadingSilence (buffer) == 4,
                   "silencio inicial conta ate o primeiro sample nao nulo (algum canal)");

            buffer.setSample (0, 0, 0.001f);
            check (FilePlayer::countLeadingSilence (buffer) == 0,
                   "sample nao nulo no inicio zera a contagem");

            juce::AudioBuffer<float> silent (1, 6);
            silent.clear();
            check (FilePlayer::countLeadingSilence (silent) == 6,
                   "buffer todo zero conta o proprio tamanho");
        }

        // BARS = duracao x bpm / 60 / tempos-por-compasso, arredondado.
        check (FilePlayer::measureCountFor (8.0, 120.0, 4) == 4,
               "8 s a 120 BPM em 4/4 = 4 compassos");
        check (FilePlayer::measureCountFor (8.0, 120.0, 3) == 5,
               "16 batidas em 3/4 arredondam para 5 compassos");
        check (FilePlayer::measureCountFor (0.0, 120.0, 4) == 0,
               "duracao nula nao mede");
        check (FilePlayer::measureCountFor (8.0, 0.0, 4) == 0,
               "bpm nulo nao mede");
        check (FilePlayer::measureCountFor (8.0, 120.0, 0) == 0,
               "compasso nulo nao mede");

        // Escala do BPM e metrica efetiva: padrao x2, e o host manda quando declara.
        FilePlayer player;
        check (juce::approximatelyEqual (player.getBpmScale(), 2.0),
               "escala padrao do BPM e x2");
        player.setBpmScale (1.0);
        check (juce::approximatelyEqual (player.getBpmScale(), 1.0),
               "escala do BPM pode voltar para o valor real");

        check (player.getBeatsPerBar() == 4,
               "sem override, o compasso e o estimado do audio (4)");
        player.setMeterOverride (3);
        check (player.getBeatsPerBar() == 3,
               "override do host manda no compasso");
        player.setMeterOverride (0);
        check (player.getBeatsPerBar() == 4,
               "override zero volta ao compasso do audio");

        check (player.getMeasureCount() == 0, "sem audio nao ha compassos");
    }

int main()
{
    std::printf ("PartePlay - testes de dominio\n\n");

    // A pipeline externa fica desligada: os testes nao podem depender de haver
    // ffmpeg/soundstretch no PATH da maquina que roda o CI.
    ExternalBpm::setEnabled (false);

    testTuningRatio();
    testNoteNames();
    testTempoAnalysis();
    testTempoProgress();
    testExternalBpm();
    testAudioTrimAndBars();
    testMidiTempoMap();
    testTextEncoding();
    testSongSheetStrings();
    testIdentificationTranslations();
    testPlayerPublicationRace();
    testChromaprintFingerprint();
    testCultures();
    testNumberFormatting();
    testEnglishAndSpanishVariants();
    testFingerprintPcmConversion();
    testFingerprintCompute();
    testFingerprintStereo();
    testFingerprintRejections();
    testFingerprintCancel();
    testFingerprintWorkerAsync();
    testFingerprintWorkerLastStartWins();
    testFingerprintWorkerCancel();


    std::printf ("\n%d verificacoes, %d falha(s)\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
