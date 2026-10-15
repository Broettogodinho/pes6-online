import random

import pytest

from tournament import MatchResult, Player
from tournament.cup import Cup, CupError


def players(n):
    return [Player(f"p{i}", f"Jogador {i:02d}", team_id=100 + i) for i in range(n)]


def full_cup(size, with_groups=False, seed=1):
    cup = Cup(size, with_groups=with_groups, seed=seed)
    for p in players(size):
        cup.register(p)
    return cup


def random_result(rng, knockout):
    h, a = rng.randint(0, 4), rng.randint(0, 4)
    if knockout and h == a:
        hp = rng.randint(3, 5)
        ap = hp - 1 if rng.random() < 0.5 else hp + 1
        return MatchResult(h, a, hp, ap)
    return MatchResult(h, a)


def play_all(cup, seed=7):
    rng = random.Random(seed)
    while cup.pending_matches():
        knockout = cup.phase == "knockout"
        for m in list(cup.pending_matches()):
            cup.report(m.id, random_result(rng, knockout))


# ---------------------------------------------------------------- inscrição
def test_tamanho_invalido():
    with pytest.raises(CupError):
        Cup(12)


def test_inscricao_rejeita_duplicados_e_lotacao():
    cup = Cup(8)
    a = Player("a", "Ana", 1)
    cup.register(a)
    with pytest.raises(CupError):
        cup.register(Player("a", "Outra", 2))  # mesmo jogador
    with pytest.raises(CupError):
        cup.register(Player("b", "Beto", 1))  # mesmo time
    for p in players(7):
        cup.register(p)
    with pytest.raises(CupError):
        cup.register(Player("z", "Zé", 999))  # lotada


def test_sorteio_exige_inscricoes_completas():
    cup = Cup(8)
    cup.register(Player("a", "Ana", 1))
    with pytest.raises(CupError):
        cup.draw()


def test_sem_inscricao_depois_do_sorteio():
    cup = full_cup(8)
    cup.draw()
    with pytest.raises(CupError):
        cup.register(Player("x", "X", 999))
    with pytest.raises(CupError):
        cup.draw()


# ---------------------------------------------------------------- mata-mata
@pytest.mark.parametrize("size,rounds", [(8, 3), (16, 4), (32, 5)])
def test_mata_mata_completo(size, rounds):
    cup = full_cup(size)
    cup.draw()
    assert cup.phase == "knockout"
    assert len(cup.pending_matches()) == size // 2
    play_all(cup)
    assert cup.phase == "finished"
    assert len(cup.rounds) == rounds
    assert cup.rounds[-1].name == "Final"
    assert cup.champion in cup.players
    # o campeão venceu todas as rodadas
    for rnd in cup.rounds:
        assert any(m.winner() == cup.champion for m in rnd.matches)


def test_nomes_das_rodadas_copa_32():
    cup = full_cup(32)
    cup.draw()
    play_all(cup)
    assert [r.name for r in cup.rounds] == [
        "Primeira fase",
        "Oitavas de final",
        "Quartas de final",
        "Semifinal",
        "Final",
    ]


def test_mata_mata_nao_aceita_empate_sem_penaltis():
    cup = full_cup(8)
    cup.draw()
    m = cup.pending_matches()[0]
    with pytest.raises(CupError):
        cup.report(m.id, MatchResult(1, 1))
    cup.report(m.id, MatchResult(1, 1, home_penalties=5, away_penalties=4))
    assert m.winner() == m.home


def test_resultado_duplicado_ou_inexistente():
    cup = full_cup(8)
    cup.draw()
    m = cup.pending_matches()[0]
    cup.report(m.id, MatchResult(2, 0))
    with pytest.raises(CupError):
        cup.report(m.id, MatchResult(3, 0))
    with pytest.raises(CupError):
        cup.report("M999", MatchResult(1, 0))


def test_proxima_rodada_so_abre_com_a_atual_completa():
    cup = full_cup(8)
    cup.draw()
    first = cup.pending_matches()
    for m in first[:-1]:
        cup.report(m.id, MatchResult(1, 0))
    assert len(cup.rounds) == 1
    cup.report(first[-1].id, MatchResult(0, 2))
    assert len(cup.rounds) == 2
    assert len(cup.pending_matches()) == 2


def test_sorteio_reproduzivel_com_seed():
    a, b = full_cup(16, seed=99), full_cup(16, seed=99)
    a.draw()
    b.draw()
    ids = lambda c: [(m.home.id, m.away.id) for m in c.pending_matches()]
    assert ids(a) == ids(b)


def test_potes_no_mata_mata_separam_cabecas_de_chave():
    cup = full_cup(8)
    seeds = ["p0", "p1", "p2", "p3"]
    cup.draw(pots=[seeds, ["p4", "p5", "p6", "p7"]])
    for m in cup.pending_matches():
        assert (m.home.id in seeds) != (m.away.id in seeds)


def test_potes_invalidos():
    cup = full_cup(8)
    with pytest.raises(CupError):
        cup.draw(pots=[["p0", "p1"], ["p2", "p3"]])  # faltam jogadores
    with pytest.raises(CupError):
        cup.draw(pots=[["p0", "p1", "p2"], ["p3", "p4", "p5", "p6", "p7"]])  # tamanhos


# ---------------------------------------------------------------- grupos
@pytest.mark.parametrize("size", [8, 16, 32])
def test_copa_com_grupos_completa(size):
    cup = full_cup(size, with_groups=True)
    cup.draw()
    assert cup.phase == "groups"
    assert len(cup.groups) == size // 4
    for g in cup.groups:
        assert len(g.players) == 4
        assert len(g.matches) == 6  # todos contra todos, turno único
        assert {m.round for m in g.matches} == {1, 2, 3}
    play_all(cup)
    assert cup.phase == "finished"
    assert cup.champion in cup.players


def test_grupos_nao_aceitam_penaltis():
    cup = full_cup(8, with_groups=True)
    cup.draw()
    m = cup.pending_matches()[0]
    with pytest.raises(CupError):
        cup.report(m.id, MatchResult(0, 0, 4, 3))
    cup.report(m.id, MatchResult(0, 0))  # empate vale na fase de grupos


def test_cruzamento_dos_grupos():
    cup = full_cup(16, with_groups=True)
    cup.draw()
    rng = random.Random(3)
    for m in list(cup.pending_matches()):
        cup.report(m.id, random_result(rng, knockout=False))
    assert cup.phase == "knockout"
    first = {g.name: g.standings()[0].player for g in cup.groups}
    second = {g.name: g.standings()[1].player for g in cup.groups}
    pairs = [(m.home, m.away) for m in cup.rounds[0].matches]
    assert pairs == [
        (first["A"], second["B"]),
        (first["B"], second["A"]),
        (first["C"], second["D"]),
        (first["D"], second["C"]),
    ]
    assert cup.rounds[0].name == "Quartas de final"


def test_potes_com_grupos_um_cabeca_por_grupo():
    cup = full_cup(16, with_groups=True)
    pots = [[f"p{i}" for i in range(k, k + 4)] for k in range(0, 16, 4)]
    cup.draw(pots=pots)
    for g in cup.groups:
        for pot in pots:
            assert sum(p.id in pot for p in g.players) == 1


def test_consulta_de_grupo():
    cup = full_cup(8, with_groups=True)
    cup.draw()
    assert cup.group("A").name == "A"
    with pytest.raises(CupError):
        cup.group("Z")
    assert cup.champion is None
