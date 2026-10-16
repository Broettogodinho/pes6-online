# Sala de jogo — papéis e regras

Especificação da sala online da Copa (SCRUM-55, SCRUM-58). Decisões do Bruno em 2026-10-16. Os itens marcados *(padrão proposto)* são preenchimentos nossos, a confirmar.

## Princípio

O **host controla a sala**. **Todos veem a tela do host**, e cada participante só tem ação no jogo quando está jogando com o seu time.

Por baixo, no netplay: cada um roda o jogo na própria máquina, mas só as entradas do host comandam os menus. Como todos recebem os mesmos comandos, todas as cópias mostram a mesma tela. Isso depende de todos estarem idênticos em executável, dados e sequência de estados (requisito de sincronização, SCRUM-38).

## Papéis

| Papel | Quem | Vê | Pode fazer |
|---|---|---|---|
| **Host** | quem cria a sala | a própria tela | menus do jogo (criar a Copa, avançar telas, iniciar partida); associar cada participante ao seu time; pausar; convidar e remover; decidir quem assume quando alguém cai |
| **Jogador da vez** | os 2 da partida atual | a tela do host | controlar **só o próprio time** durante a partida dele, inclusive o menu do time (substituições, tática, formação); pausar, com limite |
| **Espectador** | todos os outros | a tela do host | assistir e usar o chat; nenhuma ação no jogo |

## Regras

### Times
- Ao entrar no modo Copa, o **host associa cada participante ao seu time** na tela de seleção.
- A associação participante → time fica registrada na sala e define quem é "jogador da vez" em cada partida do chaveamento.

### Durante a partida
- Cada jogador da vez controla o lado do seu time e o menu do próprio time (substituições, tática, formação).
- Os outros participantes não têm nenhuma entrada no jogo.

### Pausa
- O **jogador da vez pode pausar**, inclusive para entrar no menu de pausa e fazer substituições.
- **Limite: 3 pausas por jogador, por partida.** *(padrão proposto: o limite vale para cada jogador da vez; as pausas do host não contam)*
- O host pode pausar a qualquer momento (administração da sala).

### Queda de um jogador da vez
1. O jogo **pausa** automaticamente.
2. O host escolhe:
   - **esperar** o jogador reconectar; ou
   - colocar a **CPU** para assumir o time até o fim da partida (ou até a volta do jogador *(padrão proposto)*).

### Queda do host
- **Outro participante assume como host**, pela **ordem de chegada na sala**.
- **Requisito:** o estado da Copa (chaveamento, resultados, partida em andamento) precisa estar **salvo e sincronizado** em todos, para que o novo host continue do mesmo ponto.

## Funções da sala e do chat

| Função | Quem usa |
|---|---|
| Lista de presentes com o papel de cada um (host, jogador da vez, espectador) e a ordem de chegada | todos |
| Botão "pronto" antes de cada partida | jogadores da vez |
| Aviso "você joga agora" | jogadores da vez |
| Placar, chaveamento e pausas restantes ao vivo | todos |
| Chat de texto | todos |
| Comandos: associar time, expulsar, pausar, passar a vez, CPU assume | host |

## Dependências técnicas

| Regra | Depende de |
|---|---|
| Só o host comanda os menus | Netplay com filtro de entradas por papel (SCRUM-38, SCRUM-55) |
| Jogador controla só o seu time | Mapeamento do sistema de entrada e da atribuição de controles aos lados (SCRUM-56) |
| Limite de pausas e "CPU assume" | Hooks na pausa e no controle do time pela CPU (mapeamento a fazer) |
| Novo host continua a Copa | Estado da Copa salvo e sincronizado (SCRUM-20, SCRUM-22, SCRUM-38) |
