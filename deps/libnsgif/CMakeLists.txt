cmake_minimum_required(VERSION 3.22...3.25)

add_library(libnsgif OBJECT)
add_library(OBS::libnsgif ALIAS libnsgif)

target_sources(
  libnsgif
  PRIVATE
    src/gif.c
    src/lzw.c
    src/lzw.h
  PUBLIC
    include/nsgif.h
)

target_include_directories(
  libnsgif
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_options(
  libnsgif
  PRIVATE
  $<$<PLATFORM_ID:Windows>:/wd4244>
  $<$<PLATFORM_ID:Windows>:/wd4267>
)
