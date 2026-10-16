#pragma once

// Monta o settings.dat que o jogo lê, com atribuição automática de controles
// (regras do Bruno, 2026-10-16):
//   1. os controles conectados ocupam P1, P2, P3… na ordem recebida;
//   2. o teclado só entra se não houver nenhum controle;
//   3. cada controle leva o mapeamento salvo para ele (mesmo GUID) ou para o
//      mesmo modelo (histórico de GUIDs do modelo); sem isso, o padrão Xbox.
// Cabeçalho e rodapé do arquivo original são preservados. Código puro, testável.
//
// Estrutura observada (re/notes/controles-settings.md): a partir de 0x0C,
// 9 slots de 0x2C bytes = [u32 usado][GUID 16][mapeamento 24]. Slot vazio:
// usado=0, GUID zerado, mapeamento FF.

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "guid_map.h"

namespace pes6online::settings {

inline constexpr std::size_t kSlotStart = 0x0C;
inline constexpr std::size_t kSlotSize = 0x2C;
inline constexpr std::size_t kSlotCount = 9;
inline constexpr std::size_t kMapSize = 24;

using Mapping = std::array<std::uint8_t, kMapSize>;

struct Slot {
  bool used = false;
  guid_map::Guid guid{};
  Mapping map{};
};

// Lê os slots; nullopt se o arquivo for menor que a área de slots.
std::optional<std::vector<Slot>> parse(const std::uint8_t* data, std::size_t size);

struct Controller {
  guid_map::Guid instance;
  std::uint32_t product_key;  // (PID << 16) | VID
};

// Gera o arquivo para o jogo. `report` recebe uma linha legível por jogador.
// Se o original for inválido, devolve-o sem alteração.
std::vector<std::uint8_t> build(const std::vector<std::uint8_t>& original, const std::vector<Controller>& controllers,
                                const std::map<std::uint32_t, std::vector<guid_map::Guid>>& history,
                                const Mapping& default_map, std::vector<std::string>* report);

}  // namespace pes6online::settings
