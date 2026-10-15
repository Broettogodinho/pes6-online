# ADR-001 — Base do jogo: PES 6 PC, navegador na Fase 2

- **Status:** aprovado por Bruno em 2026-10-15 — SCRUM-15
- **Data:** 2026-10-15

## Contexto

Queremos Copa Online e Liga Online dentro do PES 6, com sala única e jogadores remotos. Havia três versões candidatas como base: PSP, PS2 e PC.

## Matriz de decisão (1 = pior, 5 = melhor)

| Critério | PSP | PS2 | PC |
|---|:-:|:-:|:-:|
| Copa/Liga com vários humanos | 1 | 5 | 5 |
| Online existente reaproveitável | 2 | 1 | 5 |
| Ferramentas de engenharia reversa | 2 | 1 | 5 |
| Modificar sem decompilar tudo | 2 | 1 | 5 |
| Comunidade de mods | 2 | 3 | 5 |
| Caminho até o navegador | 5 | 2 | 2 |
| **Total** | **14** | **13** | **27** |

## Decisão

Usar o **PES 6 PC (`pes6.exe`)** como base. O torneio é implementado como:
- **servidor:** fork do Sixserver (servidor online open source do PES 6 PC) + módulo de torneio;
- **jogo:** DLL de patch com hooks nas funções de Copa, Liga, online e menus.

O navegador fica para a **Fase 2**, com a estratégia escolhida por PoC (emulação x86 em WASM, recompilação ou streaming).

## Consequências

- Não é preciso decompilar o jogo inteiro: análise seletiva + hooks.
- O caminho crítico passa a ser a engenharia reversa das funções-alvo (F4) e o patch (F5).
- Os endereços dos hooks ficam presos a uma versão específica do `pes6.exe` (hash em `re/symbols/target.toml`).

## Riscos

| Risco | Mitigação |
|---|---|
| Rodar o .exe no navegador pode ser lento ou caro | Isolado na Fase 2; acompanhar a comunidade; streaming como plano B |
| Código antigo do Sixserver (Python 2, segurança) | SCRUM-44: modernização e revisão de segurança |
| Versões diferentes do pes6.exe entre jogadores | Patch valida hash e assinaturas; instalador recusa versões não suportadas |
| Anticheat/antivírus estranhar a DLL | Instalador transparente, código aberto, sem ofuscação |
