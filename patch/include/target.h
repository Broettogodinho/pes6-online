#pragma once

// Versão-alvo do pes6.exe. Espelha re/symbols/target.toml: se o hash do
// executável carregado não for este, a DLL não instala nenhum hook.

#include <cstdint>
#include <string_view>

namespace pes6online::target {

inline constexpr std::string_view kExeSha256 =
    "c3e8176e798db707151688b93d8c7812f3602ea27da23e56f2a0e8dc04ebc568";
inline constexpr std::uint32_t kImageBase = 0x00400000;

}  // namespace pes6online::target
