#pragma once

// Envolve o objeto IDirectInput8 criado para o jogo e estabiliza o GUID de
// instância dos controles (ver guid_map.h): na enumeração o jogo recebe o GUID
// salvo no settings.dat, e ao abrir o dispositivo o GUID é traduzido de volta
// para o real. Todo o resto é repassado sem alteração.

#include <windows.h>

namespace pes6online::dinput_wrap {

// Recebe o objeto devolvido pela DirectInput8Create real e devolve o envoltório
// (ou o próprio objeto, se a interface não for IDirectInput8A/W).
void* wrap(REFIID riid, void* real);

}  // namespace pes6online::dinput_wrap
