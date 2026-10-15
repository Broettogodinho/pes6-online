"""Engine de Copa: 8, 16 ou 32 times, fase de grupos opcional e mata-mata.

Fluxo:
    cup = Cup(size=16, with_groups=True, seed=42)
    for p in jogadores: cup.register(p)
    cup.draw()                      # sorteio (aleatório ou por potes)
    for m in cup.pending_matches(): cup.report(m.id, MatchResult(...))
    cup.champion                    # quando cup.phase == "finished"

Regras (padrão das Copas do PES):
- Grupos de 4, turno único; avançam 1º e 2º.
- Cruzamento do mata-mata: 1A×2B, 1B×2A, 1C×2D, 1D×2C, ...
- No mata-mata não há empate: o resultado precisa trazer os pênaltis.
"""
from __future__ import annotations

import random
from dataclasses import dataclass, field
from string import ascii_uppercase
from typing import Sequence

from .models import Match, MatchResult, Player
from .schedule import round_robin
from .standings import Row, compute_standings

VALID_SIZES = (8, 16, 32)
GROUP_SIZE = 4

KNOCKOUT_NAMES = {
    2: "Final",
    4: "Semifinal",
    8: "Quartas de final",
    16: "Oitavas de final",
    32: "Primeira fase",
}


class CupError(ValueError):
    """Operação inválida para o estado atual da Copa."""


@dataclass
class Group:
    name: str
    players: list[Player]
    matches: list[Match] = field(default_factory=list)

    @property
    def finished(self) -> bool:
        return all(m.played for m in self.matches)

    def standings(self) -> list[Row]:
        return compute_standings(self.players, self.matches)


@dataclass
class KnockoutRound:
    name: str
    matches: list[Match]

    @property
    def finished(self) -> bool:
        return all(m.played for m in self.matches)


class Cup:
    def __init__(self, size: int, with_groups: bool = False, seed: int | None = None):
        if size not in VALID_SIZES:
            raise CupError(f"tamanho inválido: {size} (use {', '.join(map(str, VALID_SIZES))})")
        self.size = size
        self.with_groups = with_groups
        self._rng = random.Random(seed)
        self.players: list[Player] = []
        self.groups: list[Group] = []
        self.rounds: list[KnockoutRound] = []
        self.phase = "registration"
        self._match_seq = 0
        self._matches: dict[str, Match] = {}

    # ------------------------------------------------------------ inscrição
    def register(self, player: Player) -> None:
        if self.phase != "registration":
            raise CupError("inscrições encerradas")
        if len(self.players) >= self.size:
            raise CupError("Copa lotada")
        if any(p.id == player.id for p in self.players):
            raise CupError(f"jogador já inscrito: {player.id}")
        if any(p.team_id == player.team_id for p in self.players):
            raise CupError(f"time já escolhido por outro jogador: {player.team_id}")
        self.players.append(player)

    # ------------------------------------------------------------ sorteio
    def draw(self, pots: Sequence[Sequence[str]] | None = None) -> None:
        """Sorteia grupos ou o chaveamento.

        ``pots``: listas de ids de jogadores. Com grupos, cada pote precisa ter
        um jogador por grupo (cabeças de chave no pote 1). Sem grupos, use dois
        potes de tamanho igual: cada jogo da 1ª fase opõe pote 1 × pote 2.
        """
        if self.phase != "registration":
            raise CupError("sorteio já realizado")
        if len(self.players) != self.size:
            raise CupError(f"faltam inscrições: {len(self.players)}/{self.size}")

        ordered = self._ordered_by_pots(pots)
        if self.with_groups:
            n_groups = self.size // GROUP_SIZE
            buckets: list[list[Player]] = [[] for _ in range(n_groups)]
            for i, p in enumerate(ordered):
                buckets[i % n_groups].append(p)
            for i, members in enumerate(buckets):
                group = Group(ascii_uppercase[i], members)
                for rnd in round_robin(members):
                    for home, away in rnd:
                        group.matches.append(self._new_match(home, away, round_no=1))
                self.groups.append(group)
            self._renumber_group_rounds()
            self.phase = "groups"
        else:
            half = self.size // 2
            if pots:
                pairs = list(zip(ordered[:half], ordered[half:]))
            else:
                pairs = [(ordered[2 * i], ordered[2 * i + 1]) for i in range(half)]
            self._start_knockout(pairs)

    def _ordered_by_pots(self, pots: Sequence[Sequence[str]] | None) -> list[Player]:
        if not pots:
            players = list(self.players)
            self._rng.shuffle(players)
            return players

        by_id = {p.id: p for p in self.players}
        flat = [pid for pot in pots for pid in pot]
        if sorted(flat) != sorted(by_id):
            raise CupError("os potes precisam conter cada inscrito exatamente uma vez")
        expected = self.size // GROUP_SIZE if self.with_groups else self.size // 2
        if any(len(pot) != expected for pot in pots):
            raise CupError(f"cada pote precisa ter {expected} jogadores")

        ordered: list[Player] = []
        for pot in pots:
            members = [by_id[pid] for pid in pot]
            self._rng.shuffle(members)
            ordered.extend(members)
        return ordered

    def _renumber_group_rounds(self) -> None:
        for group in self.groups:
            per_round = len(group.players) // 2
            for i, m in enumerate(group.matches):
                m.round = i // per_round + 1

    # ------------------------------------------------------------ partidas
    def _new_match(self, home: Player, away: Player, round_no: int) -> Match:
        self._match_seq += 1
        match = Match(f"M{self._match_seq:03d}", round_no, home, away)
        self._matches[match.id] = match
        return match

    def pending_matches(self) -> list[Match]:
        if self.phase == "groups":
            return [m for g in self.groups for m in g.matches if not m.played]
        if self.phase == "knockout":
            return [m for m in self.rounds[-1].matches if not m.played]
        return []

    def report(self, match_id: str, result: MatchResult) -> None:
        match = self._matches.get(match_id)
        if match is None:
            raise CupError(f"partida inexistente: {match_id}")
        if match.played:
            raise CupError(f"resultado já registrado: {match_id}")
        if match not in self.pending_matches():
            raise CupError(f"partida fora da fase atual: {match_id}")
        if self.phase == "knockout" and result.winner_side() is None:
            raise CupError("mata-mata não admite empate: informe os pênaltis")
        if self.phase == "groups" and result.home_penalties is not None:
            raise CupError("fase de grupos não tem pênaltis")

        match.result = result
        self._advance()

    # ------------------------------------------------------------ avanço
    def _advance(self) -> None:
        if self.phase == "groups" and all(g.finished for g in self.groups):
            firsts = [g.standings()[0].player for g in self.groups]
            seconds = [g.standings()[1].player for g in self.groups]
            pairs = []
            for i in range(0, len(self.groups), 2):
                pairs.append((firsts[i], seconds[i + 1]))
                pairs.append((firsts[i + 1], seconds[i]))
            self._start_knockout(pairs)
        elif self.phase == "knockout" and self.rounds[-1].finished:
            winners = [m.winner() for m in self.rounds[-1].matches]
            if len(winners) == 1:
                self.phase = "finished"
            else:
                self._add_round(list(zip(winners[0::2], winners[1::2])))

    def _start_knockout(self, pairs: list[tuple[Player, Player]]) -> None:
        self.phase = "knockout"
        self._add_round(pairs)

    def _add_round(self, pairs: list[tuple[Player, Player]]) -> None:
        number = len(self.rounds) + 1
        name = KNOCKOUT_NAMES.get(len(pairs) * 2, f"Rodada {number}")
        matches = [self._new_match(h, a, round_no=number) for h, a in pairs]
        self.rounds.append(KnockoutRound(name, matches))

    # ------------------------------------------------------------ consulta
    @property
    def champion(self) -> Player | None:
        if self.phase != "finished":
            return None
        return self.rounds[-1].matches[0].winner()

    def group(self, name: str) -> Group:
        for g in self.groups:
            if g.name == name:
                return g
        raise CupError(f"grupo inexistente: {name}")
