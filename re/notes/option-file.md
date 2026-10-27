# Option file e saves de competição (SCRUM-22)

- **Arquivos:** `Documentos\KONAMI\Pro Evolution Soccer 6\save\folder1\KONAMI-WIN32PES6OPT` (1.191.936 bytes). Backup em `D:\RE\backup\`; trabalhos de decifração em `D:\RE\opt\` (fora do repositório: são dados do jogo e do usuário).
- **Como foi obtido (2026-10-27):** leitura do arquivo e do `pes6.exe` (Ghidra: `FindStrings`, `Decompile`). Nada executado.

## Cifra (tentativas 1–4, pelos dados)

| Observação | Situação |
|---|---|
| Entropia ~7,9 bits/byte e nenhuma estrutura legível | CONFIRMADO (leitura): arquivo cifrado |
| Em regiões de dados com muitos zeros, 75% dos bytes se repetem a cada **256 bytes** (0% a 4–128) | CONFIRMADO: período de 256 |
| Decifrando com a chave estimada (byte mais comum por posição mod 256), aparecem trechos legíveis ("Stadio", "Stadium", "League -German", "Data"), com erros em 1 de cada 4 caracteres | CONFIRMADO |
| Os erros ficam no **byte baixo (lane 0) de cada palavra de 32 bits**; os bytes 1–3 da chave são estáveis em todas as regiões | CONFIRMADO |
| XOR de 32 bits = XOR por byte; soma de 32 bits dá resultado pior | CONFIRMADO: é XOR |
| Ajustar a lane 0 por contexto de letras piorou o resultado | hipótese descartada |
| A chave (lanes 1–3, com curinga na lane 0) **não** aparece literalmente no `pes6.exe` | CONFIRMADO: chave gerada por código ou armazenada de outra forma |
| Entre duas versões do arquivo (15:06 e 16:23 de 16/10), só **6 bytes** mudaram (`0x1410–0x1412`, `0x163E`, `0x173C–0x173D`) | CONFIRMADO: não há checksum global que mude junto, ou ele está nesses bytes |

**INFERIDO:** cifra XOR de período 256 com a lane 0 dependente de algo além da posição mod 256 (contador, bloco ou checksum), ou dados da lane 0 raramente nulos.

## Pelo código (tentativas 5–6)

Strings e funções de nome de arquivo (endereços da versão-alvo):

| String | Usada por | Situação |
|---|---|---|
| `KONAMI-WIN32PES6OPT` | `FUN_00b30890`, `FUN_00b308e0`, `FUN_00b30a50` | INFERIDO: devolvem o nome do arquivo |
| `KONAMI-WIN32PES6000` | `FUN_00b308a0`, `FUN_00b308f0` | INFERIDO: **outro arquivo de save** (Master League, Copa ou Liga?); relevante para o estado da competição |
| `KONAMI-WIN32PES6*` | `FUN_00b30770`, `FUN_00b30990` | INFERIDO: listagem dos saves |
| `KONAMI-WIN32PES5…` | `FUN_00b2d4b0`, `FUN_00b2d4f0` | resto do PES 5 no executável |

Chamadores (`FUN_00b19fb0`, `FUN_00b1a810`, `FUN_00b1bba0`, `FUN_00b21e70`, `FUN_00b38ac0`, `FUN_00b2bcd0`, `FUN_00b379b0`) são telas de salvar e carregar (cursor, estado de tela), sem laço de XOR. A decifração está mais fundo na cadeia.

## Próximos caminhos (para retomar)

1. **Dinâmico (melhor):** com a DLL, observar `ReadFile` do `KONAMI-WIN32PES6OPT` e o buffer logo depois. A memória já decifrada dá o texto puro, e comparar com o arquivo dá a chave completa, sem adivinhar. Teste automático possível (abrir o jogo, ler log, fechar).
2. **Estático:** subir dos `CreateFile`/`ReadFile` do jogo até a função que trata o buffer lido.
3. Gerar um `KONAMI-WIN32PES6000` (salvar uma Copa) para mapear o save de competição.
