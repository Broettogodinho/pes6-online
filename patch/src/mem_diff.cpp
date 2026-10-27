#include "mem_diff.h"

#include <cstdio>
#include <cstring>
#include <format>
#include <string>
#include <vector>

#include "log.h"

namespace pes6online::mem_diff {
namespace {

// Seção .data do pes6.exe na versão-alvo (re/symbols/target.toml): VA e tamanho virtual.
constexpr std::uintptr_t kDataStart = 0x00BC4000;
constexpr std::size_t kDataSize = 0x03195000;

struct Step {
  ULONGLONG at;
  std::string rel;
};

bool g_loaded = false;
std::vector<Step> g_steps;
std::size_t g_next = 0;
std::vector<std::uint8_t> g_base, g_prev, g_now;
std::vector<bool> g_alive;  // candidato ainda vivo, por byte
bool g_done = false;

void load() {
  g_loaded = true;
  wchar_t path[MAX_PATH] = {};
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  std::wstring dir(path);
  dir = dir.substr(0, dir.find_last_of(L"\\/") + 1);
  std::FILE* f = _wfopen((dir + L"pes6online_memdiff.txt").c_str(), L"r");
  if (f == nullptr) return;
  unsigned long long at = 0;
  char rel[16] = {};
  while (std::fscanf(f, "%llu %15s", &at, rel) == 2) g_steps.push_back({at, rel});
  std::fclose(f);
  log::write(std::format("memdiff: roteiro com {} foto(s) da seção .data", g_steps.size()));
}

// Copia a seção .data; páginas não comprometidas ficam zeradas.
void snapshot(std::vector<std::uint8_t>& out) {
  out.assign(kDataSize, 0);
  std::uintptr_t a = kDataStart;
  const std::uintptr_t end = kDataStart + kDataSize;
  while (a < end) {
    MEMORY_BASIC_INFORMATION m{};
    if (VirtualQuery(reinterpret_cast<void*>(a), &m, sizeof(m)) == 0) break;
    const std::uintptr_t rbase = reinterpret_cast<std::uintptr_t>(m.BaseAddress);
    const std::uintptr_t rend = std::min<std::uintptr_t>(rbase + m.RegionSize, end);
    const bool ok = m.State == MEM_COMMIT && (m.Protect & (PAGE_NOACCESS | PAGE_GUARD)) == 0;
    if (ok) std::memcpy(out.data() + (a - kDataStart), reinterpret_cast<void*>(a), rend - a);
    a = rend;
  }
}

bool keep(const std::string& rel, std::uint8_t base, std::uint8_t prev, std::uint8_t now) {
  if (rel == "inc") return now == static_cast<std::uint8_t>(prev + 1);
  if (rel == "dec") return now == static_cast<std::uint8_t>(prev - 1);
  if (rel == "same") return now == prev;
  if (rel == "changed") return now != prev;
  if (rel == "eqbase") return now == base;
  if (rel == "nebase") return now != base;
  return true;
}

void report() {
  std::size_t alive = 0;
  for (bool b : g_alive) alive += b;
  log::write(std::format("memdiff: {} candidato(s) após {} foto(s)", alive, g_steps.size()));
  int shown = 0;
  for (std::size_t i = 0; i < g_alive.size() && shown < 300; ++i) {
    if (!g_alive[i]) continue;
    ++shown;
    log::write(std::format("memdiff:   0x{:08X} base={} final={}", kDataStart + i, g_base[i], g_now[i]));
  }
}

}  // namespace

void on_tick(ULONGLONG t) {
  if (!g_loaded) load();
  if (g_done || g_next >= g_steps.size() || t < g_steps[g_next].at) return;
  const Step& s = g_steps[g_next];
  if (g_next == 0) {
    snapshot(g_base);
    g_prev = g_base;
    g_alive.assign(kDataSize, true);
  } else {
    snapshot(g_now);
    for (std::size_t i = 0; i < kDataSize; ++i) {
      if (g_alive[i] && !keep(s.rel, g_base[i], g_prev[i], g_now[i])) g_alive[i] = false;
    }
    g_prev.swap(g_now);
    g_now = g_prev;  // g_now guarda a última foto para o relatório
  }
  std::size_t alive = 0;
  for (bool b : g_alive) alive += b;
  log::write(std::format("memdiff: foto {} ({}) aos {} ms: {} candidato(s)", g_next, s.rel, t, alive));
  if (++g_next == g_steps.size()) {
    g_done = true;
    report();
  }
}

}  // namespace pes6online::mem_diff
