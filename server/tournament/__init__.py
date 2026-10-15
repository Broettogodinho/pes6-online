"""Lógica pura de torneio (Copa e Liga) do PES 6 Online.

Sem I/O: rede, banco e integração com o Sixserver ficam em outras camadas.
As engines completas de Copa (SCRUM-28) e Liga (SCRUM-29) entram na F2.
"""

from .models import Match, MatchResult, Player

__all__ = ["Match", "MatchResult", "Player"]
