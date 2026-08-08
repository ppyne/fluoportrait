#include "fluoportrait/color_science.hpp"
#include "fluoportrait/icc_profile.hpp"
#include "fluoportrait/image_processor.hpp"
#include "fluoportrait/jpeg_io.hpp"
#include "fluoportrait/png_loader.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

using namespace fluoportrait;
namespace {
void usage(){std::cout<<"Usage: fluoportrait INPUT.png OUTPUT.jpg --icc PROFILE.icc [options]\n"
 "Options:\n  --background-pq R,G,B  Direct Rec.2020/PQ bytes (default 196,202,156)\n"
 "  --sdr-white N          Portrait reference white in nits (default 75)\n"
 "  --quality Q            JPEG quality 1..100 (default 95; always 4:4:4)\n";}
RGB8 rgb(const std::string&s){unsigned r,g,b;char c1,c2;if(std::sscanf(s.c_str(),"%u%c%u%c%u",&r,&c1,&g,&c2,&b)!=5||c1!=','||c2!=','||r>255||g>255||b>255)throw std::invalid_argument("--background-pq requires R,G,B bytes");return{(std::uint8_t)r,(std::uint8_t)g,(std::uint8_t)b};}
}
int main(int argc,char**argv){
 try{
  if(argc<2||std::string(argv[1])=="--help"){usage();return argc<2?2:0;}
  if(argc<3)throw std::invalid_argument("input and output paths are required");
  std::filesystem::path input=argv[1],output=argv[2],iccPath;ProcessOptions options;int quality=95;
  for(int i=3;i<argc;++i){std::string a=argv[i];if(i+1>=argc)throw std::invalid_argument("missing value for "+a);std::string v=argv[++i];
   if(a=="--icc")iccPath=v;else if(a=="--background-pq")options.backgroundPQ=rgb(v);else if(a=="--sdr-white")options.portraitSdrWhiteNits=std::stod(v);else if(a=="--quality")quality=std::stoi(v);else throw std::invalid_argument("unknown option: "+a);}
  if(iccPath.empty())throw std::invalid_argument("--icc PROFILE.icc is required");if(quality<1||quality>100)throw std::invalid_argument("quality must be 1..100");
  const auto source=loadPNG(input);if(source.unsupportedICC)std::cerr<<"warning: source ICC '"<<source.sourceColorDescription<<"' is not supported; conversion assumes sRGB\n";
  const auto profile=loadICCProfile(iccPath);const auto image=composePortrait(source.image,options);writeJPEG(output,image,profile,quality);
  const auto check=inspectJPEG(output);if(check.width!=image.width||check.height!=image.height||check.components!=3||check.profile.bytes!=profile.bytes)throw std::runtime_error("post-export JPEG/ICC verification failed");
  std::cout<<"Export verified: "<<check.width<<'x'<<check.height<<" RGB JPEG, ICC embedded byte-for-byte\nICC: "<<profile.description<<"\nSource: "<<source.sourceColorDescription<<"\nBackground PQ: "<<(int)options.backgroundPQ.r<<'/'<<(int)options.backgroundPQ.g<<'/'<<(int)options.backgroundPQ.b<<" (estimated "<<color::directPQLuminance(options.backgroundPQ)<<" nits)\n";
 }catch(const std::exception&e){std::cerr<<"error: "<<e.what()<<'\n';return 1;}return 0;
}

