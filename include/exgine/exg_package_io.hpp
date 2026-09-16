#pragma once
#include "exgine/exg_package.hpp"
#include <string>
#if defined(__ANDROID__)
struct AAssetManager;
#endif
namespace exgine {
bool open_exg_package_file(const std::string&,ExgPackage&,std::string&) noexcept;
#if defined(__ANDROID__)
bool open_exg_package_asset(::AAssetManager*,const char*,ExgPackage&,std::string&) noexcept;
#endif
}