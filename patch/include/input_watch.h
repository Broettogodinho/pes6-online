#pragma once

// Diagnóstico do sistema de entrada (SCRUM-56): lista periodicamente os
// controles que o DirectInput 8 enxerga (GUID de instância, GUID de produto e
// nome) e registra no log cada mudança, para entender por que o mapeamento de
// botões se perde quando um controle reconecta. Somente leitura.

namespace pes6online::input_watch {

void start();

}  // namespace pes6online::input_watch
