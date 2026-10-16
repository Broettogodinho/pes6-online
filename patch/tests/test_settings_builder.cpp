// Testes da montagem automática do settings.dat (sem Windows).

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "settings_builder.h"

using namespace pes6online;
using settings::Mapping;

static int g_failures = 0;
#define CHECK(expr)                                                          \
  do {                                                                       \
    if (!(expr)) {                                                           \
      std::fprintf(stderr, "FALHOU %s:%d: %s\n", __FILE__, __LINE__, #expr); \
      ++g_failures;                                                          \
    }                                                                        \
  } while (0)

static guid_map::Guid g(std::uint8_t a) {
  return {a, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x80, 0x02, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};
}
static const guid_map::Guid kKbd = {0x61, 0x2B, 0x1D, 0x6F, 0xA0, 0xD5, 0xCF, 0x11,
                                    0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};

static Mapping m(std::uint8_t v) {
  Mapping x{};
  x.fill(v);
  return x;
}

// Arquivo como o do Bruno: cabeçalho, slot0 teclado, slot1 com fio, slot2 BT, resto vazio, rodapé.
static std::vector<std::uint8_t> make_file() {
  std::vector<std::uint8_t> f(0x1A4, 0x00);
  for (std::size_t i = 0; i < 0x0C; ++i) f[i] = static_cast<std::uint8_t>(0xA0 + i);  // cabeçalho
  for (std::size_t i = 0x198; i < f.size(); ++i) f[i] = 0x5A;                         // rodapé
  auto put = [&](std::size_t idx, bool used, const guid_map::Guid& gu, const Mapping& mp) {
    std::uint8_t* p = f.data() + settings::kSlotStart + idx * settings::kSlotSize;
    const std::uint32_t u = used ? 1 : 0;
    std::memcpy(p, &u, 4);
    std::memcpy(p + 4, gu.data(), 16);
    std::memcpy(p + 20, mp.data(), 24);
  };
  put(0, true, kKbd, m(0x11));
  put(1, true, g(0xC0), m(0x22));  // controle com fio (modelo A)
  put(2, true, g(0xB0), m(0x33));  // Xbox BT (modelo B)
  for (std::size_t i = 3; i < settings::kSlotCount; ++i) put(i, false, {}, m(0xFF));
  return f;
}

int main() {
  const auto file = make_file();
  const std::uint32_t modelA = 0x10403537, modelB = 0x02E0045E, modelC = 0x2009057E;
  const std::map<std::uint32_t, std::vector<guid_map::Guid>> history = {
      {modelA, {g(0xC0), g(0xC1)}}, {modelB, {g(0xB0), g(0xB1)}}};
  const Mapping def = m(0x44);

  auto slots = settings::parse(file.data(), file.size());
  CHECK(slots && slots->size() == 9 && (*slots)[0].guid == kKbd && !(*slots)[3].used);
  CHECK(!settings::parse(file.data(), 0x20));

  // Só o BT conectado (com outro GUID do mesmo modelo): vira P1 com o mapeamento do modelo; teclado sai.
  std::vector<std::string> rep;
  auto out = settings::build(file, {{g(0xB1), modelB}}, history, def, &rep);
  auto s = settings::parse(out.data(), out.size());
  CHECK((*s)[0].used && (*s)[0].guid == g(0xB1) && (*s)[0].map == m(0x33));
  CHECK(!(*s)[1].used && (*s)[1].map == m(0xFF));
  CHECK(rep.size() == 1);

  // Cabeçalho e rodapé preservados, tamanho igual.
  CHECK(out.size() == file.size());
  CHECK(std::memcmp(out.data(), file.data(), 0x0C) == 0);
  CHECK(std::memcmp(out.data() + 0x198, file.data() + 0x198, file.size() - 0x198) == 0);

  // Dois controles: ordem preservada; GUID salvo usa o próprio mapeamento.
  out = settings::build(file, {{g(0xC0), modelA}, {g(0xB0), modelB}}, history, def, nullptr);
  s = settings::parse(out.data(), out.size());
  CHECK((*s)[0].guid == g(0xC0) && (*s)[0].map == m(0x22));
  CHECK((*s)[1].guid == g(0xB0) && (*s)[1].map == m(0x33));
  CHECK(!(*s)[2].used);

  // Controle nunca configurado: padrão Xbox.
  out = settings::build(file, {{g(0xD0), modelC}}, history, def, nullptr);
  s = settings::parse(out.data(), out.size());
  CHECK((*s)[0].guid == g(0xD0) && (*s)[0].map == def);

  // Nenhum controle: teclado como P1.
  rep.clear();
  out = settings::build(file, {}, history, def, &rep);
  s = settings::parse(out.data(), out.size());
  CHECK((*s)[0].guid == kKbd && (*s)[0].map == m(0x11) && !(*s)[1].used);
  CHECK(rep.size() == 1);

  // Arquivo inválido: devolve o original.
  const std::vector<std::uint8_t> tiny(10, 7);
  CHECK(settings::build(tiny, {{g(0xD0), modelC}}, history, def, nullptr) == tiny);

  if (g_failures == 0) std::puts("test_settings_builder: OK");
  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
