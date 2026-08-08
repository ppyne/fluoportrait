#include "fluoportrait/jpeg_io.hpp"
#include <jpeglib.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace fluoportrait {
namespace {
constexpr unsigned char sig[]={'I','C','C','_','P','R','O','F','I','L','E',0};
void embed(j_compress_ptr c, const ICCProfile& p) {
  constexpr std::size_t maxChunk=65519;
  const std::size_t count=(p.bytes.size()+maxChunk-1)/maxChunk;
  if (!count || count>255) throw std::runtime_error("ICC profile too large for JPEG APP2 sequence");
  for (std::size_t i=0, off=0; i<count; ++i) {
    const auto n=std::min(maxChunk,p.bytes.size()-off); std::vector<JOCTET> marker(14+n);
    std::memcpy(marker.data(),sig,12); marker[12]=static_cast<JOCTET>(i+1); marker[13]=static_cast<JOCTET>(count);
    std::memcpy(marker.data()+14,p.bytes.data()+off,n); jpeg_write_marker(c,JPEG_APP0+2,marker.data(),marker.size()); off+=n;
  }
}
ICCProfile collect(j_decompress_ptr d) {
  std::vector<jpeg_saved_marker_ptr> parts(256,nullptr); int expected=0;
  for (auto* m=d->marker_list; m; m=m->next) if (m->marker==JPEG_APP0+2 && m->data_length>=14 && !std::memcmp(m->data,sig,12)) {
    const int seq=m->data[12], total=m->data[13]; if (!seq || !total || seq>total || (expected && total!=expected) || parts[seq]) throw std::runtime_error("malformed JPEG ICC sequence");
    expected=total; parts[seq]=m;
  }
  ICCProfile p; if (!expected) return p;
  for (int i=1;i<=expected;++i) { if(!parts[i]) throw std::runtime_error("incomplete JPEG ICC sequence"); p.bytes.insert(p.bytes.end(),parts[i]->data+14,parts[i]->data+parts[i]->data_length); }
  return p;
}
FILE* openFile(const std::filesystem::path& p,const char* mode){FILE*f=std::fopen(p.string().c_str(),mode);if(!f)throw std::runtime_error("cannot open file: "+p.string());return f;}
}
void writeJPEG(const std::filesystem::path& path,const ImageRGB8& image,const ICCProfile& profile,int quality) {
  if(image.pixels.size()!=std::size_t(image.width)*image.height||profile.bytes.empty())throw std::invalid_argument("invalid image or empty ICC profile");
  FILE* f=openFile(path,"wb"); jpeg_compress_struct c{}; jpeg_error_mgr err{}; c.err=jpeg_std_error(&err); jpeg_create_compress(&c); jpeg_stdio_dest(&c,f);
  c.image_width=image.width;c.image_height=image.height;c.input_components=3;c.in_color_space=JCS_RGB;jpeg_set_defaults(&c);
  for(int i=0;i<3;++i){c.comp_info[i].h_samp_factor=1;c.comp_info[i].v_samp_factor=1;} // 4:4:4
  jpeg_set_quality(&c,quality,TRUE);jpeg_start_compress(&c,TRUE);embed(&c,profile);
  std::vector<JSAMPLE> row(std::size_t(image.width)*3);
  while(c.next_scanline<c.image_height){for(unsigned x=0;x<image.width;++x){auto p=image.pixels[std::size_t(c.next_scanline)*image.width+x];row[3*x]=p.r;row[3*x+1]=p.g;row[3*x+2]=p.b;}JSAMPROW rp=row.data();jpeg_write_scanlines(&c,&rp,1);}
  jpeg_finish_compress(&c);jpeg_destroy_compress(&c);std::fclose(f);
}
JPEGInspection inspectJPEG(const std::filesystem::path& path){FILE*f=openFile(path,"rb");jpeg_decompress_struct d{};jpeg_error_mgr e{};d.err=jpeg_std_error(&e);jpeg_create_decompress(&d);jpeg_stdio_src(&d,f);jpeg_save_markers(&d,JPEG_APP0+2,0xFFFF);jpeg_read_header(&d,TRUE);auto p=collect(&d);JPEGInspection r{d.image_width,d.image_height,d.num_components,std::move(p)};jpeg_destroy_decompress(&d);std::fclose(f);return r;}
ImageRGB8 readJPEG(const std::filesystem::path& path){FILE*f=openFile(path,"rb");jpeg_decompress_struct d{};jpeg_error_mgr e{};d.err=jpeg_std_error(&e);jpeg_create_decompress(&d);jpeg_stdio_src(&d,f);jpeg_read_header(&d,TRUE);d.out_color_space=JCS_RGB;jpeg_start_decompress(&d);ImageRGB8 r{d.output_width,d.output_height,{}};r.pixels.resize(std::size_t(r.width)*r.height);std::vector<JSAMPLE> row(std::size_t(r.width)*3);while(d.output_scanline<d.output_height){JSAMPROW rp=row.data();jpeg_read_scanlines(&d,&rp,1);auto y=d.output_scanline-1;for(unsigned x=0;x<r.width;++x)r.pixels[std::size_t(y)*r.width+x]={row[3*x],row[3*x+1],row[3*x+2]};}jpeg_finish_decompress(&d);jpeg_destroy_decompress(&d);std::fclose(f);return r;}
}
