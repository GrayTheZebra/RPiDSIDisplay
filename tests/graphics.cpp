#include "RPiDSIDisplay.h"
#include <cassert>
#include <vector>
#include <algorithm>
static std::vector<uint8_t> memory;
static bool failStart=false;
static int flushes=0;
extern "C" esp_err_t rpi_dsi_backend_begin(const rpi_dsi_config_t *c,uint8_t **p){
 if(failStart)return ESP_ERR_INVALID_STATE;
 memory.assign((size_t)c->width*c->height*3+32,0xa5);
 *p=memory.data()+16;return ESP_OK;
}
extern "C" esp_err_t rpi_dsi_backend_end(){return ESP_OK;}
extern "C" esp_err_t rpi_dsi_backend_flush(){++flushes;return ESP_OK;}
extern "C" esp_err_t rpi_dsi_backend_brightness(uint8_t){return ESP_OK;}
int main(){
 RPiDSIDisplay d, second;
 RPiDSIConfig c;c.width=11;c.height=7;
 assert(c.hbp==46 && c.hfp==23 && c.pixelMHz==20 && c.laneMbps==480 && c.useVtg);
 failStart=true;assert(!d.begin(c));failStart=false;
 assert(d.begin(c));assert(!second.begin(c));
 for(int r=0;r<4;++r){
  d.setRotation(r);
  for(int x=-3;x<14;++x)for(int y=-3;y<14;++y){
   d.fillScreen(0);
   d.fillRect(x,y,5,4,0xf800);
   std::vector<uint8_t> expected(11*7*3,0);
   for(int py=y;py<y+4;++py)for(int px=x;px<x+5;++px){
    if(px<0||py<0||px>=d.width()||py>=d.height())continue;
    int nx=px,ny=py;
    if(r==1){nx=10-py;ny=px;}
    if(r==2){nx=10-px;ny=6-py;}
    if(r==3){nx=py;ny=6-px;}
    expected[(ny*11+nx)*3+2]=255;
   }
   assert(std::equal(expected.begin(),expected.end(),d.getFramebuffer()));
   for(int i=0;i<16;++i){assert(memory[i]==0xa5);assert(memory[memory.size()-1-i]==0xa5);}
  }
 }
 d.setRotation(0);d.fillScreen(0);d.drawPixel(0,0,0x07e0);d.drawPixel(1,0,0x001f);
 assert(d.getFramebuffer()[1]==255 && d.getFramebuffer()[3]==255);
 assert(d.flushChecked());d.flush();assert(flushes==2);
 assert(d.end());assert(!d.ready());assert(!d.flushChecked());
 assert(second.begin(c));assert(second.end());
}
