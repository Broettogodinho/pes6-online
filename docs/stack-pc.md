# Stack PC do PES 6 e direção da Fase 2 (SCRUM-14)

- **Status:** pesquisa revisada em 2026-10-15; itens marcados **A VERIFICAR** só viram fato quando testados na F4/F5.
- **Origem:** pesquisa do Bruno + revisão técnica do agente.

Legenda: **CONFIRMADO** = fonte primária ou teste · **A VERIFICAR** = plausível, sem confirmação · **CORRIGIDO** = a pesquisa original continha erro.

## 1. Versão-alvo do `pes6.exe`

| Item | Situação |
|---|---|
| Versão-alvo | **DECIDIDO (Bruno, 2026-10-15): v1.00 original, sem patches.** Base "zerada"; todas as nossas mudanças ficam na DLL e no servidor. Compatibilidade do Kitserver 6 e do Sixserver com a 1.00 é **A VERIFICAR** na SCRUM-24/25. |
| Hash | **Será medido na cópia do Bruno** (SCRUM-24) e gravado em `re/symbols/target.toml`. |
| Executável "decrypted / No-CD" | **Não adotado.** Ver regra abaixo. |

**Regra:** o projeto parte de um executável **original, de cópia legítima**. Não distribuímos, não exigimos e não usamos como referência executáveis modificados para burlar proteção contra cópia. Se a proteção original atrapalhar o carregamento da DLL, a solução precisa preservar o executável (por exemplo, carregamento por DLL proxy, que não altera o `pes6.exe`).

## 2. Sixserver (online)

| Item | Situação |
|---|---|
| Arquitetura híbrida: servidor para login/lobby/salas, partida P2P | **A VERIFICAR** no protocolo do Sixserver (SCRUM-21) |
| Forks em Python 3, rodando em Docker | **A VERIFICAR**; senão, a modernização é a SCRUM-44 |
| Resultado da partida registrado em banco MySQL/MariaDB | **CONFIRMADO** que o Sixserver usa MySQL e ajusta pontos após cada partida; quais campos (cartões etc.) chegam ao banco é **A VERIFICAR** |
| "Bypass de DNAS" | **CORRIGIDO:** DNAS é a autenticação de rede do **PS2**. No PC, o jogo autentica nos servidores da Konami, e o Sixserver emula esse servidor. |

## 3. Ferramentas da comunidade

| Ferramenta | Uso no projeto | Situação |
|---|---|---|
| Kitserver 6 | Carrega conteúdo extra (GDB) sem mexer nos `.afs` | Compatibilidade com a DLL do PES 6 Online **A VERIFICAR** (SCRUM-25) |
| Redirecionamento para o nosso servidor | Kitserver (módulo de rede) ou PES6-EXE-modifier | Qual dos dois funciona sem alterar o executável é **A VERIFICAR** |
| PESFan Editor 6 | Edição de option files (elencos) | Uso pelos jogadores; nenhum option file entra no repositório |
| Game Graphic Studio | Inspecionar `.afs` para a UI do torneio | Só localmente, na cópia do Bruno |

## 4. Técnica de hook

| Item | Situação |
|---|---|
| DLL proxy carregada pelo jogo | Abordagem mantida |
| `d3d8.dll` ou `dxgi.dll` | **CORRIGIDO (provável):** PES 6 PC usa **Direct3D 9**, então o proxy natural é `d3d9.dll` (ou `dinput8.dll`). `dxgi.dll` é de DX10+ e não é carregada pelo jogo. **A VERIFICAR** na lista de imports do `pes6.exe` (F4). |
| MinHook para desviar funções | Mantido (SCRUM-25) |
| Hooks em Winsock `send/recv` | Mantido para observar a rede; o resultado oficial vem do servidor |
| Overlay de UI via `IDirect3DDevice9::Present` | Mantido como **plano B** para a UI do torneio, caso inserir itens no menu nativo seja difícil |

## 5. Fase 2: navegador

### O que aproveitamos
- Execução **local no navegador** (WebAssembly + WebGPU) como alvo, em vez de streaming de vídeo.
- Carregamento **sob demanda** de assets, com cache local, para o jogo abrir rápido.

### Referência: port de GTA V no navegador (playgta5.com, out/2026) — checagem

| Afirmação | Resultado |
|---|---|
| Execução local no navegador, sem cloud gaming | **CONFIRMADO** (várias fontes) |
| Motor C++ compilado para WebAssembly (`game.wasm`) | **CONFIRMADO**: ~63 MB, ~91 mil funções, nomes do motor RAGE, compilado com Emscripten |
| Partiu do código-fonte do jogo | **CONFIRMADO** que partiu de C/C++ do jogo; uma fonte diz que era código **vazado** |
| Renderização em WebGPU | **CONFIRMADO**: comandos Direct3D 11 traduzidos para WebGPU e ~6.600 shaders para WGSL |
| Canvas transferido para um Worker | **PARCIAL**: confirmado o uso de vários Web Workers; transferência do canvas não citada |
| Núcleo inicial de ~700 MB | **CONFIRMADO**: ~582 MB de conjunto inicial, ~743 MB até o mundo aparecer |
| Streaming sob demanda | **CONFIRMADO**: ~20,9 GB em 5.814 arquivos no servidor do site, pedidos em trechos por HTTP Range, cache em OPFS |
| Uso de IA na conversão | **NÃO CONFIRMADO**: só uma fonte afirma |
| "Análise do código vazado no GitHub" | **NÃO CONFIRMADO**: as análises foram feitas sobre o `game.wasm` publicado |
| Desfecho | Saiu do ar horas depois de viralizar; uma fonte atribui a uma notificação da Take-Two, sem confirmação oficial |

Fontes: SecurityLab (análise do `game.wasm`), Ubergizmo, Lowyat, Games.gg, ProPakistani, GodIsAGeek.

**Conclusão:** o padrão técnico (motor em WASM + tradução gráfica para WebGPU + leitura em trechos com cache OPFS) é real e é o alvo da Fase 2. As duas diferenças abaixo continuam valendo.

### O que muda em relação à pesquisa original

**1. Não temos código-fonte.** O GTA V web foi compilado de C/C++ com Emscripten. No PES 6 não existe fonte: o caminho equivalente é **recompilação estática do binário x86 para C** (técnica dos projetos N64Recomp/PS2Recomp/PSPRecomp) e só então Emscripten → WASM. A tradução gráfica também muda: **Direct3D 9 → WebGPU** (no GTA V era Direct3D 11).

**2. Os bytes do jogo vêm da cópia do jogador, não de um servidor nosso.** Mantemos o mesmo mecanismo de streaming (leitura em trechos + cache OPFS), trocando a origem:
- o jogador **seleciona a própria instalação** do PES 6 (File System Access API ou upload local);
- um I/O Worker lê **trechos sob demanda** desses arquivos locais e guarda no **OPFS**;
- a CDN entrega **só o que é nosso**: motor recompilado, patch, portal.

Vantagem extra: o PES 6 tem ~1,5 GB de assets (contra ~20 GB do GTA V), então ler direto do disco do jogador é rápido e dispensa servidor de arquivos.

**3. A estratégia final continua sendo decidida por PoC** (SCRUM-36 → ADR-002), comparando recompilação estática x86→WASM com emulação x86 em WASM, e mantendo streaming apenas como último recurso.
