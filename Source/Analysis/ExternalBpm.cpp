#include "ExternalBpm.h"

#include <atomic>

namespace ExternalBpm
{
    namespace
    {
        std::atomic<bool> pipelineEnabled { true };

        // Nomes fixos no Windows. Estes binarios so sao empacotados para Windows;
        // em outra plataforma a pipeline fica indisponivel e o nativo assume.
       #if JUCE_WINDOWS
        const char* const ffmpegName = "ffmpeg.exe";
        const char* const soundstretchName = "soundstretch.exe";
       #else
        const char* const ffmpegName = "ffmpeg";
        const char* const soundstretchName = "soundstretch";
       #endif

        bool directoryHasTools (const juce::File& dir, Tools& out)
        {
            if (dir == juce::File())
                return false;

            const juce::File ffmpeg (dir.getChildFile (ffmpegName));
            const juce::File soundstretch (dir.getChildFile (soundstretchName));

            if (ffmpeg.existsAsFile() && soundstretch.existsAsFile())
            {
                out = { ffmpeg, soundstretch };
                return true;
            }

            return false;
        }

        juce::File findOnPath (const char* const exeName)
        {
            const auto path = juce::SystemStats::getEnvironmentVariable ("PATH", {});

            juce::StringArray dirs;
            dirs.addTokens (path, ";", "\"");

            for (const auto& dir : dirs)
            {
                if (dir.trim().isEmpty())
                    continue;

                const juce::File candidate (juce::File (dir.trim()).getChildFile (exeName));
                if (candidate.existsAsFile())
                    return candidate;
            }

            return {};
        }
    }

    bool Tools::valid() const noexcept
    {
        return ffmpeg.existsAsFile() && soundstretch.existsAsFile();
    }

    void setEnabled (bool enabled) noexcept
    {
        pipelineEnabled.store (enabled, std::memory_order_relaxed);
    }

    bool isEnabled() noexcept
    {
        return pipelineEnabled.load (std::memory_order_relaxed);
    }

    Tools locateTools()
    {
        Tools tools;

        const auto envDir = juce::SystemStats::getEnvironmentVariable ("PARTEPLAY_TOOLS_DIR", {});
        if (envDir.isNotEmpty() && directoryHasTools (juce::File (envDir), tools))
            return tools;

        const auto exeDir = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                                .getParentDirectory();
        if (directoryHasTools (exeDir.getChildFile ("resources").getChildFile ("bin"), tools))
            return tools;

        const auto userDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                 .getChildFile ("PartePlay").getChildFile ("bin");
        if (directoryHasTools (userDir, tools))
            return tools;

        const juce::File ffmpeg (findOnPath (ffmpegName));
        const juce::File soundstretch (findOnPath (soundstretchName));

        if (ffmpeg.existsAsFile() && soundstretch.existsAsFile())
            tools = { ffmpeg, soundstretch };

        return tools;
    }

    juce::File temporaryWavFor (const juce::File& source)
    {
        const auto stem = source.getFileNameWithoutExtension();
        const auto name = (stem.isNotEmpty() ? stem : juce::String ("audio")) + ".parteplay-bpm.wav";

        return juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (name);
    }

    juce::StringArray buildFfmpegArgs (const juce::File& source, const juce::File& wav)
    {
        return { "-hide_banner", "-loglevel", "error", "-y",
                 "-i", source.getFullPathName(),
                 "-ac", "1", "-ar", "44100",
                 wav.getFullPathName() };
    }

    juce::StringArray buildSoundStretchArgs (const juce::File& wav)
    {
        return { wav.getFullPathName(), "-bpm" };
    }

    double parseBpm (const juce::String& output)
    {
        juce::StringArray lines;
        lines.addLines (output);

        for (const auto& line : lines)
        {
            if (! line.containsIgnoreCase ("bpm"))
                continue;

            // Primeiro numero decimal que aparece na linha. O soundstretch escreve
            // o valor depois da palavra (por exemplo, "Detected BPM rate 120.0"),
            // entao varrer da esquerda para a direita pega o valor e ignora o
            // rotulo. Um "-" inicial nao e sinal de BPM negativo: e ignorado.
            juce::String number;
            bool started = false;

            for (const auto c : line)
            {
                if (c >= '0' && c <= '9')
                {
                    number << c;
                    started = true;
                }
                else if (c == '.' && started)
                {
                    number << c;
                }
                else if (started)
                {
                    break;
                }
            }

            if (number.isNotEmpty())
                return number.getDoubleValue();
        }

        return 0.0;
    }

    double estimateBpm (const juce::File& source, const Tools& tools, const ProgressFn& onProgress)
    {
        if (! isEnabled() || ! tools.valid() || ! source.existsAsFile())
            return 0.0;

        const auto report = [&onProgress] (float fraction)
        {
            if (onProgress)
                onProgress (juce::jlimit (0.0f, 1.0f, fraction));
        };

        const juce::File wav (temporaryWavFor (source));

        // O WAV e intermediario e some sempre, tenha o processo dado certo ou nao.
        struct WavCleanup
        {
            juce::File file;
            ~WavCleanup() { if (file.existsAsFile()) file.deleteFile(); }
        } cleanup { wav };

        report (0.0f);

        {
            juce::StringArray command;
            command.add (tools.ffmpeg.getFullPathName());
            command.addArray (buildFfmpegArgs (source, wav));

            juce::ChildProcess process;

            if (! process.start (command, juce::ChildProcess::wantStdOut | juce::ChildProcess::wantStdErr))
                return 0.0;

            process.readAllProcessOutput();

            if (! process.waitForProcessToFinish (120000) || process.getExitCode() != 0)
                return 0.0;
        }

        if (! wav.existsAsFile())
            return 0.0;

        report (0.6f);

        juce::String output;
        {
            juce::StringArray command;
            command.add (tools.soundstretch.getFullPathName());
            command.addArray (buildSoundStretchArgs (wav));

            juce::ChildProcess process;

            if (! process.start (command, juce::ChildProcess::wantStdOut | juce::ChildProcess::wantStdErr))
                return 0.0;

            output = process.readAllProcessOutput();

            if (! process.waitForProcessToFinish (120000))
                return 0.0;
        }

        report (1.0f);

        return parseBpm (output);
    }
}
