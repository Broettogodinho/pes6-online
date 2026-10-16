# ADR-002 — Fase 1: sofá virtual com Parsec; Fase 2: netplay

- **Status:** aprovado por Bruno em 2026-10-15
- **Data:** 2026-10-15
- **Revisa:** ADR-001 (a base continua sendo o PES 6 PC na versão-alvo de `re/symbols/target.toml`; muda *como* o online é feito)

## Contexto

O objetivo é jogar a **Copa nativa do PES 6** com vários amigos remotos, como em 2005: todos na mesma "sala", um único jogo rodando o torneio, e o controle passando para quem joga a próxima partida.

O plano anterior (DLL de hooks + servidor de torneio sincronizando várias cópias da Copa) exigia muita engenharia reversa antes do primeiro teste.

## Decisão

**Fase 1 — Sofá virtual (PoC com Parsec)**
- Um **anfitrião** roda a instalação padrão do PES 6 (`docs/stack-pc.md` §1) com a **Copa nativa** e N jogadores humanos.
- Os convidados entram via **Parsec**: todos assistem à partida; só os **2 jogadores da vez** recebem controle.
- Antes de cada partida, os dois da vez escolhem o lado na **tela nativa** de controles, como em 2005.
- Nenhuma alteração no jogo. Só o anfitrião precisa ter a cópia do jogo.
- A fase é validada por um protocolo de testes com métricas e critérios de aprovação (`docs/poc-parsec.md`).

**Fase 2 — Netplay (se a Fase 1 for aprovada)**
- Cada jogador roda o jogo no navegador (port WASM) e só os comandos dos controles trafegam (lockstep, como no netplay de emuladores).
- Cada jogador usa a própria cópia; a estratégia de execução no navegador continua decidida por PoC.

**Portão de decisão:** ADR-003, ao final da Fase 1, registra go/no-go para o netplay com base nas métricas.

## Consequências

- A Copa nativa roda intacta: sorteio, tabela e regras são do próprio PES.
- A engenharia reversa e a DLL de hooks saem do caminho crítico, mas **seguem ativas como trilha paralela de mods** (revisão de 2026-10-15, épico SCRUM-8): primeiro o mapeamento do sistema de entrada e o mod de mais controles, usado no teste T4.
- Os mods usam DLL proxy (preferência pelo dinput8, já que o d3d9 é do dgVoodoo) com hooks validados por endereço + assinatura; nenhum arquivo do jogo é alterado em disco.
- O modo "Torneio Online" nativo no menu do jogo fica como **visão de longo prazo**, apoiado no mapeamento de menus, Copa e save (SCRUM-20 a 23) e no servidor de torneio já escrito.
- O caminho crítico passa a ser: cópia do jogo → anfitrião → sessões de teste.
- A engine de Copa em Python já escrita (SCRUM-28) fica disponível, mas não é usada na Fase 1.

## Riscos

| Risco | Mitigação |
|---|---|
| Limite de convidados/controles do plano gratuito do Parsec | Confirmar na documentação atual (PoC-01); na Copa só 2 controles ativos por vez |
| Upload do anfitrião (~10 Mbps por convidado) | Medir; reduzir resolução/FPS para espectadores; avaliar anfitrião em nuvem |
| Quantos controles o PES 6 PC aceita na Copa | Mapear na PoC-02 |
| Troca de slot de controle no meio da Copa (jogo não reconhecer o controle do novo convidado ou trocar a ordem) | Confirmar atribuição de slots no Parsec (PoC-01) e reconexão de controles no PES 6 (PoC-02) |
| Atraso de entrada perceptível | Metas de latência no protocolo; cabo de rede; anfitrião central em relação aos jogadores |
| Anfitrião cair no meio da Copa | Salvar a Copa nativa entre rodadas; testar retomada (T5) |
| Uso de uma cópia por várias pessoas | Fase 1 restrita a grupo privado de amigos; na Fase 2 cada jogador usa a própria cópia |
