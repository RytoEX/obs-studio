project(obs-qsv-test)

include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/libobs)

add_executable(obs-qsv-test)

find_package(VPL REQUIRED)

target_sources(obs-qsv-test PRIVATE obs-qsv-test.cpp)
target_link_libraries(obs-qsv-test d3d11 dxgi dxguid VPL::VPL)

set_target_properties(obs-qsv-test PROPERTIES FOLDER "plugins/obs-qsv11")

setup_binary_target(obs-qsv-test)
