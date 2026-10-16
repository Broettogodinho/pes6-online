// Ponto de entrada da DLL do PES 6 Online, carregada como proxy do dinput8.dll.
//
// No DllMain só se registra a carga e se dispara uma thread: calcular o hash do
// executável e instalar hooks dentro do DllMain (loader lock) pode travar o jogo.
// A thread confere a versão-alvo e só então instala os hooks (AGENTS.md §3).

#include <windows.h>

#include <format>

#include "exe_check.h"
#include "hooks.h"
#include "log.h"

namespace {

// Hooks ativos. Vazio até as funções-alvo serem mapeadas e assinadas (SCRUM-24);
// cada entrada vem de re/symbols/target.toml.
const pes6online::hooks::HookSpec kHooks[] = {
    {"(nenhum)", 0, "", nullptr, nullptr},
};

DWORD WINAPI init_thread(LPVOID) {
  if (!pes6online::exe::is_target()) return 0;
  if (!pes6online::hooks::init()) return 0;
  int installed = 0;
  int declared = 0;
  for (const auto& spec : kHooks) {
    if (spec.address == 0) continue;
    ++declared;
    if (pes6online::hooks::install(spec)) ++installed;
  }
  pes6online::log::write(std::format("hooks: {} de {} instalados", installed, declared));
  return 0;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(module);
    pes6online::log::init();
    pes6online::log::write("PES 6 Online: DLL carregada (v0.2.0, proxy dinput8)");
    if (HANDLE t = CreateThread(nullptr, 0, init_thread, nullptr, 0, nullptr)) CloseHandle(t);
  } else if (reason == DLL_PROCESS_DETACH && reserved == nullptr) {
    pes6online::hooks::shutdown();
  }
  return TRUE;
}
