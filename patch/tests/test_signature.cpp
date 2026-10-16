// Testes da leitura e comparação de assinaturas (sem dependência do jogo).

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "signature.h"

using pes6online::sig::count;
using pes6online::sig::matches_at;
using pes6online::sig::parse;

static int g_failures = 0;

#define CHECK(expr)                                                     \
  do {                                                                  \
    if (!(expr)) {                                                      \
      std::fprintf(stderr, "FALHOU %s:%d: %s\n", __FILE__, __LINE__, #expr); \
      ++g_failures;                                                     \
    }                                                                   \
  } while (0)

int main() {
  // leitura
  auto p = parse("55 8B EC ?? 83 ec 10 ?");
  CHECK(p.has_value());
  CHECK(p->size() == 8);
  CHECK(p->bytes[0] == 0x55 && !p->wildcard[0]);
  CHECK(p->wildcard[3] && p->wildcard[7]);
  CHECK(p->bytes[5] == 0xEC);
  CHECK(!parse("").has_value());
  CHECK(!parse("5").has_value());
  CHECK(!parse("GG").has_value());
  CHECK(!parse("55 8B EC1").has_value());
  CHECK(parse("  55\t8B\n").has_value());

  // comparação
  const std::uint8_t code[] = {0x90, 0x55, 0x8B, 0xEC, 0x12, 0x83, 0xEC, 0x10, 0x34, 0x55, 0x8B};
  auto q = parse("55 8B EC ?? 83 EC 10 ??");
  CHECK(matches_at(code + 1, sizeof(code) - 1, *q));
  CHECK(!matches_at(code, sizeof(code), *q));
  CHECK(!matches_at(code + 9, 2, *q));      // poucos bytes disponíveis
  CHECK(!matches_at(nullptr, 100, *q));

  // contagem (unicidade)
  CHECK(count(code, sizeof(code), *q) == 1);
  CHECK(count(code, sizeof(code), *parse("55 8B")) == 2);
  CHECK(count(code, sizeof(code), *parse("55 8B"), 1) == 1);
  CHECK(count(code, sizeof(code), *parse("AA BB")) == 0);

  if (g_failures == 0) std::puts("test_signature: OK");
  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
