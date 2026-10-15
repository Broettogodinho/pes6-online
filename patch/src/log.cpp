#include "log.h"

#include <chrono>
#include <cstdio>
#include <format>
#include <mutex>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace pes6online::log {
namespace {
std::mutex g_mutex;
std::FILE* g_file = nullptr;

std::string log_path() {
#ifdef _WIN32
  // Mesmo diretório da DLL.
  HMODULE self = nullptr;
  GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                         GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                     reinterpret_cast<LPCSTR>(&log_path), &self);
  char buf[MAX_PATH] = {};
  GetModuleFileNameA(self, buf, MAX_PATH);
  std::string path(buf);
  const auto slash = path.find_last_of("\\/");
  return (slash == std::string::npos ? std::string() : path.substr(0, slash + 1)) +
         "pes6online.log";
#else
  return "pes6online.log";
#endif
}
}  // namespace

void init() {
  std::lock_guard lock(g_mutex);
  if (g_file == nullptr) {
    g_file = std::fopen(log_path().c_str(), "a");
  }
}

void write(std::string_view message) {
  std::lock_guard lock(g_mutex);
  if (g_file == nullptr) return;
  const auto now = std::chrono::system_clock::now();
  const auto line = std::format("[{:%Y-%m-%d %H:%M:%S}] {}\n",
                                std::chrono::floor<std::chrono::seconds>(now), message);
  std::fputs(line.c_str(), g_file);
  std::fflush(g_file);
}

}  // namespace pes6online::log
