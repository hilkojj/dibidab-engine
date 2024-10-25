cmake_minimum_required(VERSION 3.5)
project(dibidab)

# ---Dibidab Header Tool---:
include(${CMAKE_CURRENT_LIST_DIR}/external/dibidab-header/tool/dibidab_header.cmake)
process_dibidab_headers(
    # Source directory (to search for .dibidab.h headers):
    ${CMAKE_CURRENT_LIST_DIR}/source
    # Output directory:
    ${CMAKE_CURRENT_LIST_DIR}/source/generated
    # namespace to put the generated registry in:
    dibidab
)

file(GLOB_RECURSE source source/*)

# ---
add_executable(dibidab ${source})
target_include_directories(dibidab PUBLIC source/)

set_property(TARGET dibidab PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET dibidab PROPERTY CXX_STANDARD 17)
set_property(TARGET dibidab PROPERTY CXX_STANDARD_REQUIRED ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS  "/Ox /EHsc /bigobj /D NDEBUG")
else()
    set(CMAKE_CXX_FLAGS  "-B /usr/libexec/mold")
endif()

# ---GU Game Utilities Library---:
add_subdirectory(external/gu/library ./bin/gu)
target_link_libraries(dibidab gameutils)

# ---LUA---
add_subdirectory(external/lua ./bin/lua)
target_link_libraries(dibidab lua)

add_subdirectory(external/lua/sol2/single ./bin/lua/sol2)
# TODO: check necessity, and consider disabling for release builds: https://sol2.readthedocs.io/en/latest/safety.html
target_compile_definitions(sol2_single INTERFACE SOL_ALL_SAFETIES_ON=1)
target_link_libraries(dibidab sol2_single)

# ---EnTT---
include_directories(dibidab PUBLIC external/entt/src)

# ---Dibidab Header Library---
target_include_directories(dibidab PUBLIC external/dibidab-header/include)
