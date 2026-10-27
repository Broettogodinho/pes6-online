#include "settings_redirect.h"

#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>

#include <MinHook.h>

#include <intrin.h>

#include <algorithm>
#include <cstdio>
#include <cwctype>
#include <format>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "dinput_wrap.h"
#include "guid_map.h"
#include "log.h"
#include "settings_builder.h"

namespace pes6online::settings_redirect {
namespace {

using CreateFileAFn = HANDLE(WINAPI*)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
using CreateFileWFn = HANDLE(WINAPI*)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
using DirectInput8CreateFn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);

CreateFileAFn g_create_a = nullptr;
CreateFileWFn g_create_w = nullptr;
thread_local bool t_busy = false;  // evita recursão: o próprio módulo abre arquivos
std::wstring g_virtual_path;

// Padrão Xbox (regra 3): o mapeamento que o Bruno salvou para o controle Xbox
// Bluetooth (045E/02E0) em 2026-10-16. Dado do usuário, não do jogo.
constexpr settings::Mapping kXboxDefault = {0x10, 0x12, 0x13, 0x11, 0x1A, 0x1B, 0x1C, 0x1D,
                                            0x09, 0x08, 0x0E, 0x0F, 0x0D, 0x0B, 0x0A, 0x0C,
                                            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

bool ends_with_settings(std::wstring path) {
  std::transform(path.begin(), path.end(), path.begin(), [](wchar_t c) { return std::towlower(c); });
  std::replace(path.begin(), path.end(), L'/', L'\\');
  static const std::wstring kTail = L"\\konami\\pro evolution soccer 6\\settings.dat";
  return path.size() >= kTail.size() && path.compare(path.size() - kTail.size(), kTail.size(), kTail) == 0;
}

std::wstring widen(LPCSTR s) {
  if (s == nullptr) return {};
  const int n = MultiByteToWideChar(CP_ACP, 0, s, -1, nullptr, 0);
  std::wstring out(n > 0 ? n - 1 : 0, L'\0');
  if (n > 0) MultiByteToWideChar(CP_ACP, 0, s, -1, out.data(), n);
  return out;
}

BOOL CALLBACK collect(LPCDIDEVICEINSTANCEW inst, LPVOID ctx) {
  auto* out = static_cast<std::vector<settings::Controller>*>(ctx);
  settings::Controller c{};
  std::memcpy(c.instance.data(), &inst->guidInstance, 16);
  c.product_key = inst->guidProduct.Data1;
  out->push_back(c);
  return DIENUM_CONTINUE;
}

// Controles conectados, na ordem do DirectInput (INFERIDO: ordem de conexão).
std::vector<settings::Controller> connected_controllers() {
  std::vector<settings::Controller> out;
  wchar_t dir[MAX_PATH] = {};
  const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
  if (n == 0 || n >= MAX_PATH) return out;
  HMODULE sys = LoadLibraryW((std::wstring(dir) + L"\\dinput8.dll").c_str());
  auto create = sys == nullptr ? nullptr
                               : reinterpret_cast<DirectInput8CreateFn>(GetProcAddress(sys, "DirectInput8Create"));
  IDirectInput8W* di = nullptr;
  if (create != nullptr && SUCCEEDED(create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W,
                                            reinterpret_cast<LPVOID*>(&di), nullptr))) {
    di->EnumDevices(DI8DEVCLASS_GAMECTRL, collect, &out, DIEDFL_ATTACHEDONLY);
    di->Release();
  }
  return out;
}

// Monta o arquivo virtual a partir do real. Devolve o caminho a abrir no lugar
// do original, ou vazio para abrir o original.
std::wstring prepare_virtual(const std::wstring& real_path) {
  std::FILE* f = _wfopen(real_path.c_str(), L"rb");
  if (f == nullptr) return {};
  std::vector<std::uint8_t> original(4096);
  original.resize(std::fread(original.data(), 1, original.size(), f));
  std::fclose(f);

  const auto controllers = connected_controllers();
  std::map<std::uint32_t, std::vector<guid_map::Guid>> history;
  for (const auto& c : controllers) {
    if (!history.contains(c.product_key)) history[c.product_key] = dinput_wrap::load_history(c.product_key);
  }
  std::vector<std::string> report;
  const auto built = settings::build(original, controllers, history, kXboxDefault, &report);
  if (built == original) {
    log::write("controles: settings.dat sem mudança (arquivo inválido ou nada a atribuir)");
    return {};
  }

  wchar_t tmp[MAX_PATH] = {};
  GetTempPathW(MAX_PATH, tmp);
  const std::wstring dir = std::wstring(tmp) + L"pes6online";
  CreateDirectoryW(dir.c_str(), nullptr);
  const std::wstring out_path = dir + L"\\settings.dat";
  std::FILE* o = _wfopen(out_path.c_str(), L"wb");
  if (o == nullptr) return {};
  std::fwrite(built.data(), 1, built.size(), o);
  std::fclose(o);

  log::write(std::format("controles: atribuição automática ({} controle(s) conectado(s))", controllers.size()));
  for (const auto& line : report) log::write("controles:   " + line);
  return out_path;
}

// Decide o caminho efetivo para uma abertura do settings.dat.
std::wstring redirect_for(const std::wstring& path, DWORD access) {
  if (t_busy || !ends_with_settings(path)) return {};
  if ((access & (GENERIC_WRITE | FILE_WRITE_DATA | FILE_APPEND_DATA)) != 0) {
    log::write("controles: settings.dat aberto para escrita pelo jogo; repassado sem alteração");
    return {};
  }
  t_busy = true;
  if (g_virtual_path.empty()) g_virtual_path = prepare_virtual(path);  // monta uma vez por execução
  t_busy = false;
  return g_virtual_path;
}

// --- Rastreio de leitura dos saves (SCRUM-22) -------------------------------
// Para achar a rotina que decifra o option file: guarda o handle quando o jogo
// abre um arquivo KONAMI-WIN32PES6*, e registra cada ReadFile dele com a pilha
// de chamadas. Só observa; nada é alterado.
using ReadFileFn = BOOL(WINAPI*)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
ReadFileFn g_read = nullptr;
std::mutex g_trace_mutex;
std::map<HANDLE, std::wstring> g_traced;
int g_trace_logs = 0;

bool is_save_file(const std::wstring& path) {
  std::wstring p = path;
  std::transform(p.begin(), p.end(), p.begin(), [](wchar_t c) { return std::towupper(c); });
  return p.find(L"KONAMI-WIN32PES6") != std::wstring::npos;
}

std::string stack_text(int skip) {
  void* frames[8] = {};
  const USHORT n = RtlCaptureStackBackTrace(static_cast<DWORD>(skip), 8, frames, nullptr);
  std::string out;
  for (USHORT i = 0; i < n; ++i) out += std::format("{}0x{:08X}", i ? " < " : "", reinterpret_cast<std::uintptr_t>(frames[i]));
  return out;
}

int g_open_logs = 0;

void note_open(HANDLE h, const std::wstring& path) {
  {
    // Diagnóstico: registra os primeiros arquivos abertos pelo jogo (caminho e quem abriu).
    std::lock_guard lock(g_trace_mutex);
    // Ignora dispositivos (caminhos que começam com duas barras invertidas, ex.: controles HID).
    const bool device = path.size() >= 2 && path[0] == L'\\' && path[1] == L'\\';
    if (!device && g_open_logs < 150) {
      ++g_open_logs;
      std::string name;
      for (wchar_t c : path) name.push_back(c < 0x80 ? static_cast<char>(c) : '?');
      log::write(std::format("arquivos: abre \"{}\" -> {} ret 0x{:08X}", name,
                             h == INVALID_HANDLE_VALUE ? "falhou" : "ok",
                             reinterpret_cast<std::uintptr_t>(_ReturnAddress())));
    }
  }
  if (h == INVALID_HANDLE_VALUE || !is_save_file(path)) return;
  std::lock_guard lock(g_trace_mutex);
  g_traced[h] = path;
  std::string name;
  for (wchar_t c : path.substr(path.find_last_of(L"\\/") + 1)) name.push_back(c < 0x80 ? static_cast<char>(c) : '?');
  log::write(std::format("saves: aberto {} (handle 0x{:X}) pilha: {}", name, reinterpret_cast<std::uintptr_t>(h),
                         stack_text(2)));
}

BOOL WINAPI read_detour(HANDLE h, LPVOID buf, DWORD want, LPDWORD got, LPOVERLAPPED ov) {
  const BOOL ok = g_read(h, buf, want, got, ov);
  bool traced = false;
  {
    std::lock_guard lock(g_trace_mutex);
    traced = g_traced.contains(h) && g_trace_logs < 200;
    if (traced) ++g_trace_logs;
  }
  if (traced) {
    log::write(std::format("saves: ReadFile handle 0x{:X} buffer 0x{:08X} pedido {} lido {} pilha: {}",
                           reinterpret_cast<std::uintptr_t>(h), reinterpret_cast<std::uintptr_t>(buf), want,
                           got ? *got : 0, stack_text(1)));
  }
  return ok;
}

HANDLE WINAPI create_a_detour(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa, DWORD disp,
                              DWORD flags, HANDLE tmpl) {
  if (name != nullptr && !t_busy) {
    const std::wstring wide = widen(name);
    const std::wstring target = redirect_for(wide, access);
    if (!target.empty()) return g_create_w(target.c_str(), access, share, sa, disp, flags, tmpl);
    const HANDLE h = g_create_a(name, access, share, sa, disp, flags, tmpl);
    note_open(h, wide);
    return h;
  }
  return g_create_a(name, access, share, sa, disp, flags, tmpl);
}

HANDLE WINAPI create_w_detour(LPCWSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa, DWORD disp,
                              DWORD flags, HANDLE tmpl) {
  if (name != nullptr && !t_busy) {
    const std::wstring target = redirect_for(name, access);
    if (!target.empty()) return g_create_w(target.c_str(), access, share, sa, disp, flags, tmpl);
    const HANDLE h = g_create_w(name, access, share, sa, disp, flags, tmpl);
    note_open(h, name);
    return h;
  }
  return g_create_w(name, access, share, sa, disp, flags, tmpl);
}

bool is_game_process() {
  wchar_t path[MAX_PATH] = {};
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  std::wstring p(path);
  std::transform(p.begin(), p.end(), p.begin(), [](wchar_t c) { return std::towlower(c); });
  return p.size() >= 9 && p.compare(p.size() - 9, 9, L"\\pes6.exe") == 0;
}

}  // namespace

void install() {
  if (!is_game_process()) {
    log::write("controles: processo não é o pes6.exe; atribuição automática desligada");
    return;
  }
  const MH_STATUS init = MH_Initialize();
  if (init != MH_OK && init != MH_ERROR_ALREADY_INITIALIZED) {
    log::write(std::format("controles: MinHook não inicializou ({})", MH_StatusToString(init)));
    return;
  }
  const MH_STATUS a = MH_CreateHookApi(L"kernel32", "CreateFileA", reinterpret_cast<LPVOID>(&create_a_detour),
                                       reinterpret_cast<LPVOID*>(&g_create_a));
  const MH_STATUS w = MH_CreateHookApi(L"kernel32", "CreateFileW", reinterpret_cast<LPVOID>(&create_w_detour),
                                       reinterpret_cast<LPVOID*>(&g_create_w));
  if (a != MH_OK || w != MH_OK) {
    log::write(std::format("controles: hooks de CreateFile não criados (A={}, W={})", MH_StatusToString(a),
                           MH_StatusToString(w)));
    return;
  }
  const MH_STATUS r = MH_CreateHookApi(L"kernel32", "ReadFile", reinterpret_cast<LPVOID>(&read_detour),
                                       reinterpret_cast<LPVOID*>(&g_read));
  if (r != MH_OK) log::write(std::format("saves: hook de ReadFile não criado ({})", MH_StatusToString(r)));
  MH_EnableHook(MH_ALL_HOOKS);
  dinput_wrap::set_guid_remap(false);  // o arquivo virtual já usa os GUIDs reais
  log::write("controles: atribuição automática ativa (settings.dat virtual na leitura do jogo)");
}

}  // namespace pes6online::settings_redirect
