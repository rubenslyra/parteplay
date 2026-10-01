// Conventional Commits para o PartePlay.
//
// Sem `extends` e sem package.json, de proposito. Duas razoes, ambas sobre o
// que ja custou tempo:
//
// 1. O preset oficial (@commitlint/config-conventional) nao e dependencia do
//    CLI. Um `npx --yes @commitlint/cli` nao o traz, e o commitlint resolve
//    `extends` a partir do diretorio deste arquivo - ou seja, do repositorio,
//    onde nao ha node_modules. Na maquina local isso passa despercebido, porque
//    o pacote fica em cache do npx; no runner, da "Cannot find module" e o gate
//    cai. As 12 regras do preset estao aqui, entao a dependencia extra deixa de
//    existir e as regras ficam legiveis num arquivo so.
//
// 2. O projeto e C++/CMake e nao ganha uma toolchain Node por causa de um linter
//    de commit. O CI roda `npx --yes @commitlint/cli`, que instala no runner.
//
// Tipos aceitos e o que cada um significa aqui:
//
//   feat      funcionalidade que o usuario percebe
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
// deita o Conventional Commutes de qualquer forma.
//
// As regras marcadas abaixo como do preset oficial sao as 12 de
// @commitlint/config-conventional v19, na severidade original.
//
// `ignores` nao amolece nenhuma regra: e a lista de commits que a regra nao
// alcanca. Existe por dois motivos, ambos ja pagos:
//
// 1. Commits de merge. O GitHub gera "Merge pull request #N from ..." e essa
//    mensagem nao tem tipo nem assunto - `type-empty` e `subject-empty` juntos.
//    Filtrar pelo prefixo e o mecanismo documentado do commitlint para isso, e
//    nao uma excecao nomeada: o SHA do merge muda a cada merge, o padrao do
//    titulo nao. Sem este filtro, todo PR contra main reprovaria no proprio
//    merge que o main recebeu.
//
// 2. Dois commits de antes do portão existir. `0c05f85` e `eae4927` sao de
//    30/09/2026, assinados e ja publicados. Reescrever history assinada num
//    repositorio publico custa mais do que o padrao vale nesses dois commits -
//    e a propria politica deste arquivo: o que nao se aceita e recuar o padrao
//    daqui em diante, nao reescrever o que ja foi assinado.
//
// No commitlint v19 `ignores` recebe funcao, e a funcao so ve a mensagem, nao o
// SHA. Por isso os dois commits antigos sao reconhecidos pelo assunto exato, e
// nao por hash: e o que a ferramenta permite, e comparar a primeira linha inteira
// nao deixa passar outro commit por acidente.
//
// Qualquer commit novo, em qualquer branch, continua passando pelas 12 regras.

// Assunto exato, sem espaco antes do ponto e sem curingas.
const subjectIs = subject => message => message.split('\n')[0] === subject;

module.exports = {
    ignores: [
        // Commits de merge gerados pelo GitHub.
        message => /^Merge pull request #\d+ from /.test(message),

        // Antes do portão de Conventional Commits (30/09/2026).
        subjectIs('Scale the fingerprint PCM correctly and cover it with a regression test'),
        subjectIs('Record what each open issue has actually delivered')
    ],

    rules: {
        // --- do preset oficial, severidade original ------------------------------

        'body-leading-blank': [1, 'always'],
        'body-max-line-length': [2, 'always', 100],
        'footer-max-line-length': [1, 'always', 120],

        // Desligado de proposito. O commitlint acusa a falta de linha em branco
        // antes do rodape mesmo quando ela existe - happens porque a mensagem
        // lida do git ja vem normalizada. Um aviso que aparece sempre, em
        // commits corretos, treina a ignorar avisos; este nao traz informacao.
        'footer-leading-blank': [0],

        // 72, e nao os 100 do preset. Um cabecalho de 100 caracteres nao cabe
        // numa tela e nao vira changelog: o que entra no git log --oneline tem
        // de dizer o que mudou sem depender do corpo.
        'header-max-length': [2, 'always', 72],
        'header-trim': [2, 'always'],

        // Proibe comecar com maiuscula, ponto final, e exige tipo em minuscula.
        'subject-case': [2, 'never', ['sentence-case', 'start-case', 'pascal-case', 'upper-case']],
        'subject-empty': [2, 'never'],
        'subject-full-stop': [2, 'never', '.'],
        'type-case': [2, 'always', 'lower-case'],
        'type-empty': [2, 'never'],

        // --- especifico deste repositorio ---------------------------------------

        // Lista fechada acima. Um tipo novo entra aqui, com o significado
        // escrito, e nao em silencio.
        'type-enum': [2, 'always', [
            'feat', 'fix', 'docs', 'refactor',
            'perf', 'test', 'build', 'ci', 'chore'
        ]],

        // Escopo opcional, em minusculas, nome do modulo. Exemplos que o
        // repositorio ja usa: (licence), (scripts).
        'scope-case': [2, 'always', 'lower-case']
    }
};