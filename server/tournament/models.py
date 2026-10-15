"""Modelos básicos compartilhados por Copa e Liga."""
from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class Player:
    """Jogador inscrito no torneio, com o time escolhido."""

    id: str
    name: str
    team_id: int


@dataclass(frozen=True)
class MatchResult:
    """Placar final de uma partida. Pênaltis só existem em mata-mata."""

    home_goals: int
    away_goals: int
    home_penalties: int | None = None
    away_penalties: int | None = None

    def __post_init__(self) -> None:
        if self.home_goals < 0 or self.away_goals < 0:
            raise ValueError("gols não podem ser negativos")
        if (self.home_penalties is None) != (self.away_penalties is None):
            raise ValueError("informe os pênaltis dos dois lados ou de nenhum")

    @property
    def is_draw(self) -> bool:
        return self.home_goals == self.away_goals and self.home_penalties is None

    def winner_side(self) -> str | None:
        """'home', 'away' ou None (empate sem pênaltis)."""
        if self.home_goals != self.away_goals:
            return "home" if self.home_goals > self.away_goals else "away"
        if self.home_penalties is None:
            return None
        if self.home_penalties == self.away_penalties:
            raise ValueError("disputa de pênaltis não pode terminar empatada")
        return "home" if self.home_penalties > self.away_penalties else "away"


@dataclass
class Match:
    """Partida de uma rodada do torneio."""

    id: str
    round: int
    home: Player
    away: Player
    result: MatchResult | None = None

    @property
    def played(self) -> bool:
        return self.result is not None

    def winner(self) -> Player | None:
        if self.result is None:
            return None
        side = self.result.winner_side()
        return {"home": self.home, "away": self.away}.get(side) if side else None
