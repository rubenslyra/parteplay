cmake_minimum_required(VERSION 3.22)

# Versao do projeto. Este e o UNICO lugar onde a versao mora: badges, notas de
# release e o formulario do AcoustID derivam daqui (regra de reconciliacao).
project(PartePlay VERSION 0.3.1 LANGUAGES C CXX)

# Versao da JUCE usada na build. Fato de DEPENDENCIA, e nao derivavel da versao
# do projeto: a versao do PartePlay nao tem relacao com a 9.0.2 da JUCE. A CI le
# este numero para clonar a tag certa, entao ele mora aqui e em nenhum outro
# lugar. (Um teste local pegou o erro: derivar major.minor do projeto produzia
# a tag "0.1.0", que nao existe no repositorio da JUCE.)
set(PARTEPLAY_JUCE_VERSION "9.0.2" CACHE STRING
    "Versao da JUCE usada na build local e na CI")

# A JUCE nao e dependencia versionada pelo CMake. A precedencia e
# 1) variavel de ambiente JUCE_ROOT, 2) -DJUCE_ROOT do cache, 3) o default da
# plataforma. O default do Windows e o da instalacao oficial desta maquina e NAO
# existe em Linux/macOS nem em nenhum runner de CI, entao ele so entra como
# ultimo recurso e avisa: um path aceito pelo configure e so falharia depois, no
# meio do build. Um path com barra invertida e espaco e o pior caso, porque o
# shell o quebra em duas partes.
if(DEFINED ENV{JUCE_ROOT} AND NOT "$ENV{JUCE_ROOT}" STREQUAL "")
    set(PartePlayJuceRoot "$ENV{JUCE_ROOT}")
    set(PartePlayJuceRootOrigin "variavel de ambiente JUCE_ROOT")
else()
    set(PartePlayJuceRoot "")
    set(PartePlayJuceRootOrigin "nenhuma")
endif()

if(PartePlayJuceRoot STREQUAL "" AND WIN32)
    set(PartePlayJuceRoot "D:/Program Files/JUCE")
    set(PartePlayJuceRootOrigin "default do Windows desta maquina")
endif()

if(PartePlayJuceRoot STREQUAL "")
    message(FATAL_ERROR
        "JUCE_ROOT nao definido. Passe -DJUCE_ROOT=<caminho para o checkout da JUCE 9>, "
        "ou exporte a variavel de ambiente JUCE_ROOT.")
endif()

set(JUCE_ROOT "${PartePlayJuceRoot}" CACHE PATH
    "Path to the JUCE framework source")

if(NOT EXISTS "${JUCE_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
        "JUCE_ROOT invalido: '${JUCE_ROOT}/CMakeLists.txt' nao existe. "
        "Origem do valor: ${PartePlayJuceRootOrigin}.")
endif()

message(STATUS "JUCE_ROOT: ${JUCE_ROOT} (origem: ${PartePlayJuceRootOrigin})")

if(PartePlayJuceRootOrigin STREQUAL "default do Windows desta maquina")
    message(WARNING
        "JUCE_ROOT veio do default fixo desta maquina. Em CI e em qualquer outra "
        "maquina isso nao existe: exporte JUCE_ROOT ou passe -DJUCE_ROOT.")
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

# O Chromaprint entra como biblioteca ESTATICA (exigencia da LGPL 2.1), mas o
# VST3 no Linux e um shared object. No ELF, codigo de objeto que vai para uma
# .so precisa de -fPIC, e sem ele o link morre com "relocation R_X86_64_PC32
# against symbol ... can not be used when making a shared object". No Windows
# o conceito nao existe e a variavel e ignorada.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory("${CHROMAPRINT_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/chromaprint" EXCLUDE_FROM_ALL)
# Restore: o valor acima nao pode vazar para os alvos do JUCE, que tem as
# propriasPoliticas de PIC.
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

unset(BUILD_TESTS             CACHE)
unset(BUILD_TOOLS             CACHE)
unset(BUILD_SHARED_LIBS       CACHE)
unset(USE_INTERNAL_AVRESAMPLE CACHE)
unset(AUDIO_PROCESSOR_LIB      CACHE)
unset(FFT_LIB                  CACHE)
unset(KISSFFT_ROOT             CACHE)

# Fonte unica da verdade da lista de arquivos. Os dois targets gerados pelo
# juce_add_plugin (PartePlay e PartePlay_VST3) consomem esta mesma lista, para
# que adicionar um arquivo novo nunca precise ser feito em dois lugares.
#
# Source/ esta organizado por modulo, nao por tipo nem por camada:
#
#   Core/      identificadores e matematica pura; nao conhece JUCE nem host
#   Audio/     o caminho do audio: decodificacao, publicacao, pitch, picos
#   Analysis/  o que mede o arquivo (fingerprint, andamento, afinacao)
#   Ui/        apresentacao: i18n e cores
#   Plugin/    a casca VST3, o que o hospedeiro enxerga
#   Export/    MIDI para o DAW
#
# Waveform.h fica em Audio/ e nao em Ui/ porque e dado derivado do audio (pares
# min/max) e nao apresentacao: o FilePlayer o produz, o editor o consome.
#
# Todos os diretorios acima entram no include path, e por isso nenhum include do
# codigo mudou quando a arvore foi criada: sao sempre #include "Arquivo.h", sem
# caminho. O preco e que o preprocessor nao impõe a direcao das dependencias
# entre modulos; a organizacao documenta, e nao restringe. Fixa-la exigiria
# alvos de biblioteca por modulo, o que para 14 arquivos seria arquitetura de
# guarda-chuva a mais do que de codigo.
set(PARTEPLAY_MODULE_DIRS
    Core Audio Analysis Ui Plugin Export
)

set(PARTEPLAY_SOURCES
    Source/Plugin/PluginProcessor.cpp
    Source/Plugin/PluginEditor.cpp
    Source/Audio/FilePlayer.cpp
    Source/Analysis/FingerprintWorker.cpp
    Source/Analysis/ExternalBpm.cpp
    Source/Export/MidiMapExporter.cpp
    Source/Audio/PitchShifter.cpp
    Source/Ui/Text.cpp
    Source/Analysis/TempoAnalyser.cpp
    Source/Core/Tuning.cpp
)

# Os mesmos diretorios, mas como caminhos absolutos: target_sources() precisa de
# caminho relativo ao fonte, target_include_directories() precisa de caminho
# utilizavel no compilador. Montar os dois da mesma lista e o que impede os dois
# de divergirem.
set(PARTEPLAY_MODULE_PATHS)
foreach (dir IN LISTS PARTEPLAY_MODULE_DIRS)
    list(APPEND PARTEPLAY_MODULE_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Source/${dir}")
endforeach()

# ------------------------------------------------------------------------------
# Warning como erro, so nos fontes deste repositorio.
#
# Por que: a sessao da 0.2.2 terminou com 9 warnings no build, todos um unico
# C4244 dentro do Chromaprint, e um deles apontava um bug real - a conversao de
# PCM do fingerprint perdia escala. O build "passava" com o bug dentro. Sem
# promociao, a mesma classe de defeito volta em silencio e ninguem ve.
#
# Por que nao e exatamente "liga /WX": o nivel de warning ja vem do toolchain
# (juce::juce_recommended_warning_flags liga /W4 no MSVC e -Wall no GCC/Clang).
# Esta funcao nao escolhe o nivel, ela promove a erro o que ja era reportado -
# que e o que /WX faz no MSVC e o equivalente mais honesto nos outros dois.
#
# Por que os headers de terceiros ficam SYSTEM: o compilador passa a classificar
# o que acha por /external:I como externo e nao o submete ao nosso nivel. Os 9
# C4244 sao do Chromaprint e nao sao nosso para corrigir; silenciar warning de
# terceiro e o que permite exigir /WX no resto sem produzir ruido que ninguem
# pode resolver.
# ------------------------------------------------------------------------------
function(parteplay_warnings_as_errors target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Werror)
    endif()
endfunction()

juce_add_plugin(PartePlay
    # Derivado do PROJECT_VERSION, nunca escrito a mao: manter o numero escrito
    # aqui alem do project() era a mesma duplicacao que a regra de reconciliacao
    # proibe, e o numero da release e o do bundle sao coisas que nao podem
    # divergir em silencio.
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "PartePlay"
    PRODUCT_NAME "PartePlay"
    PLUGIN_NAME "PartePlay"
    PLUGIN_MANUFACTURER_CODE "PRPL"
    PLUGIN_CODE "PTPY"
    # Descreve so o que existe. A ficha da cancao (ISRC, ano) nao esta neste
    # codigo: ate la, anunciar recurso inexistente no bundle e o que faz um
    # usuario reportar bug em funcao que nunca foi entregue.
    DESCRIPTION "Reproducao de referencia em sincronia com o transporte, com velocidade de treino, loop por compassos e fingerprint acustico local"
    FORMATS VST3 Standalone
    VST3_CATEGORIES "Fx|Utilities"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_SYNTH FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/PartePlay-Icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/PartePlay-Icon.png"
)

# Fix para Windows: o target VST3 (PartePlay_VST3) precisa das fontes e linkar a lib estatica
if(WIN32)
    # O juce_add_plugin cria dois targets: PartePlay (static lib) e PartePlay_VST3 (DLL)
    # Garantir que as fontes vao para o target VST3 tambem
    target_sources(PartePlay_VST3 PRIVATE ${PARTEPLAY_SOURCES})
    target_link_libraries(PartePlay_VST3 PRIVATE PartePlay juce::juce_audio_formats juce::juce_dsp chromaprint BinaryData)
    target_include_directories(PartePlay_VST3 PRIVATE ${PARTEPLAY_MODULE_PATHS} "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_BinaryData/JuceLibraryCode")
    # SYSTEM: header de terceiro nao e julgado pelo nosso nivel de warning.
    target_include_directories(PartePlay_VST3 SYSTEM PRIVATE "${CHROMAPRINT_ROOT}/src")
    # A versao mostrada na interface vem daqui, nunca de um literal no .cpp: e a
    # CI que reprova X.Y.Z escrito em Source/, entao a macro e o unico caminho
    # limpo entre o project(...VERSION) e o rodape do editor.
    target_compile_definitions(PartePlay_VST3 PRIVATE CHROMAPRINT_NODLL=1 JUCE_VST3_CAN_REPLACE_VST2=0
                                                PARTEPLAY_VERSION="${PROJECT_VERSION}")
    target_compile_definitions(PartePlay PRIVATE PARTEPLAY_VERSION="${PROJECT_VERSION}")
    parteplay_warnings_as_errors(PartePlay_VST3)
    parteplay_warnings_as_errors(PartePlay)
    
    # Nao mexa em MSVC_LINK_OBJECTS achando que o VST3 nao linka. O CMake
    # escreve <LinkObjects>false</LinkObjects> no .vcxproj e isso parece a causa
    # do bug, mas o bloco onde ele aparece e um <CustomBuild> (a geracao do
    # JuceHeader.h pelo juceaide), nao o tool Link -- e la false esta correto,
    # porque um passo de codegen nao linka objeto nenhum. O que de fato faltava
    # era este bloco: sem ele o target do VST3 recebia so os fontes do
    # juce_audio_plugin_client, nenhum codigo do plugin, e o build terminava sem
    # erro e sem binario.
endif()

target_sources(PartePlay PRIVATE ${PARTEPLAY_SOURCES})

# O wrapper Standalone se recusa a compilar sem os modulos de dispositivo de audio
# e de utilitarios (juce_audio_plugin_client_Standalone.cpp tem um #error pedindo
# os dois). Vinculados SO ao alvo do Standalone, e nao ao codigo compartilhado:
# o VST3 nao usa AudioDeviceManager, e linkar aqui evita arrastar gui_extra e
# audio_devices para dentro do bundle .vst3.
if(TARGET PartePlay_Standalone)
    target_link_libraries(PartePlay_Standalone PRIVATE
        juce::juce_audio_devices
        juce::juce_audio_utils)
endif()

target_link_libraries(PartePlay PRIVATE juce::juce_audio_formats juce::juce_dsp chromaprint)

juce_add_binary_data(BinaryData SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/PartePlay-Icon.png")
target_link_libraries(PartePlay PRIVATE BinaryData)

target_include_directories(PartePlay PRIVATE ${PARTEPLAY_MODULE_PATHS} "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_BinaryData/JuceLibraryCode")

# O target chromaprint usa include_directories() de diretorio, e nao
# INTERFACE_INCLUDE_DIRECTORIES, entao vincular a lib nao propaga o header.
# Usamos src/ (header real) em vez de src/include/ (symlink) para nao
# depender de suporte a symlink na extracao. SYSTEM: sao 9 C4244 dentro do
# Chromaprint, e nenhum deles e nosso para corrigir.
target_include_directories(PartePlay SYSTEM PRIVATE "${CHROMAPRINT_ROOT}/src")

# Sem isso o header se declara __declspec(dllimport) e o link estatico falha
# com LNK2001 nos simbolos __imp_chromaprint_*. Ver chromaprint.h.
target_compile_definitions(PartePlay PRIVATE CHROMAPRINT_NODLL=1)

target_compile_definitions(PartePlay PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)

parteplay_warnings_as_errors(PartePlay)

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
    Source/Audio/FilePlayer.cpp
    Source/Analysis/FingerprintWorker.cpp
    Source/Analysis/ExternalBpm.cpp
    Source/Analysis/TempoAnalyser.cpp
    Source/Export/MidiMapExporter.cpp
    Source/Ui/Text.cpp
    Source/Core/Tuning.cpp
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
target_include_directories(PartePlayTests PRIVATE ${PARTEPLAY_MODULE_PATHS})
target_include_directories(PartePlayTests SYSTEM PRIVATE "${CHROMAPRINT_ROOT}/src")
target_compile_definitions(PartePlayTests PRIVATE CHROMAPRINT_NODLL=1)
target_compile_features(PartePlayTests PRIVATE cxx_std_17)
target_compile_definitions(PartePlayTests PUBLIC JUCE_STANDALONE_APPLICATION=1)

parteplay_warnings_as_errors(PartePlayTests)

juce_generate_juce_header(PartePlayTests)

# ---------------------------------------------------------------------------
# Harness de carga do VST3 (epic/vst3-load-harness-ci, adr/0001).
#
# Alvo separado do PartePlayTests e, sobretudo, sem PARTEPLAY_SOURCES: o
# harness carrega o bundle .vst3 como dado externo, e linkar o codigo do plugin
# aqui faria o teste provar a copia compilada neste executavel em vez do
# artefato que o usuario baixa.
#
# Depende so de juce_audio_processors_headless, que declara apenas
# juce_audio_basics e juce_events: nenhum modulo de GUI entra no link, que e o
# que permite rodar isto num agente de CI sem display.
juce_add_console_app(PartePlayHarness PRODUCT_NAME "PartePlayHarness")

target_sources(PartePlayHarness PRIVATE
    Tests/LoadHarness.cpp
)

target_link_libraries(PartePlayHarness PRIVATE
    juce::juce_audio_processors_headless
    juce::juce_audio_basics
    juce::juce_core
    juce::juce_events
)

# O modulo headless injeta JUCE_PLUGINHOST_LADSPA=1 e JUCE_PLUGINHOST_LV2=1 por
# via INTERFACE, e no Linux os dois precisam de cabecalhos que o runner nao tem:
# com LADSPA ligado o build morre em `#include <ladspa.h>`, que derrubou Ubuntu e
# macOS em 02/10.
#
# Nao da para vencer por `target_compile_definitions(...=0)`. Medido: o define da
# interface vai DEPOIS na linha de comando e o compilador aplica o ultimo, entao
# -DFLAVOR=0 -DFLAVOR=1 resolve em FLAVOR=1. Remover o item da propriedade e o
# que funciona, e e o que JUCE le: o cabecalho do modulo tem
# `#ifndef JUCE_PLUGINHOST_LADSPA / #define ... 0`, entao ausente significa
# desabilitado.
get_target_property(_harness_host_defs juce_audio_processors_headless INTERFACE_COMPILE_DEFINITIONS)
if (NOT _harness_host_defs)
    message(FATAL_ERROR
        "juce_audio_processors_headless nao expoe INTERFACE_COMPILE_DEFINITIONS. "
        "A JUCE mudou a forma de habilitar os plugin hosts; o harness deixaria de "
        "ser VST3-only sem ninguem perceber.")
endif()
list(REMOVE_ITEM _harness_host_defs JUCE_PLUGINHOST_LADSPA=1 JUCE_PLUGINHOST_LV2=1)
set_target_properties(juce_audio_processors_headless
    PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${_harness_host_defs}")

foreach (_still_on JUCE_PLUGINHOST_LADSPA JUCE_PLUGINHOST_LV2)
    list(FIND _harness_host_defs "${_still_on}=1" _found)
    if (NOT _found EQUAL -1)
        message(FATAL_ERROR
            "${_still_on}=1 continua na interface do modulo apos a remocao. "
            "O harness so pode linkar o VST3 SDK.")
    endif()
endforeach()

# Header do modulo tem `#ifndef` nesses tres: quem nao define fica em 0. O VST3
# e 1 explicito porque e o unico que o harness abre.
target_compile_definitions(PartePlayHarness PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_STANDALONE_APPLICATION=1
)

target_compile_features(PartePlayHarness PRIVATE cxx_std_17)

parteplay_warnings_as_errors(PartePlayHarness)

juce_generate_juce_header(PartePlayHarness)

# O harness NAO e registrado em CTest, e uma decisao, nao uma omissao: o
# bundle .vst3 so existe depois da etapa de empacotamento do CI, portanto nao
# ha caminho para resolver no configure time. Ele roda como passo da workflow
# logo apos o empacotamento, recebendo o bundle pelo argv - que e a T9.
# Registrar aqui exigiria um caminho falso e o harness passaria a verde sem
# nunca ter carregado nada.

include(CTest)

add_test(NAME domain COMMAND PartePlayTests)
set_tests_properties(domain PROPERTIES TIMEOUT 120)