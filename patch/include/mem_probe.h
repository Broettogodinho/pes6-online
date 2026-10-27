#pragma once

// Sonda de memória do option file (SCRUM-22). Ver mem_probe.cpp.

namespace pes6online::mem_probe {

// Inicia a busca em segundo plano; não faz nada sem pes6online_probe.txt.
void start();

}  // namespace pes6online::mem_probe
