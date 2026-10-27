#include "dinput_wrap.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <shlobj.h>

#include <intrin.h>

#include <cstdio>
#include <cstring>
#include <format>
#include <map>
#include <set>
#include <string_view>
#include <string>
#include <vector>

#include <type_traits>

#include "guid_map.h"
#include "log.h"
#include "mem_diff.h"

namespace pes6online::dinput_wrap {
namespace {

using guid_map::Guid;

Guid to_bytes(const GUID& g) {
  Guid b{};
  std::memcpy(b.data(), &g, 16);
  return b;
}

GUID from_bytes(const Guid& b) {
  GUID g{};
  std::memcpy(&g, b.data(), 16);
  return g;
}

std::string text(const Guid& b) {
  const GUID g = from_bytes(b);
  return std::format("{{{:08X}-{:04X}-{:04X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}}}", g.Data1, g.Data2,
                     g.Data3, g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3], g.Data4[4], g.Data4[5], g.Data4[6],
                     g.Data4[7]);
}

// GUIDs de instância salvos no settings.dat do jogo (pasta Documentos do usuário).
std::vector<Guid> load_saved() {
  wchar_t docs[MAX_PATH] = {};
  if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, 0, docs))) return {};
  const std::wstring path = std::wstring(docs) + L"\\KONAMI\\Pro Evolution Soccer 6\\settings.dat";
  std::FILE* f = _wfopen(path.c_str(), L"rb");
  if (f == nullptr) return {};
  std::vector<std::uint8_t> data(4096);
  const std::size_t n = std::fread(data.data(), 1, data.size(), f);
  std::fclose(f);
  return guid_map::saved_guids_from_settings(data.data(), n);
}

}  // namespace

// GUIDs de instância que o Windows já atribuiu a um modelo (VID/PID), pelo
// registro do DirectInput: ...\DirectInput\VID_xxxx&PID_yyyy\Calibration\N\GUID.
std::vector<guid_map::Guid> load_history(std::uint32_t product_key) {
  using guid_map::Guid;
  std::vector<Guid> out;
  const unsigned vid = product_key & 0xFFFF;
  const unsigned pid = product_key >> 16;
  const std::wstring base =
      std::format(L"System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\DirectInput\\"
                  L"VID_{:04X}&PID_{:04X}\\Calibration",
                  vid, pid);
  HKEY root = nullptr;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, base.c_str(), 0, KEY_READ, &root) != ERROR_SUCCESS) return out;
  wchar_t name[64];
  for (DWORD i = 0;; ++i) {
    DWORD len = 64;
    if (RegEnumKeyExW(root, i, name, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
    Guid g{};
    DWORD size = sizeof(g);
    if (RegGetValueW(root, name, L"GUID", RRF_RT_REG_BINARY, nullptr, g.data(), &size) == ERROR_SUCCESS &&
        size == 16) {
      out.push_back(g);
    }
  }
  RegCloseKey(root);
  return out;
}

namespace {

bool g_remap = true;

#include "dinput_device_wrap.inl"

struct TraitsA {
  using DI = IDirectInput8A;
  using Dev = IDirectInputDevice8A;
  using Inst = DIDEVICEINSTANCEA;
  using EnumCb = LPDIENUMDEVICESCALLBACKA;
  using Str = LPCSTR;
  using ActionFormat = LPDIACTIONFORMATA;
  using SemCb = LPDIENUMDEVICESBYSEMANTICSCBA;
  using CfgParams = LPDICONFIGUREDEVICESPARAMSA;
  static const IID& iid() { return IID_IDirectInput8A; }
};

struct TraitsW {
  using DI = IDirectInput8W;
  using Dev = IDirectInputDevice8W;
  using Inst = DIDEVICEINSTANCEW;
  using EnumCb = LPDIENUMDEVICESCALLBACKW;
  using Str = LPCWSTR;
  using ActionFormat = LPDIACTIONFORMATW;
  using SemCb = LPDIENUMDEVICESBYSEMANTICSCBW;
  using CfgParams = LPDICONFIGUREDEVICESPARAMSW;
  static const IID& iid() { return IID_IDirectInput8W; }
};

template <typename T>
class DirectInputWrap final : public T::DI {
 public:
  explicit DirectInputWrap(typename T::DI* real) : real_(real) {}

  // IUnknown
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out) override {
    if (out == nullptr) return E_POINTER;
    if (riid == IID_IUnknown || riid == T::iid()) {
      *out = this;
      AddRef();
      return S_OK;
    }
    return real_->QueryInterface(riid, out);
  }
  ULONG STDMETHODCALLTYPE AddRef() override { return ++refs_; }
  ULONG STDMETHODCALLTYPE Release() override {
    const ULONG left = --refs_;
    if (left == 0) {
      real_->Release();
      delete this;
    }
    return left;
  }

  // IDirectInput8
  HRESULT STDMETHODCALLTYPE CreateDevice(REFGUID guid, typename T::Dev** dev, LPUNKNOWN outer) override {
    const GUID real = to_real(guid);
    const HRESULT hr = real_->CreateDevice(real, dev, outer);
    if constexpr (std::is_same_v<typename T::Dev, IDirectInputDevice8W>) {
      // Envolve o dispositivo para observar a leitura (só a versão W, que o jogo usa).
      if (SUCCEEDED(hr) && dev != nullptr && *dev != nullptr && outer == nullptr) {
        *dev = new DeviceWrapW(*dev, describe(real));
      }
    }
    return hr;
  }

  HRESULT STDMETHODCALLTYPE EnumDevices(DWORD type, typename T::EnumCb cb, LPVOID ref, DWORD flags) override {
    std::vector<typename T::Inst> items;
    const HRESULT hr = real_->EnumDevices(type, &collect, &items, flags);
    if (FAILED(hr) || cb == nullptr) return hr;

    std::vector<guid_map::RealDevice> devices;
    std::map<std::uint32_t, std::vector<Guid>> history;
    for (const auto& it : items) {
      const std::uint32_t key = it.guidProduct.Data1;
      devices.push_back({to_bytes(it.guidInstance), key});
      if (!history.contains(key)) history[key] = load_history(key);
    }
    const auto presented = guid_map::decide(devices, load_saved(), history);

    for (std::size_t i = 0; i < items.size(); ++i) {
      typename T::Inst copy = items[i];
      if (g_remap && presented[i] != devices[i].instance) {
        presented_to_real_[presented[i]] = items[i].guidInstance;
        copy.guidInstance = from_bytes(presented[i]);
        const auto seen = logged_.find(presented[i]);
        if (seen == logged_.end() || seen->second != devices[i].instance) {
          logged_[presented[i]] = devices[i].instance;
          log::write(std::format("controles: GUID real {} apresentado como {} (mapeamento salvo)",
                                 text(devices[i].instance), text(presented[i])));
        }
      }
      if (cb(&copy, ref) == DIENUM_STOP) break;
    }
    return hr;
  }

  HRESULT STDMETHODCALLTYPE GetDeviceStatus(REFGUID guid) override { return real_->GetDeviceStatus(to_real(guid)); }
  HRESULT STDMETHODCALLTYPE RunControlPanel(HWND owner, DWORD flags) override {
    return real_->RunControlPanel(owner, flags);
  }
  HRESULT STDMETHODCALLTYPE Initialize(HINSTANCE inst, DWORD version) override {
    return real_->Initialize(inst, version);
  }
  HRESULT STDMETHODCALLTYPE FindDevice(REFGUID cls, typename T::Str name, LPGUID out) override {
    const HRESULT hr = real_->FindDevice(cls, name, out);
    if (SUCCEEDED(hr) && out != nullptr) {
      for (const auto& [presented, real] : presented_to_real_) {
        if (IsEqualGUID(real, *out)) *out = from_bytes(presented);
      }
    }
    return hr;
  }
  HRESULT STDMETHODCALLTYPE EnumDevicesBySemantics(typename T::Str user, typename T::ActionFormat af,
                                                   typename T::SemCb cb, LPVOID ref, DWORD flags) override {
    return real_->EnumDevicesBySemantics(user, af, cb, ref, flags);
  }
  HRESULT STDMETHODCALLTYPE ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK cb, typename T::CfgParams params,
                                             DWORD flags, LPVOID ref) override {
    return real_->ConfigureDevices(cb, params, flags, ref);
  }

 private:
  static BOOL PASCAL collect(const typename T::Inst* inst, LPVOID ctx) {
    static_cast<std::vector<typename T::Inst>*>(ctx)->push_back(*inst);
    return DIENUM_CONTINUE;
  }

  std::string describe(const GUID& real) const {
    if (IsEqualGUID(real, GUID_SysKeyboard)) return "teclado";
    if (IsEqualGUID(real, GUID_SysMouse)) return "mouse";
    return text(to_bytes(real));
  }

  GUID to_real(REFGUID guid) const {
    const auto it = presented_to_real_.find(to_bytes(guid));
    return it == presented_to_real_.end() ? guid : it->second;
  }

  typename T::DI* real_;
  ULONG refs_ = 1;
  std::map<Guid, GUID> presented_to_real_;
  std::map<Guid, Guid> logged_;
};

}  // namespace

void set_guid_remap(bool on) { g_remap = on; }

void* wrap(REFIID riid, void* real) {
  if (real == nullptr) return real;
  if (riid == IID_IDirectInput8A) {
    log::write("controles: IDirectInput8A envolvido (GUID estável dos controles ativo)");
    return static_cast<IDirectInput8A*>(new DirectInputWrap<TraitsA>(static_cast<IDirectInput8A*>(real)));
  }
  if (riid == IID_IDirectInput8W) {
    log::write("controles: IDirectInput8W envolvido (GUID estável dos controles ativo)");
    return static_cast<IDirectInput8W*>(new DirectInputWrap<TraitsW>(static_cast<IDirectInput8W*>(real)));
  }
  return real;
}

}  // namespace pes6online::dinput_wrap
