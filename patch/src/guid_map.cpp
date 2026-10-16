#include "guid_map.h"

#include <algorithm>
#include <cstring>

namespace pes6online::guid_map {
namespace {

// GUID_SysKeyboard {6F1D2B61-D5A0-11CF-BFC7-444553540000} e
// GUID_SysMouse    {6F1D2B60-D5A0-11CF-BFC7-444553540000}, em bytes de memória.
constexpr Guid kSysKeyboard = {0x61, 0x2B, 0x1D, 0x6F, 0xA0, 0xD5, 0xCF, 0x11,
                               0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};
constexpr Guid kSysMouse = {0x60, 0x2B, 0x1D, 0x6F, 0xA0, 0xD5, 0xCF, 0x11,
                            0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};

bool contains(const std::vector<Guid>& v, const Guid& g) {
  return std::find(v.begin(), v.end(), g) != v.end();
}

}  // namespace

std::vector<Guid> saved_guids_from_settings(const std::uint8_t* data, std::size_t size) {
  static constexpr std::uint8_t kTail[6] = {0x44, 0x45, 0x53, 0x54, 0x00, 0x00};  // "DEST\0\0"
  std::vector<Guid> out;
  if (data == nullptr) return out;
  for (std::size_t off = 0; off + 16 <= size; ++off) {
    if (std::memcmp(data + off + 10, kTail, 6) != 0) continue;
    Guid g{};
    std::memcpy(g.data(), data + off, 16);
    if (g == kSysKeyboard || g == kSysMouse || contains(out, g)) continue;
    out.push_back(g);
    off += 15;  // não reler bytes do mesmo GUID
  }
  return out;
}

std::vector<Guid> decide(const std::vector<RealDevice>& devices, const std::vector<Guid>& saved,
                         const std::map<std::uint32_t, std::vector<Guid>>& history) {
  std::vector<Guid> presented(devices.size());
  std::vector<Guid> used;
  // 1ª passada: quem já tem o GUID salvo fica com ele.
  for (std::size_t i = 0; i < devices.size(); ++i) {
    presented[i] = devices[i].instance;
    if (contains(saved, devices[i].instance)) used.push_back(devices[i].instance);
  }
  // 2ª passada: os demais herdam um GUID salvo livre do mesmo modelo.
  for (std::size_t i = 0; i < devices.size(); ++i) {
    if (contains(saved, devices[i].instance)) continue;
    const auto h = history.find(devices[i].product_key);
    if (h == history.end()) continue;
    for (const Guid& s : saved) {
      if (contains(used, s) || !contains(h->second, s)) continue;
      presented[i] = s;
      used.push_back(s);
      break;
    }
  }
  return presented;
}

}  // namespace pes6online::guid_map
