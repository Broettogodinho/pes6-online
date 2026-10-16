# PoC Sofá Virtual (Parsec) — protocolo de testes

Objetivo: provar que uma Copa nativa do PES 6 pode ser jogada por vários amigos remotos, com um único anfitrião transmitindo o jogo via Parsec, com qualidade suficiente para justificar a Fase 2 (netplay). Decisão em ADR-002.

**Dependências:** o anfitrião usa a instalação padrão do PES 6 ([`docs/stack-pc.md`](stack-pc.md) §1, hash em `re/symbols/target.toml`); o T4 também depende do mod de mais controles (SCRUM-57), instalado só no anfitrião. Os convidados não instalam o jogo: precisam apenas do app do Parsec e de um controle.

## 1. Arquitetura do teste

```
              ┌──────────────── Anfitrião (Windows) ────────────────┐
              │ pes6.exe (versão-alvo)  ←  Copa nativa, N humanos   │
              │ Parsec Host  →  vídeo/áudio para todos               │
              │              ←  controle só dos 2 jogadores da vez   │
              └─────────────────────────────────────────────────────┘
                 ▲ controle + vídeo      ▲ controle + vídeo     ▲ só vídeo
            Jogador da vez A        Jogador da vez B       Espectadores (C..H)
```

## 2. Pré-requisitos

### Anfitrião
- Windows 10/11 com a **instalação padrão do PES 6** (`docs/stack-pc.md` §1): `pes6.exe` com o hash de `re/symbols/target.toml`, dgVoodoo (imagem) e Kitserver. Nada além disso. **Exceção:** no T4, a DLL de mods (proxy; nenhum arquivo do jogo é alterado em disco).
- Conferir o hash do `pes6.exe` antes da primeira sessão e sempre que reinstalar ou mexer no Kitserver.
- GPU com encoder de vídeo por hardware (NVENC, AMF ou Quick Sync). Sem isso, o Parsec usa codificação por software e a latência sobe.
- **Cabo de rede.** Upload medido (speedtest) registrado antes de cada sessão. Referência: ~10 Mbps por convidado, ou seja, ~30 Mbps no T3 e ~70 Mbps no T4. Se o upload não comportar, aplicar o plano B do ADR-002 (reduzir resolução/FPS, anfitrião em nuvem) e registrar.
- Parsec instalado, conta criada; jogo em tela cheia ou janela, conforme o teste.
- Pasta de saves do PES 6 com backup antes de cada sessão.

### Convidados
- **App nativo do Parsec** (o cliente web não tem decodificação por hardware).
- Convidados jogam com **controle USB/Bluetooth** (Xbox, DualShock ou DualSense). No anfitrião, o Parsec cria um controle virtual para cada convidado com permissão de controle.
- Cabo de rede ou Wi-Fi 5 GHz; sem VPN.

## 3. Pontos a confirmar antes das sessões

### PoC-01 · Parsec (SCRUM-47, respondido em 2026-10-15)

Fonte: documentação oficial do Parsec (links abaixo). As páginas de suporte bloqueiam leitura automatizada; os trechos vieram da busca e devem ser conferidos no app. **CONFIRMADO** = dito na documentação atual · **A VERIFICAR** = fonte antiga, divergente ou ausente; testar na sessão T1.

| Pergunta | Resposta | Situação |
|---|---|---|
| Quantos convidados no plano gratuito? | Até **20 conexões simultâneas** por padrão (ajustável pelo anfitrião); acima disso, a documentação alerta para a necessidade de muito upload. A Copa de 8 cabe com folga. | **CONFIRMADO** [1] |
| Quantos controles simultâneos? | Post antigo do blog: "qualquer número conecta, até **4** com controle". A documentação atual não cita limite. Na Copa só **2** controles ficam ativos por vez. | **A VERIFICAR** [2] |
| Dar/retirar controle durante a sessão? | Sim. Permissões por convidado (controle, teclado, mouse): clicar na foto do convidado na barra do Parsec. **Controle vem ligado por padrão** para convidados; teclado e mouse, desligados. Padrões ajustáveis por amigo. | **CONFIRMADO** [3] |
| O anfitrião fixa o slot de cada convidado? | **Não.** O Parsec não atribui slots: a ordem segue a ordem em que os controles se conectam, e um controle só conecta quando o convidado aperta um botão. Para reordenar, usar **"Reset gamepad"** no ícone da bandeja e pedir que apertem um botão na ordem desejada. | **CONFIRMADO** [4] |
| Tipo de controle virtual | Configurável no anfitrião ("Virtual Gamepad Type" / `host_gamepad_type`): **Xbox 360 (padrão, mais compatível)**, DualShock 4 ou DualSense. Driver próprio (VUSB), com ViGEm como reserva. | **CONFIRMADO** [4] |
| Banda dos espectadores / qualidade só para eles | Não existe qualidade separada para espectadores: o limite de banda do anfitrião é **dividido entre todos** os convidados, e a qualidade cai para todos. Referência: 10 Mbps de upload por convidado; 50 Mbps mínimo para 2+ convidados. | **CONFIRMADO** [5] |
| Cliente web × app nativo | Manter o app nativo (decodificação por hardware), como já está no §2. | **CONFIRMADO** (decisão do protocolo) |
| Onde ver a perda de pacotes | Não encontrado na documentação. | **A VERIFICAR** no T1 |
| Plano para o T4 | **Gratuito.** Os recursos pagos (Warp: vários monitores, 4:4:4, monitor virtual) não afetam a Copa. O uso tem de ser pessoal e sem fins comerciais (licença do plano gratuito). | Recomendação |
| Alternativas se o Parsec travar | **Steam Remote Play Together:** base de 4 jogadores e exige jogo na Steam (**A VERIFICAR**). **Sunshine/Moonlight:** aceita vários clientes, mas todos dividem o mesmo controle, sem permissão por convidado. Nenhuma é melhor que o Parsec para a Copa. | **A VERIFICAR** [6] |

**Consequências para as sessões:**
- **Desligar o controle por padrão** para os amigos e ligar só para os 2 da vez. Senão, um espectador que aperte um botão cria um controle virtual e bagunça a ordem.
- A troca de jogador entre partidas segue: retirar a permissão dos dois que saem → **Reset gamepad** → dar permissão aos dois que entram → cada um aperta um botão na ordem (lado esquerdo primeiro). Se o PES 6 não reconhecer o controle com o jogo aberto (PoC-02), essa é a primeira coisa a testar no T1.
- No T4, com 7 convidados assistindo, o upload necessário fica perto do limite. Medir no T3 e, se preciso, baixar o limite de banda do anfitrião.

Fontes: [1] [Max Client Connections To Your Host](https://support.parsec.app/hc/en-us/articles/32361376782228-Max-Client-Connections-To-Your-Host) · [2] [Play Co-Op Games Over The Internet](https://parsec.app/blog/play-co-op-games-over-the-internet-for-virtual-couch-gaming-5d84b550a8d8) · [3] [Hosting and Permissions](https://support.parsec.app/hc/en-us/articles/32381747079572-Hosting-and-Permissions) · [4] [Setup Gamepad](https://support.parsec.app/hc/en-us/articles/32381705301908-Setup-Gamepad) · [5] [Parsec Connectivity Requirements](https://support.parsec.app/hc/en-us/articles/32381460716180-Parsec-Connectivity-Requirements) e [Max Client Connections](https://support.parsec.app/hc/en-us/articles/32361376782228-Max-Client-Connections-To-Your-Host) · [6] [Sunshine: Advanced Usage](https://sunshinestream.readthedocs.io/en/latest/about/advanced_usage.html) · [Planos do Parsec](https://parsec.app/pricing)

### PoC-02 · PES 6 (SCRUM-49, a testar)

| ID | Pergunta | Onde |
|---|---|---|
| PoC-01 | Onde aparece a perda de pacotes (overlay ou console do anfitrião)? | Teste no T1 |
| PoC-02 | Quantos controles o PES 6 PC reconhece ao mesmo tempo? Como é a tela de escolha de lados antes da partida? | Teste no anfitrião |
| PoC-02 | A Copa nativa aceita quantos jogadores humanos? | Teste no anfitrião |
| PoC-02 | O PES 6 reconhece um controle conectado com o jogo já aberto? A ordem dos controles muda quando outro convidado assume o slot? A configuração do `settings.exe` continua valendo depois da troca? | Teste no anfitrião |

> A troca de controle entre partidas depende das respostas sobre slots (PoC-01) e reconexão de controles (PoC-02). Se falharem, a Copa com troca de jogadores não funciona como planejado: é o risco central da PoC.

## 4. Cenários de teste

| Cenário | Participantes | O que se valida | Amostra mínima |
|---|---|---|---|
| **T1** | Anfitrião + 1, mesma rede local | Linha de base: controle remoto funciona, latência mínima | 1 partida |
| **T2** | Anfitrião + 1, cidades diferentes | Latência real de internet em partida inteira | 3 partidas |
| **T3** | Anfitrião + 3 (4 no total), Copa de 4 humanos | Troca de controle entre partidas, espectadores, carga no upload | 6 partidas |
| **T4** | Anfitrião + 7 (8 no total), Copa completa, **com o mod de mais controles (SCRUM-57)** | Experiência do torneio inteiro: duração, estabilidade, cansaço | Copa completa |
| **T5** | Durante o T3 (queda de convidado e do anfitrião); no T4, só queda de convidado | Retomada a partir do save da Copa | 1 queda de cada tipo |

- No T4, registrar o commit/versão da DLL de mods e guardar o log de hooks da sessão.
- O anfitrião pode jogar nos cenários T3 e T4. As partidas dele são marcadas na planilha (ver §5) porque ele joga sem latência de rede.

## 5. Métricas por sessão

### Condições fixas (registrar no início de cada sessão)

| Grupo | O que registrar |
|---|---|
| Parsec | Versão, resolução, FPS, bitrate, codec (H.264/H.265) |
| Partida | Tempo de jogo e dificuldade: **iguais em todas as sessões** |
| Ambiente | Data/horário, estado/região de cada jogador, upload medido do anfitrião |

**Anonimização:** os resultados são versionados no git, então a planilha usa só apelidos (J1…J8; anfitrião = ANF) e estado/região. Nome, cidade e provedor não entram; a tabela apelido → nome fica só com o anfitrião, fora do repositório.
| Mod (só T4) | Commit/versão da DLL de mods |

### Por partida

Coletar em cada partida (planilha [`poc-parsec-resultados.xlsx`](poc-parsec-resultados.xlsx); a aba Resumo calcula medianas e portões do §6):

| Métrica | Fonte | Meta | Tipo |
|---|---|---|---|
| Ping (Network) de cada jogador da vez | Overlay do Parsec | ≤ 30 ms bom · ≤ 60 ms aceitável | Portão |
| Encode (anfitrião) | Overlay do Parsec | < 15 ms a 60 FPS · < 32 ms a 30 FPS | Portão |
| Nota de resposta do controle (1–5) | Cada jogador da vez, ao fim da partida | ≥ 4 | Portão |
| Quedas de conexão | Anotação | 0 por partida | Portão |
| Decode (cada convidado) | Overlay do Parsec | < 15 ms | Informativa |
| Perda de pacotes | Overlay ou console (confirmar na PoC-01) | Próximo de 0 | Informativa |
| FPS do stream | Overlay do Parsec | Estável (30 ou 60, conforme configurado) | Informativa |
| FPS do jogo | Observação no anfitrião | Estável, sem quedas visíveis | Informativa |
| Upload usado pelo anfitrião | Gerenciador de tarefas | Abaixo de 80% do upload medido | Informativa |
| Tempo de troca de controle entre partidas | Cronômetro | < 60 s | Informativa |
| Anfitrião jogou? (sim/não) | Anotação | — | Marcação |
| Nota geral da experiência (1–5) | Todos, ao fim da sessão | ≥ 4 | Informativa |

Partidas em que o **anfitrião jogou** entram nas notas gerais, mas ficam **fora das medianas** de ping, encode, decode e nota de controle.

## 6. Critérios de aprovação (go para netplay)

As medianas só valem se o cenário atingiu a amostra mínima (§4). A Fase 1 é **aprovada** se, em T2 a T4:
1. Mediana de ping dos jogadores da vez ≤ 60 ms;
2. Mediana de encode dentro da meta do FPS configurado;
3. Mediana da nota de resposta do controle ≥ 4;
4. No máximo 1 queda de conexão por sessão (fora as provocadas no T5), sem perda do progresso da Copa;
5. T4 termina com campeão definido, sem perder o progresso da Copa;
6. Nenhuma limitação do Parsec ou do jogo impede a Copa com 8 humanos.

As métricas informativas não reprovam a fase, mas entram no ADR-003 para explicar o resultado e orientar ajustes.

**Reprovação parcial** (ex.: só funciona com 4 jogadores ou só em uma região) vira ajuste do plano no ADR-003, em vez de cancelar o projeto.

## 7. Roteiro de uma sessão

1. Anfitrião e convidados: [checklist de pré-sessão](poc-parsec-checklist.md). Anfitrião: backup de saves, speedtest, Parsec aberto, jogo aberto no menu. Conferir e registrar as condições fixas (§5). No T4, conferir se a DLL de mods carregou e se o log de hooks não tem erros.
2. Convidados entram; anfitrião confere nomes e libera **só os controles dos 2 primeiros jogadores**.
3. Criar a Copa nativa com o número de humanos do cenário; cada jogador escolhe o time quando for chamado.
4. A cada partida: retirar o controle dos que saíram → **Reset gamepad** (bandeja do Parsec) → liberar controle dos 2 da vez → cada um aperta um botão, lado esquerdo primeiro (§3) → tela de lados → jogar → registrar métricas (anotando se o anfitrião jogou) → salvar a Copa.
5. Ao fim: notas gerais e observações livres.
