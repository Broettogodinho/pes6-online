import itertools

import pytest

from tournament import Match, MatchResult, Player
from tournament.schedule import round_robin
from tournament.standings import compute_standings


def players(n):
    return [Player(f"p{i}", f"Jogador {i:02d}", team_id=i) for i in range(n)]


@pytest.mark.parametrize("n", [2, 3, 4, 5, 10, 20])
def test_round_robin_todos_se_enfrentam_uma_vez(n):
    ps = players(n)
    rounds = round_robin(ps)
    pairs = [frozenset((a.id, b.id)) for rnd in rounds for a, b in rnd]
    expected = {frozenset((a.id, b.id)) for a, b in itertools.combinations(ps, 2)}
    assert len(pairs) == len(expected)
    assert set(pairs) == expected
    # ninguém joga duas vezes na mesma rodada
    for rnd in rounds:
        ids = [p.id for pair in rnd for p in pair]
        assert len(ids) == len(set(ids))


def test_round_robin_numero_de_rodadas():
    assert len(round_robin(players(20))) == 19
    assert len(round_robin(players(5))) == 5  # ímpar: um folga por rodada


def test_round_robin_returno_inverte_mandos():
    ps = players(4)
    rounds = round_robin(ps, double=True)
    assert len(rounds) == 6
    first = {(a.id, b.id) for rnd in rounds[:3] for a, b in rnd}
    second = {(a.id, b.id) for rnd in rounds[3:] for a, b in rnd}
    assert second == {(b, a) for a, b in first}


def test_round_robin_mandos_equilibrados():
    ps = players(20)
    homes = {p.id: 0 for p in ps}
    for rnd in round_robin(ps):
        for home, _ in rnd:
            homes[home.id] += 1
    assert max(homes.values()) - min(homes.values()) <= 1


def test_round_robin_invalido():
    with pytest.raises(ValueError):
        round_robin(players(1))
    p = players(2)
    with pytest.raises(ValueError):
        round_robin([p[0], p[0]])


def test_tabela_pontos_saldo_e_gols():
    a, b, c = players(3)
    ms = [
        Match("1", 1, a, b, MatchResult(2, 0)),
        Match("2", 1, b, c, MatchResult(1, 1)),
        Match("3", 1, c, a, MatchResult(0, 3)),
    ]
    rows = compute_standings([a, b, c], ms)
    assert [r.player.id for r in rows] == ["p0", "p1", "p2"]
    assert rows[0].points == 6 and rows[0].goal_diff == 5
    assert (rows[1].points, rows[1].goal_diff) == (1, -2)
    assert (rows[2].played, rows[2].draws, rows[2].losses) == (2, 1, 1)


def test_desempate_por_confronto_direto():
    a, b, c, d = players(4)
    # a e b terminam iguais em pontos, saldo e gols; b venceu o confronto direto
    ms = [
        Match("1", 1, b, a, MatchResult(1, 0)),
        Match("2", 1, a, c, MatchResult(2, 1)),
        Match("3", 1, b, d, MatchResult(0, 1)),
        Match("4", 1, a, d, MatchResult(0, 0)),
        Match("5", 1, b, c, MatchResult(1, 1)),
    ]
    rows = compute_standings([a, b, c, d], ms)
    ra = next(r for r in rows if r.player is a)
    rb = next(r for r in rows if r.player is b)
    assert (ra.points, ra.goal_diff, ra.goals_for) == (rb.points, rb.goal_diff, rb.goals_for)
    assert rows.index(rb) < rows.index(ra)


def test_tabela_ignora_partidas_nao_jogadas():
    a, b = players(2)
    rows = compute_standings([a, b], [Match("1", 1, a, b)])
    assert all(r.played == 0 for r in rows)
