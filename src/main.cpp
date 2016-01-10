#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "static_headers.h"

#include "SDFF.h"
#include "Crosy.h"

int main(int argc, char * argv[])
{

  SDFF_Builder sdff;
  sdff.init(256, 64, 8);
  SDFF_Font font;
  std::string inFileName = Crosy::getExePath() + "Montserrat-Bold.otf";
  SDFF_Error error = sdff.addFont(inFileName.c_str(), 0, &font);
  error = sdff.addChars(font, '0', '9');
  error = sdff.addChars(font, 'A', 'Z');
  error = sdff.addChars(font, 'a', 'z');
  error = sdff.addChar(font, ' ');
  error = sdff.addChar(font, '\'');
  SDFF_Bitmap textureBitmap;
  sdff.composeTexture(textureBitmap, true);
  
  std::string outFileName = Crosy::getExePath() + "sdff_font.png";
  stbi_write_png(outFileName.c_str(), textureBitmap.width, textureBitmap.height, 1, textureBitmap.pixels.data(), 0);
	return 0;
}

