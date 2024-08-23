cmake_minimum_required(VERSION 3.22...3.25)

add_library(libnsgif INTERFACE)
add_library(OBS::libnsgif ALIAS libnsgif)

target_sources(
  libnsgif
  INTERFACE src/gif.c
            src/lzw.c
            src/lzw.h
            include/nsgif.h
)

target_include_directories(
  libnsgif
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_options(
  libnsgif
  INTERFACE
  $<$<PLATFORM_ID:Windows>:/wd4244>
  $<$<PLATFORM_ID:Windows>:/wd4267>
)
