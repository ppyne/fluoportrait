#include "fluoportrait/icc_profile.hpp"
#include <fstream>
#include <stdexcept>

namespace fluoportrait {
namespace {
std::uint32_t be32(const std::uint8_t* p) { return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|(std::uint32_t(p[2])<<8)|p[3]; }
std::string tagText(const std::uint8_t* p, std::size_t n) {
  if (n < 12) return {};
  const std::string kind(reinterpret_cast<const char*>(p), 4);
  if (kind == "desc") {
    const auto len=be32(p+8); if (len && 12ull+len <= n) return std::string(reinterpret_cast<const char*>(p+12), len-1);
  }
  if (kind == "mluc" && n >= 28) {
    const auto count=be32(p+8), recSize=be32(p+12);
    if (count && recSize >= 12 && 16ull+recSize <= n) {
      const auto len=be32(p+20), off=be32(p+24); if (off+len <= n) {
        std::string out; out.reserve(len/2);
        for (std::uint32_t i=0; i+1<len; i+=2) {
          const auto cp=std::uint16_t(p[off+i])<<8|p[off+i+1];
          out.push_back(cp < 128 ? static_cast<char>(cp) : '?');
        } return out;
      }
    }
  }
  return {};
}
}
ICCProfile loadICCProfile(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary); if (!in) throw std::runtime_error("cannot open ICC profile: "+path.string());
  ICCProfile p; p.bytes.assign(std::istreambuf_iterator<char>(in), {});
  if (p.bytes.size() < 132 || std::string(reinterpret_cast<char*>(p.bytes.data()+36),4) != "acsp") throw std::runtime_error("not a valid ICC profile");
  const auto declared=be32(p.bytes.data()); if (declared < 132 || declared > p.bytes.size()) throw std::runtime_error("invalid ICC profile size");
  p.bytes.resize(declared);
  const auto count=be32(p.bytes.data()+128);
  if (132ull+12ull*count > p.bytes.size()) throw std::runtime_error("invalid ICC tag table");
  for (std::uint32_t i=0; i<count; ++i) {
    const auto* e=p.bytes.data()+132+12*i;
    if (std::string(reinterpret_cast<const char*>(e),4) != "desc") continue;
    const auto off=be32(e+4), len=be32(e+8); if (off+len <= p.bytes.size()) p.description=tagText(p.bytes.data()+off,len);
  }
  if (p.description.empty()) p.description="(ICC description unavailable)";
  return p;
}
}
