"""Geração de confrontos: todos contra todos (método do círculo)."""
from __future__ import annotations

from typing import Sequence, TypeVar

T = TypeVar("T")


def round_robin(participants: Sequence[T], double: bool = False) -> list[list[tuple[T, T]]]:
    """Rodadas de todos contra todos.

    Cada rodada é uma lista de pares (mandante, visitante). Com número ímpar de
    participantes, um folga por rodada. Os mandos alternam para equilibrar
    jogos em casa e fora. Com ``double=True``, o returno inverte os mandos.
    """
    items: list[T | None] = list(participants)
    if len(items) < 2:
        raise ValueError("são necessários pelo menos 2 participantes")
    if len(set(map(id, items))) != len(items):
        raise ValueError("participantes repetidos")
    if len(items) % 2:
        items.append(None)  # folga

    n = len(items)
    rounds: list[list[tuple[T, T]]] = []
    for r in range(n - 1):
        pairs: list[tuple[T, T]] = []
        for i in range(n // 2):
            a, b = items[i], items[n - 1 - i]
            if a is None or b is None:
                continue
            # Alterna o mando para o pivô (posição 0) e para os demais.
            if (i == 0 and r % 2) or (i > 0 and i % 2):
                a, b = b, a
            pairs.append((a, b))
        rounds.append(pairs)
        items = [items[0], items[-1], *items[1:-1]]  # gira mantendo o pivô

    if double:
        rounds += [[(b, a) for a, b in rnd] for rnd in rounds]
    return rounds
