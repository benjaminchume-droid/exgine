cmake_minimum_required(VERSION 3.20)
project(exgine VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_library(exgine_core
    src/diagnostic.cpp src/ir.cpp src/world.cpp src/token.cpp src/lexer.cpp
    src/parser.cpp src/semantic.cpp src/compiler.cpp src/runtime.cpp src/engine.cpp
    src/geometry.cpp src/character.cpp src/material.cpp src/texture.cpp src/resources.cpp
    src/scene.cpp src/lighting.cpp src/render.cpp src/shaders.cpp src/building.cpp
    src/vehicle.cpp)
target_include_directories(exgine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(exgine_core PUBLIC cxx_std_20)
target_compile_options(exgine_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>)
add_executable(exgine_demo examples/hello_world.cpp)
target_link_libraries(exgine_demo PRIVATE exgine_core)
enable_testing()
add_executable(exgine_core_tests tests/core_tests.cpp)
target_link_libraries(exgine_core_tests PRIVATE exgine_core)
target_compile_options(exgine_core_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)
add_test(NAME exgine_core_tests COMMAND exgine_core_tests)
