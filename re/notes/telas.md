# Sistema de telas (cenas) do pes6.exe (SCRUM-23)

- **Versão-alvo:** sha256 `c3e8176e…ebc568`.
- **Como foi obtido:** projeto Ghidra `D:\RE\pes6`, scripts `Decompile`, `XrefsTo` e `MakeSignature` (2026-10-16), a partir da máquina de estados achada pela sonda de `FUN_00864ac0` (ver `FUN_00864ac0.md`).

## Peças

| Função | Endereço | Assinatura | Resumo do pseudocódigo | Papel | Situação |
|---|---|---|---|---|---|
| `FUN_00666a20` | `0x00666A20` | `A1 ?? ?? ?? ?? 85 C0 74 06 8B 4C 24 04 89 08 C3 A1 ?? ?? ?? ??` | `if (cena) cena[0] = id` | troca a tela atual (`Scene_Set`) | INFERIDO; `cdecl`, 1 arg (CONFIRMADO nos bytes) |
| `FUN_00666a30` | `0x00666A30` | — (curta demais; assinatura não única) | `return cena ? cena[0] : 0` | lê a tela atual (`Scene_Get`) | INFERIDO |
| `FUN_00666a40` | `0x00666A40` | `A1 ?? ?? ?? ?? 85 C0 74 07 8B 4C 24 04 89 48 04` | `if (cena) cena[4] = passo` | avança o passo da tela (`Scene_SetStep`); ~100 chamadores | INFERIDO |
| `FUN_0068d260` | `0x0068D260` | `56 83 CE FF E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 F8 49` | `switch (Scene_Get())` com um tratador por tela | despachante de telas | INFERIDO |
| `FUN_00864c50` | `0x00864C50` | — | espera 6 carregamentos (`DAT_01131fbc[0..5]`) e devolve 1 quando terminam | espera de recursos (`Load_WaitPending`) | INFERIDO |

- **Objeto da tela atual:** ponteiro global `PTR_DAT_00bd7c7c` (232 referências em 205 funções). `+0` = id da tela, `+4` = passo dentro da tela.
- **Quantidade de telas:** a assinatura do despachante contém `cmp eax, 0x49`. **INFERIDO:** ids de 0 a 0x49 (74 telas).
- **Global muito usada:** `DAT_03be12c9` (604 referências em 407 funções), comparada com `0x0e` na tela 0. **INFERIDO:** modo de jogo atual.

## Tela 0: `FUN_006be3c0`

Máquina de estados de carregamento que roda ao entrar em competição (Copa e Liga, CONFIRMADO pela sonda): casos 0 → 1 → 2 → 3 → 4 → 5 → 7, avançando com `FUN_00666a40`. O caso 2 espera `FUN_00864c50` e registra as 4 tabelas fixas via `FUN_006b05b0`.

## Tratadores conhecidos (casos do despachante)

| id | Tratador |
|---|---|
| 0x00 | `FUN_006be3c0` |
| 0x01 | `FUN_006bcb90` |
| 0x02 | `FUN_006bce60` |
| 0x03 | `FUN_006bd0a0` |
| … | um por tela (lista completa em `D:\RE\out\decomp-5.txt`) |
| 0x1f | `FUN_006bcb00` |

O que cada id significa (menu principal, Copa, Liga, opções…) depende do teste no jogo.

## Tentativa 1: sonda de telas (2026-10-16)

- **Hipótese:** registrar cada `FUN_00666a20(id)` (tela anterior → nova, quem pediu), enquanto o Bruno navega anotando o que abriu, produz o mapa id → tela.
- **Como:** hook de observação em `patch/src/probes.cpp` (DLL v0.4.0); registra até 500 trocas.
- **Resultado (2026-10-16):** o Bruno navegou por menu principal → partida → Copa → Liga → Master League → opções → edit, voltando ao menu entre cada uma. A sonda instalou (assinatura validada), mas registrou **0 chamadas** a `FUN_00666a20`.
- **Conclusão (INFERIDO):** o despachante de 74 telas não comanda a navegação dos menus. Provavelmente comanda o fluxo de jogo (carregamento, partida…), e os menus usam outro mecanismo ou gravam o estado sem passar pelo setter.

## Tentativa 2: observar a memória (2026-10-16)

- **Hipótese:** observar o estado direto na memória pega mudanças venham de onde vierem.
- **Como:** DLL v0.5.0, com uma thread que lê a cada 100 ms `PTR_DAT_00bd7c7c` (objeto, id da tela `+0`, passo `+4`) e `DAT_03be12c9` (modo) e registra cada mudança.
- **Resultado (2026-10-16, sessão 15:28; ordem: partida → Master League → Copa → Liga → edit → opções):** o estado **não mudou nenhuma vez** (`tela=0 passo=0 modo=0x00`, objeto `0x03C8AA40`).
- **Conclusão (INFERIDO):** o objeto de tela em `PTR_DAT_00bd7c7c` e `DAT_03be12c9` não participam da navegação dos menus; servem ao fluxo de partida e competição. Os menus usam outra estrutura, ainda a localizar (próximo: comparar a memória gravável entre telas).

## Tentativa 3: busca por comparação de memória (2026-10-27): cursor do menu principal CONFIRMADO

- **Hipótese:** o cursor do menu principal é um byte da `.data` que sobe 1 a cada seta para baixo e desce 1 a cada seta para cima.
- **Como:** DLL v0.10.0 com `mem_diff` (`patch/src/mem_diff.cpp`). Fotos da `.data` (`0x00BC4000`, `0x03195000` bytes) em momentos do roteiro de teclas (`pes6online_inject.txt`), mantendo só os bytes que seguiram a relação esperada:
  - teclas: Enter aos 6 s e 27 s (abertura e título), ↓ aos 37 s e 40 s, ↑ aos 43 s e 48 s;
  - fotos: `base` 36 s, `inc` 39 s, `inc` 42 s, `dec` 45 s, `same` 47 s, `dec` 50 s.
- **Resultado:** **1 candidato, `0x03A70CA8`**. Captura de tela aos 42 s com o cursor em LEAGUE (valor 2). **CONFIRMADO** (teste automático, sem ninguém no PC).
- **Itens do menu principal (valor do cursor):** 0 MATCH, 1 MASTER LEAGUE, 2 LEAGUE, 3 CUP, 4 TRAINING, 5 EDIT, 6 OPTIONS, 7 INTERNATIONAL CHALLENGE, 8 RANDOM SELECTION MATCH, 9 NETWORK, 10 EXIT. Ordem CONFIRMADA na tela; valores 0–3 CONFIRMADOS pela busca, os demais INFERIDOS pela ordem.
- **Xrefs de `0x03A70CA8`:** 43 referências, todas no bloco `0x009C1xxx`–`0x009EBxxx` (`FUN_009c1340`, `009c1800`, `009c1ac0`, `009c3090`, `009d72d0`, `009d83f0`…`009d9f20`, `009ebac0`).

## Cadeia do menu principal (leitura no Ghidra, INFERIDO salvo indicação)

| Função | Endereço | Assinatura (única) | Resumo | Proposta |
|---|---|---|---|---|
| `FUN_00661550` | `0x00661550` | `A1 ?? ?? ?? ?? 83 F8 65 0F 87 ?? ?? ?? ?? 0F B6 80 ?? ?? ?? ??` | `switch (DAT_03becc60)` com 18 casos (0–0xF, 100, 0x65); caso **4** = `FUN_009c1a30` (menus). Incrementa 3 contadores de quadro. Chamada por `FUN_00411490` | `App_RunMode` |
| `FUN_009c1a30` | `0x009C1A30` | `0F BE 05 ?? ?? ?? ?? 83 E8 00 74 1D 48 74 6A 48` | estado `DAT_03a70cac` 0 → 1 → 2; no estado 2 chama `FUN_009c1800` | `Menu_Run` |
| `FUN_009c1800` | `0x009C1800` | `0F BF 05 ?? ?? ?? ?? 83 F8 08 77 0C FF 24 85 ?? ?? ?? ??` | `switch (DAT_03a79ad0)`; caso 2 chama `FUN_009d8310` (2 → estado 7; 0xB → sai). Casos 3/4/6 chamam `FUN_00661190(0, N)` com N = 7, 6, 0xF, 3 | `Menu_Update` |
| `FUN_009d8310` | `0x009D8310` | `0F BF 05 ?? ?? ?? ?? 83 F8 0E 0F 87 ?? ?? ?? ??` | `switch (DAT_03a79ad2)` (0–0xE): 0 = abre o menu principal (`FUN_009d9f20`), 1 = confirma a escolha (`FUN_009d72d0`), 4…0xD = uma função por modo escolhido | `MainMenu_Update` |
| `FUN_009d72d0` | `0x009D72D0` | `8B 0D ?? ?? ?? ?? 83 F9 08 8B 15 ?? ?? ?? ?? B8 01 00 00 00` | converte o cursor em modo de jogo e no próximo subestado (tabela abaixo) | `MainMenu_ConfirmItem` |
| `FUN_00661190` | `0x00661190` | — (17 bytes) | salto indireto por `_DAT_04465a7c` (alvo preenchido em execução; seção `.rld`). 18 chamadores, entre eles o menu | `App_SetMode`? a confirmar |
| `FUN_009c3090` | `0x009C3090` | — | `return cursor == 8` | — |

### `FUN_009d72d0`: cursor → modo (`DAT_03be12c9`) → subestado (`DAT_03a79ad2`)

| Cursor | Item | `DAT_03be12c9` | `DAT_03a79ad2` |
|---|---|---|---|
| 0 | MATCH | 0 (sub-escolha `DAT_03a79b44` 0–2) ou 7 (sub-escolha 3) | 8 (sub-escolha 1–2) ou 5 |
| 1 | MASTER LEAGUE | 4 | 0xC |
| 2 | LEAGUE | 1 | 0xB |
| 3 | CUP | **2** | 0xA |
| 4 | TRAINING | 5 | 0xD |
| 6 | OPTIONS | 6 | 0xE |
| 7 | INTERNATIONAL CHALLENGE | 0xE | 9 |
| 8 | RANDOM SELECTION MATCH | 0 | 7 |
| 5, 9, 10 | EDIT, NETWORK, EXIT | não tratados aqui (outro caminho) | — |

- Se `DAT_03be6324 == 1`, o subestado vai sempre para 4 (INFERIDO: modo especial, talvez demonstração ou rede).
- **`DAT_03be12c9` = modo de jogo**, agora com valores: 0 partida, 1 liga, 2 copa (CONFIRMADO), 4 Master League, 5 treino, 6 opções, 7 partida (variante), 0xE International Challenge. INFERIDO pelo código.
- **Teste automático (2026-10-27, DLL v0.11.0): CUP → modo 2 CONFIRMADO.**
  - Roteiro: Enter, Enter (título), ↓↓↓ até CUP, Enter → tela **MODE SELECT** (New Game / Load Data, cursor em Load Data), ↑, Enter em New Game.
  - A sonda de estado registrou `modo=0x02` cerca de 52 s após o início, logo depois do Enter em New Game; na tela MODE SELECT o modo ainda era 0.
  - Bytes conferidos no executável: `FUN_009d72d0` grava o byte `0x03BE12C9` (`mov byte [0x03BE12C9], imm8` em `0x9D72FB`…`0x9D7338`).
  - **Conclusão:** o modo é gravado só ao confirmar o submenu (New Game), não ao escolher o item no menu principal. Isso explica a Tentativa 2 se a navegação daquela sessão voltou antes da confirmação (INFERIDO).

## Consequências para o mod (SCRUM-58/59)

- **Item "Torneio Online" no menu:** o cursor tem 11 posições, e `FUN_009d72d0` decide o destino de cada uma. Caminhos possíveis:
  1. trocar o texto de um item pouco usado (ex.: NETWORK, valor 9) via tabela de textos (SCRUM-63) e desviar o tratamento dele para o nosso fluxo;
  2. hook em `FUN_009d72d0` (assinatura acima, com trampolim): quando o cursor for o item escolhido, ligar o nosso modo e seguir pelo caminho da Copa (`modo = 2`).
- **Ainda falta:** onde fica a lista de itens (quantidade 11 e textos) e como desenhá-los, para acrescentar um 12º item em vez de reaproveitar um existente.
