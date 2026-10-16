# PES 6 Online

[![CI](https://github.com/Broettogodinho/pes6-online/actions/workflows/ci.yml/badge.svg)](https://github.com/Broettogodinho/pes6-online/actions/workflows/ci.yml)

A **Copa nativa** do Pro Evolution Soccer 6 (PC) jogada por amigos remotos como em 2005: todos na mesma sala, um único torneio e o controle passando para quem joga a próxima partida.

- **Fase 1 (atual): sofá virtual.** Um anfitrião roda a instalação padrão do PES 6 ([versão-alvo](docs/stack-pc.md)) com a **Copa nativa** e vários jogadores humanos; os amigos entram via Parsec, todos assistem e o controle passa para os 2 jogadores da vez ([ADR-002](docs/adr/ADR-002-sofa-virtual-parsec.md), [protocolo de testes](docs/poc-parsec.md)).
- **Fase 2: netplay no navegador**, se a Fase 1 for aprovada: cada jogador roda o jogo no navegador e só os comandos dos controles trafegam.
- **Trilha paralela: engenharia reversa e mods** (`re/`, `patch/`). DLL proxy com hooks validados por assinatura, sem alterar arquivos do jogo em disco. Primeiro mod: suporte a mais controles na Copa.
- **Ordem do roadmap:** engenharia reversa ∥ Parsec → torneio local (mod) → netplay → torneio online nativo. Cada etapa tem um plano B que volta ao básico ([roadmap](docs/roadmap.md)).

Backlog e planejamento: Jira, projeto **PES 6 Online** (chave `SCRUM`).
Documentação navegável: [Confluence · PES 6 Online](https://admdasantas.atlassian.net/wiki/spaces/PES6/overview) (espelho; a fonte da verdade é `docs/` neste repositório).

## ⚠️ Aviso legal e Conformidade Arquitetural

Este projeto é desenvolvido sem fins lucrativos, destinado puramente à preservação técnica e diversão dentro de um círculo privado de usuários. O jogo-alvo não está mais disponível em lojas oficiais (abandonware). 

Este repositório de código **não distribui o jogo**[cite: 3]. Nenhum executável original, DLL, asset (`.afs`, `.img`), option file ou save do PES 6 faz parte deste repositório, e nunca deve fazer[cite: 3]. 

**Arquitetura Web (Fase 2):** A versão web atua estritamente como um framework de execução em memória via WebAssembly (WASM). A plataforma apresenta uma tela de seleção que aponta e carrega os recursos necessários diretamente na memória (RAM) do navegador do visitante sob demanda[cite: 3]. A arquitetura é desenhada exclusivamente para execução online do motor, não permitindo ou distribuindo os arquivos originais da Konami para download local no computador do usuário[cite: 3].

Este repositório contém apenas código original: o patch (DLL de hooks), o servidor de torneio, o portal web e a documentação de engenharia reversa[cite: 3]. Pro Evolution Soccer é uma marca registrada da Konami. Este projeto não possui vínculos, patrocínios ou afiliações com a Konami[cite: 3].

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
