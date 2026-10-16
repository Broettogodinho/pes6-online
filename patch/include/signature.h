#pragma once

// Assinaturas (padrões de bytes) das funções do jogo, no formato gerado por
// tools/ghidra/MakeSignature.java: "55 8B EC ?? ?? 83 EC 10" (?? = curinga).
// Código puro, sem dependência do Windows, para ser testado fora do jogo.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace pes6online::sig {

struct Pattern {
  std::vector<std::uint8_t> bytes;
  std::vector<bool> wildcard;  // true = qualquer byte

  std::size_t size() const { return bytes.size(); }
};

// Lê "55 8B ?? EC". Aceita "?" ou "??" como curinga. Devolve nullopt se o texto
// estiver malformado ou vazio.
std::optional<Pattern> parse(std::string_view text);

// O padrão casa com os bytes que começam em data? (data precisa ter size() bytes)
bool matches_at(const std::uint8_t* data, std::size_t available, const Pattern& pattern);

// Quantas vezes o padrão aparece em [data, data+size), parando em `limit`.
std::size_t count(const std::uint8_t* data, std::size_t size, const Pattern& pattern,
                  std::size_t limit = 2);

}  // namespace pes6online::sig
