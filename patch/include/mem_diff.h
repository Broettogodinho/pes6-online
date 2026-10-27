#pragma once

// Busca de variáveis por comparação de memória (SCRUM-23), no estilo dos
// "memory scanners": fotos da seção .data do jogo em momentos do roteiro de
// teclas, mantendo só os bytes cujo valor seguiu a relação esperada em todas as
// etapas (ex.: cursor do menu sobe 1 a cada seta para baixo).
//
// Roteiro opcional `pes6online_memdiff.txt` na pasta do jogo, uma linha por foto:
//   <ms desde a 1ª leitura do teclado> <relação>
// relações: base | inc | dec | same | changed | eqbase | nebase
// (inc/dec = ±1 em relação à foto anterior; eqbase/nebase = igual/diferente da 1ª).

#include <windows.h>

namespace pes6online::mem_diff {

// Chamado a cada leitura do teclado com o tempo do roteiro (ms).
void on_tick(ULONGLONG t);

}  // namespace pes6online::mem_diff
