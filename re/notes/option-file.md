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

## Execução (tentativas 7–10, 2026-10-27, testes automáticos sem o Bruno)

| Tentativa | Resultado |
|---|---|
| 7. Sonda de memória (DLL `mem_probe`): procura um trecho do option file decifrado na memória e grava uma cópia | Achou só os **dados padrão** embutidos no jogo: o jogo ainda não tinha lido o arquivo |
| 8. Rastreio de `CreateFile`/`ReadFile` | Nada em 30 s: **CONFIRMADO** que o jogo só carrega o option file **depois do "PRESS START"** |
| 9. Injeção de teclas por roteiro (`pes6online_inject.txt`, Enter aos 6 s) | **CONFIRMADO:** passou da tela inicial; o jogo abriu `…/save/folder1/KONAMI-WIN32PES6OPT` 2 vezes e leu 1.191.936 bytes de uma vez (buffers `0x2EE2A020`, `0x2EE25020`) |
| 10. Cópias decifradas na memória (2, iguais entre si em 99,8%) × arquivo | Chave efetiva (cifrado XOR decifrado) periódica a cada 256 bytes em **99,8%** nos bytes 2 e 3 de cada palavra, **89%** no byte 1 e **1,4%** no byte 0. Nem XOR nem soma de 32 bits fecham |

**Pilha da abertura:** `0x044744BC` (CRT) < **`0x005FECE9`** (jogo) < `0x0448EF36` < `0x0041C12A`.
**Pilha da leitura:** `0x04474DB3` < `0x00423EC1` (CRT `fread`) < `0x0447DE54` < `0x04491F30` < `0x0448D380`.
Os quadros em `0x044…` ficam na seção extra `.rld` (código reorganizado). Criar funções nesses endereços de retorno gera fragmentos: falta localizar o início real das funções.

**INFERIDO:** os bytes 2–3 usam XOR com chave de 256 bytes; os bytes 0–1 têm um componente que depende de algo além da posição (encadeamento ou contador). A regra exata ainda não é conhecida.

**Consequência prática:** para ler o estado (times, competição), dá para usar a **cópia já decifrada na memória** pela DLL, sem implementar a cifra. Para gravar um save, a alternativa é chamar a rotina de salvar do próprio jogo.

## Próximos caminhos (para retomar)

1. **Dinâmico (melhor):** com a DLL, observar `ReadFile` do `KONAMI-WIN32PES6OPT` e o buffer logo depois. A memória já decifrada dá o texto puro, e comparar com o arquivo dá a chave completa, sem adivinhar. Teste automático possível (abrir o jogo, ler log, fechar).
2. **Estático:** subir dos `CreateFile`/`ReadFile` do jogo até a função que trata o buffer lido.
3. Gerar um `KONAMI-WIN32PES6000` (salvar uma Copa) para mapear o save de competição.
