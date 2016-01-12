#include "static_headers.h"

#include "SDFF.h"
#include "Crosy.h"

int main(int argc, char * argv[])
{
  SDFF_Builder sdff;
  int renderFontSize = 256;
  int sdffFontSize = 64;
  int sdffFontFalloff = 8;
  std::string sourceFontFileName = "Montserrat-Bold.otf";
  std::string destFileName = "font";

  sdff.init(renderFontSize, sdffFontSize, sdffFontFalloff);
  SDFF_Font font;
  std::string inFileName = Crosy::getExePath() + sourceFontFileName;
  SDFF_Error error = sdff.addFont(inFileName.c_str(), 0, &font);
  error = sdff.addChars(font, '0', '9');
  error = sdff.addChars(font, 'A', 'Z');
  error = sdff.addChars(font, 'a', 'z');
  error = sdff.addChar(font, ' ');
  error = sdff.addChar(font, '\'');
  SDFF_Bitmap textureBitmap;
  sdff.composeTexture(textureBitmap, true);
  
  // writing texture image
  std::string outImageFileName = Crosy::getExePath() + destFileName + ".png";
  textureBitmap.savePNG(outImageFileName.c_str());

  // writing metadata
  std::string outJsonFileName = Crosy::getExePath() + destFileName + ".json";
  font.save(outJsonFileName.c_str());

	return 1;
}

