#include "fluoportrait/color_science.hpp"
#include "fluoportrait/image_processor.hpp"
#include "fluoportrait/jpeg_io.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace fluoportrait;
namespace {
int failures=0;
void check(bool c,const char*n){if(!c){std::cerr<<"FAIL: "<<n<<'\n';++failures;}}
void near(double a,double b,double e,const char*n){check(std::abs(a-b)<=e,n);}
void put32(std::vector<std::uint8_t>&b,std::size_t o,std::uint32_t v){b[o]=v>>24;b[o+1]=v>>16;b[o+2]=v>>8;b[o+3]=v;}
ICCProfile fixtureICC(){ICCProfile p;p.description="Rec2020 Gamut with PQ Transfer";p.bytes.assign(132,0);put32(p.bytes,0,p.bytes.size());p.bytes[8]=4;p.bytes[12]='m';p.bytes[13]='n';p.bytes[14]='t';p.bytes[15]='r';p.bytes[16]='R';p.bytes[17]='G';p.bytes[18]='B';p.bytes[19]=' ';p.bytes[20]='X';p.bytes[21]='Y';p.bytes[22]='Z';p.bytes[23]=' ';p.bytes[36]='a';p.bytes[37]='c';p.bytes[38]='s';p.bytes[39]='p';return p;}
}
int main(){
 near(color::srgbToLinear(0),0,0,"sRGB black");near(color::srgbToLinear(1),1,1e-12,"sRGB white");
 near(color::pqEncode(100),0.5080784215,1e-9,"PQ 100 nits");near(color::pqEncode(1000),0.7518270962,1e-9,"PQ 1000 nits");
 for(double l:{0.0,0.1,1.0,50.0,100.0,400.0,1000.0,10000.0})near(color::pqDecode(color::pqEncode(l)),l,std::max(1e-8,l*1e-9),"PQ round trip");
 // The specified rounded conversion matrix does not sum to exactly 1.0 in
 // every row, so converted white is approximate but must quantize correctly.
 const auto white=color::portraitToPQ({1,1,1},100);near(white.r,color::pqEncode(100),3e-5,"white R");near(white.g,color::pqEncode(100),3e-5,"white G");near(white.b,color::pqEncode(100),3e-5,"white B");check(color::quantizePQ(white.r)==130,"white quantizes to 130");
 ImageRGBA8 src{3,1,{{255,255,255,255},{255,255,255,128},{3,4,5,0}}};ProcessOptions o;o.portraitSdrWhiteNits=100;o.backgroundPQ={196,202,156};auto out=composePortrait(src,o);
 check(out.pixels[0].r==130&&out.pixels[0].g==130&&out.pixels[0].b==130,"opaque portrait");check(out.pixels[2].r==196&&out.pixels[2].g==202&&out.pixels[2].b==156,"transparent exact direct PQ");
 const double a=128/255.0;check(out.pixels[1].r==color::quantizePQ(color::pqEncode(100*a+color::pqDecode(196/255.0)*(1-a))),"alpha blends in absolute light");
 ImageRGB8 uniform{32,32,std::vector<RGB8>(1024,o.backgroundPQ)};auto path=std::filesystem::temp_directory_path()/"fluoportrait-test.jpg";auto profile=fixtureICC();writeJPEG(path,uniform,profile,100);auto inspection=inspectJPEG(path);check(inspection.profile.bytes==profile.bytes,"ICC APP2 byte round trip");check(inspection.width==32&&inspection.height==32&&inspection.components==3,"JPEG structure");auto decoded=readJPEG(path);for(auto p:decoded.pixels)check(std::abs((int)p.r-196)<=2&&std::abs((int)p.g-202)<=2&&std::abs((int)p.b-156)<=2,"JPEG direct PQ fidelity");std::filesystem::remove(path);
 if(failures){std::cerr<<failures<<" test(s) failed\n";return 1;}std::cout<<"All color-science and export tests passed\n";
}
