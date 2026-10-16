// Proxy do dinput8.dll: o jogo importa DINPUT8.dll, então uma DLL com esse nome
// na pasta do jogo é carregada no lugar da do sistema. Cada exportação repassa a
// chamada para a dinput8.dll verdadeira (System32/SysWOW64), sem alterar nada.

#include <windows.h>
#include <unknwn.h>

#include <string>

#include "dinput_wrap.h"

namespace {
HMODULE g_real = nullptr;

HMODULE real_dinput8() {
  if (g_real != nullptr) return g_real;
  wchar_t dir[MAX_PATH] = {};
  // Num processo 32 bits em Windows 64 bits, devolve SysWOW64.
  const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
  if (n == 0 || n >= MAX_PATH) return nullptr;
  g_real = LoadLibraryW((std::wstring(dir) + L"\\dinput8.dll").c_str());
  return g_real;
}

template <typename Fn>
Fn real(const char* name) {
  HMODULE m = real_dinput8();
  return m == nullptr ? nullptr : reinterpret_cast<Fn>(GetProcAddress(m, name));
}
}  // namespace

extern "C" {

HRESULT WINAPI Proxy_DirectInput8Create(HINSTANCE inst, DWORD version, REFIID riid, LPVOID* out,
                                        LPUNKNOWN outer) {
  using Fn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
  auto fn = real<Fn>("DirectInput8Create");
  if (fn == nullptr) return E_FAIL;
  const HRESULT hr = fn(inst, version, riid, out, outer);
  // Envolve o objeto para estabilizar o GUID dos controles (dinput_wrap.h).
  if (SUCCEEDED(hr) && out != nullptr && outer == nullptr) *out = pes6online::dinput_wrap::wrap(riid, *out);
  return hr;
}

HRESULT WINAPI Proxy_DllCanUnloadNow() {
  using Fn = HRESULT(WINAPI*)();
  auto fn = real<Fn>("DllCanUnloadNow");
  return fn == nullptr ? S_FALSE : fn();
}

HRESULT WINAPI Proxy_DllGetClassObject(REFCLSID clsid, REFIID riid, LPVOID* out) {
  using Fn = HRESULT(WINAPI*)(REFCLSID, REFIID, LPVOID*);
  auto fn = real<Fn>("DllGetClassObject");
  return fn == nullptr ? E_FAIL : fn(clsid, riid, out);
}

HRESULT WINAPI Proxy_DllRegisterServer() {
  using Fn = HRESULT(WINAPI*)();
  auto fn = real<Fn>("DllRegisterServer");
  return fn == nullptr ? E_FAIL : fn();
}

HRESULT WINAPI Proxy_DllUnregisterServer() {
  using Fn = HRESULT(WINAPI*)();
  auto fn = real<Fn>("DllUnregisterServer");
  return fn == nullptr ? E_FAIL : fn();
}

const void* WINAPI Proxy_GetdfDIJoystick() {
  using Fn = const void*(WINAPI*)();
  auto fn = real<Fn>("GetdfDIJoystick");
  return fn == nullptr ? nullptr : fn();
}

}  // extern "C"
