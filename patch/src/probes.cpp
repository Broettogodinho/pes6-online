#include "probes.h"

#include <intrin.h>

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

// Primeiros `n` bytes em `p` em hex ("?" se a memória não for legível).
std::string peek_bytes(const void* p, std::size_t n) {
  MEMORY_BASIC_INFORMATION mbi{};
  if (p == nullptr || VirtualQuery(p, &mbi, sizeof(mbi)) == 0 || mbi.State != MEM_COMMIT ||
      (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) {
    return "?";
  }
  const auto* s = static_cast<const unsigned char*>(p);
  const auto* end = static_cast<const unsigned char*>(mbi.BaseAddress) + mbi.RegionSize;
  std::string out;
  for (std::size_t i = 0; i < n && s + i < end; ++i) {
    if (!out.empty()) out.push_back(' ');
    out += std::format("{:02X}", s[i]);
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
  // Endereço de retorno = instrução logo após o call no chamador (cruzar no Ghidra).
  const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  const int n = ++g_slot_set_calls;
  if (n <= kSlotSetLogLimit) {
    const std::string text = peek_text(value);
    log::write(std::format("probe FUN_00864ac0 #{}: slot={} value=0x{:08X} ret=0x{:08X} bytes=[{}]{}", n, slot,
                           reinterpret_cast<std::uintptr_t>(value), caller, peek_bytes(value, 16),
                           text.empty() ? "" : std::format(" \"{}\"", text)));
  } else if (n == kSlotSetLogLimit + 1) {
    log::write("probe FUN_00864ac0: limite de log atingido; chamadas seguintes não são registradas");
  }
  g_slot_set_original(value, slot);
}

// ---------------------------------------------------------------------------
// FUN_00666a20(id): troca a tela atual (cena[0] = id; cena em PTR_DAT_00bd7c7c).
// O despachante FUN_0068d260 faz switch sobre a tela atual (cmp eax, 0x49:
// 74 telas). Convenção: cdecl, 1 argumento (mov ecx,[esp+4]; ...; ret).
// Ver re/notes/telas.md.
using SceneSetFn = void(__cdecl*)(int id);
SceneSetFn g_scene_set_original = nullptr;
std::atomic<int> g_scene_set_calls{0};
constexpr int kSceneSetLogLimit = 500;
constexpr std::uintptr_t kScenePtr = 0x00BD7C7C;  // PTR_DAT_00bd7c7c

int current_scene() {
  const auto* obj = *reinterpret_cast<int* const*>(kScenePtr);
  return obj == nullptr ? -1 : obj[0];
}

void __cdecl scene_set_detour(int id) {
  const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  const int before = current_scene();
  const int n = ++g_scene_set_calls;
  if (n <= kSceneSetLogLimit) {
    log::write(std::format("probe tela #{}: {} -> {} (0x{:02X}) ret=0x{:08X}", n, before, id, id, caller));
  } else if (n == kSceneSetLogLimit + 1) {
    log::write("probe tela: limite de log atingido");
  }
  g_scene_set_original(id);
}

const hooks::HookSpec kSpecs[] = {
    {"FUN_00666a20 (sonda de telas)", 0x00666A20,
     "A1 ?? ?? ?? ?? 85 C0 74 06 8B 4C 24 04 89 08 C3 A1 ?? ?? ?? ??",
     reinterpret_cast<void*>(&scene_set_detour), reinterpret_cast<void**>(&g_scene_set_original)},
    {"FUN_00864ac0 (sonda)", 0x00864AC0,
     "8B 44 24 08 83 F8 16 0F 87 ?? ?? ?? ?? FF 24 85 ?? ?? ?? ??",
     reinterpret_cast<void*>(&slot_set_detour), reinterpret_cast<void**>(&g_slot_set_original)},
};

}  // namespace

namespace {

// Lê um valor só se a memória estiver legível; devolve `fallback` caso contrário.
template <typename T>
T safe_read(std::uintptr_t addr, T fallback) {
  MEMORY_BASIC_INFORMATION mbi{};
  const void* p = reinterpret_cast<const void*>(addr);
  if (addr == 0 || VirtualQuery(p, &mbi, sizeof(mbi)) == 0 || mbi.State != MEM_COMMIT ||
      (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) {
    return fallback;
  }
  return *reinterpret_cast<const T*>(addr);
}

struct WatchedState {
  std::uint32_t scene_obj = 0;  // PTR_DAT_00bd7c7c
  int scene = -1;               // [obj+0]
  int step = -1;                // [obj+4]
  int mode = -1;                // DAT_03be12c9 (byte)
  bool operator==(const WatchedState&) const = default;
};

WatchedState read_state() {
  WatchedState s;
  s.scene_obj = safe_read<std::uint32_t>(kScenePtr, 0);
  if (s.scene_obj != 0) {
    s.scene = safe_read<int>(s.scene_obj, -1);
    s.step = safe_read<int>(s.scene_obj + 4, -1);
  }
  s.mode = safe_read<std::uint8_t>(0x03BE12C9, 0xFF);
  return s;
}

DWORD WINAPI state_watch_thread(LPVOID) {
  WatchedState last{};
  last.scene_obj = 0xFFFFFFFF;  // força o primeiro registro
  int changes = 0;
  constexpr int kLimit = 2000;
  for (;;) {
    const WatchedState now = read_state();
    if (!(now == last)) {
      if (++changes <= kLimit) {
        log::write(std::format("estado: tela={} passo={} modo=0x{:02X} obj=0x{:08X}", now.scene, now.step,
                               now.mode, now.scene_obj));
      } else if (changes == kLimit + 1) {
        log::write("estado: limite de log atingido");
      }
      last = now;
    }
    Sleep(100);
  }
}

}  // namespace

void start_state_watch() {
  if (HANDLE t = CreateThread(nullptr, 0, state_watch_thread, nullptr, 0, nullptr)) CloseHandle(t);
  log::write("estado: observação iniciada (tela, passo e modo a cada 100 ms)");
}

const hooks::HookSpec* specs(int* count) {
  *count = static_cast<int>(sizeof(kSpecs) / sizeof(kSpecs[0]));
  return kSpecs;
}

}  // namespace pes6online::probes
