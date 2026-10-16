#include "input_watch.h"

#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>

#include <format>
#include <string>
#include <vector>

#include "log.h"

namespace pes6online::input_watch {
namespace {

using DirectInput8CreateFn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);

std::string guid_text(const GUID& g) {
  return std::format("{{{:08X}-{:04X}-{:04X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}}}", g.Data1, g.Data2,
                     g.Data3, g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3], g.Data4[4], g.Data4[5], g.Data4[6],
                     g.Data4[7]);
}

std::string narrow(const wchar_t* w) {
  std::string out;
  for (; *w != 0; ++w) out.push_back(*w < 0x80 ? static_cast<char>(*w) : '?');
  return out;
}

BOOL CALLBACK collect(LPCDIDEVICEINSTANCEW inst, LPVOID ctx) {
  auto* lines = static_cast<std::vector<std::string>*>(ctx);
  lines->push_back(std::format("instancia={} produto={} nome=\"{}\"", guid_text(inst->guidInstance),
                               guid_text(inst->guidProduct), narrow(inst->tszInstanceName)));
  return DIENUM_CONTINUE;
}

DWORD WINAPI watch_thread(LPVOID) {
  // Usa a dinput8 do sistema diretamente (não a nossa proxy) com uma instância
  // própria: não interfere nos objetos do jogo.
  wchar_t dir[MAX_PATH] = {};
  const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
  if (n == 0 || n >= MAX_PATH) return 0;
  HMODULE sys = LoadLibraryW((std::wstring(dir) + L"\\dinput8.dll").c_str());
  auto create = sys == nullptr ? nullptr
                               : reinterpret_cast<DirectInput8CreateFn>(GetProcAddress(sys, "DirectInput8Create"));
  IDirectInput8W* di = nullptr;
  if (create == nullptr ||
      FAILED(create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W,
                    reinterpret_cast<LPVOID*>(&di), nullptr))) {
    log::write("controles: não foi possível criar o DirectInput para o diagnóstico");
    return 0;
  }
  log::write("controles: diagnóstico iniciado (lista a cada 5 s, registra mudanças)");
  std::vector<std::string> last;
  bool first = true;
  for (;;) {
    std::vector<std::string> now;
    di->EnumDevices(DI8DEVCLASS_GAMECTRL, collect, &now, DIEDFL_ATTACHEDONLY);
    if (first || now != last) {
      log::write(std::format("controles: {} conectado(s){}", now.size(), first ? "" : " (mudou)"));
      for (std::size_t i = 0; i < now.size(); ++i) log::write(std::format("controles:   #{} {}", i, now[i]));
      last = std::move(now);
      first = false;
    }
    Sleep(5000);
  }
}

}  // namespace

void start() {
  if (HANDLE t = CreateThread(nullptr, 0, watch_thread, nullptr, 0, nullptr)) CloseHandle(t);
}

}  // namespace pes6online::input_watch
