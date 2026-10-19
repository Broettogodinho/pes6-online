#!/usr/bin/env python3
"""Extrai e remonta as tabelas de texto do PES 6 (SCRUM-63, tradução PT-BR).

As tabelas ficam dentro dos `*_text.afs` como arquivos comprimidos com zlib
(contêiner `00 06 01 00`). Formato em re/notes/textos-afs.md.

Uso:
    python tools/text/pes6text.py extract <x_text.afs> <pasta_json>
        Um JSON por tabela (indice_NNN.json): cada texto com "en" (original) e
        "text" (a traduzir). Só "text" deve ser editado.
    python tools/text/pes6text.py pack <x_text.afs> <pasta_json> <pasta_saida>
        Remonta as tabelas com os textos editados. Textos não alterados ficam
        com os bytes originais; os alterados vão para o fim da área de textos
        da seção. Saída com o nome que o afs2fs do Kitserver espera.
    python tools/text/pes6text.py selftest <x_text.afs>
        Altera textos em memória, remonta e confere: os alterados mudam, todos os
        outros continuam iguais e tabelas sem alteração saem idênticas.

As saídas são derivadas do jogo: guardar FORA do repositório (AGENTS.md §1).
"""
from __future__ import annotations

import json
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path

CONTAINER_MAGIC = b"\x00\x06\x01\x00"
CONTAINER_HEADER = 0x20
SECTION_HEADER = 0x20
ENTRY_SIZE = 12
SECTION_ALIGN = 16
ENCODING = "latin-1"  # INFERIDO: 1 byte por caractere (as versões FR/DE/ES usam acentos)


# --------------------------------------------------------------------------- AFS
def read_afs(path: Path) -> list[bytes]:
    """Arquivos internos de um .afs (cabeçalho 'AFS\\0', quantidade, tabela offset/tamanho)."""
    data = path.read_bytes()
    if data[:4] != b"AFS\x00":
        raise ValueError(f"{path}: não é um AFS")
    (count,) = struct.unpack_from("<I", data, 4)
    out = []
    for i in range(count):
        off, size = struct.unpack_from("<II", data, 8 + 8 * i)
        out.append(data[off:off + size])
    return out


def table_indices(files: list[bytes]) -> list[int]:
    return [i for i, f in enumerate(files) if f[:4] == CONTAINER_MAGIC]


# --------------------------------------------------------------------- contêiner
def unpack_container(blob: bytes) -> tuple[bytes, bytes]:
    """(cabeçalho de 0x20 bytes, tabela descomprimida)."""
    if blob[:4] != CONTAINER_MAGIC:
        raise ValueError("não é um contêiner 00 06 01 00")
    _, usize = struct.unpack_from("<II", blob, 4)
    raw = zlib.decompress(blob[CONTAINER_HEADER:])
    if len(raw) != usize:
        raise ValueError(f"tamanho descomprimido {len(raw)} != {usize}")
    return blob[:CONTAINER_HEADER], raw


def pack_container(header: bytes, raw: bytes) -> bytes:
    comp = zlib.compress(raw, 9)
    head = bytearray(header)
    struct.pack_into("<II", head, 4, len(comp), len(raw))
    return bytes(head) + comp


# ------------------------------------------------------------------------ tabela
@dataclass
class Entry:
    index: int   # posição do registro na seção
    id: int
    text: str


@dataclass
class Section:
    raw: bytes        # bytes originais da seção (cabeçalho + dados + alinhamento)
    entries: list[Entry]


def _section_layout(raw: bytes) -> tuple[int, int, int]:
    """(tamanho dos dados, quantidade de registros, deslocamento do 1º registro), relativos à seção."""
    (data_size,) = struct.unpack_from("<I", raw, 4)
    (count,) = struct.unpack_from("<I", raw, SECTION_HEADER)
    return data_size, count, SECTION_HEADER + 8


def parse_table(raw: bytes) -> tuple[bytes, list[Section]]:
    """Tabela descomprimida -> (cabeçalho global, seções)."""
    (nsec,) = struct.unpack_from("<I", raw, 0)
    offsets = list(struct.unpack_from(f"<{nsec}I", raw, 8))
    bounds = offsets + [len(raw)]
    sections = []
    for s in range(nsec):
        sec = raw[bounds[s]:bounds[s + 1]]
        _, count, first = _section_layout(sec)
        entries = []
        for k in range(count):
            eid, _size, length, off = struct.unpack_from("<IHHI", sec, first + ENTRY_SIZE * k)
            text = sec[SECTION_HEADER + off: SECTION_HEADER + off + length].decode(ENCODING)
            entries.append(Entry(k, eid, text))
        sections.append(Section(sec, entries))
    return raw[:offsets[0]], sections


def rebuild_section(sec: Section, changed: dict[int, str]) -> bytes:
    """Seção com os textos de `changed` (índice do registro -> texto) substituídos."""
    if not changed:
        return sec.raw
    data_size, _count, first = _section_layout(sec.raw)
    data = bytearray(sec.raw[SECTION_HEADER:SECTION_HEADER + data_size])
    first_in_data = first - SECTION_HEADER
    for k, text in sorted(changed.items()):
        b = text.encode(ENCODING)
        off = len(data)
        data += b + b"\x00"
        pos = first_in_data + ENTRY_SIZE * k
        eid, _, _, _ = struct.unpack_from("<IHHI", data, pos)
        struct.pack_into("<IHHI", data, pos, eid, len(b) + 1, len(b), off)
    head = bytearray(sec.raw[:SECTION_HEADER])
    struct.pack_into("<II", head, 4, len(data), len(data))  # INFERIDO: campos 1 e 2 = tamanho dos dados
    body = bytes(head) + bytes(data)
    pad = (-len(body)) % SECTION_ALIGN
    return body + b"\x00" * pad


def build_table(global_header: bytes, sections: list[Section], changes: dict[tuple[int, int], str]) -> bytes:
    """Remonta a tabela; `changes` = {(seção, índice do registro): novo texto}."""
    blobs = []
    for s, sec in enumerate(sections):
        mine = {k: t for (ss, k), t in changes.items() if ss == s}
        blobs.append(rebuild_section(sec, mine))
    head = bytearray(global_header)
    off = len(global_header)
    for s, blob in enumerate(blobs):
        struct.pack_into("<I", head, 8 + 4 * s, off)
        off += len(blob)
    return bytes(head) + b"".join(blobs)


# ---------------------------------------------------------------------- comandos
def cmd_extract(afs: Path, out: Path) -> None:
    files = read_afs(afs)
    out.mkdir(parents=True, exist_ok=True)
    total = 0
    for i in table_indices(files):
        _, raw = unpack_container(files[i])
        _, secs = parse_table(raw)
        doc = {"afs_index": i, "sections": [
            [{"k": e.index, "id": e.id, "en": e.text, "text": e.text} for e in s.entries] for s in secs]}
        (out / f"indice_{i:03d}.json").write_text(json.dumps(doc, ensure_ascii=False, indent=1), encoding="utf-8")
        total += sum(len(s.entries) for s in secs)
    print(f"extract: {len(table_indices(files))} tabelas, {total} textos -> {out}")


def changes_from_doc(doc: dict) -> dict[tuple[int, int], str]:
    return {(s, e["k"]): e["text"] for s, sec in enumerate(doc["sections"]) for e in sec if e["text"] != e["en"]}


def cmd_pack(afs: Path, src: Path, out: Path) -> None:
    files = read_afs(afs)
    out.mkdir(parents=True, exist_ok=True)
    written = 0
    for js in sorted(src.glob("indice_*.json")):
        doc = json.loads(js.read_text(encoding="utf-8"))
        changes = changes_from_doc(doc)
        if not changes:
            continue
        head, raw = unpack_container(files[doc["afs_index"]])
        gh, secs = parse_table(raw)
        blob = pack_container(head, build_table(gh, secs, changes))
        (out / f"unnamed_{doc['afs_index']}.bin").write_bytes(blob)
        written += 1
    print(f"pack: {written} tabela(s) alterada(s) -> {out}")


def cmd_selftest(afs: Path) -> int:
    files = read_afs(afs)
    failures = 0
    for i in table_indices(files):
        head, raw = unpack_container(files[i])
        gh, secs = parse_table(raw)
        # 1) sem alteração: idêntica
        if build_table(gh, secs, {}) != raw:
            print(f"  tabela {i}: sem alteração, mas remontada diferente")
            failures += 1
            continue
        # 2) altera o 1º e o último texto de cada seção (mais longos, com acentos de PT)
        changes = {}
        for s, sec in enumerate(secs):
            if sec.entries:
                for e in (sec.entries[0], sec.entries[-1]):
                    changes[(s, e.index)] = f"[PT] {e.text} ação"
        rebuilt = build_table(gh, secs, changes)
        _, again = unpack_container(pack_container(head, rebuilt))
        _, secs2 = parse_table(again)
        for s, (a, b) in enumerate(zip(secs, secs2)):
            for ea, eb in zip(a.entries, b.entries):
                want = changes.get((s, ea.index), ea.text)
                if eb.text != want or eb.id != ea.id:
                    print(f"  tabela {i} seção {s} registro {ea.index}: esperado {want!r}, lido {eb.text!r}")
                    failures += 1
    n = len(table_indices(files))
    print(f"selftest: {n} tabelas; {'OK' if failures == 0 else f'{failures} falha(s)'}")
    return 0 if failures == 0 else 1


def main(argv: list[str]) -> int:
    if len(argv) >= 3 and argv[0] == "extract":
        cmd_extract(Path(argv[1]), Path(argv[2]))
        return 0
    if len(argv) >= 4 and argv[0] == "pack":
        cmd_pack(Path(argv[1]), Path(argv[2]), Path(argv[3]))
        return 0
    if len(argv) >= 2 and argv[0] == "selftest":
        return cmd_selftest(Path(argv[1]))
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
