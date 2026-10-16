# CLAUDE.md

@AGENTS.md

As regras acima (AGENTS.md) valem integralmente. Este arquivo só existe para que o Claude Code as carregue: com um CLAUDE.md presente, o AGENTS.md não é lido sozinho, por isso a importação `@AGENTS.md` na primeira linha.

## Prioridade do projeto

- **Engenharia reversa é a trilha principal** (épico SCRUM-8): define o que dá para fazer dentro do jogo.
- **Objetivo final:** um mod dentro do PES 6, com o modo **"Torneio Online" no menu do jogo** (SCRUM-58/59). O Parsec (SCRUM-7) é trilha de apoio.
- Ordem dos cards e cadeia da engenharia reversa: `docs/roadmap.md`.

## Atalhos

- Auditoria anti-arquivos do jogo (antes de todo commit): `python tools/audit_game_files.py`
- Testes do servidor: `cd server && pytest`
- Build da DLL (Windows, x86): `cmake -S patch -B build/patch -A Win32 && cmake --build build/patch --config Release`

## Ambiente local (Windows)

- Projeto em `D:\Documentos\projetos\pes6-online`; ambiente Python do servidor em `server\.venv`.
- A cópia do PES 6 fica **fora** do repositório.
- Ao concluir uma ação, atualize o card correspondente no Jira (projeto PES 6 Online, chave SCRUM).
