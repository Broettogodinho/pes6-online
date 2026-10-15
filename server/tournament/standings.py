"""Tabela de classificação (usada pela fase de grupos e pela Liga)."""
from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable, Sequence

from .models import Match, Player

POINTS_WIN = 3
POINTS_DRAW = 1


@dataclass
class Row:
    player: Player
    played: int = 0
    wins: int = 0
    draws: int = 0
    losses: int = 0
    goals_for: int = 0
    goals_against: int = 0

    @property
    def points(self) -> int:
        return self.wins * POINTS_WIN + self.draws * POINTS_DRAW

    @property
    def goal_diff(self) -> int:
        return self.goals_for - self.goals_against


def _apply(row: Row, scored: int, conceded: int) -> None:
    row.played += 1
    row.goals_for += scored
    row.goals_against += conceded
    if scored > conceded:
        row.wins += 1
    elif scored == conceded:
        row.draws += 1
    else:
        row.losses += 1


def compute_standings(players: Sequence[Player], matches: Iterable[Match]) -> list[Row]:
    """Classificação por: pontos, saldo, gols pró, confronto direto, nome.

    Pênaltis não contam para a tabela (só decidem mata-mata).
    """
    rows = {p.id: Row(p) for p in players}
    played = [m for m in matches if m.played and m.home.id in rows and m.away.id in rows]
    for m in played:
        r = m.result
        _apply(rows[m.home.id], r.home_goals, r.away_goals)
        _apply(rows[m.away.id], r.away_goals, r.home_goals)

    def head_to_head(a: Player, b: Player) -> int:
        """Pontos de ``a`` contra ``b`` menos pontos de ``b`` contra ``a``."""
        score = 0
        for m in played:
            ids = {m.home.id, m.away.id}
            if ids != {a.id, b.id}:
                continue
            side = m.result.winner_side()
            if side is None:
                continue
            winner = m.home if side == "home" else m.away
            score += 1 if winner.id == a.id else -1
        return score

    ordered = sorted(
        rows.values(),
        key=lambda r: (-r.points, -r.goal_diff, -r.goals_for, r.player.name),
    )
    # Desempate por confronto direto entre pares empatados nos três critérios.
    changed = True
    while changed:
        changed = False
        for i in range(len(ordered) - 1):
            a, b = ordered[i], ordered[i + 1]
            if (a.points, a.goal_diff, a.goals_for) == (b.points, b.goal_diff, b.goals_for):
                if head_to_head(b.player, a.player) > 0:
                    ordered[i], ordered[i + 1] = b, a
                    changed = True
    return ordered
