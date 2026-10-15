# Roadmap e planos B

Ordem acordada em 2026-10-15:

```
engenharia reversa ∥ Parsec  →  torneio local (mod)  →  netplay  →  torneio online nativo
```

Princípio: **nenhuma ideia é descartada.** Cada etapa tem um plano B que volta ao básico; o que foi mapeado ou escrito fica no repositório para ser retomado depois.

## Etapas

| # | Etapa | Épico | Filas | Entrega |
|---|---|---|---|---|
| 1 | Engenharia reversa e mods (paralela) | SCRUM-8 | F3–F4 | Sistema de entrada mapeado, DLL de mods, mod de mais controles |
| 1 | Sofá virtual com Parsec (paralela) | SCRUM-7 | F2–F4 | Copa nativa com 8 humanos via Parsec; ADR-003 |
| 2 | Torneio local por mod | SCRUM-59 | F5 (26/11 → 24/12) | Item "Torneio" no menu, Copa criada por código, jogado via Parsec |
| 3 | Netplay | SCRUM-11 / SCRUM-12 | F6–F9 (24/12 → 29/04/2027) | Cada jogador com a própria cópia; só os controles trafegam |
| 4 | Torneio online nativo | SCRUM-58 (+ guardados: SCRUM-9 servidor, SCRUM-10 patch) | Após F9 | Item "Torneio Online", participantes da sala, servidor como autoridade |

## Planos B (voltar ao básico)

| Se falhar… | Voltamos para… | O que se aproveita |
|---|---|---|
| Mod de mais controles | Controles nativos do jogo + troca de controle pelo Parsec | Mapeamento do sistema de entrada |
| Torneio local por mod | Copa nativa do jogo via Parsec (Fase 1) | Mapeamento de menus e Copa; item de menu, se só a criação por código falhar |
| Netplay no navegador | Netplay nativo no Windows (DLL lockstep sobre o `pes6.exe`) ou anfitrião em nuvem com Parsec | Lockstep e sala de rede |
| Lockstep (dessincronização) | Anfitrião central transmitindo vídeo (Parsec ou similar) | Sala, espectadores e troca de controle |
| Torneio online nativo | Torneio controlado pelo servidor/portal (engine em `server/`) com partidas online avulsas; ou torneio local via Parsec | Engine de Copa, item de menu, rede |

## Ideias guardadas para depois

- Servidor de torneio próprio a partir do Sixserver/Fiveserver (fluxo online do PC, SCRUM-21).
- Leitor/escritor de save da competição (SCRUM-22).
- Modo Liga online (mesma base do torneio).
- Portal web do torneio (`web/`): lobby, inscrição, chaveamento e tabela.
