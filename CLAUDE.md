# CLAUDE.md

Siga integralmente as regras de [`AGENTS.md`](AGENTS.md).

Atalhos úteis:
- Auditoria anti-arquivos do jogo: `python tools/audit_game_files.py`
- Testes do servidor: `cd server && pytest`
- Build da DLL (Windows): `cmake -S patch -B build/patch -A Win32 && cmake --build build/patch --config Release`
