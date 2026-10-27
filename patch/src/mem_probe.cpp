// Sonda de memória (SCRUM-22): procura na memória do jogo o conteúdo já
// decifrado do option file e grava uma cópia na pasta do jogo, para derivar a
// chave da cifra comparando com o arquivo em disco (re/notes/option-file.md).
//
// Só LÊ a memória. A configuração vem de `pes6online_probe.txt` na pasta do
// jogo (gerado localmente; contém um trecho dos dados do usuário e nunca vai
// para o repositório). Sem esse arquivo, a sonda não faz nada.
//   opt_offset=<deslocamento do padrão dentro do arquivo>
//   opt_size=<tamanho do arquivo>
//   opt_pattern=<bytes em hex, ?? = curinga>

#include "mem_probe.h"

#include <windows.h>

#include <cstdio>
#include <format>
#include <fstream>
#include <string>

#include "log.h"
#include "signature.h"

namespace pes6online::mem_probe {
namespace {

std::wstring game_dir() {
  wchar_t path[MAX_PATH] = {};
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  std::wstring p(path);
  return p.substr(0, p.find_last_of(L"\\/") + 1);
}

bool readable(const MEMORY_BASIC_INFORMATION& m) {
  if (m.State != MEM_COMMIT || (m.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) return false;
  return (m.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ |
                       PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
}

DWORD WINAPI probe_thread(LPVOID) {
  std::ifstream cfg(game_dir() + L"pes6online_probe.txt");
  if (!cfg) return 0;
  std::size_t offset = 0, size = 0;
  std::string pattern_text, line;
  while (std::getline(cfg, line)) {
    if (line.rfind("opt_offset=", 0) == 0) offset = std::stoul(line.substr(11));
    else if (line.rfind("opt_size=", 0) == 0) size = std::stoul(line.substr(9));
    else if (line.rfind("opt_pattern=", 0) == 0) pattern_text = line.substr(12);
  }
  const auto pattern = sig::parse(pattern_text);
  if (!pattern || size == 0) {
    log::write("sonda-memória: configuração inválida");
    return 0;
  }
  log::write("sonda-memória: aguardando o jogo carregar o option file (20 s)");
  Sleep(20000);

  int found = 0;
  auto* addr = static_cast<std::uint8_t*>(nullptr);
  MEMORY_BASIC_INFORMATION mbi{};
  while (VirtualQuery(addr, &mbi, sizeof(mbi)) == sizeof(mbi)) {
    auto* base = static_cast<std::uint8_t*>(mbi.BaseAddress);
    if (readable(mbi) && mbi.RegionSize >= pattern->size()) {
      for (std::size_t i = 0; i + pattern->size() <= mbi.RegionSize; i += 4) {
        if (!sig::matches_at(base + i, mbi.RegionSize - i, *pattern)) continue;
        ++found;
        auto* start = base + i - offset;  // início presumido da cópia decifrada
        const bool fits = i >= offset && i - offset + size <= mbi.RegionSize;
        log::write(std::format("sonda-memória: padrão #{} em 0x{:08X} (região 0x{:08X}, {} bytes){}", found,
                               reinterpret_cast<std::uintptr_t>(base + i),
                               reinterpret_cast<std::uintptr_t>(base), mbi.RegionSize,
                               fits ? "" : " — bloco não cabe na região"));
        if (fits && found <= 3) {
          const std::wstring out = game_dir() + std::format(L"pes6online_opt_dump_{}.bin", found);
          if (std::FILE* f = _wfopen(out.c_str(), L"wb")) {
            std::fwrite(start, 1, size, f);
            std::fclose(f);
            log::write(std::format("sonda-memória: {} bytes gravados (dump {})", size, found));
          }
        }
      }
    }
    addr = base + mbi.RegionSize;
    if (reinterpret_cast<std::uintptr_t>(addr) >= 0x7FFF0000) break;
  }
  log::write(std::format("sonda-memória: varredura concluída, {} ocorrência(s)", found));
  return 0;
}

}  // namespace

void start() {
  if (HANDLE t = CreateThread(nullptr, 0, probe_thread, nullptr, 0, nullptr)) CloseHandle(t);
}

}  // namespace pes6online::mem_probe
