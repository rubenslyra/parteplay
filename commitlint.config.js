// Conventional Commits para o PartePlay.
//
// Sem package.json de proposito: o projeto e C++/CMake e nao ganha uma toolchain
// Node por causa de um linter de commit. O CI roda `npx --yes @commitlint/cli`,
// que instala em cache no runner. Para rodar local, o mesmo comando serve.
//
// Tipos aceitos e o que cada um significa aqui:
//
//   feat      functionality que o usuario percebe
//   fix       correcao de defeito
//   docs      README, CHANGELOG, docs/, CONTRIBUTING
//   refactor  mudanca interna sem alterar comportamento observavel
//   perf      desempenho
//   test      suite de testes e infra de teste
//   build     CMakeLists, CMakePresets, scripts de build
//   ci        workflows e gates
//   chore     manutencao que nao entra em nenhuma categoria acima
//
// `style` e `revert` ficam de fora de proposito: formatacao isolada nao passa
// pelo code review deste repositorio (o .editorconfig resolve), e um revert
// automatico deita o Conventional Commutes de qualquer forma.

module.exports = {
    extends: ['@commitlint/config-conventional'],

    rules: {
        // Lista fechada acima. Um tipo novo entra aqui, com o significado
        // escrito, e nao em silencio.
        'type-enum': [2, 'always', [
            'feat', 'fix', 'docs', 'refactor',
            'perf', 'test', 'build', 'ci', 'chore'
        ]],

        // Escopo opcional, em minusculas, nome do modulo. Exemplos que o
        // repositorio ja usa: (licence), (scripts).
        'scope-case': [2, 'always', 'lower-case'],

        // Cabecalho curto: o que muda, nao o porque. O porque e do corpo.
        'header-max-length': [2, 'always', 72],
        'body-max-line-length': [2, 'always', 100],

        // Nao se exige que o corpo exista: commit de uma linha e valido. O que
        // se exige e que, quando ele existe, explique o porque.
        'body-leading-blank': [2, 'always'],

        // Assinatura de commit verificada e gate da main; manter a regra de que
        // todo commit tem Signed-off-by nao e coisa do commitlint, e do hook de
        // pre-push e do CI.
        'footer-max-line-length': [1, 'always', 120]
    }
};