"""Testes do pes6text com tabelas sintéticas (nenhum dado do jogo).

    python -m unittest discover tools/text
"""
import struct
import unittest

import pes6text as t


def make_section(entries: list[tuple[int, str]], shared_last: bool = False) -> bytes:
    """Seção no formato observado: cabeçalho 0x20, [count][campo], registros de 12 bytes, strings."""
    count = len(entries)
    first = 8
    strings_start = first + t.ENTRY_SIZE * count
    regs, strs = bytearray(), bytearray()
    offsets = []
    for eid, text in entries:
        b = text.encode(t.ENCODING)
        offsets.append(strings_start + len(strs))
        strs += b + b"\x00"
    if shared_last and count >= 2:  # último registro reaproveita a string do primeiro (como o jogo faz)
        offsets[-1] = offsets[0]
    for (eid, text), off in zip(entries, offsets):
        b = (entries[0][1] if (shared_last and off == offsets[0]) else text).encode(t.ENCODING)
        regs += struct.pack("<IHHI", eid, len(b) + 1, len(b), off)
    data = struct.pack("<II", count, 0x2B) + bytes(regs) + bytes(strs)
    head = struct.pack("<IIII", 0x400, len(data), len(data), 0) + b"\x00" * 16
    body = head + data
    return body + b"\x00" * ((-len(body)) % t.SECTION_ALIGN)


def make_table(sections: list[bytes]) -> bytes:
    nsec = len(sections)
    gh_size = 8 + 4 * nsec
    gh_size += (-gh_size) % 16
    head = bytearray(gh_size)
    struct.pack_into("<II", head, 0, nsec, 8)
    off = gh_size
    for s, sec in enumerate(sections):
        struct.pack_into("<I", head, 8 + 4 * s, off)
        off += len(sec)
    return bytes(head) + b"".join(sections)


class TableTest(unittest.TestCase):
    def setUp(self):
        self.raw = make_table([
            make_section([(0, "Champion"), (2, "Please Wait..."), (3, "Now Loading")]),
            make_section([(10, "Home team"), (11, "Away team"), (12, "Home team")], shared_last=True),
        ])

    def test_parse(self):
        _, secs = t.parse_table(self.raw)
        self.assertEqual([e.text for e in secs[0].entries], ["Champion", "Please Wait...", "Now Loading"])
        self.assertEqual([e.id for e in secs[1].entries], [10, 11, 12])
        self.assertEqual(secs[1].entries[2].text, "Home team")  # string compartilhada

    def test_no_change_is_identical(self):
        gh, secs = t.parse_table(self.raw)
        self.assertEqual(t.build_table(gh, secs, {}), self.raw)

    def test_change_keeps_others(self):
        gh, secs = t.parse_table(self.raw)
        changes = {(0, 1): "Por favor, aguarde... ação", (1, 0): "Time da casa"}
        rebuilt = t.build_table(gh, secs, changes)
        _, again = t.parse_table(rebuilt)
        self.assertEqual([e.text for e in again[0].entries], ["Champion", "Por favor, aguarde... ação", "Now Loading"])
        # o registro 2 compartilhava a string do 0: continua com o texto original
        self.assertEqual([e.text for e in again[1].entries], ["Time da casa", "Away team", "Home team"])
        self.assertEqual([e.id for e in again[1].entries], [10, 11, 12])
        # seções continuam alinhadas e o cabeçalho global aponta para elas
        (nsec,) = struct.unpack_from("<I", rebuilt, 0)
        for off in struct.unpack_from(f"<{nsec}I", rebuilt, 8):
            self.assertEqual(off % t.SECTION_ALIGN, 0)

    def test_container_roundtrip(self):
        head = t.CONTAINER_MAGIC + b"\x00" * (t.CONTAINER_HEADER - 4)
        blob = t.pack_container(head, self.raw)
        h2, raw2 = t.unpack_container(blob)
        self.assertEqual(raw2, self.raw)
        csize, usize = struct.unpack_from("<II", h2, 4)
        self.assertEqual(usize, len(self.raw))
        self.assertEqual(csize, len(blob) - t.CONTAINER_HEADER)

    def test_changes_from_doc(self):
        doc = {"sections": [[{"k": 0, "en": "A", "text": "A"}, {"k": 1, "en": "B", "text": "Bê"}]]}
        self.assertEqual(t.changes_from_doc(doc), {(0, 1): "Bê"})


if __name__ == "__main__":
    unittest.main()
