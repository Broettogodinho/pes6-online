# Leitura da entrada (teclado e controles) no pes6.exe (SCRUM-56, base do netplay SCRUM-38)

- **Versão-alvo:** sha256 `c3e8176e…ebc568`.
- **Como foi obtido (2026-10-19):**
  1. DLL v0.9.0 em teste automático: o envoltório do `IDirectInputDevice8W` registrou o teclado aberto (`SetDataFormat` de 256 bytes; `SetCooperativeLevel` `0x6` = não exclusivo + primeiro plano) e o primeiro `GetDeviceState` com **retorno `0x0041748F`** (CONFIRMADO em execução).
  2. Ghidra (`D:\RE\pes6`): `Decompile`, `XrefsTo`, `MakeSignature` e leitura da tabela virtual no arquivo.

## Classes de entrada (tabelas virtuais em .rdata)

Duas classes irmãs com tabelas de 13 métodos, uma após a outra; os métodos 1 e 2 (`0x4172C0`, `0x419000`) são comuns (classe base).

| Posição | Teclado (`0x00B7E2D0`) | Controle (`0x00B7E304`) | Papel (INFERIDO) |
|---|---|---|---|
| 0 | `0x4187B0` | `0x418880` | destrutor (`__thiscall`, flag de liberar memória) |
| 1–2 | `0x4172C0`, `0x419000` | `0x4172C0`, `0x419000` | base comum |
| 12 | **`FUN_00417430`** | **`FUN_004176f0`** | ler o estado do dispositivo |

## Layout do objeto (`this`, convenção `__thiscall`/`__fastcall` com `ecx`)

| Campo | Conteúdo | Situação |
|---|---|---|
| `+0x460` | ponteiro `IDirectInputDevice8` | INFERIDO (usado como objeto COM: `+0x1C` Acquire, `+0x24` GetDeviceState, `+0x64` Poll) |
| `+0x464` | byte "dispositivo adquirido" | INFERIDO |
| `+0x46C` | estado bruto: 256 bytes (teclado) ou `DIJOYSTATE2` de 0x110 bytes (controle, leitura anterior) | INFERIDO |
| `+0x57C` | byte "primeira leitura" (força a tradução mesmo sem mudança) | INFERIDO |
| `+0x59C` | lista de "objetos de controle", registros de 0x30 bytes: `+0` valor traduzido, `+8` tipo, `+0x10` deslocamento no estado bruto | INFERIDO |
| `+0x1180` | quantidade de objetos | INFERIDO |
| `+0x216C` | limite usado no teclado para zerar teclas da tabela `DAT_00B7E280` | INFERIDO |

## Funções

### Leitura do teclado: `FUN_00417430` (proposta: `Input_KeyboardRead`)
- **Endereço:** `0x00417430` (287 bytes). **Assinatura:** `53 56 8B F1 8B 86 60 04 00 00 33 DB 3B C3 0F 84 ?? ?? ?? ??` (única).
- **Resumo:** se não adquirido, `Acquire`; `GetDeviceState(0x100, this+0x46C)`; zera as teclas listadas em `DAT_00B7E280` (18 códigos) abaixo de `this+0x216C`; em falha, zera o buffer e marca "não adquirido".
- **Situação:** chamada a partir de `0x0041748F` **CONFIRMADA** em execução; papel INFERIDO.

### Leitura do controle: `FUN_004176f0` (proposta: `Input_JoystickRead`)
- **Endereço:** `0x004176F0` (544 bytes; a análise automática não a criou, porque só é chamada pela tabela virtual). **Assinatura:** `81 EC 14 01 00 00 55 8B E9 8B 85 60 04 00 00 85 C0` (única).
- **Resumo:**
  1. zera os valores dos objetos;
  2. `Acquire` (se preciso) e `Poll`;
  3. `GetDeviceState(0x110)` (`DIJOYSTATE2`) em buffer local;
  4. se o estado é igual ao anterior (`this+0x46C`), sai;
  5. senão, percorre os objetos: tipo `1` = botão (valor de `DAT_00B7D6BC`/`DAT_00B82ED4`, provavelmente 1.0/0.0), tipos ímpares `3…0xF` = eixos (normalizados por `FUN_00417550`), tipo `0x13` = POV (expandido em 4 entradas por `thunk_FUN_044d3ee0`);
  6. em falha, marca "não adquirido".
- **Situação:** INFERIDO (leitura do código; ainda sem execução com controle conectado).

## Injeção de teclas (2026-10-27): CONFIRMADO

O envoltório do teclado (`dinput_device_wrap.inl`) aplica um roteiro opcional `pes6online_inject.txt` na pasta do jogo: uma linha por tecla, `"<ms desde a 1ª leitura> <código DIK hex> <duração ms>"`. Teste automático: Enter (`0x1C`) aos 6 s passou do "PRESS START" e o jogo carregou o option file.
- **Testes automáticos:** dá para navegar no jogo sem ninguém no PC.
- **Netplay:** demonstra a injeção no mesmo ponto em que entrará a entrada remota.

## Consequências

- **Netplay (SCRUM-38):** tudo o que o jogo sabe do controle vem do `GetDeviceState`, que passa pelo envoltório da nossa DLL. Ponto de captura e injeção sem alterar o jogo: capturar o `DIJOYSTATE2` local e, no lockstep, entregar ao jogo o estado do jogador remoto no mesmo quadro.
- **Mapeamento do `settings.dat`:** os 24 bytes por slot provavelmente são índices na lista de objetos (`+0x59C`). A confirmar.
- **Reconexão:** em falha de leitura o jogo marca "não adquirido" e tenta `Acquire` de novo na próxima leitura, mas no **mesmo** dispositivo. Se o controle volta como outro dispositivo, nunca é reaberto (INFERIDO; explica a perda do mapeamento na desconexão).
