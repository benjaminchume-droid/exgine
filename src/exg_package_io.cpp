#include "exgine/exg_package_io.hpp"
#include <cstring>
#include <fstream>
namespace exgine {
bool open_exg_package_file(const std::string&path,ExgPackage&o,std::string&e)noexcept{std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f){e="unable to open EXG package: "+path;return false;}auto n=f.tellg();if(n<=0){e="EXG package is empty";return false;}f.seekg(0);std::vector<std::byte>b(static_cast<std::size_t>(n));if(!f.read(reinterpret_cast<char*>(b.data()),n)){e="unable to read EXG package: "+path;return false;}return open_exg_package(b,o,e);}
#if defined(__ANDROID__)
#include <android/asset_manager.h>
bool open_exg_package_asset(::AAssetManager*m,const char*name,ExgPackage&o,std::string&e)noexcept{if(!m||!name){e="invalid Android EXG asset request";return false;}AAsset*a=AAssetManager_open(m,name,AASSET_MODE_BUFFER);if(!a){e="unable to open EXG asset";return false;}const void*n=AAsset_getBuffer(a);auto sz=AAsset_getLength64(a);if(!n||sz<=0){AAsset_close(a);e="EXG asset is empty";return false;}std::vector<std::byte>b(static_cast<std::size_t>(sz));std::memcpy(b.data(),n,b.size());AAsset_close(a);return open_exg_package(b,o,e);}
#endif
}