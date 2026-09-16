#include "exgine/exg_package.hpp"
#include <algorithm>
#include <array>
#include <condition_variable>
#include <cstring>
#include <limits>
#include <mutex>
#include <queue>

namespace exgine {
namespace {
constexpr std::uint32_t MAGIC=0x4b505845u; // EXPK
constexpr std::uint16_t VERSION=2;
constexpr std::size_t HEADER=16;
constexpr std::size_t ENTRY=24;
constexpr std::uint8_t LZ4=2;
constexpr std::uint8_t LZ4HC=3;
void put16(std::vector<std::byte>&o,std::uint16_t v){o.push_back(std::byte(v&255));o.push_back(std::byte(v>>8));}
void put32(std::vector<std::byte>&o,std::uint32_t v){for(int i=0;i<4;i++)o.push_back(std::byte((v>>(8*i))&255));}
void put64(std::vector<std::byte>&o,std::uint64_t v){for(int i=0;i<8;i++)o.push_back(std::byte((v>>(8*i))&255));}
std::uint16_t get16(std::span<const std::byte>s,std::size_t p){return std::uint16_t(std::to_integer<unsigned>(s[p]))|(std::uint16_t(std::to_integer<unsigned>(s[p+1]))<<8);}
std::uint32_t get32(std::span<const std::byte>s,std::size_t p){std::uint32_t v=0;for(int i=0;i<4;i++)v|=std::uint32_t(std::to_integer<unsigned>(s[p+i]))<<(8*i);return v;}
std::uint64_t get64(std::span<const std::byte>s,std::size_t p){std::uint64_t v=0;for(int i=0;i<8;i++)v|=std::uint64_t(std::to_integer<unsigned>(s[p+i]))<<(8*i);return v;}
void emit_len(std::vector<std::byte>&o,std::size_t n){while(n>=255){o.push_back(std::byte{255});n-=255;}o.push_back(std::byte(n));}
void emit_match(std::vector<std::byte>&o,std::size_t n){emit_len(o,n-4);}
std::vector<std::byte> lz4_encode(std::span<const std::byte>s,bool hc){
    std::vector<std::byte> out; if(s.empty()) return out;
    constexpr std::size_t HS=1u<<16; std::vector<std::int32_t> head(HS,-1); std::vector<std::int32_t> prev(hc?s.size():0,-1);
    auto hash=[&](std::size_t p){std::uint32_t x=0;std::memcpy(&x,s.data()+p,4);return (x*2654435761u)>>16;};
    std::size_t anchor=0,p=0;
    while(p+4<=s.size()){
        std::size_t best_len=0,best_pos=0; auto h=hash(p); int cur=head[h];
        int tries=hc?64:1;
        while(cur>=0&&tries--){std::size_t q=std::size_t(cur); if(p-q<=65535&&std::memcmp(s.data()+q,s.data()+p,4)==0){std::size_t n=4;while(p+n<s.size()&&s[q+n]==s[p+n]&&n<65535+4)n++;if(n>best_len){best_len=n;best_pos=q;if(n>=256)break;}} if(!hc)break;cur=prev[q];}
        if(hc){prev[p]=head[h];} head[h]=std::int32_t(p);
        if(best_len<4){++p;continue;}
        // Lazy matching: prefer a match at p+1 if it is materially longer.
        if(hc&&p+5<=s.size()){
            auto h2=hash(p+1);int c2=head[h2];if(c2>=0&&p+1-std::size_t(c2)<=65535&&std::memcmp(s.data()+c2,s.data()+p+1,4)==0){std::size_t n2=4;while(p+1+n2<s.size()&&s[std::size_t(c2)+n2]==s[p+1+n2]&&n2<65539)n2++;if(n2>best_len+1){++p;prev[p]=head[h2];head[h2]=std::int32_t(p);continue;}}
        }
        std::size_t lit=p-anchor; std::size_t token_pos=out.size();out.push_back(std::byte{0});std::uint8_t token=std::uint8_t(std::min<std::size_t>(15,lit)<<4);if(lit>=15)emit_len(out,lit-15);out.insert(out.end(),s.begin()+std::ptrdiff_t(anchor),s.begin()+std::ptrdiff_t(p));
        out.push_back(std::byte(best_pos&255));out.push_back(std::byte((best_pos>>8)&255));std::size_t ml=best_len-4;token|=std::uint8_t(std::min<std::size_t>(15,ml));if(ml>=15)emit_match(out,best_len-11);out[token_pos]=std::byte(token);
        p+=best_len;anchor=p;
        if(hc){for(std::size_t q=p-best_len+1;q<p&&q+4<=s.size();++q){auto hh=hash(q);prev[q]=head[hh];head[hh]=std::int32_t(q);}}
    }
    std::size_t lit=s.size()-anchor;std::size_t tp=out.size();out.push_back(std::byte(std::min<std::size_t>(15,lit)<<4));if(lit>=15)emit_len(out,lit-15);out.insert(out.end(),s.begin()+std::ptrdiff_t(anchor),s.end());(void)tp;return out;
}
bool lz4_decode(std::span<const std::byte>s,std::size_t expected,std::vector<std::byte>&out){
    out.clear();out.reserve(expected);std::size_t p=0;while(p<s.size()){std::uint8_t t=std::to_integer<std::uint8_t>(s[p++]);std::size_t lit=t>>4;if(lit==15){std::uint8_t x=255;while(x==255){if(p>=s.size())return false;x=std::to_integer<std::uint8_t>(s[p++]);lit+=x;}}if(p+lit>s.size()||out.size()+lit>expected)return false;out.insert(out.end(),s.begin()+std::ptrdiff_t(p),s.begin()+std::ptrdiff_t(p+lit));p+=lit;if(p==s.size())break;if(p+2>s.size())return false;std::size_t off=std::to_integer<unsigned>(s[p])|(std::to_integer<unsigned>(s[p+1])<<8);p+=2;if(off==0||off>out.size())return false;std::size_t ml=t&15;if(ml==15){std::uint8_t x=255;while(x==255){if(p>=s.size())return false;x=std::to_integer<std::uint8_t>(s[p++]);ml+=x;}}ml+=4;if(out.size()+ml>expected)return false;for(std::size_t i=0;i<ml;i++)out.push_back(out[out.size()-off]);}return out.size()==expected;
}
}
std::uint32_t exg_crc32(std::span<const std::byte>d)noexcept{std::uint32_t c=0xffffffffu;for(auto b:d){c^=std::to_integer<unsigned char>(b);for(int i=0;i<8;i++)c=(c>>1)^(0xedb88320u&-(c&1));}return ~c;}

bool build_exg_package(std::span<const std::byte>payload,const ExgPackageBuildOptions&o,ExgPackage&out,std::string&err)noexcept{
    out={};if(o.block_size<4096){err="EXPK block size must be at least 4096 bytes";return false;}if(payload.size()>std::numeric_limits<std::uint32_t>::max()){err="EXPK payload too large";return false;}
    std::vector<std::vector<std::byte>> blocks;std::vector<ExgPackageBlock> idx;
    for(std::size_t p=0;p<payload.size();p+=o.block_size){auto n=std::min(o.block_size,payload.size()-p);auto src=payload.subspan(p,n);auto hc=lz4_encode(src,o.high_compression);auto fast=lz4_encode(src,false);std::vector<std::byte> enc=src;ExgCodec codec=ExgCodec::Raw;if(hc.size()<enc.size()){enc=std::move(hc);codec=ExgCodec::Lz4HighCompression;}else if(fast.size()<enc.size()){enc=std::move(fast);codec=ExgCodec::Lz4;}idx.push_back({0,std::uint32_t(enc.size()),std::uint32_t(n),codec,exg_crc32(src)});blocks.push_back(std::move(enc));}
    if(idx.empty()){err="cannot package empty payload";return false;}
    std::uint64_t data_offset=HEADER+std::uint64_t(idx.size())*ENTRY;out.bytes.reserve(std::size_t(data_offset)+payload.size());put32(out.bytes,MAGIC);put16(out.bytes,VERSION);put16(out.bytes,0);put32(out.bytes,std::uint32_t(idx.size()));put32(out.bytes,0);
    std::uint64_t cursor=data_offset;for(auto&e:idx){e.offset=cursor;put64(out.bytes,e.offset);put32(out.bytes,e.compressed_size);put32(out.bytes,e.decoded_size);out.bytes.push_back(std::byte(std::uint8_t(e.codec)));out.bytes.push_back(std::byte{0});put16(out.bytes,0);put32(out.bytes,e.checksum);cursor+=e.compressed_size;}
    for(auto&b:blocks)out.bytes.insert(out.bytes.end(),b.begin(),b.end());out.blocks=std::move(idx);return true;
}
bool open_exg_package(std::span<const std::byte>p,ExgPackage&out,std::string&err)noexcept{out={};if(p.size()<HEADER||get32(p,0)!=MAGIC||get16(p,4)!=VERSION){err="invalid EXPK header";return false;}auto n=get32(p,8);if(n==0||HEADER+std::uint64_t(n)*ENTRY>p.size()){err="invalid EXPK index";return false;}out.bytes.assign(p.begin(),p.end());out.blocks.reserve(n);for(std::uint32_t i=0;i<n;i++){auto q=HEADER+std::size_t(i)*ENTRY;ExgPackageBlock e{get64(p,q),get32(p,q+8),get32(p,q+12),ExgCodec(std::to_integer<unsigned char>(p[q+16])),get32(p,q+20)};if(e.offset<HEADER+std::uint64_t(n)*ENTRY||e.offset+e.compressed_size>p.size()||e.decoded_size==0){err="invalid EXPK block";out={};return false;}out.blocks.push_back(e);}return true;}
bool decode_exg_block(std::span<const std::byte>p,const ExgPackageBlock&e,std::vector<std::byte>&out,std::string&err)noexcept{if(e.offset+e.compressed_size>p.size()){err="EXPK block out of bounds";return false;}auto s=p.subspan(std::size_t(e.offset),e.compressed_size);bool ok=false;if(e.codec==ExgCodec::Raw){out.assign(s.begin(),s.end());ok=out.size()==e.decoded_size;}else if(e.codec==ExgCodec::Lz4||e.codec==ExgCodec::Lz4HighCompression)ok=lz4_decode(s,e.decoded_size,out);else {err="unsupported EXPK codec";return false;}if(!ok||exg_crc32(out)!=e.checksum){err="EXPK decode/checksum failure";out.clear();return false;}return true;}

ExgPackageStream::ExgPackageStream(ExgPackage p):package_(std::move(p)),mutex_(new std::mutex),condition_(new std::condition_variable),jobs_(new std::queue<Job>),completed_(new std::queue<Completed>),worker_(&ExgPackageStream::run,this){}
ExgPackageStream::~ExgPackageStream(){{std::lock_guard<std::mutex>l(*mutex_);stop_=true;}condition_->notify_all();if(worker_.joinable())worker_.join();delete completed_;delete jobs_;delete condition_;delete mutex_;}
bool ExgPackageStream::request(std::uint64_t id,DecodeCallback cb){if(id>=package_.blocks.size()||!cb)return false;{std::lock_guard<std::mutex>l(*mutex_);jobs_->push({id,std::move(cb)});}condition_->notify_one();return true;}
void ExgPackageStream::run(){for(;;){Job j;{std::unique_lock<std::mutex>l(*mutex_);condition_->wait(l,[&]{return stop_||!jobs_->empty();});if(stop_&&jobs_->empty())return;j=std::move(jobs_->front());jobs_->pop();}Completed c;c.block_id=j.block_id;c.callback=std::move(j.callback);decode_exg_block(package_.bytes,package_.blocks[std::size_t(j.block_id)],c.data,c.error);{std::lock_guard<std::mutex>l(*mutex_);completed_->push(std::move(c));}}}
std::size_t ExgPackageStream::pump(std::size_t max){std::size_t n=0;while(n<max){Completed c;{std::lock_guard<std::mutex>l(*mutex_);if(completed_->empty())break;c=std::move(completed_->front());completed_->pop();}if(c.callback)c.callback(c.block_id,c.data,c.error);++n;}return n;}
std::size_t ExgPackageStream::pending()const noexcept{std::lock_guard<std::mutex>l(*mutex_);return jobs_->size()+completed_->size();}

bool ExgGpuResidencyStreamer::submit_mesh_block(ExgPackageStream&s,std::uint64_t asset,std::uint32_t gen,std::uint64_t block,std::function<bool(std::span<const std::byte>,Mesh&,std::string&)> decoder){if(!gpu_.request(asset,0.0f,gen))return false;return s.request(block,[this,asset,gen,decoder=std::move(decoder)](std::uint64_t,std::span<const std::byte>data,std::string&err){Mesh m;if(!err.empty()){gpu_.mark_decoded(asset,gen,0);return;}if(!decoder(data,m,err)){gpu_.mark_decoded(asset,gen,0);return;}gpu_.mark_decoded(asset,gen,data.size());/* upload is intentionally deferred to pump/render thread */});}
std::size_t ExgGpuResidencyStreamer::pump(ExgPackageStream&s,std::uint64_t frame,std::size_t max){return s.pump(max);}
}