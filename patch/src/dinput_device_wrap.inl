// Envoltório do IDirectInputDevice8W (incluído por dinput_wrap.cpp).
//
// Observa, sem alterar nada, como o jogo lê cada dispositivo: formato de dados,
// cooperação, aquisição e cada GetDeviceState/GetDeviceData (quem chama, a
// frequência, o resultado e as mudanças de botões e direções). É o ponto onde
// a entrada chega ao jogo; no netplay, é onde se captura a entrada local e se
// injeta a remota (SCRUM-38/56).

class DeviceWrapW final : public IDirectInputDevice8W {
 public:
  DeviceWrapW(IDirectInputDevice8W* real, std::string label) : real_(real), label_(std::move(label)) {
    log::write(std::format("entrada: dispositivo aberto {}", label_));
  }

  // IUnknown
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out) override {
    if (out == nullptr) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IDirectInputDevice8W) {
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
      log::write(std::format("entrada: dispositivo liberado {} ({} leituras)", label_, reads_));
      real_->Release();
      delete this;
    }
    return left;
  }

  // Métodos observados
  HRESULT STDMETHODCALLTYPE SetDataFormat(LPCDIDATAFORMAT fmt) override {
    const HRESULT hr = real_->SetDataFormat(fmt);
    if (fmt != nullptr) {
      log::write(std::format("entrada: {} SetDataFormat dwDataSize={} objetos={} -> 0x{:08X}", label_,
                             fmt->dwDataSize, fmt->dwNumObjs, static_cast<std::uint32_t>(hr)));
    }
    return hr;
  }
  HRESULT STDMETHODCALLTYPE SetCooperativeLevel(HWND wnd, DWORD flags) override {
    const HRESULT hr = real_->SetCooperativeLevel(wnd, flags);
    log::write(std::format("entrada: {} SetCooperativeLevel flags=0x{:X} -> 0x{:08X}", label_, flags,
                           static_cast<std::uint32_t>(hr)));
    return hr;
  }
  HRESULT STDMETHODCALLTYPE Acquire() override {
    const HRESULT hr = real_->Acquire();
    note_result("Acquire", hr);
    return hr;
  }
  HRESULT STDMETHODCALLTYPE Unacquire() override {
    const HRESULT hr = real_->Unacquire();
    note_result("Unacquire", hr);
    return hr;
  }
  HRESULT STDMETHODCALLTYPE Poll() override {
    const HRESULT hr = real_->Poll();
    if (FAILED(hr)) note_result("Poll", hr);
    return hr;
  }
  HRESULT STDMETHODCALLTYPE GetDeviceState(DWORD size, LPVOID data) override {
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    const HRESULT hr = real_->GetDeviceState(size, data);
    ++reads_;
    if (callers_.insert(caller).second) {
      log::write(std::format("entrada: {} GetDeviceState(size={}) chamado de ret=0x{:08X}", label_, size, caller));
    }
    if (FAILED(hr)) {
      note_result("GetDeviceState", hr);
    } else if (data != nullptr) {
      if (size == 256) inject_keys(static_cast<std::uint8_t*>(data));
      observe_state(size, static_cast<const std::uint8_t*>(data));
    }
    rate_tick();
    return hr;
  }
  HRESULT STDMETHODCALLTYPE GetDeviceData(DWORD objSize, LPDIDEVICEOBJECTDATA items, LPDWORD count,
                                          DWORD flags) override {
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    const HRESULT hr = real_->GetDeviceData(objSize, items, count, flags);
    if (callers_.insert(caller).second) {
      log::write(std::format("entrada: {} GetDeviceData chamado de ret=0x{:08X}", label_, caller));
    }
    if (FAILED(hr)) note_result("GetDeviceData", hr);
    return hr;
  }

  // Métodos só repassados
  HRESULT STDMETHODCALLTYPE GetCapabilities(LPDIDEVCAPS caps) override { return real_->GetCapabilities(caps); }
  HRESULT STDMETHODCALLTYPE EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKW cb, LPVOID ref, DWORD flags) override {
    return real_->EnumObjects(cb, ref, flags);
  }
  HRESULT STDMETHODCALLTYPE GetProperty(REFGUID prop, LPDIPROPHEADER hdr) override {
    return real_->GetProperty(prop, hdr);
  }
  HRESULT STDMETHODCALLTYPE SetProperty(REFGUID prop, LPCDIPROPHEADER hdr) override {
    return real_->SetProperty(prop, hdr);
  }
  HRESULT STDMETHODCALLTYPE SetEventNotification(HANDLE ev) override { return real_->SetEventNotification(ev); }
  HRESULT STDMETHODCALLTYPE GetObjectInfo(LPDIDEVICEOBJECTINSTANCEW info, DWORD obj, DWORD how) override {
    return real_->GetObjectInfo(info, obj, how);
  }
  HRESULT STDMETHODCALLTYPE GetDeviceInfo(LPDIDEVICEINSTANCEW info) override { return real_->GetDeviceInfo(info); }
  HRESULT STDMETHODCALLTYPE RunControlPanel(HWND owner, DWORD flags) override {
    return real_->RunControlPanel(owner, flags);
  }
  HRESULT STDMETHODCALLTYPE Initialize(HINSTANCE inst, DWORD ver, REFGUID guid) override {
    return real_->Initialize(inst, ver, guid);
  }
  HRESULT STDMETHODCALLTYPE CreateEffect(REFGUID guid, LPCDIEFFECT eff, LPDIRECTINPUTEFFECT* out,
                                         LPUNKNOWN outer) override {
    return real_->CreateEffect(guid, eff, out, outer);
  }
  HRESULT STDMETHODCALLTYPE EnumEffects(LPDIENUMEFFECTSCALLBACKW cb, LPVOID ref, DWORD type) override {
    return real_->EnumEffects(cb, ref, type);
  }
  HRESULT STDMETHODCALLTYPE GetEffectInfo(LPDIEFFECTINFOW info, REFGUID guid) override {
    return real_->GetEffectInfo(info, guid);
  }
  HRESULT STDMETHODCALLTYPE GetForceFeedbackState(LPDWORD out) override { return real_->GetForceFeedbackState(out); }
  HRESULT STDMETHODCALLTYPE SendForceFeedbackCommand(DWORD flags) override {
    return real_->SendForceFeedbackCommand(flags);
  }
  HRESULT STDMETHODCALLTYPE EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK cb, LPVOID ref,
                                                     DWORD flags) override {
    return real_->EnumCreatedEffectObjects(cb, ref, flags);
  }
  HRESULT STDMETHODCALLTYPE Escape(LPDIEFFESCAPE esc) override { return real_->Escape(esc); }
  HRESULT STDMETHODCALLTYPE SendDeviceData(DWORD objSize, LPCDIDEVICEOBJECTDATA items, LPDWORD count,
                                           DWORD flags) override {
    return real_->SendDeviceData(objSize, items, count, flags);
  }
  HRESULT STDMETHODCALLTYPE EnumEffectsInFile(LPCWSTR file, LPDIENUMEFFECTSINFILECALLBACK cb, LPVOID ref,
                                              DWORD flags) override {
    return real_->EnumEffectsInFile(file, cb, ref, flags);
  }
  HRESULT STDMETHODCALLTYPE WriteEffectToFile(LPCWSTR file, DWORD count, LPDIFILEEFFECT effects,
                                              DWORD flags) override {
    return real_->WriteEffectToFile(file, count, effects, flags);
  }
  HRESULT STDMETHODCALLTYPE BuildActionMap(LPDIACTIONFORMATW af, LPCWSTR user, DWORD flags) override {
    return real_->BuildActionMap(af, user, flags);
  }
  HRESULT STDMETHODCALLTYPE SetActionMap(LPDIACTIONFORMATW af, LPCWSTR user, DWORD flags) override {
    return real_->SetActionMap(af, user, flags);
  }
  HRESULT STDMETHODCALLTYPE GetImageInfo(LPDIDEVICEIMAGEINFOHEADERW info) override {
    return real_->GetImageInfo(info);
  }

 private:
  // Injeção de teclas por roteiro (testes automáticos sem ninguém no PC; base do
  // netplay, que injeta a entrada remota no mesmo ponto). Roteiro opcional em
  // `pes6online_inject.txt` na pasta do jogo: uma linha por tecla,
  // "<ms desde a 1ª leitura> <código DIK em hex> <duração ms>", ex.: "8000 1C 200".
  struct KeyPress {
    ULONGLONG at;
    std::uint8_t dik;
    ULONGLONG duration;
  };

  void load_script() {
    script_loaded_ = true;
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring dir(path);
    dir = dir.substr(0, dir.find_last_of(L"\\/") + 1);
    std::FILE* f = _wfopen((dir + L"pes6online_inject.txt").c_str(), L"r");
    if (f == nullptr) return;
    unsigned long long at = 0, dur = 0;
    unsigned dik = 0;
    while (std::fscanf(f, "%llu %x %llu", &at, &dik, &dur) == 3) {
      script_.push_back({at, static_cast<std::uint8_t>(dik), dur});
    }
    std::fclose(f);
    log::write(std::format("entrada: roteiro de injeção carregado ({} tecla(s))", script_.size()));
  }

  void inject_keys(std::uint8_t* keys) {
    if (!script_loaded_) load_script();
    if (script_.empty()) return;
    const ULONGLONG now = GetTickCount64();
    if (script_start_ == 0) script_start_ = now;
    const ULONGLONG t = now - script_start_;
    mem_diff::on_tick(t);
    for (std::size_t i = 0; i < script_.size(); ++i) {
      const KeyPress& k = script_[i];
      if (t >= k.at && t < k.at + k.duration) {
        keys[k.dik] = 0x80;
        if (injected_.insert(i).second) {
          log::write(std::format("entrada: injetada tecla DIK 0x{:02X} aos {} ms", k.dik, t));
        }
      }
    }
  }

  bool script_loaded_ = false;
  std::vector<KeyPress> script_;
  ULONGLONG script_start_ = 0;
  std::set<std::size_t> injected_;

  // Registra resultados de erro (ex.: DIERR_INPUTLOST na desconexão) sem repetir em série.
  void note_result(const char* what, HRESULT hr) {
    if (hr == last_hr_ && std::string_view(what) == last_what_) {
      ++repeats_;
      return;
    }
    if (repeats_ > 0) log::write(std::format("entrada: {}   (repetiu {}x)", label_, repeats_));
    repeats_ = 0;
    last_hr_ = hr;
    last_what_ = what;
    log::write(std::format("entrada: {} {} -> 0x{:08X}{}", label_, what, static_cast<std::uint32_t>(hr),
                           hr == DIERR_INPUTLOST     ? " (DIERR_INPUTLOST: controle perdido)"
                           : hr == DIERR_NOTACQUIRED ? " (DIERR_NOTACQUIRED)"
                           : hr == DIERR_UNPLUGGED   ? " (DIERR_UNPLUGGED)"
                                                     : ""));
  }

  // Botões e direções: para DIJOYSTATE (80 bytes) e DIJOYSTATE2 (272 bytes).
  void observe_state(DWORD size, const std::uint8_t* data) {
    if (size != sizeof(DIJOYSTATE) && size != sizeof(DIJOYSTATE2)) return;
    const auto* js = reinterpret_cast<const DIJOYSTATE*>(data);
    std::string buttons;
    for (int b = 0; b < 32; ++b) {
      if (js->rgbButtons[b] & 0x80) buttons += std::format("{}{}", buttons.empty() ? "" : ",", b);
    }
    // Eixos em 3 faixas (-, 0, +) para não registrar ruído.
    auto zone = [](LONG v) { return v < 16384 ? '-' : v > 49151 ? '+' : '0'; };
    const std::string summary = std::format("botões=[{}] pov={} x{}y{} rx{}ry{} z{}", buttons,
                                            static_cast<int>(js->rgdwPOV[0]), zone(js->lX), zone(js->lY),
                                            zone(js->lRx), zone(js->lRy), zone(js->lZ));
    if (summary != last_summary_ && state_logs_ < 400) {
      ++state_logs_;
      log::write(std::format("entrada: {} estado {}", label_, summary));
      last_summary_ = summary;
    }
  }

  // Frequência de leitura: registra leituras por segundo a cada ~10 s.
  void rate_tick() {
    const ULONGLONG now = GetTickCount64();
    if (window_start_ == 0) window_start_ = now;
    ++window_reads_;
    if (now - window_start_ >= 10000) {
      log::write(std::format("entrada: {} {:.1f} leituras/s", label_,
                             window_reads_ * 1000.0 / static_cast<double>(now - window_start_)));
      window_start_ = now;
      window_reads_ = 0;
    }
  }

  IDirectInputDevice8W* real_;
  std::string label_;
  ULONG refs_ = 1;
  std::uint64_t reads_ = 0;
  std::set<std::uintptr_t> callers_;
  HRESULT last_hr_ = S_OK;
  std::string last_what_;
  int repeats_ = 0;
  std::string last_summary_;
  int state_logs_ = 0;
  ULONGLONG window_start_ = 0;
  std::uint64_t window_reads_ = 0;
};
