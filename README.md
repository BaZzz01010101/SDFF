@@ -0,0 +1,73 @@
# SDFF
Signed Distance Field Font utility.

## Overview
This tool designed for building signed distance field font texture image and metadata, 
saving it to files and further using in projects that needs quality and scalable text rendering.  
Building distance field based on [Meijster algorithm]( http://www.rug.nl/research/portal/publications/a-general-algorithm-for-computing-distance-transforms-in-linear-time(15dd2ec9-d221-45da-b2b0-1164978717dc).html) due to its efficiency and accuracy.  
In addition, it can be easily parallelized that makes possible to implement it on shaders in advance.

##License
This software released under GNU GPL v3

##Examples  
Building:
```c++
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "static_headers.h"

#include "sdff_builder.h"

int main(int argc, char * argv[])
{
  static const char * srcFontFileName = "C:\\Windows\\Fonts\\arial.ttf";
  static const char * dstPngFileName = "D:\\MyProject\\Font\\arial.png";
  static const char * dstJsonFileName = "D:\\MyProject\\Font\\arial.json";

  // create builder object
  SDFF_Builder sdff;
  // size of rendered font glyphs
  int renderFontSize = 2048;
  // size of result distance field font glyphs
  int sdffFontSize = 64;
  // size of distance field falloff as fraction of the glyph size
  float sdffFontFalloff = 0.125f;
  // initialize builder with selected parameters
  sdff.init(renderFontSize, sdffFontSize, sdffFontFalloff);
  // create font object
  SDFF_Font font;
  // add font into builder
  SDFF_Error error = sdff.addFont(srcFontFileName, 0, &font);
  // add font chars into builder
  error = sdff.addChars(font, 'A', 'Z');
  error = sdff.addChar(font, ' ');
  // create bitmap object
  SDFF_Bitmap textureBitmap;
  // fill bitmap with composed SDF font texture
  sdff.composeTexture(textureBitmap, true);
  // writing texture image
  textureBitmap.savePNG(dstPngFileName);
  // writing font metadata
  font.save(dstJsonFileName);
  
  return 1;
}
```
Using:
```c++
#include "static_headers.h"

#include "sdff_font.h"

int main(int argc, char * argv[])
{
  static const char * fontMetadataFileName = "D:\\MyProject\\Font\\arial.json";

  // create font object
  SDFF_Font font;
  // loading font metadata
  font.load(fontMetadataFileName);
  // retrieving glyph metadata
  const SDFF_Glyph * glyph = font.getGlyph('A');
  // retrieving kerning
  float kern = font.getKerning('W', 'A');

  return 1;
}
```
[<img src="https://github.com/BaZzz01010101/SDFF/blob/085ca5e59d28a0d57e7fbb5a7f4aadd4e97c8f05/bin/font.png"/>](https://github.com/BaZzz01010101/SDFF/blob/085ca5e59d28a0d57e7fbb5a7f4aadd4e97c8f05/bin/font.png)
