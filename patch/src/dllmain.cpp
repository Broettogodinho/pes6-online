// Ponto de entrada da DLL do PES 6 Online, carregada como proxy do dinput8.dll.
//
// No DllMain só se registra a carga e se dispara uma thread: calcular o hash do
// executável e instalar hooks dentro do DllMain (loader lock) pode travar o jogo.
// A thread confere a versão-alvo e só então instala os hooks (AGENTS.md §3).

#include <windows.h>

#include <format>

#include "exe_check.h"
#include "hooks.h"
#include "input_watch.h"
#include "log.h"
#include "mem_probe.h"
#include "probes.h"
#include "settings_redirect.h"

namespace {

DWORD WINAPI init_thread(LPVOID) {
  if (!pes6online::exe::is_target()) return 0;
  if (!pes6online::hooks::init()) return 0;
  int count = 0;
  const auto* specs = pes6online::probes::specs(&count);
  int installed = 0;
  for (int i = 0; i < count; ++i) {
    if (pes6online::hooks::install(specs[i])) ++installed;
  }
  pes6online::log::write(std::format("hooks: {} de {} instalados", installed, count));
  pes6online::probes::start_state_watch();
  pes6online::input_watch::start();
  pes6online::mem_probe::start();
  return 0;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(module);
    pes6online::log::init();
    pes6online::log::write("PES 6 Online: DLL carregada (v0.10.0, proxy dinput8, atribuição automática, observação dos controles, sondas de RE)");
    // Cedo, antes de o jogo ler o settings.dat: atribuição automática dos controles.
    pes6online::settings_redirect::install();
    if (HANDLE t = CreateThread(nullptr, 0, init_thread, nullptr, 0, nullptr)) CloseHandle(t);
  } else if (reason == DLL_PROCESS_DETACH && reserved == nullptr) {
    pes6online::hooks::shutdown();
  }
  return TRUE;
}
