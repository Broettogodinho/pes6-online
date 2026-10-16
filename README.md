# PES 6 Online

[![CI](https://github.com/Broettogodinho/pes6-online/actions/workflows/ci.yml/badge.svg)](https://github.com/Broettogodinho/pes6-online/actions/workflows/ci.yml)

A **Copa nativa** do Pro Evolution Soccer 6 (PC) jogada por amigos remotos como em 2005: todos na mesma sala, um único torneio e o controle passando para quem joga a próxima partida.

- **Fase 1 (atual): sofá virtual.** Um anfitrião roda a instalação padrão do PES 6 ([versão-alvo](docs/stack-pc.md)) com a **Copa nativa** e vários jogadores humanos; os amigos entram via Parsec, todos assistem e o controle passa para os 2 jogadores da vez ([ADR-002](docs/adr/ADR-002-sofa-virtual-parsec.md), [protocolo de testes](docs/poc-parsec.md)).
- **Fase 2: netplay no navegador**, se a Fase 1 for aprovada: cada jogador roda o jogo no navegador e só os comandos dos controles trafegam.
- **Trilha paralela: engenharia reversa e mods** (`re/`, `patch/`). DLL proxy com hooks validados por assinatura, sem alterar arquivos do jogo em disco. Primeiro mod: suporte a mais controles na Copa.
- **Ordem do roadmap:** engenharia reversa ∥ Parsec → torneio local (mod) → netplay → torneio online nativo. Cada etapa tem um plano B que volta ao básico ([roadmap](docs/roadmap.md)).

Backlog e planejamento: Jira, projeto **PES 6 Online** (chave `SCRUM`).
Documentação navegável: [Confluence · PES 6 Online](https://admdasantas.atlassian.net/wiki/spaces/PES6/overview) (espelho; a fonte da verdade é `docs/` neste repositório).

## ⚠️ Aviso legal

Este projeto **não distribui o jogo**. Nenhum executável, DLL original, asset (`.afs`, `.img`), option file ou save do PES 6 faz parte deste repositório, e nunca deve fazer.

Cada jogador usa a **própria cópia legítima** do PES 6 PC. Este repositório contém apenas código original: o patch (DLL de hooks), o servidor de torneio, o portal web e documentação de engenharia reversa (nomes de funções, endereços e anotações em texto).

Pro Evolution Soccer é marca da Konami. Este projeto não é afiliado nem endossado pela Konami.

## Estrutura

| Pasta | Conteúdo |
|---|---|
| `patch/` | DLL de hooks carregada no `pes6.exe` (C++) |
| `server/` | Servidor de torneio (Copa e Liga) — fork do Sixserver + módulo de torneio (Python) |
| `web/` | Portal do torneio: lobby, inscrição, chaveamento, tabela |
| `re/` | Engenharia reversa: símbolos, assinaturas e anotações (texto, sem binários) |
| `tools/` | Scripts auxiliares (auditoria, agente, relatórios) |
| `docs/` | Documentação e ADRs |

## Ambiente de desenvolvimento

- **Windows 10/11** com a sua cópia do PES 6 PC instalada **fora** deste repositório
- VS Code + Visual Studio Build Tools (C++, x86)
- CMake 3.24+
- Python 3.11+
- Node 22+ (portal web e REA)
- Ghidra (Java 21) e REA (`npx rea-agents setup`) para engenharia reversa

### Servidor de torneio

```bash
cd server
python -m venv .venv && source .venv/bin/activate   # Windows: .venv\Scripts\activate
pip install -e ".[dev]"
pytest
```

### Patch (DLL)

```bash
cmake -S patch -B build/patch -A Win32
cmake --build build/patch --config Release
```

## Regras para agentes de IA

Veja [`AGENTS.md`](AGENTS.md).
