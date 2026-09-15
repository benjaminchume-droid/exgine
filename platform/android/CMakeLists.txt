cmake_minimum_required(VERSION 3.20)
project(exgine_android LANGUAGES C CXX)

if(NOT ANDROID)
  message(FATAL_ERROR "platform/android requires an Android NDK CMake toolchain")
endif()

set(EXGINE_BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(EXGINE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../.. ${CMAKE_BINARY_DIR}/exgine-core)

set(ANDROID_GLUE_DIR "${ANDROID_NDK}/sources/android/native_app_glue")
set(ANDROID_GLUE_SOURCE "${ANDROID_GLUE_DIR}/android_native_app_glue.c")
if(NOT EXISTS "${ANDROID_GLUE_SOURCE}")
  message(FATAL_ERROR "Android NDK native_app_glue not found: ${ANDROID_GLUE_SOURCE}")
endif()

add_library(exgine_android SHARED
  src/main/cpp/native_activity.cpp
  ${ANDROID_GLUE_SOURCE}
)

target_include_directories(exgine_android PRIVATE
  ${ANDROID_GLUE_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/../../include
)
target_compile_features(exgine_android PRIVATE cxx_std_20)
target_link_libraries(exgine_android PRIVATE
  exgine_core
  android
  log
  EGL
  GLESv3
)
