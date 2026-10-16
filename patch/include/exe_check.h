#pragma once

#include <string>

namespace pes6online::exe {

// SHA-256 (hex minúsculo) do executável do processo atual; vazio se falhar.
std::string sha256_of_current();

// true se o processo atual é a versão-alvo (target.h). Registra o resultado no log.
bool is_target();

}  // namespace pes6online::exe
