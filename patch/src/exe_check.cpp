#include "exe_check.h"

#include <cstdio>
#include <format>
#include <vector>

#include <windows.h>
#include <bcrypt.h>

#include "log.h"
#include "target.h"

namespace pes6online::exe {

std::string sha256_of_current() {
  wchar_t path[MAX_PATH] = {};
  if (GetModuleFileNameW(nullptr, path, MAX_PATH) == 0) return {};
  std::FILE* f = _wfopen(path, L"rb");
  if (f == nullptr) return {};

  BCRYPT_ALG_HANDLE alg = nullptr;
  BCRYPT_HASH_HANDLE hash = nullptr;
  std::string hex;
  if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0 &&
      BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0) == 0) {
    std::vector<unsigned char> buf(1 << 20);
    bool ok = true;
    for (std::size_t n; (n = std::fread(buf.data(), 1, buf.size(), f)) > 0;) {
      if (BCryptHashData(hash, buf.data(), static_cast<ULONG>(n), 0) != 0) {
        ok = false;
        break;
      }
    }
    unsigned char digest[32] = {};
    if (ok && BCryptFinishHash(hash, digest, sizeof(digest), 0) == 0) {
      for (unsigned char b : digest) hex += std::format("{:02x}", b);
    }
  }
  if (hash != nullptr) BCryptDestroyHash(hash);
  if (alg != nullptr) BCryptCloseAlgorithmProvider(alg, 0);
  std::fclose(f);
  return hex;
}

bool is_target() {
  const std::string actual = sha256_of_current();
  const bool ok = actual == target::kExeSha256;
  if (ok) {
    log::write("exe: pes6.exe confere com a versão-alvo");
  } else {
    log::write(std::format("exe: hash diferente da versão-alvo ({}); nenhum hook será instalado",
                           actual.empty() ? "não calculado" : actual));
  }
  return ok;
}

}  // namespace pes6online::exe
