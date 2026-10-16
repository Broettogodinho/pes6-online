#pragma once

// Registro de hooks. Cada hook declara endereço + assinatura (AGENTS.md §3):
// a DLL confere a assinatura na memória antes de instalar e, se não bater,
// não instala e registra no log.

#include <cstdint>
#include <string_view>

namespace pes6online::hooks {

struct HookSpec {
  std::string_view name;       // ex.: "Cup_AdvanceRound"
  std::uint32_t address;       // endereço virtual na versão-alvo
  std::string_view signature;  // padrão de bytes, ex.: "55 8B EC ?? ??"
  void* detour;                // nossa função
  void** original;             // recebe o trampolim para chamar a original
};

// Inicializa o motor de hooks (MinHook). Chamar uma vez.
bool init();

// Valida a assinatura e, se bater, instala e ativa o hook. Registra o resultado.
bool install(const HookSpec& spec);

// Remove todos os hooks (usado ao descarregar a DLL).
void shutdown();

}  // namespace pes6online::hooks
