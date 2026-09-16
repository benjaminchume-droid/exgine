#pragma once
#include "exgine/exg_package.hpp"
#include <string>
namespace exgine {
bool open_exg_package_file(const std::string&path,ExgPackage&,std::string&) noexcept;
#if defined(__ANDROID__)
struct AAssetManager;
bool open_exg_package_asset(AAssetManager*,const char*,ExgPackage&,std::string&) noexcept;
#endif
}