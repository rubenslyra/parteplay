#!/usr/bin/env python3
"""Gate local: nenhum script bash da CI pode usar builtin que o macOS nao tem.

O runner de macOS usa o bash 3.2 da Apple. Os outros usam bash 5. Todo builtin
introduzido depois do 3.2 funciona nos tres menos no macOS, e o sintoma e sempre
o mesmo e nuncaauto-explicativo: `mapfile: command not found` e exit 127, sem
dizer qual linha do YAML causou. O Ubuntu passa, entao so um job de quatro
reprova, e a primeira hipotese quase nunca e a certa - ja foi "o caminho do
executavel esta errado", quando o caminho estava certo e o shell era que nao.

Este gate existe para pegar a classe, nao o caso. Roda em segundos e nao precisa
de GitHub.

Alvo: todo passo com shell POSIX dentro de .github/workflows/*.yml e
.github/actions/**/action.yml, porque qualquer um deles pode rodar no macOS.

Uso:
    python scripts/check-ci-shell.py            # confere o repo
    python scripts/check-ci-shell.py --self-test # confere o proprio gate
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

# (padrao, versao minima do bash, sintoma no 3.2, modo).
#
# modo "comando": so e uso se estiver em posicao de comando, porque e assim que
# um builtin e invocado. Marcar a palavra solta pegaria texto.
# modo "sempre": a construcao nao e um comando, e um operando. `${var^^}`
# dentro de um `echo` ainda e bash 4, e nao ha como o macOS executar.
#
# So entram aqui os que tememcontrado documentado como builtin do bash 4 ou
# superior. A lista e curta de proposito: um item errado nesta tabela produz um
# gate que reprova codigo correto, e um gate que reprova codigo correto e
# desativado em uma semana.
REGRAS: "tuple[tuple[re.Pattern[str], str, str, str], ...]" = (
    (re.compile(r"\bmapfile\b"), "4.0", "command not found", "comando"),
    (re.compile(r"\breadarray\b"), "4.0", "command not found", "comando"),
    (re.compile(r"\b(?:declare|typeset|local)\s+-[A-Za-z]*A"), "4.0", "opcao invalida", "comando"),
    (re.compile(r"\b(?:declare|typeset|local)\s+-[A-Za-z]*g\b"), "4.2", "opcao invalida", "comando"),
    (re.compile(r"\b(?:declare|typeset|local)\s+-n\b"), "4.3", "opcao invalida", "comando"),
    (re.compile(r"\bcoproc\b"), "4.0", "reserved word", "comando"),
    (re.compile(r"\bglobstar\b"), "4.0", "shopt: opcao invalida", "sempre"),
    (re.compile(r"&>>"), "4.0", "redirecionamento invalido", "sempre"),
    (re.compile(r"\bwait\s+-n\b"), "4.3", "opcao invalida", "sempre"),
    (re.compile(r"\$\{[A-Za-z_][A-Za-z0-9_]*\^\^"), "4.0", "bad substitution", "sempre"),
    (re.compile(r"\$\{[A-Za-z_][A-Za-z0-9_]*,,"), "4.0", "bad substitution", "sempre"),
    (re.compile(r"\[\[\s+-v\b"), "4.2", "operador invalido", "sempre"),
    (re.compile(r"\btest\s+-v\b"), "4.2", "operador invalido", "sempre"),
    (re.compile(r"\$\{[A-Za-z_][A-Za-z0-9_]*@[QEPAa]\}"), "4.4/5.1", "bad substitution", "sempre"),
)

# Um builtin so e usado em posicao de comando: no comeco da linha, ou logo apos
# um separador. Um builtin so no meio da linha e texto - o comentario deste
# arquivo, ou `echo "mapfile: command not found"`, que e o falso positivo que
# faz um gate ser desativado em uma semana.
#
# O separador tem que estar IMEDIATAMENTE antes do builtin, entao o padrao e
# ancorado no fim do prefixo. Ancorar no comeco dele casaria com `^` em qualquer
# prefixo nao vazio, e marcaria tudo.
FIM_COMANDO = re.compile(
    r"(?:^|[;&|({]|\b(?:then|else|do|if|while|until)\b|\$\(|!)\s*$"
)

RUN_BLOCO = re.compile(r"^(\s*)run:\s*([|>])([-+]?\d*)\s*$")
RUN_LINHA = re.compile(r"^(\s*)run:\s*(\S.*?)\s*$")
SHELL_CHAVE = re.compile(r"^\s*shell:\s*(\S+)")
INDENTACAO = re.compile(r"^(\s*)\S")

SHELLS_NAO_BASH = {"pwsh", "powershell", "cmd", "python", "sh"}
CANDIDATOS = ("*.yml", "*.yaml")


def arquivos(repo: pathlib.Path) -> "list[pathlib.Path]":
    achados = list((repo / ".github" / "workflows").glob("*"))
    achados = [a for a in achados if a.suffix in (".yml", ".yaml")]
    acoes = (repo / ".github" / "actions")
    if acoes.is_dir():
        achados += [p for p in acoes.rglob("action.yml")]
    return sorted(achados)


def bloco_step(linhas: "list[str]", i: int, recuo: int) -> "tuple[list[str], int]":
    """Linhas do `run:` ate a proxima chave no mesmo ou menor nivel."""
    corpo: "list[str]" = []
    j = i + 1
    while j < len(linhas):
        linha = linhas[j]
        if not linha.strip():
            corpo.append(linha)
            j += 1
            continue
        atual = INDENTACAO.match(linha)
        if atual and len(atual.group(1)) <= recuo:
            break
        corpo.append(linha)
        j += 1
    return corpo, j


def shell_do_step(linhas: "list[str]", i: int, recuo: int, fim: int) -> "str":
    """Shell declarado no step. Sem declaracao, GitHub usa bash em Linux/macOS."""
    for k in range(i, fim):
        achado = SHELL_CHAVE.match(linhas[k])
        if achado and len(INDENTACAO.match(linhas[k]).group(1)) == recuo:
            return achado.group(1).lower()
    return "bash"


def passos(linhas: "list[str]", nome: str) -> "list[tuple[str, int, str]]":
    """(nome do arquivo, linha 1-based, script) de cada passo com shell POSIX."""
    saida = []
    i = 0
    while i < len(linhas):
        bloco = RUN_BLOCO.match(linhas[i])
        simples = RUN_LINHA.match(linhas[i])
        if bloco:
            recuo = len(bloco.group(1))
            corpo, fim = bloco_step(linhas, i, recuo)
            shell = shell_do_step(linhas, i, recuo, fim)
            if shell not in SHELLS_NAO_BASH:
                texto = "\n".join(l[recuo + 2:] if len(l) > recuo else l for l in corpo)
                if texto.strip():
                    saida.append((nome, i + 1, texto))
            i = fim
            continue
        if simples and not linhas[i].lstrip().startswith("#"):
            recuo = len(simples.group(1))
            corpo, fim = bloco_step(linhas, i, recuo)
            shell = shell_do_step(linhas, i, recuo, fim)
            if shell not in SHELLS_NAO_BASH and simples.group(2).strip():
                saida.append((nome, i + 1, simples.group(2)))
            i = fim
            continue
        i += 1
    return saida


def avaliar(script: str) -> "list[tuple[int, str]]":
    """(linha relativa, descricao) de cada uso proibido."""
    achados = []
    for n, linha in enumerate(script.split("\n"), 1):
        if not linha.strip() or linha.lstrip().startswith("#"):
            continue
        for padrao, versao, sintoma, modo in REGRAS:
            for m in padrao.finditer(linha):
                if modo == "comando" and not FIM_COMANDO.search(linha[: m.start()]):
                    continue
                achados.append((n, f"bash {versao}: {sintoma}"))
                break
    return achados


def conferir(repo: pathlib.Path) -> int:
    problemas = 0
    total = 0
    for caminho in arquivos(repo):
        linhas = caminho.read_text(encoding="utf-8").split("\n")
        nome = caminho.relative_to(repo).as_posix()
        for nome_arq, linha_ini, script in passos(linhas, nome):
            total += 1
            for rel, descricao in avaliar(script):
                problemas += 1
                print(f"ERRO {nome_arq}:{linha_ini + rel - 1}  {descricao}")
                print(f"     {script.split(chr(10))[rel - 1].strip()[:110]}")
    if problemas:
        print(f"\n{problemas} uso(s) incompativel(is) com o bash 3.2 do macOS em {total} passo(s).")
        return 1
    print(f"ok: {total} passo(s) POSIX, nenhum builtin posterior ao bash 3.2.")
    return 0


CASOS_SELF_TEST = (
    # (script, deve_reprovar)
    ("mapfile -t cands < <(find . -name x)", True),
    ("mapfile -t cands < <(find .)", True),
    ("echo hi | mapfile -t a", True),
    ("if mapfile -t a < <(x); then :; fi", True),
    ("for f in *; do readarray -t a < <(x); done", True),
    ("declare -A mapa", True),
    ("local -A cache=()", True),
    ("declare -g x=1", True),
    ("shopt -s globstar", True),
    ("cmd &>> arquivo", True),
    ("coproc NAME { read x; }", True),
    ("wait -n", True),
    ("echo ${var^^}", True),
    ("echo ${var,,}", True),
    ("[[ -v arr ]] && echo sim", True),
    ("test -v x", True),
    ("echo ${v@Q}", True),
    # nao pode reprovar
    ("# mapfile e builtin do bash 4, e nao existe no macOS", False),
    ('echo "mapfile: command not found"', False),
    ("cands=()", False),
    ('while IFS= read -r line || [ -n "$line" ]; do cands+=("$line"); done < <(find .)', False),
    ('echo "${#cands[@]}"', False),
    ("declare -i n=0", False),
    ("printf -v alvo '%s' texto", False),
    ("echo $'linha\\nquebrada'", False),
    ('echo "declare -A mapa"', False),
    ("cmd 2>&1 | tee log", False),
    ("echo a >> arquivo", False),
)


def self_test() -> int:
    falhas = 0
    for script, deve_reprovar in CASOS_SELF_TEST:
        reprovou = bool(avaliar(script))
        if reprovou != deve_reprovar:
            falhas += 1
            print(f"FALHA  {script!r}\n       esperado={deve_reprovar} obtido={reprovou}")
    if falhas:
        print(f"\n{falhas} caso(s) do proprio gate estao errados.")
        return 1
    print(f"ok: {len(CASOS_SELF_TEST)} casos do proprio gate.")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--root", default=".", help="raiz do repositorio")
    ap.add_argument("--self-test", action="store_true", help="confere o gate, nao o repo")
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    return conferir(pathlib.Path(args.root).resolve())


if __name__ == "__main__":
    sys.exit(main())