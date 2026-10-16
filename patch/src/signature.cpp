#include "signature.h"

#include <cctype>

namespace pes6online::sig {
namespace {
int hex_value(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
}  // namespace

std::optional<Pattern> parse(std::string_view text) {
  Pattern p;
  std::size_t i = 0;
  while (i < text.size()) {
    if (std::isspace(static_cast<unsigned char>(text[i]))) {
      ++i;
      continue;
    }
    std::size_t j = i;
    while (j < text.size() && !std::isspace(static_cast<unsigned char>(text[j]))) ++j;
    const std::string_view token = text.substr(i, j - i);
    if (token == "?" || token == "??") {
      p.bytes.push_back(0);
      p.wildcard.push_back(true);
    } else if (token.size() == 2 && hex_value(token[0]) >= 0 && hex_value(token[1]) >= 0) {
      p.bytes.push_back(static_cast<std::uint8_t>(hex_value(token[0]) * 16 + hex_value(token[1])));
      p.wildcard.push_back(false);
    } else {
      return std::nullopt;
    }
    i = j;
  }
  if (p.bytes.empty()) return std::nullopt;
  return p;
}

bool matches_at(const std::uint8_t* data, std::size_t available, const Pattern& pattern) {
  if (data == nullptr || available < pattern.size()) return false;
  for (std::size_t k = 0; k < pattern.size(); ++k) {
    if (!pattern.wildcard[k] && data[k] != pattern.bytes[k]) return false;
  }
  return true;
}

std::size_t count(const std::uint8_t* data, std::size_t size, const Pattern& pattern,
                  std::size_t limit) {
  std::size_t found = 0;
  if (pattern.size() == 0 || size < pattern.size()) return 0;
  for (std::size_t off = 0; off + pattern.size() <= size && found < limit; ++off) {
    if (matches_at(data + off, size - off, pattern)) ++found;
  }
  return found;
}

}  // namespace pes6online::sig
