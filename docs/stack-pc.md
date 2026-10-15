# Stack PC do PES 6 e direção da Fase 2 (SCRUM-14)

- **Status:** pesquisa revisada em 2026-10-15; itens marcados **A VERIFICAR** só viram fato quando testados na F4/F5.
- **Origem:** pesquisa do Bruno + revisão técnica do agente.

Legenda: **CONFIRMADO** = fonte primária ou teste · **A VERIFICAR** = plausível, sem confirmação · **CORRIGIDO** = a pesquisa original continha erro.

## 1. Versão-alvo do `pes6.exe`

| Item | Situação |
|---|---|
| Versão-alvo | **A VERIFICAR.** A pesquisa indicou v1.00; a escolha entre 1.00 e 1.10 depende do que Kitserver 6 e Sixserver suportam e é decidida na SCRUM-24. |
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

### O que muda em relação à pesquisa original

**1. Não temos código-fonte.** O port de GTA V citado foi, segundo as análises publicadas, compilado a partir do **código C++ do jogo**, possivelmente vazado. No PES 6 não existe fonte: "recompilar para WASM" significa **recompilação estática do binário x86** (técnica dos projetos N64Recomp/PS2Recomp/PSPRecomp), muito mais difícil que compilar C++. A tradução de Direct3D 9 para WebGPU também precisa ser construída.

**2. Assets não podem ficar numa CDN nossa.** Hospedar os `.afs` (e GDB com conteúdo de terceiros) para download é **distribuir o jogo**. O port de GTA V servia os arquivos do jogo a partir do próprio site e saiu do ar horas depois de viralizar; já houve DMCA da Take-Two contra um port de Vice City no navegador. O desenho da Fase 2 fica:
- o jogador **seleciona a própria instalação** do PES 6;
- os arquivos ficam num **cache local do navegador** (OPFS/IndexedDB), lidos sob demanda por um I/O Worker;
- a CDN entrega **só o que é nosso**: motor recompilado, patch, portal.

**3. A estratégia final continua sendo decidida por PoC** (SCRUM-36 → ADR-002), comparando recompilação estática x86→WASM com emulação x86 em WASM, e mantendo streaming apenas como último recurso.
