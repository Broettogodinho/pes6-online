#include "settings_builder.h"

#include <algorithm>
#include <cstring>
#include <format>

namespace pes6online::settings {
namespace {

// GUID_SysKeyboard {6F1D2B61-D5A0-11CF-BFC7-444553540000} em bytes de memória.
constexpr guid_map::Guid kSysKeyboard = {0x61, 0x2B, 0x1D, 0x6F, 0xA0, 0xD5, 0xCF, 0x11,
                                         0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};

void write_slot(std::vector<std::uint8_t>& out, std::size_t index, const Slot& s) {
  std::uint8_t* p = out.data() + kSlotStart + index * kSlotSize;
  const std::uint32_t used = s.used ? 1 : 0;
  std::memcpy(p, &used, 4);
  std::memcpy(p + 4, s.guid.data(), 16);
  std::memcpy(p + 20, s.map.data(), kMapSize);
}

Slot empty_slot() {
  Slot s;
  s.map.fill(0xFF);
  return s;
}

}  // namespace

std::optional<std::vector<Slot>> parse(const std::uint8_t* data, std::size_t size) {
  if (data == nullptr || size < kSlotStart + kSlotCount * kSlotSize) return std::nullopt;
  std::vector<Slot> slots(kSlotCount);
  for (std::size_t i = 0; i < kSlotCount; ++i) {
    const std::uint8_t* p = data + kSlotStart + i * kSlotSize;
    std::uint32_t used = 0;
    std::memcpy(&used, p, 4);
    slots[i].used = used != 0;
    std::memcpy(slots[i].guid.data(), p + 4, 16);
    std::memcpy(slots[i].map.data(), p + 20, kMapSize);
  }
  return slots;
}

std::vector<std::uint8_t> build(const std::vector<std::uint8_t>& original, const std::vector<Controller>& controllers,
                                const std::map<std::uint32_t, std::vector<guid_map::Guid>>& history,
                                const Mapping& default_map, std::vector<std::string>* report) {
  const auto slots = parse(original.data(), original.size());
  if (!slots) return original;
  std::vector<std::uint8_t> out = original;
  std::vector<Slot> result;

  if (controllers.empty()) {
    // Regra 2: sem controle, o teclado (se configurado) é o P1.
    for (const Slot& s : *slots) {
      if (s.used && s.guid == kSysKeyboard) {
        result.push_back(s);
        if (report) report->push_back("P1 = teclado (nenhum controle conectado)");
        break;
      }
    }
  }

  for (const Controller& c : controllers) {
    if (result.size() >= kSlotCount) break;
    Slot s;
    s.used = true;
    s.guid = c.instance;
    std::string origin = "padrão Xbox";
    // Regra 3: mapeamento do próprio GUID, senão do mesmo modelo, senão o padrão.
    const Slot* own = nullptr;
    const Slot* same_model = nullptr;
    const auto h = history.find(c.product_key);
    for (const Slot& o : *slots) {
      if (!o.used) continue;
      if (o.guid == c.instance) own = &o;
      if (h != history.end() && std::find(h->second.begin(), h->second.end(), o.guid) != h->second.end() &&
          same_model == nullptr) {
        same_model = &o;
      }
    }
    if (own != nullptr) {
      s.map = own->map;
      origin = "mapeamento salvo deste controle";
    } else if (same_model != nullptr) {
      s.map = same_model->map;
      origin = "mapeamento salvo do mesmo modelo";
    } else {
      s.map = default_map;
    }
    result.push_back(s);
    if (report) {
      report->push_back(std::format("P{} = controle VID {:04X}/PID {:04X} ({})", result.size(),
                                    c.product_key & 0xFFFF, c.product_key >> 16, origin));
    }
  }

  for (std::size_t i = 0; i < kSlotCount; ++i) write_slot(out, i, i < result.size() ? result[i] : empty_slot());
  return out;
}

}  // namespace pes6online::settings
