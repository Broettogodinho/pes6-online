# Importações do pes6.exe (SCRUM-18 / SCRUM-24 / SCRUM-56)

- **Versão-alvo:** sha256 `c3e8176e…ebc568` (`re/symbols/target.toml`).
- **Como foi obtido:** projeto Ghidra 12.1.4 em `D:\RE\pes6` (fora do repositório), análise automática completa (766 s), script `tools/ghidra/ExportImports.java` em 2026-10-16.
- **Legenda:** "fato do arquivo" = lido na tabela de importação, sem depender de interpretação; **INFERIDO** = conclusão sobre comportamento, ainda não testada em depurador.

## Resumo

| DLL | Nº de funções importadas |
|---|---|
| KERNEL32 | 130 |
| USER32 | 56 |
| WS2_32 | 24 |
| OLE32, IPHLPAPI, ADVAPI32 | 6 cada |
| WINMM | 5 |
| SHELL32 | 3 |
| GDI32, **DINPUT8**, **D3D8** | 1 cada |

## Entrada (SCRUM-56)

| Importação | Refs | Quem usa | Situação |
|---|---|---|---|
| `DINPUT8!DirectInput8Create` | 1 (só a entrada da IAT) | Nenhuma função analisada referencia a IAT diretamente | Fato do arquivo. **INFERIDO:** a chamada é indireta ou parte das seções extras (`age`, `quod`, `.rld`); localizar no depurador |
| `USER32!GetAsyncKeyState` | 3 | `FUN_006608f0` | Fato do arquivo. **INFERIDO:** leitura de teclado |
| `USER32!GetKeyState` | 7 | — | Fato do arquivo |
| XInput | — | — | **Não importado** (fato do arquivo). Controles chegam pelo DirectInput 8 |

`FUN_006608f0` começa com uma tabela indexada por código de tecla (`sub` de 0x10, limite 0x95). **INFERIDO:** tradução tecla → ação.

## Gráficos

| Importação | Quem usa | Situação |
|---|---|---|
| `D3D8!Direct3DCreate8` | `FUN_00404c60` (e o thunk em `0042c050`) | Fato do arquivo: o jogo usa **Direct3D 8**, não 9. O dgVoodoo (`d3d8.dll`) traduz essas chamadas |

## Consequências

- O proxy da DLL de mods continua no `dinput8.dll`. O `d3d8.dll` e o `d3d9.dll` da pasta do jogo são do dgVoodoo.
- A correção "PES 6 PC usa Direct3D 9" foi feita em `docs/stack-pc.md` §4.
- A tradução gráfica da Fase 2 seria Direct3D 8 → WebGPU, não D3D9.

## Assinaturas de teste (validação do `MakeSignature.java`)

Funções ainda sem nome. Servem só para provar que as assinaturas saem únicas; não são alvos de hook.

| Entrada | Bytes | Ocorrências | Padrão |
|---|---|---|---|
| `0070eef0` | 16 | 1 | `81 EC 44 04 00 00 55 E8 ?? ?? ?? ?? 8B E8 85 ED` |
| `006608f0` | 21 | 1 | `8B 44 24 04 83 C0 F0 3D 95 00 00 00 77 47 0F B6 80 ?? ?? ?? ??` |
