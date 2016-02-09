#include "static_headers.h"

#include "sdff_bitmap.h"

int SDFF_Bitmap::savePNG(const char * fileName)
{
  return stbi_write_png(fileName, width_, height_, 1, pixels.data(), 0);
}


void SDFF_Bitmap::resize(int width, int height)
{
  width_ = width;
  height_ = height;
  pixels.resize(width * height);
}