cmake_minimum_required(VERSION 3.20)
project(exgine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(exgine_core
    src/ir.cpp
    src/world.cpp
)

target_include_directories(exgine_core PUBLIC include)

target_compile_features(exgine_core PUBLIC cxx_std_20)

add_executable(exgine_demo
    examples/hello_world.cpp
)

target_link_libraries(exgine_demo PRIVATE exgine_core)
