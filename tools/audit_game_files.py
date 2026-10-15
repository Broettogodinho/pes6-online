#!/usr/bin/env python3
"""Bloqueia arquivos do jogo no repositório.

Verifica os arquivos versionados (git ls-files) — ou os caminhos passados como
argumento — e falha se encontrar:
  * extensões proibidas (executáveis, DLLs fora de build, assets do PES 6, projetos de RE)
  * nomes conhecidos de arquivos do jogo
  * binários PE (cabeçalho "MZ") em qualquer lugar
  * arquivos binários grandes fora das exceções

Uso:
    python tools/audit_game_files.py            # audita o que está no git
    python tools/audit_game_files.py a.txt b/   # audita caminhos específicos
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

FORBIDDEN_EXTENSIONS = {
    ".exe", ".dll", ".afs", ".img", ".iso", ".mdf", ".mds", ".cue", ".opt",
    ".gpr", ".idb", ".i64", ".id0", ".id1", ".id2", ".nam", ".til", ".dmp",
}
FORBIDDEN_NAME_PARTS = ("konami-win32pes6opt", "pes6.exe", "0_text.afs", "0_sound.afs")
ALLOWED_PREFIXES = ("patch/third_party/",)
MAX_BINARY_BYTES = 512 * 1024


def tracked_files(root: Path) -> list[Path]:
    out = subprocess.run(
        ["git", "ls-files", "-z"], cwd=root, check=True, capture_output=True
    ).stdout.decode()
    return [root / p for p in out.split("\0") if p]


def expand(paths: list[str]) -> list[Path]:
    files: list[Path] = []
    for p in map(Path, paths):
        files.extend(f for f in p.rglob("*") if f.is_file()) if p.is_dir() else files.append(p)
    return files


def is_binary(head: bytes) -> bool:
    return b"\0" in head


def check(path: Path, rel: str) -> list[str]:
    problems: list[str] = []
    low = rel.lower()
    if low.startswith(ALLOWED_PREFIXES):
        return problems
    if path.suffix.lower() in FORBIDDEN_EXTENSIONS:
        problems.append(f"extensão proibida ({path.suffix})")
    if any(part in low for part in FORBIDDEN_NAME_PARTS):
        problems.append("nome de arquivo do jogo")
    try:
        with path.open("rb") as fh:
            head = fh.read(4096)
        size = path.stat().st_size
    except OSError as exc:
        return problems + [f"não foi possível ler ({exc})"]
    if head[:2] == b"MZ":
        problems.append("binário Windows (cabeçalho MZ)")
    elif is_binary(head) and size > MAX_BINARY_BYTES:
        problems.append(f"binário grande ({size // 1024} KB)")
    return problems


def main(argv: list[str]) -> int:
    root = Path(__file__).resolve().parent.parent
    files = expand(argv) if argv else tracked_files(root)
    failures = 0
    for f in files:
        try:
            rel = f.resolve().relative_to(root).as_posix()
        except ValueError:
            rel = f.as_posix()
        for problem in check(f, rel):
            print(f"BLOQUEADO: {rel} — {problem}")
            failures += 1
    if failures:
        print(f"\n{failures} problema(s). Arquivos do jogo não podem entrar no repositório (ver AGENTS.md).")
        return 1
    print(f"OK: {len(files)} arquivo(s) auditados, nenhum arquivo do jogo encontrado.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
