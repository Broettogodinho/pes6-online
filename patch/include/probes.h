#pragma once

// Sondas de engenharia reversa: hooks que só OBSERVAM (registram no log e chamam a
// função original sem alterar argumentos nem retorno). Servem para confirmar em
// execução o que foi INFERIDO no Ghidra (AGENTS.md §2).

#include "hooks.h"

namespace pes6online::probes {

// Hooks de observação ativos. Cada um tem endereço + assinatura (re/notes/).
const hooks::HookSpec* specs(int* count);

// Inicia uma thread que observa (somente leitura) globais de estado do jogo e
// registra no log cada mudança: tela atual, passo da tela e modo de jogo.
void start_state_watch();

}  // namespace pes6online::probes
