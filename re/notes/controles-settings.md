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
- **Resultado:** aguardando o teste.

## Correção pretendida (se a hipótese se confirmar)

A DLL proxy apresenta cada controle ao jogo com um **GUID estável** (derivado do produto e da ordem), mapeando-o para o dispositivo real em `CreateDevice`. Assim o GUID salvo no `settings.dat` sempre volta a existir. Base do mod de controles (SCRUM-57).
