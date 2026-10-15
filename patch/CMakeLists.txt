cmake_minimum_required(VERSION 3.24)
project(pes6_online_patch LANGUAGES CXX)

# O pes6.exe é 32 bits: a DLL precisa ser Win32 (cmake -A Win32).
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(WARNING "PES 6 é 32 bits: configure com -A Win32 para gerar uma DLL compatível.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(pes6online SHARED
  src/dllmain.cpp
  src/log.cpp
)
target_include_directories(pes6online PRIVATE include)

if(MSVC)
  target_compile_options(pes6online PRIVATE /W4 /permissive-)
  # Runtime estático: o jogador não precisa instalar o VC++ Redistributable.
  set_property(TARGET pes6online PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Próximos passos (F5, SCRUM-25): MinHook como dependência e o loader (DLL proxy).
