# PES 6 Online

[![CI](https://github.com/Broettogodinho/pes6-online/actions/workflows/ci.yml/badge.svg)](https://github.com/Broettogodinho/pes6-online/actions/workflows/ci.yml)

Modo **Copa Online** e **Liga Online** dentro do Pro Evolution Soccer 6 (PC), com servidor de torneio próprio.

- **Fase 1 (atual):** PC — patch em DLL sobre o `pes6.exe` + servidor baseado no Sixserver.
- **Fase 2:** levar o PES 6 Online ao navegador (estratégia definida por PoC).

Backlog e planejamento: Jira, projeto **PES 6 Online** (chave `SCRUM`).

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
