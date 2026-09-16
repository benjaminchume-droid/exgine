cmake_minimum_required(VERSION 3.16)
project(NeonRunner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/W4)
endif()

set(EXGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
add_subdirectory(${EXGINE_ROOT} exgine_build)

add_executable(neon_runner main.cpp)

target_link_libraries(neon_runner PRIVATE exgine::exgine)
target_include_directories(neon_runner PRIVATE ${EXGINE_ROOT}/include)

if(WIN32)
    target_compile_definitions(neon_runner PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
elseif(APPLE)
    target_compile_definitions(neon_runner PRIVATE GL_SILENCE_DEPRECATION)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    target_link_libraries(neon_runner PRIVATE ${COCOA_FRAMEWORK} ${IOKIT_FRAMEWORK} ${COREVIDEO_FRAMEWORK})
elseif(UNIX)
    find_package(X11 REQUIRED)
    find_package(OpenGL REQUIRED)
    target_link_libraries(neon_runner PRIVATE ${X11_LIBRARIES} ${OPENGL_LIBRARIES} dl pthread)
endif()

install(TARGETS neon_runner RUNTIME DESTINATION bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Building Neon Runner - ${CMAKE_BUILD_TYPE} mode")
