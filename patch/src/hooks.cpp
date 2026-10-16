#include "hooks.h"

#include <format>
#include <string>

#include <windows.h>

#include <MinHook.h>

#include "log.h"
#include "signature.h"

namespace pes6online::hooks {
namespace {
bool g_ready = false;

// Bytes legíveis a partir de addr (até `wanted`), para não ler memória inválida.
std::size_t readable_bytes(const void* addr, std::size_t wanted) {
  MEMORY_BASIC_INFORMATION mbi{};
  if (VirtualQuery(addr, &mbi, sizeof(mbi)) == 0) return 0;
  if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) return 0;
  const auto end = static_cast<const std::uint8_t*>(mbi.BaseAddress) + mbi.RegionSize;
  const auto avail = static_cast<std::size_t>(end - static_cast<const std::uint8_t*>(addr));
  return avail < wanted ? avail : wanted;
}
}  // namespace

bool init() {
  if (g_ready) return true;
  const MH_STATUS st = MH_Initialize();
  g_ready = st == MH_OK || st == MH_ERROR_ALREADY_INITIALIZED;
  log::write(std::format("hooks: MinHook {}", g_ready ? "inicializado" : MH_StatusToString(st)));
  return g_ready;
}

bool install(const HookSpec& spec) {
  if (!g_ready) {
    log::write(std::format("hooks: {} NÃO instalado (motor de hooks não iniciado)", spec.name));
    return false;
  }
  const auto pattern = sig::parse(spec.signature);
  if (!pattern) {
    log::write(std::format("hooks: {} NÃO instalado (assinatura malformada)", spec.name));
    return false;
  }
  auto* target = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(spec.address));
  const std::size_t avail = readable_bytes(target, pattern->size());
  if (!sig::matches_at(target, avail, *pattern)) {
    log::write(std::format("hooks: {} NÃO instalado (assinatura não confere em 0x{:08X})", spec.name,
                           spec.address));
    return false;
  }
  MH_STATUS st = MH_CreateHook(target, spec.detour, spec.original);
  if (st == MH_OK) st = MH_EnableHook(target);
  if (st != MH_OK) {
    log::write(std::format("hooks: {} NÃO instalado ({})", spec.name, MH_StatusToString(st)));
    return false;
  }
  log::write(std::format("hooks: {} instalado em 0x{:08X}", spec.name, spec.address));
  return true;
}

void shutdown() {
  if (!g_ready) return;
  MH_DisableHook(MH_ALL_HOOKS);
  MH_Uninitialize();
  g_ready = false;
}

}  // namespace pes6online::hooks
