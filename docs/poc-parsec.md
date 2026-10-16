# PoC Sofá Virtual (Parsec) — protocolo de testes

Objetivo: provar que uma Copa nativa do PES 6 pode ser jogada por vários amigos remotos, com um único anfitrião transmitindo o jogo via Parsec, com qualidade suficiente para justificar a Fase 2 (netplay). Decisão em ADR-002.

**Dependências:** as sessões só acontecem depois que o anfitrião tiver a cópia original do jogo (SCRUM-46); o T4 também depende do mod de mais controles (SCRUM-57), instalado só no anfitrião. Os convidados não instalam o jogo: precisam apenas do app do Parsec e de um controle.

## 1. Arquitetura do teste

```
              ┌──────────────── Anfitrião (Windows) ────────────────┐
              │ pes6.exe 1.00 original  ←  Copa nativa, N humanos   │
              │ Parsec Host  →  vídeo/áudio para todos               │
              │              ←  controle só dos 2 jogadores da vez   │
              └─────────────────────────────────────────────────────┘
                 ▲ controle + vídeo      ▲ controle + vídeo     ▲ só vídeo
            Jogador da vez A        Jogador da vez B       Espectadores (C..H)
```

## 2. Pré-requisitos

### Anfitrião
- Windows 10/11, PES 6 PC **1.00 original** instalado da mídia, sem patches. **Exceção:** no T4, a DLL de mods (proxy; nenhum arquivo do jogo é alterado em disco).
- GPU com encoder de vídeo por hardware (NVENC, AMF ou Quick Sync). Sem isso, o Parsec usa codificação por software e a latência sobe.
- **Cabo de rede.** Upload medido (speedtest) registrado antes de cada sessão. Referência: ~10 Mbps por convidado, ou seja, ~30 Mbps no T3 e ~70 Mbps no T4. Se o upload não comportar, aplicar o plano B do ADR-002 (reduzir resolução/FPS, anfitrião em nuvem) e registrar.
- Parsec instalado, conta criada; jogo em tela cheia ou janela, conforme o teste.
- Pasta de saves do PES 6 com backup antes de cada sessão.

### Convidados
- **App nativo do Parsec** (o cliente web não tem decodificação por hardware).
- Convidados jogam com **controle USB/Bluetooth** (Xbox, DualShock ou DualSense). No anfitrião, o Parsec cria um controle virtual para cada convidado com permissão de controle.
- Cabo de rede ou Wi-Fi 5 GHz; sem VPN.

## 3. Pontos a confirmar antes das sessões

| ID | Pergunta | Onde |
|---|---|---|
| PoC-01 | Quantos convidados e quantos controles simultâneos o plano gratuito permite hoje? | Documentação oficial do Parsec |
| PoC-01 | Dá para dar/retirar permissão de controle de um convidado durante a sessão? | Documentação + teste |
| PoC-01 | O anfitrião consegue fixar ou atribuir a vaga (slot) de controle de cada convidado? | Documentação + teste |
| PoC-01 | Onde aparece a perda de pacotes (overlay ou console do anfitrião)? | Documentação + teste |
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
| Ambiente | Data/horário, cidade e provedor de cada jogador, upload medido do anfitrião |
| Mod (só T4) | Commit/versão da DLL de mods |

### Por partida

Coletar em cada partida (planilha `poc-parsec-resultados`):

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

1. Anfitrião: backup de saves, speedtest, Parsec aberto, jogo aberto no menu. Conferir e registrar as condições fixas (§5). No T4, conferir se a DLL de mods carregou e se o log de hooks não tem erros.
2. Convidados entram; anfitrião confere nomes e libera **só os controles dos 2 primeiros jogadores**.
3. Criar a Copa nativa com o número de humanos do cenário; cada jogador escolhe o time quando for chamado.
4. A cada partida: liberar controle dos 2 da vez → tela de lados → jogar → registrar métricas (anotando se o anfitrião jogou) → salvar a Copa.
5. Ao fim: notas gerais e observações livres.
