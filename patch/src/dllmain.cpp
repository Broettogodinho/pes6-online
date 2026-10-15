// Ponto de entrada da DLL do PES 6 Online.
// F1: só registra que foi carregada. Hooks entram na F5 (SCRUM-25), sempre
// validando o hash do pes6.exe e a assinatura de cada função (ver AGENTS.md §3).

#include "log.h"

#ifdef _WIN32
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID /*reserved*/) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(module);
    pes6online::log::init();
    pes6online::log::write("PES 6 Online: DLL carregada (v0.1.0, sem hooks ativos)");
  }
  return TRUE;
}
#endif
