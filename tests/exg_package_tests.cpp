#include "exgine/exg_package.hpp"
#include <cassert>
#include <cstddef>
#include <string>
#include <vector>
int main(){std::vector<std::byte> payload(1024*1024);for(std::size_t i=0;i<payload.size();++i)payload[i]=std::byte((i/17)%7);exgine::ExgPackage p;std::string e;assert(exgine::build_exg_package(payload,{64*1024,true},p,e));assert(p.valid());assert(p.blocks.size()>1);exgine::ExgPackage parsed;assert(exgine::open_exg_package(p.bytes,parsed,e));std::vector<std::byte> decoded;std::size_t total=0;for(const auto&b:parsed.blocks){assert(exgine::decode_exg_block(parsed.bytes,b,decoded,e));total+=decoded.size();}assert(total==payload.size());exgine::ExgPackageStream stream(parsed);bool called=false;assert(stream.request(0,[&](std::uint64_t,std::span<const std::byte>d,std::string&err){assert(err.empty());assert(!d.empty());called=true;return true;}));for(int i=0;i<100&&!called;i++)stream.pump();assert(called);return 0;}