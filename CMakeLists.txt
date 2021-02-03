cmake_minimum_required(VERSION 3.0)

project(dibidab)
add_compile_definitions(PUT_A_SOCK_IN_IT)   # disable verbose logging of external/gu


find_package(PythonInterp 3.7 REQUIRED)
set(LUA_SERDE_COMMAND ${PYTHON_EXECUTABLE} external/lua-serde/lua-json-serde.py source/generated/ source ../macro_magic/component.h)

execute_process(COMMAND ${LUA_SERDE_COMMAND} WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})


# dependencies for https://github.com/atasak/lua-serde
execute_process(COMMAND python -m pip install --user jinja2)
execute_process(COMMAND python -m pip install --user pyYAML)


set(EXECUTABLE_OUTPUT_PATH "out")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

file(GLOB_RECURSE source source/*)
add_library(dibidab ${source})
target_include_directories(dibidab PUBLIC source/)
set_target_properties(dibidab PROPERTIES LINKER_LANGUAGE CXX)

set_property(TARGET dibidab PROPERTY CXX_STANDARD 17)
set_property(TARGET dibidab PROPERTY CXX_STANDARD_REQUIRED ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS  "/Ox /EHsc /bigobj")
endif()

# gameutils repo:
add_subdirectory(external/gu/library ./bin/gu)
target_link_libraries(dibidab gameutils)


# ---WEBSOCKETPP---
include_directories(external/websocketpp)
target_include_directories(dibidab PUBLIC external/websocketpp)


# ---ASIO---
add_definitions(-D_WEBSOCKETPP_CPP11_STL_)
include_directories(external/asio/asio/include)
target_include_directories(dibidab PUBLIC external/asio/asio/include)


# ---LUA---
add_subdirectory(external/lua ./bin/lua)
target_link_libraries(dibidab lua)

add_subdirectory(external/lua/sol2/single ./bin/lua/sol2)
target_link_libraries(dibidab sol2_single)


# ---Generate structs with LUA SERDE---
add_custom_target(
        generate_structs ALL
        COMMAND ${LUA_SERDE_COMMAND}
        COMMENT "Generating structs from yaml"
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

add_dependencies(dibidab generate_structs)
