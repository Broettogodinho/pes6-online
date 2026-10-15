import pytest

from tournament import Match, MatchResult, Player

A = Player("a", "Ana", team_id=1)
B = Player("b", "Beto", team_id=2)


def test_vitoria_no_tempo_normal():
    m = Match("m1", 1, A, B, MatchResult(2, 1))
    assert m.played
    assert m.winner() == A


def test_empate_sem_penaltis_nao_tem_vencedor():
    r = MatchResult(1, 1)
    assert r.is_draw
    assert Match("m2", 1, A, B, r).winner() is None


def test_penaltis_decidem_empate():
    r = MatchResult(0, 0, home_penalties=3, away_penalties=4)
    assert not r.is_draw
    assert Match("m3", 1, A, B, r).winner() == B


def test_partida_sem_resultado():
    m = Match("m4", 1, A, B)
    assert not m.played
    assert m.winner() is None


@pytest.mark.parametrize(
    "kwargs",
    [
        {"home_goals": -1, "away_goals": 0},
        {"home_goals": 0, "away_goals": 0, "home_penalties": 3},
    ],
)
def test_resultados_invalidos(kwargs):
    with pytest.raises(ValueError):
        MatchResult(**kwargs)


def test_penaltis_empatados_invalidos():
    with pytest.raises(ValueError):
        MatchResult(1, 1, 4, 4).winner_side()
