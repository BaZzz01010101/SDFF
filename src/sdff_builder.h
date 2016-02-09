#pragma once
#include "sdff_error.h"
#include "sdff_bitmap.h"
#include "sdff_font.h"

class SDFF_Builder
{
public:
  SDFF_Builder();
  ~SDFF_Builder();

  SDFF_Error init(int sourceFontSize, int sdfFontSize, float falloff);
  SDFF_Error addFont(const char * fileName, int faceIndex, SDFF_Font * out_font);
  SDFF_Error addChar(SDFF_Font & font, SDFF_Char charCode);
  SDFF_Error addChars(SDFF_Font & font, SDFF_Char firstCharCode, SDFF_Char lastCharCode);
  SDFF_Error addChars(SDFF_Font & font, const char * charString);
  SDFF_Error composeTexture(SDFF_Bitmap & bitmap, bool powerOfTwo);

private:

  typedef std::map<SDFF_Char, SDFF_Bitmap> CharMap;
  
  struct FontData
  {
    FT_Face ftFace;
    CharMap chars;
  };

  typedef std::map<SDFF_Font *, FontData> FontMap;
  typedef std::vector<float> DistanceFieldVector;

  FT_Library ftLibrary_;
  FontMap fonts_;
  int sourceFontSize_; 
  int sdfFontSize_;
  float falloff_;
  bool initialized_;
  int maxSrcDfSize_;
  int maxDstDfSize_;

  unsigned int firstPowerOfTwoGreaterThen(unsigned int value);
  float createSdf(const FT_Bitmap & ftBitmap, int falloff, DistanceFieldVector & result) const;
  float createDf(const FT_Bitmap & ftBitmap, int falloff, bool invert, DistanceFieldVector & result) const;
  void copyBitmap(const SDFF_Bitmap & srcBitmap, SDFF_Bitmap & destBitmap, int posX, int posY) const;
};
