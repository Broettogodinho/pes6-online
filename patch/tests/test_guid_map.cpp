// Testes da decisão de GUID estável dos controles (sem Windows).

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "guid_map.h"

using namespace pes6online::guid_map;

static int g_failures = 0;
#define CHECK(expr)                                                          \
  do {                                                                       \
    if (!(expr)) {                                                           \
      std::fprintf(stderr, "FALHOU %s:%d: %s\n", __FILE__, __LINE__, #expr); \
      ++g_failures;                                                          \
    }                                                                        \
  } while (0)

// GUID de instância no estilo do DirectInput: varia o 1º byte e o índice em Data4[1].
static Guid inst(std::uint8_t a, std::uint8_t idx) {
  return {a, 0xED, 0x82, 0x82, 0x23, 0x09, 0xF1, 0x11, 0x80, idx, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};
}

int main() {
  const Guid g1 = inst(0xB0, 0x01), g2 = inst(0xC0, 0x02), g3 = inst(0xD0, 0x03);
  const std::uint32_t pad = 0x10403537;  // PID 1040, VID 3537
  const std::uint32_t other = 0x02E0045E;

  // settings.dat: teclado + um controle (g2) + lixo
  std::vector<std::uint8_t> file(0x60, 0xFF);
  const std::uint8_t kbd[16] = {0x61, 0x2B, 0x1D, 0x6F, 0xA0, 0xD5, 0xCF, 0x11,
                                0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};
  std::copy(std::begin(kbd), std::end(kbd), file.begin() + 0x10);
  std::copy(g2.begin(), g2.end(), file.begin() + 0x3C);
  auto saved = saved_guids_from_settings(file.data(), file.size());
  CHECK(saved.size() == 1);
  CHECK(saved[0] == g2);
  CHECK(saved_guids_from_settings(nullptr, 10).empty());

  const std::map<std::uint32_t, std::vector<Guid>> history = {{pad, {g1, g2, g3}}};

  // controle chega com o GUID salvo: nada muda
  auto r = decide({{g2, pad}}, saved, history);
  CHECK(r.size() == 1 && r[0] == g2);

  // controle chega com outro GUID do mesmo modelo: apresenta o salvo
  r = decide({{g3, pad}}, saved, history);
  CHECK(r[0] == g2);

  // modelo diferente não herda o GUID salvo
  r = decide({{g3, other}}, saved, history);
  CHECK(r[0] == g3);

  // GUID fora do histórico do modelo: não herda
  r = decide({{g1, pad}}, saved, {{pad, {g1}}});
  CHECK(r[0] == g1);

  // dois controles do mesmo modelo, um já com o salvo: o outro não "rouba"
  r = decide({{g1, pad}, {g2, pad}}, saved, history);
  CHECK(r[0] == g1 && r[1] == g2);

  // dois controles, nenhum com o salvo: só o primeiro herda
  r = decide({{g1, pad}, {g3, pad}}, saved, history);
  CHECK(r[0] == g2 && r[1] == g3);

  // sem nada salvo: tudo como está
  r = decide({{g3, pad}}, {}, history);
  CHECK(r[0] == g3);

  if (g_failures == 0) std::puts("test_guid_map: OK");
  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
