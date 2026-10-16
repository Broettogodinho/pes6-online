cmake_minimum_required(VERSION 3.24)
project(pes6_online_patch LANGUAGES C CXX)

# O pes6.exe é 32 bits: a DLL precisa ser Win32 (cmake -A Win32).
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(WARNING "PES 6 é 32 bits: configure com -A Win32 para gerar uma DLL compatível.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Runtime estático: o jogador não precisa instalar o VC++ Redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# MinHook (BSD-2-Clause), versão fixada e conferida por SHA-256.
include(FetchContent)
FetchContent_Declare(minhook
  URL https://github.com/TsudaKageyu/minhook/archive/refs/tags/v1.3.4.tar.gz
  URL_HASH SHA256=1aebeae4ca898330c507860acc2fca2eb335fe446a3a2b8444c3bf8b2660a14e
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# O CMakeLists do MinHook declara VERSION 3.0...3.5; o CMake 4 avisa que < 3.10
# está obsoleto. Fixamos o mínimo de política só para a dependência.
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
FetchContent_MakeAvailable(minhook)
unset(CMAKE_POLICY_VERSION_MINIMUM)

# Núcleo sem dependência do Windows: assinaturas (testável fora do jogo).
add_library(pes6online_core STATIC src/signature.cpp)
target_include_directories(pes6online_core PUBLIC include)

# DLL proxy: gerada como dinput8.dll para ser carregada pelo pes6.exe.
add_library(pes6online SHARED
  src/dllmain.cpp
  src/log.cpp
  src/exe_check.cpp
  src/hooks.cpp
  src/proxy_dinput8.cpp
  src/dinput8.def
)
set_target_properties(pes6online PROPERTIES OUTPUT_NAME dinput8)
target_link_libraries(pes6online PRIVATE pes6online_core minhook bcrypt)

if(MSVC)
  target_compile_options(pes6online PRIVATE /W4 /permissive-)
  target_compile_definitions(pes6online PRIVATE _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
  target_compile_options(pes6online_core PRIVATE /W4 /permissive-)
endif()

# Testes (ctest -C Release)
enable_testing()
add_executable(test_signature tests/test_signature.cpp)
target_link_libraries(test_signature PRIVATE pes6online_core)
add_test(NAME signature COMMAND test_signature)
