cmake_minimum_required(VERSION 3.22)

# Versao do projeto. Este e o UNICO lugar onde a versao mora: badges, notas de
# release e o formulario do AcoustID derivam daqui (regra de reconciliacao).
project(PartePlay VERSION 0.1.0 LANGUAGES C CXX)

# Versao da JUCE usada na build. Fato de DEPENDENCIA, e nao derivavel da versao
# do projeto: a 0.1.0 do PartePlay nao tem relacao com a 9.0.2 da JUCE. A CI le
# este numero para clonar a tag certa, entao ele mora aqui e em nenhum outro
# lugar. (Um teste local pegou o erro: derivar major.minor do projeto produzia
# a tag "0.1.0", que nao existe no repositorio da JUCE.)
set(PARTEPLAY_JUCE_VERSION "9.0.2" CACHE STRING
    "Versao da JUCE usada na build local e na CI")

# A JUCE nao e dependencia versionada pelo CMake. O caminho default e o da
# instalacao oficial desta maquina e NAO funciona em Linux/macOS, onde a CI
# passa -DJUCE_ROOT=<checkout>. Por isso o default e sobrescrito por condicao
# de plataforma: um path com barra invertida e espaco seria aceito pelo
# configure e so falharia depois, no meio do build.
if(WIN32)
    set(PartePlayDefaultJuceRoot "D:/Program Files/JUCE")
else()
    set(PartePlayDefaultJuceRoot "$ENV{JUCE_ROOT}")
endif()

if(NOT PartePlayDefaultJuceRoot)
    message(FATAL_ERROR
        "JUCE_ROOT nao definido. Passe -DJUCE_ROOT=<caminho para o checkout da JUCE 9>, "
        "ou exporte a variavel de ambiente JUCE_ROOT.")
endif()

set(JUCE_ROOT "${PartePlayDefaultJuceRoot}" CACHE PATH
    "Path to the JUCE framework source")

if(NOT EXISTS "${JUCE_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
        "JUCE_ROOT invalido: '${JUCE_ROOT}/CMakeLists.txt' nao existe.")
endif()

add_subdirectory("${JUCE_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/juce" EXCLUDE_FROM_ALL)

# ------------------------------------------------------------------------------
# Chromaprint 1.6.1 (LGPL 2.1) — geracao de fingerprint acustico.
#
# O tarball upstream e extraido no configure-time em vez de ser commitado
# descriptografado: o historico git fica limpo e a fonte original segue
# intacta, o que e justamente o que a LGPL 2.1 exige para link estatico.
#
# Nao ha dependencia externa de FFT nem de ffmpeg: o KissFFT vem em
# src/3rdparty e o avresample interno (ffmpeg) cobre a conversao de audio.

set(CHROMAPRINT_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/Chromaprint-Dependences/chromaprint-1.6.1.tar.gz")
set(CHROMAPRINT_ROOT   "${CMAKE_CURRENT_BINARY_DIR}/chromaprint-src/chromaprint-1.6.1")

if(NOT EXISTS "${CHROMAPRINT_ARCHIVE}")
    message(FATAL_ERROR
        "Chromaprint ausente: ${CHROMAPRINT_ARCHIVE}\n"
        "Baixe o tarball oficial em https://github.com/acoustid/chromaprint/releases")
endif()

if(NOT EXISTS "${CHROMAPRINT_ROOT}/CMakeLists.txt")
    message(STATUS "Extraindo Chromaprint de ${CHROMAPRINT_ARCHIVE}")
    file(ARCHIVE_EXTRACT INPUT "${CHROMAPRINT_ARCHIVE}"
                         DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/chromaprint-src")
endif()

# Estas precisam entrar como CACHE, e nao como variaveis de escopo normal: o
# Chromaprint declara cmake_minimum_required(3.10), o queixa o escopo dele para
# CMP0077 OLD, e nessa politica option() descarta a variavel normal e escreve o
# default no cache. Sem CACHE, BUILD_TESTS voltaria ON (puxando o googletest
# inteiro) e BUILD_SHARED_LIBS voltaria ON (gerando DLL em vez de lib estatica,
# o que quebraria o link estatico exigido pela LGPL). As entradas sao removidas
# em seguida para que o JUCE nunca as veja.
set(BUILD_TESTS             OFF CACHE INTERNAL "" FORCE)
set(BUILD_TOOLS             OFF CACHE INTERNAL "" FORCE)
set(BUILD_SHARED_LIBS       OFF CACHE INTERNAL "" FORCE)
set(USE_INTERNAL_AVRESAMPLE ON  CACHE INTERNAL "" FORCE)
set(AUDIO_PROCESSOR_LIB      avresample CACHE STRING "Conversao de audio do Chromaprint" FORCE)
set(FFT_LIB                  kissfft    CACHE STRING "Backend de FFT do Chromaprint" FORCE)

# FindKissFFT.cmake so procura ${CMAKE_SOURCE_DIR}/src/3rdparty/kissfft com
# NO_DEFAULT_PATH. Como o Chromaprint aqui e subdiretorio, o CMAKE_SOURCE_DIR
# aponta para a raiz do PartePlay e o caminho bundled nunca e encontrado —
# a busca so passa com KISSFFT_ROOT explicito.
set(KISSFFT_ROOT "${CHROMAPRINT_ROOT}/src/3rdparty/kissfft" CACHE PATH "KissFFT embutido" FORCE)

add_subdirectory("${CHROMAPRINT_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/chromaprint" EXCLUDE_FROM_ALL)

unset(BUILD_TESTS             CACHE)
unset(BUILD_TOOLS             CACHE)
unset(BUILD_SHARED_LIBS       CACHE)
unset(USE_INTERNAL_AVRESAMPLE CACHE)
unset(AUDIO_PROCESSOR_LIB      CACHE)
unset(FFT_LIB                  CACHE)
unset(KISSFFT_ROOT             CACHE)

juce_add_plugin(PartePlay
    VERSION "0.1.0"
    COMPANY_NAME "PartePlay"
    PRODUCT_NAME "PartePlay"
    PLUGIN_NAME "PartePlay"
    PLUGIN_MANUFACTURER_CODE "PRPL"
    PLUGIN_CODE "PTPY"
    DESCRIPTION "Ficha da cancao (ISRC, ano, BPM) + reproducao sincronizada ao transport"
    FORMATS VST3
    VST3_CATEGORIES "Fx|Utilities"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_SYNTH FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/PartePlay-Icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/PartePlay-Icon.png"
)

target_sources(PartePlay PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/FilePlayer.cpp
    Source/FingerprintWorker.cpp
    Source/MidiMapExporter.cpp
    Source/PitchShifter.cpp
    Source/Text.cpp
    Source/TempoAnalyser.cpp
    Source/Tuning.cpp
)

target_link_libraries(PartePlay PRIVATE juce::juce_audio_formats juce::juce_dsp chromaprint)

juce_add_binary_data(BinaryData SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/PartePlay-Icon.png")
target_link_libraries(PartePlay PRIVATE BinaryData)

target_include_directories(PartePlay PRIVATE Source)

# O target chromaprint usa include_directories() de diretorio, e nao
# INTERFACE_INCLUDE_DIRECTORIES, entao vincular a lib nao propaga o header.
# Usamos src/ (header real) em vez de src/include/ (symlink) para nao
# depender de suporte a symlink na extracao.
target_include_directories(PartePlay PRIVATE "${CHROMAPRINT_ROOT}/src")

# Sem isso o header se declara __declspec(dllimport) e o link estatico falha
# com LNK2001 nos simbolos __imp_chromaprint_*. Ver chromaprint.h.
target_compile_definitions(PartePlay PRIVATE CHROMAPRINT_NODLL=1)

target_compile_definitions(PartePlay PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)

if(MSVC)
    target_compile_options(PartePlay PRIVATE /utf-8)
endif()

juce_generate_juce_header(PartePlay)

# ------------------------------------------------------------------------------
# Testes de dominio (sem UI): travam as invariantes de encoding/i18n, a matematica
# de afinacao e o contrato de concorrencia do player. O FilePlayer entra aqui de
# proposito: a publicacao por atomic_load/shared_ptr so tem teste se ha audio de
# verdade sendo trocado de buffer enquanto o fillOutput roda.
# ------------------------------------------------------------------------------

juce_add_console_app(PartePlayTests PRODUCT_NAME "PartePlayTests")

target_sources(PartePlayTests PRIVATE
    Tests/DomainTests.cpp
    Source/FilePlayer.cpp
    Source/FingerprintWorker.cpp
    Source/TempoAnalyser.cpp
    Source/Text.cpp
    Source/Tuning.cpp
)

target_link_libraries(PartePlayTests PRIVATE
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_core
    juce::juce_dsp
    juce::juce_events
    chromaprint
)
target_include_directories(PartePlayTests PRIVATE Source)
target_include_directories(PartePlayTests PRIVATE "${CHROMAPRINT_ROOT}/src")
target_compile_definitions(PartePlayTests PRIVATE CHROMAPRINT_NODLL=1)
target_compile_features(PartePlayTests PRIVATE cxx_std_17)
target_compile_definitions(PartePlayTests PUBLIC JUCE_STANDALONE_APPLICATION=1)

if(MSVC)
    target_compile_options(PartePlayTests PRIVATE /utf-8)
endif()

juce_generate_juce_header(PartePlayTests)

include(CTest)

add_test(NAME domain COMMAND PartePlayTests)
set_tests_properties(domain PROPERTIES TIMEOUT 120)