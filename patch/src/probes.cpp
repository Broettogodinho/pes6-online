#include "probes.h"

#include <atomic>
#include <format>
#include <string>

#include <windows.h>

#include "log.h"

namespace pes6online::probes {
namespace {

// Texto curto e imprimível em `p`, ou vazio se não for legível como string.
std::string peek_text(const void* p) {
  if (p == nullptr) return {};
  MEMORY_BASIC_INFORMATION mbi{};
  if (VirtualQuery(p, &mbi, sizeof(mbi)) == 0 || mbi.State != MEM_COMMIT ||
      (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) {
    return {};
  }
  const auto* s = static_cast<const unsigned char*>(p);
  const auto* end = static_cast<const unsigned char*>(mbi.BaseAddress) + mbi.RegionSize;
  std::string out;
  for (; s < end && *s != 0 && out.size() < 48; ++s) {
    if (*s < 0x20 || *s > 0x7e) return {};
    out.push_back(static_cast<char>(*s));
  }
  return out;
}

// ---------------------------------------------------------------------------
// FUN_00864ac0(value, slot): grava `value` na tabela global de 23 posições em
// 0x01132040 (switch 0..0x16). Chamada por FUN_00808490 com a tabela de 10 pares
// (nome, id) em 0x00C8EB38 ("Club House", "International League", ...).
// Convenção: cdecl, 2 argumentos (o chamador faz add esp, 8). Ver
// re/notes/FUN_00864ac0.md.
using SlotSetFn = void(__cdecl*)(void* value, int slot);
SlotSetFn g_slot_set_original = nullptr;
std::atomic<int> g_slot_set_calls{0};
constexpr int kSlotSetLogLimit = 200;

void __cdecl slot_set_detour(void* value, int slot) {
  const int n = ++g_slot_set_calls;
  if (n <= kSlotSetLogLimit) {
    const std::string text = peek_text(value);
    log::write(std::format("probe FUN_00864ac0 #{}: slot={} value=0x{:08X}{}", n, slot,
                           reinterpret_cast<std::uintptr_t>(value),
                           text.empty() ? "" : std::format(" \"{}\"", text)));
  } else if (n == kSlotSetLogLimit + 1) {
    log::write("probe FUN_00864ac0: limite de log atingido; chamadas seguintes não são registradas");
  }
  g_slot_set_original(value, slot);
}

const hooks::HookSpec kSpecs[] = {
    {"FUN_00864ac0 (sonda)", 0x00864AC0,
     "8B 44 24 08 83 F8 16 0F 87 ?? ?? ?? ?? FF 24 85 ?? ?? ?? ??",
     reinterpret_cast<void*>(&slot_set_detour), reinterpret_cast<void**>(&g_slot_set_original)},
};

}  // namespace

const hooks::HookSpec* specs(int* count) {
  *count = static_cast<int>(sizeof(kSpecs) / sizeof(kSpecs[0]));
  return kSpecs;
}

}  // namespace pes6online::probes
