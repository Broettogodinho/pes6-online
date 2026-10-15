# PoC Sofá Virtual (Parsec) — protocolo de testes

Objetivo: provar que uma Copa nativa do PES 6 pode ser jogada por vários amigos remotos, com um único anfitrião transmitindo o jogo via Parsec, com qualidade suficiente para justificar a Fase 2 (netplay). Decisão em ADR-002.

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
- Windows 10/11, PES 6 PC **1.00 original** instalado da mídia, sem patches.
- GPU com encoder de vídeo por hardware (NVENC, AMF ou Quick Sync). Sem isso, o Parsec usa codificação por software e a latência sobe.
- **Cabo de rede.** Upload medido (speedtest) registrado antes de cada sessão. Referência: ~10 Mbps por convidado.
- Parsec instalado, conta criada; jogo em tela cheia ou janela, conforme o teste.
- Pasta de saves do PES 6 com backup antes de cada sessão.

### Convidados
- **App nativo do Parsec** (o cliente web não tem decodificação por hardware).
- Controle USB/Bluetooth (Xbox ou DualShock/DualSense) ou teclado.
- Cabo de rede ou Wi-Fi 5 GHz; sem VPN.

## 3. Pontos a confirmar antes das sessões

| ID | Pergunta | Onde |
|---|---|---|
| PoC-01 | Quantos convidados e quantos controles simultâneos o plano gratuito permite hoje? | Documentação oficial do Parsec |
| PoC-01 | Dá para dar/retirar permissão de controle de um convidado durante a sessão? | Documentação + teste |
| PoC-02 | Quantos controles o PES 6 PC reconhece ao mesmo tempo? Como é a tela de escolha de lados antes da partida? | Teste no anfitrião |
| PoC-02 | A Copa nativa aceita quantos jogadores humanos? | Teste no anfitrião |

## 4. Cenários de teste

| Cenário | Participantes | O que se valida |
|---|---|---|
| **T1** | Anfitrião + 1, mesma rede local | Linha de base: controle remoto funciona, latência mínima |
| **T2** | Anfitrião + 1, cidades diferentes | Latência real de internet em partida inteira |
| **T3** | Anfitrião + 3 (4 no total), Copa de 4 humanos | Troca de controle entre partidas, espectadores, carga no upload |
| **T4** | Anfitrião + 7 (8 no total), Copa completa | Experiência do torneio inteiro: duração, estabilidade, cansaço |
| **T5** | Durante T3 ou T4 | Queda de um convidado e do anfitrião; retomada a partir do save da Copa |

## 5. Métricas por sessão

Coletar em cada partida (planilha `poc-parsec-resultados`):

| Métrica | Fonte | Meta |
|---|---|---|
| Ping (Network) de cada jogador da vez | Overlay do Parsec | ≤ 30 ms bom · ≤ 60 ms aceitável |
| Encode (anfitrião) | Overlay do Parsec | < 15 ms a 60 FPS · < 32 ms a 30 FPS |
| Decode (cada convidado) | Overlay do Parsec | < 15 ms |
| Perda de pacotes (N:x/x/x) | Console do anfitrião | Próximo de 0/0/0 |
| FPS do jogo | Overlay | Estável (30 ou 60, conforme configurado) |
| Upload usado pelo anfitrião | Gerenciador de tarefas | Abaixo de 80% do upload medido |
| Tempo de troca de controle entre partidas | Cronômetro | < 60 s |
| Quedas de conexão | Anotação | 0 por partida |
| Nota de resposta do controle (1–5) | Cada jogador da vez, ao fim da partida | ≥ 4 |
| Nota geral da experiência (1–5) | Todos, ao fim da sessão | ≥ 4 |

## 6. Critérios de aprovação (go para netplay)

A Fase 1 é **aprovada** se, em T2 a T4:
1. Mediana de ping dos jogadores da vez ≤ 60 ms e encode dentro da meta;
2. Mediana da nota de resposta do controle ≥ 4;
3. T4 termina com campeão definido, sem perder o progresso da Copa;
4. Nenhuma limitação do Parsec ou do jogo impede a Copa com 8 humanos.

**Reprovação parcial** (ex.: só funciona com 4 jogadores ou só em uma região) vira ajuste do plano no ADR-003, em vez de cancelar o projeto.

## 7. Roteiro de uma sessão

1. Anfitrião: backup de saves, speedtest, Parsec aberto, jogo aberto no menu.
2. Convidados entram; anfitrião confere nomes e libera **só os controles dos 2 primeiros jogadores**.
3. Criar a Copa nativa com o número de humanos do cenário; cada jogador escolhe o time quando for chamado.
4. A cada partida: liberar controle dos 2 da vez → tela de lados → jogar → registrar métricas → salvar a Copa.
5. Ao fim: notas gerais e observações livres.
