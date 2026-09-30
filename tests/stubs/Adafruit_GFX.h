#pragma once
#include <stdint.h>
#include <stddef.h>
// Minimal host test double. Does not validate Arduino/GFX binary compatibility.
class Adafruit_GFX {
protected:
 int16_t WIDTH, HEIGHT, _width, _height;
 uint8_t rotation=0;
public:
 Adafruit_GFX(int16_t w,int16_t h):WIDTH(w),HEIGHT(h),_width(w),_height(h){}
 virtual ~Adafruit_GFX()=default;
 virtual void flush(){}
 virtual void drawPixel(int16_t,int16_t,uint16_t)=0;
 virtual void fillRect(int16_t,int16_t,int16_t,int16_t,uint16_t){}
 virtual void fillScreen(uint16_t){}
 virtual void drawFastHLine(int16_t,int16_t,int16_t,uint16_t){}
 virtual void drawFastVLine(int16_t,int16_t,int16_t,uint16_t){}
 void setRotation(uint8_t r){rotation=r&3;_width=(rotation&1)?HEIGHT:WIDTH;_height=(rotation&1)?WIDTH:HEIGHT;}
 void setCursor(int16_t,int16_t){}
 int16_t width()const{return _width;}
 int16_t height()const{return _height;}
};
