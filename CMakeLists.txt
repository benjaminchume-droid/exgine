cmake_minimum_required(VERSION 3.20)
project(exgine VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_library(exgine_core
    src/diagnostic.cpp src/ir.cpp src/world.cpp src/open_world.cpp src/performance.cpp src/token.cpp src/lexer.cpp
    src/parser.cpp src/semantic.cpp src/compiler.cpp src/runtime.cpp src/engine.cpp src/editor.cpp
    src/geometry.cpp src/character.cpp src/material.cpp src/texture.cpp src/resources.cpp
    src/scene.cpp src/lighting.cpp src/render.cpp src/shaders.cpp src/gpu.cpp src/building.cpp
    src/vehicle.cpp src/physics.cpp src/gameplay.cpp src/animation.cpp)
target_include_directories(exgine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(exgine_core PUBLIC cxx_std_20)
target_compile_options(exgine_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>)
add_executable(exgine_demo examples/hello_world.cpp)
target_link_libraries(exgine_demo PRIVATE exgine_core)
enable_testing()
function(exgine_add_test target source)
  add_executable(${target} ${source})
  target_link_libraries(${target} PRIVATE exgine_core)
  target_compile_options(${target} PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-UNDEBUG> $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)
  add_test(NAME ${target} COMMAND ${target})
endfunction()
exgine_add_test(exgine_core_tests tests/core_tests.cpp)
exgine_add_test(exgine_physics_tests tests/physics_tests.cpp)
exgine_add_test(exgine_physics_phase10_tests tests/physics_phase10_tests.cpp)
exgine_add_test(exgine_gameplay_phase11_tests tests/gameplay_phase11_tests.cpp)
exgine_add_test(exgine_open_world_phase12_tests tests/open_world_phase12_tests.cpp)
exgine_add_test(exgine_performance_phase13_tests tests/performance_phase13_tests.cpp)
exgine_add_test(exgine_editor_phase14_tests tests/editor_phase14_tests.cpp)
exgine_add_test(exgine_gpu_phase15_tests tests/gpu_phase15_tests.cpp)
exgine_add_test(exgine_animation_phase16_tests tests/animation_phase16_tests.cpp)
exgine_add_test(exgine_animation_phase16_runtime_tests tests/animation_phase16_runtime_tests.cpp)
exgine_add_test(exgine_gpu_animation_phase17_tests tests/gpu_animation_phase17_tests.cpp)
