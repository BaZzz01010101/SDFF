#pragma once

class SDFF_Bitmap
{
public:
  int width() const { return width_; }
  int height() const { return height_; }
  void resize(int width, int height);
  int savePNG(const char * fileName);
  unsigned char * data() { return pixels.data(); }
  const unsigned char & operator[](int ind) const { return pixels[ind]; }
  unsigned char & operator[](int ind) { return pixels[ind]; }

private:
  int width_;
  int height_;
  typedef std::vector<unsigned char> SDFF_PixelVector;
  SDFF_PixelVector pixels;
};
