#pragma once

// Estabilização do GUID de instância dos controles (SCRUM-56/57).
//
// O jogo grava o mapeamento de botões junto com o GUID de instância do
// DirectInput (settings.dat). O Windows pode atribuir outro GUID de instância
// ao mesmo modelo de controle (outra porta, nova conexão), e o jogo então não
// reconhece o controle. Este módulo decide, sem depender do Windows, qual GUID
// apresentar ao jogo para cada controle real. Ver re/notes/controles-settings.md.

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace pes6online::guid_map {

using Guid = std::array<std::uint8_t, 16>;  // bytes como estão na memória (GUID do Windows)

// GUIDs de instância gravados no settings.dat: todo bloco de 16 bytes que
// termina em "DEST\0\0" (44 45 53 54 00 00), exceto GUID_SysKeyboard/SysMouse.
std::vector<Guid> saved_guids_from_settings(const std::uint8_t* data, std::size_t size);

struct RealDevice {
  Guid instance;
  std::uint32_t product_key;  // guidProduct.Data1 = (PID << 16) | VID
};

// Decide o GUID apresentado ao jogo para cada controle real (mesma ordem de `devices`).
// - Controle cujo GUID real já está salvo: mantém.
// - Senão, se houver um GUID salvo, ainda não usado, que o Windows já atribuiu ao
//   MESMO modelo (`history[product_key]`, ex.: registro Calibration\N), usa o salvo.
// - Senão, mantém o real.
std::vector<Guid> decide(const std::vector<RealDevice>& devices, const std::vector<Guid>& saved,
                         const std::map<std::uint32_t, std::vector<Guid>>& history);

}  // namespace pes6online::guid_map
