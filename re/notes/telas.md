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
