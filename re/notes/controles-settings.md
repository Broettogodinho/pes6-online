# Mapeamento de controles: onde fica e por que se perde (SCRUM-56/57)

- **Relato do Bruno (2026-10-16):** o mapeamento de botões feito no jogo se perde quando o controle desconecta. Ele usa dois controles Xbox via Bluetooth ("Gamepad" e "Gamepad Laranja", VID 045E/PID 02E0) e um controle Xbox 360 USB.

## Onde o jogo guarda

| Arquivo | Conteúdo |
|---|---|
| `D:\Documentos\KONAMI\Pro Evolution Soccer 6\settings.dat` (420 bytes) | configuração do `settings.exe`: vídeo e **controles** (pasta Documentos do Windows = `D:\Documentos`) |
| `...\save\folder1\KONAMI-WIN32PES6OPT` | option file (dados e configurações de dentro do jogo) |

Backup dos dois em `D:\RE\backup\pes6-config-<data>` (fora do repositório; são dados do usuário).

## Estrutura observada do settings.dat (leitura do arquivo do Bruno)

| Offset | Conteúdo | Leitura |
|---|---|---|
| `0x10` | `{6F1D2B61-D5A0-11CF-BFC7-444553540000}` | `GUID_SysKeyboard` (teclado do DirectInput), fixo em qualquer PC |
| `0x20` | 24 bytes | mapeamento do teclado |
| `0x3C` | `{8282EDC0-0923-11F1-8002-444553540000}` | **GUID de instância** do controle no DirectInput, gerado pelo Windows por dispositivo |
| `0x4C` | `10 12 13 11 14 15 16 17 09 08 0E 0F 0D 0B 0A 0C 00 01 02 03 04 05 06 07` | **mapeamento dos botões** desse controle |
| `0x64` em diante | blocos `00…`/`FF…` | vagas de outros dispositivos, vazias |

## Hipótese

- **INFERIDO:** o jogo reencontra o controle pelo GUID de instância. Se, ao reconectar, o Windows apresentar o controle com outro GUID de instância, ou trocar a ordem dos dois controles Bluetooth, o jogo não acha o GUID salvo e volta ao padrão.

## Tentativa 1: diagnóstico (2026-10-16)

- **Como:** DLL v0.6.0 (`patch/src/input_watch.cpp`). Uma instância própria do DirectInput (não mexe nos objetos do jogo) lista os controles a cada 5 s e registra no log GUID de instância, GUID de produto e nome sempre que algo muda.
- **Teste:** com o jogo aberto, desconectar e reconectar um controle e conferir se o GUID de instância muda.
- **Resultado (2026-10-16, sessão 15:28):** o DirectInput listou **1 controle**: `Controller (XBOX 360 For Windows)`, produto `{10403537-…}` (VID 3537/PID 1040), instância `{8282EDC0-0923-11F1-8002-444553540000}`, a mesma gravada no `settings.dat`. Os dois controles Xbox Bluetooth não apareceram na enumeração. Nenhuma mudança registrada durante a sessão (checagem a cada 5 s).
- **Pendente:** saber qual controle foi desconectado e por quanto tempo, e em qual controle o mapeamento se perde. Checagem passou para 1 s.

## Evidência no registro (2026-10-16)

`HKCU\System\CurrentControlSet\Control\MediaProperties\PrivateProperties\DirectInput\VID_3537&PID_1040\Calibration` tem **3** GUIDs de instância para o mesmo modelo: `\0` `{8282C6B0-…-8001-…}`, `\1` `{8282EDC0-…-8002-…}` (o gravado no `settings.dat`) e `\2` `{828314D0-…-8003-…}`.
- **CONFIRMADO** (leitura do registro): o Windows já deu GUIDs de instância diferentes ao mesmo controle.
- **INFERIDO:** quando o controle volta como `…8001` ou `…8003`, o jogo não acha o `…8002` e perde o mapeamento.

## Correção implementada (DLL v0.7.0, commit `f554dd8`)

- `patch/src/guid_map.cpp` decide o GUID apresentado, sem depender do Windows, com testes em `tests/test_guid_map.cpp`. Um controle cujo GUID real não está salvo herda um GUID salvo **do mesmo modelo** (pelo histórico `Calibration\N`) que ainda não esteja em uso.
- `patch/src/dinput_wrap.cpp` envolve o `IDirectInput8A/W` que o jogo cria: `EnumDevices` entrega o GUID decidido; `CreateDevice`, `GetDeviceStatus` e `FindDevice` traduzem de volta para o real. Cada substituição é registrada no log.
- **Limite conhecido:** `IDirectInputDevice8::GetDeviceInfo` ainda devolve o GUID real. Se o jogo usar essa função para conferir o controle, será preciso envolver também o dispositivo.
- **Teste pendente:** ligar o controle em outra porta USB (para forçar outro GUID) e conferir no jogo se o mapeamento se mantém, com o log mostrando "GUID real … apresentado como …".

## Teste no jogo da correção (2026-10-16)

- **Achado:** o jogo só usa controles **cadastrados pelo `settings.exe`**. O controle Nintendo (057E/2009), não cadastrado, não aparecia para mapeamento (INFERIDO). O Bruno cadastrou o Xbox Bluetooth (045E/02E0) no `settings.exe`, que ficou no slot 2 do `settings.dat` (`{A7528520-74AE-11F0-8002-…}`). O registro mostra 2 GUIDs já dados a esse modelo (`A7528520`, `35CA4A70`).
- **Resultado:** com a DLL v0.7.0, ao **desconectar o Xbox Bluetooth com o jogo aberto**, o mapeamento se perde.
- **Hipótese (INFERIDO):** o jogo só enumera e abre os controles uma vez. Quando o dispositivo cai, ele não o reabre (a correção atua na enumeração, que não volta a acontecer). Prioridade secundária (Bruno): primeiro mapear como a leitura entra no jogo.

## Tentativa 2: observar a leitura (DLL v0.8.0)

`patch/src/dinput_device_wrap.inl` envolve o `IDirectInputDevice8W` de cada dispositivo aberto pelo jogo e registra:
- `SetDataFormat` (tamanho do estado), `SetCooperativeLevel`, `Acquire` e `Unacquire`;
- cada endereço de quem chama `GetDeviceState`/`GetDeviceData` (onde a entrada entra no jogo, para analisar no Ghidra);
- os erros (`DIERR_INPUTLOST`/`NOTACQUIRED` na desconexão);
- as mudanças de botões, POV e eixos;
- a frequência de leitura (leituras por segundo).
