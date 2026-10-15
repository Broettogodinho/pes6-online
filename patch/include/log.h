#pragma once

#include <string_view>

namespace pes6online::log {

// Abre o arquivo de log (pes6online.log ao lado da DLL). Seguro chamar mais de uma vez.
void init();

// Escreve uma linha com data/hora. Thread-safe.
void write(std::string_view message);

}  // namespace pes6online::log
