#include "static_headers.h"

#include "SDFF.h"
#include "Crosy.h"

//-------------------------------------------------------------------------------------------------

SDFF_Builder::SDFF_Builder()
{
  FT_Init_FreeType(&ftLibrary);
}

//-------------------------------------------------------------------------------------------------

SDFF_Builder::~SDFF_Builder()
{
  FT_Done_FreeType(ftLibrary);
}

//-------------------------------------------------------------------------------------------------

SDFF_Error SDFF_Builder::init(int sourceFontSize, int sdfFontSize, int falloff)
{
  this->sourceFontSize = sourceFontSize;
  this->sdfFontSize = sdfFontSize;
  this->falloff = falloff;
  fonts.clear();

  return SDFF_OK;
}

//-------------------------------------------------------------------------------------------------

SDFF_Error SDFF_Builder::addFont(const char * fileName, int faceIndex, SDFF_Font * out_font)
{
  if (fonts.find(out_font) != fonts.end())
    return SDFF_FONT_ALREADY_EXISTS;

  FontData & fontData = fonts[out_font];
  FT_Face & ftFace = fontData.ftFace;

  FT_Error ftError;
  ftError = FT_New_Face(ftLibrary, fileName, faceIndex, &ftFace);
  assert(!ftError);

  if (ftError)
    return SDFF_FT_NEW_FACE_ERROR;

  ftError = FT_Set_Char_Size(ftFace, sourceFontSize * 64, sourceFontSize * 64, 64, 64);
  assert(!ftError);

  if (ftError)
    return SDFF_FT_SET_CHAR_SIZE_ERROR;

  return SDFF_OK;
}

//-------------------------------------------------------------------------------------------------

SDFF_Error SDFF_Builder::addChar(SDFF_Font & font, SDFF_Char charCode)
{
  if (fonts.find(&font) == fonts.end())
    return SDFF_FONT_NOT_EXISTS;

  FontData & fontData = fonts[&font];
  FT_Face & ftFace = fontData.ftFace;
  CharMap & chars = fontData.chars;

  if (chars.find(charCode) != chars.end())
    return SDFF_CHAR_ALREADY_EXISTS;

  FT_Error ftError = FT_Load_Char(ftFace, (const FT_UInt)charCode, FT_LOAD_DEFAULT | FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_RENDER | FT_LOAD_TARGET_MONO | FT_LOAD_FORCE_AUTOHINT);
  assert(!ftError);

  if (ftError)
    return SDFF_FT_SET_CHAR_SIZE_ERROR;

  SDFF_Bitmap & charBitmap = chars[charCode];

  if (ftFace->glyph->bitmap.width && ftFace->glyph->bitmap.rows)
  {

    static DistanceFieldVector srcSdf;
    static DistanceFieldVector destSdf;
    //int maxWidth = fontData.width * (ftFace->bbox.xMax - ftFace->bbox.xMin) / ftFace->units_per_EM;
    //int maxHeight = fontData.height * (ftFace->bbox.yMax - ftFace->bbox.yMin) / ftFace->units_per_EM;
    //sdf.reserve(maxWidth * maxHeight);
    int srcFalloff = falloff * sourceFontSize / sdfFontSize;
    createSdf(ftFace->glyph->bitmap, srcFalloff, srcSdf);

    int srcWidth = ftFace->glyph->bitmap.width + 2 * srcFalloff;
    int srcHeight = ftFace->glyph->bitmap.rows + 2 * srcFalloff;
    float scale = (float)sdfFontSize / sourceFontSize;
    int destWidth = (int)glm::ceil(srcWidth * scale);
    int destHeight = (int)glm::ceil(srcHeight * scale);
    destSdf.assign(destWidth * destHeight, 0.0f);

    for (int y = 0; y < srcHeight; y++)
    for (int x = 0; x < srcWidth; x++)
    {
      int srcIndex = x + y * srcWidth;
      assert(srcIndex < srcWidth * srcHeight);
      int destX = int(x * scale);
      int destY = int(y * scale);
      int destIndex = destX + destY * destWidth;
      assert(destIndex < destWidth * destHeight);

      // if (x < srcWidth)
      // TODO subpixels

      destSdf[destIndex] += srcSdf[srcIndex];
    }

    charBitmap.width = destWidth;
    charBitmap.height = destHeight;
    charBitmap.pixels.resize(destWidth * destHeight);
    float sqScale = scale * scale;
    const int midPoint = 64;
    const float mul = float(glm::max(midPoint, 255 - midPoint)) / srcFalloff;

    for (int y = 0; y < destHeight; y++)
    for (int x = 0; x < destWidth; x++)
    {
      int ind = x + y * destWidth;
      charBitmap.pixels[ind] = (unsigned char)glm::clamp(midPoint + int(destSdf[ind] * sqScale * mul), 0, 255);
    }
  }
  else
  {
    charBitmap.width = 0;
    charBitmap.height = 0;
    charBitmap.pixels.resize(0);
  }
  
  if (chars.size() > 1)
  {
    for (CharMap::iterator charIt = chars.begin(); charIt != chars.end(); ++charIt)
    {
      if (charIt->first != charCode)
      {
        char c1 = charIt->first;
        char c2 = charCode;

        FT_UInt glyphIndex1 = FT_Get_Char_Index(ftFace, FT_ULong(charIt->first));
        FT_UInt glyphIndex2 = FT_Get_Char_Index(ftFace, FT_ULong(charCode));

        SDFF_Font::CharPair charPair = { charIt->first, charCode };
        FT_Vector kern = { 0, 0 };
        ftError = FT_Get_Kerning(ftFace, glyphIndex1, glyphIndex2, FT_KERNING_UNFITTED, &kern);
        assert(!ftError);

        if (kern.x)
          font.kerning[charPair] = float(kern.x) / sourceFontSize;

        charPair = { charCode, charIt->first };
        kern = { 0, 0 };
        ftError = FT_Get_Kerning(ftFace, glyphIndex2, glyphIndex1, FT_KERNING_UNFITTED, &kern);
        assert(!ftError);

        if (kern.x)
          font.kerning[charPair] = float(kern.x) / sourceFontSize;
      }
    }
  }
  SDFF_Glyph & glyph = font.glyphs[charCode];
  glyph.horiBearingX = float(ftFace->glyph->metrics.horiBearingX) / 64 / sourceFontSize;
  glyph.horiBearingY = float(ftFace->glyph->metrics.horiBearingY) / 64 / sourceFontSize;
  glyph.horiAdvance = float(ftFace->glyph->metrics.horiAdvance) / 64 / sourceFontSize;

  return SDFF_OK;
}

//-------------------------------------------------------------------------------------------------

SDFF_Error SDFF_Builder::addChars(SDFF_Font & font, SDFF_Char firstCharCode, SDFF_Char lastCharCode)
{
  for (SDFF_Char charCode = firstCharCode; charCode <= lastCharCode; charCode++)
  {
    SDFF_Error error = addChar(font, charCode);

    if (error != SDFF_OK)
      return error;
  }

  return SDFF_OK;
}

//-------------------------------------------------------------------------------------------------

SDFF_Error SDFF_Builder::addChars(SDFF_Font & font, const char * charString)
{

  return SDFF_OK;
}

//-------------------------------------------------------------------------------------------------

struct Rect
{
  int left;
  int top;
  int width;
  int height;
  SDFF_Font * font;
  SDFF_Char charCode;

  int bottom() const { return top + height - 1; }
  int right() const { return left + width - 1; }
  bool intersect(const Rect & rect) const { return rect.left < left + width && rect.left + rect.width > left && rect.top < top + height && rect.top + rect.height > top; }
  bool contain(int x, int y) const { return x >= left && x < left + width && y >= top && y < top + height; }
  bool fitIn(const Rect & rect) const { return rect.width >= width && rect.height >= height; }
  bool operator ==(const Rect & rect) const { return rect.left == left && rect.top == top && rect.width == width && rect.height == height; }
};

template<>
struct std::hash<Rect>
{
  std::size_t operator()(const Rect & rect) const
  {
    return hash<uint64_t>()(
      (uint64_t(rect.left & 0xFFFF)) << 48 |
      (uint64_t(rect.top & 0xFFFF)) << 32 |
      (uint64_t(rect.width & 0xFFFF)) << 16 |
      (uint64_t(rect.height & 0xFFFF)) << 0);
  }
};

SDFF_Error SDFF_Builder::composeTexture(SDFF_Bitmap & bitmap, bool powerOfTwo)
{
  typedef std::multimap<int, Rect> CharRectMap;
  typedef std::unordered_set<Rect> FreeRectSet;
  typedef std::vector<FreeRectSet::iterator> EraseVector;
  typedef std::vector<Rect> InsertVector;
  CharRectMap charRects;
  FreeRectSet freeRects;
  EraseVector eraseVector;
  InsertVector insertVector;
  eraseVector.reserve(1024);
  insertVector.reserve(1024);

  // add all our char rects into the array
  for (FontMap::iterator fontIt = fonts.begin(); fontIt != fonts.end(); ++fontIt)
  {
    CharMap & chars = fontIt->second.chars;

    for (CharMap::iterator charIt = chars.begin(); charIt != chars.end(); ++charIt)
    {
      int width = charIt->second.width;
      int height = charIt->second.height;
      Rect charRect = { 0, 0, width, height, fontIt->first, charIt->first };
      charRects.insert(std::make_pair(width * height, charRect));
    }
  }

  // statring with one free very big rect
  freeRects.insert({ 0, 0, INT_MAX, INT_MAX });
  int maxRight = 0;
  int maxBottom = 0;

  // enumerating all chars
  for (CharRectMap::reverse_iterator charRectIt = charRects.rbegin(); charRectIt != charRects.rend(); ++charRectIt)
  {
    if (!charRectIt->second.width || !charRectIt->second.height)
      continue;

    char cc = charRectIt->second.charCode;

    // find best fit free rect for placing our char
    Rect & charRect = charRectIt->second;
    const Rect * bestRectPtr = NULL;
    float bestEstimator = FLT_MAX;

    // for each char searching for most appropriate free rect using estimator
    for (FreeRectSet::iterator freeRectIt = freeRects.begin(); freeRectIt != freeRects.end(); ++freeRectIt)
    {
      freeRects;
      const Rect & freeRect = *freeRectIt;

      if (charRect.fitIn(freeRect))
      {
        int thisRight = freeRect.left + charRect.width;
        int thisBottom = freeRect.top + charRect.height;
        int thisMaxRight = glm::max(maxRight, thisRight);
        int thisMaxBottom = glm::max(maxBottom, thisBottom);
        int minBounds = glm::max(thisMaxRight, thisMaxBottom);
        int minLeftTop = (freeRect.left + freeRect.top);
        float thisEstimator = 10.0f * minBounds + 0.1f * minLeftTop;

        if (thisEstimator < bestEstimator)
        {
          bestEstimator = thisEstimator;
          bestRectPtr = &freeRect;
        }
      }
    }

    assert(bestRectPtr);

    if (bestRectPtr)
    {
      // placing char into selected best free rect
      charRect.left = bestRectPtr->left;
      charRect.top = bestRectPtr->top;
      maxRight = glm::max(maxRight, charRect.left + charRect.width);
      maxBottom = glm::max(maxBottom, charRect.top + charRect.height);

      eraseVector.clear();
      insertVector.clear();

      // excluding char rect from any free rects that intersects with it
      for (FreeRectSet::iterator freeRectIt = freeRects.begin(); freeRectIt != freeRects.end(); ++freeRectIt)
      {
        const Rect & freeRect = *freeRectIt;

        if (charRect.intersect(freeRect))
        {
          bool leftTopIn = freeRect.contain(charRect.left, charRect.top);
          bool leftBottomIn = freeRect.contain(charRect.left, charRect.bottom());
          bool rightTopIn = freeRect.contain(charRect.right(), charRect.top);
          bool rightBottomIn = freeRect.contain(charRect.right(), charRect.bottom());
          int vertexInCount = (int)leftTopIn + (int)leftBottomIn + (int)rightTopIn + (int)rightBottomIn;

          if (vertexInCount == 0)
          {
            bool leftSideCross = charRect.left >= freeRect.left && charRect.left <= freeRect.right();
            bool rightSideCross = charRect.right() >= freeRect.left && charRect.right() <= freeRect.right();
            bool topSideCross = charRect.top >= freeRect.top && charRect.top <= freeRect.bottom();
            bool bottomSideCross = charRect.bottom() >= freeRect.top && charRect.bottom() <= freeRect.bottom();
            int crossCount = (int)leftSideCross + (int)rightSideCross + (int)topSideCross + (int)bottomSideCross;

            if (crossCount == 1)
            {
              Rect newRect = freeRect;

              if (leftSideCross)
                newRect.width = charRect.left - newRect.left;
              else if (topSideCross)
                newRect.height = charRect.top - newRect.top;
              else if (rightSideCross)
              {
                int dLeft = charRect.left + charRect.width - newRect.left;
                newRect.left += dLeft;
                newRect.width -= dLeft;
              }
              else if (bottomSideCross)
              {
                int dTop = charRect.top + charRect.height - newRect.top;
                newRect.top += dTop;
                newRect.height -= dTop;
              }
              else assert(0);

              if (newRect.width && newRect.height)
                insertVector.push_back(newRect);
            }
            else if (crossCount == 2)
            {
              Rect newRect1 = freeRect;
              Rect newRect2 = freeRect;
              
              if (leftSideCross) // and respectively rightSideCross
              {
                newRect1.width = charRect.left - newRect1.left;
                int dLeft = charRect.left + charRect.width - newRect2.left;
                newRect2.left += dLeft;
                newRect2.width -= dLeft;
              }
              else if (topSideCross) // and respectively bottomSideCross
              {
                newRect1.height = charRect.top - newRect1.top;
                int dTop = charRect.top + charRect.height - newRect2.top;
                newRect2.top += dTop;
                newRect2.height -= dTop;
              }
              else assert(0);

              if (newRect1.width && newRect1.height)
                insertVector.push_back(newRect1);

              if (newRect2.width && newRect2.height)
                insertVector.push_back(newRect2);
            }
            else assert(0);

          }
          else if(vertexInCount == 1)
          {
            Rect newRect1 = freeRect;
            Rect newRect2 = freeRect;
            
            if (leftTopIn)
            {
              newRect1.height = charRect.top - newRect1.top;
              newRect2.width = charRect.left - newRect2.left;

              if (newRect1.height)
                insertVector.push_back(newRect1);

              if (newRect2.width)
                insertVector.push_back(newRect2);
            }
            else if (leftBottomIn)
            {
              newRect1.width = charRect.left - newRect1.left;
              int dTop = charRect.top + charRect.height - newRect2.top;
              newRect2.top += dTop;
              newRect2.height -= dTop;
            }
            else if (rightTopIn)
            {
              newRect1.height = charRect.top - newRect1.top;
              int dLeft = charRect.left + charRect.width - newRect2.left;
              newRect2.left += dLeft;
              newRect2.width -= dLeft;
            }
            else if (rightBottomIn)
            {
              int dLeft = charRect.left + charRect.width - newRect1.left;
              newRect1.left += dLeft;
              newRect1.width -= dLeft;
              int dTop = charRect.top + charRect.height - newRect2.top;
              newRect2.top += dTop;
              newRect2.height -= dTop;
            }
            else assert(0);

            if (newRect1.width && newRect1.height)
              insertVector.push_back(newRect1);

            if (newRect2.width && newRect2.height)
              insertVector.push_back(newRect2);
          }
          else if (vertexInCount == 2)
          {
            bool leftSideIn = leftTopIn && leftBottomIn;
            bool topSideIn = leftTopIn && rightTopIn;
            bool rightSideIn = rightTopIn && rightBottomIn;
            bool bottomSideIn = leftBottomIn && rightBottomIn;
            Rect newRect1 = freeRect;
            Rect newRect2 = freeRect;
            Rect newRect3 = freeRect;
            
            if (leftSideIn)
            {
              newRect1.width = charRect.left - newRect1.left;
              newRect2.height = charRect.top - newRect2.top;
              int dTop = charRect.top + charRect.height - newRect3.top;
              newRect3.top += dTop;
              newRect3.height -= dTop;
            }
            else if (topSideIn)
            {
              newRect1.width = charRect.left - newRect1.left;
              newRect2.height = charRect.top - newRect2.top;
              int dLeft = charRect.left + charRect.width - newRect3.left;
              newRect3.left += dLeft;
              newRect3.width -= dLeft;
            }
            else if (rightSideIn)
            {
              newRect1.height = charRect.top - newRect1.top;
              int dLeft = charRect.left + charRect.width - newRect2.left;
              newRect2.left += dLeft;
              newRect2.width -= dLeft;
              int dTop = charRect.top + charRect.height - newRect3.top;
              newRect3.top += dTop;
              newRect3.height -= dTop;
            }
            else if (bottomSideIn)
            {
              newRect1.width = charRect.left - newRect1.left;
              int dLeft = charRect.left + charRect.width - newRect2.left;
              newRect2.left += dLeft;
              newRect2.width -= dLeft;
              int dTop = charRect.top + charRect.height - newRect3.top;
              newRect3.top += dTop;
              newRect3.height -= dTop;
            }
            else assert(0);

            if (newRect1.width && newRect1.height)
              insertVector.push_back(newRect1);

            if (newRect2.width && newRect2.height)
              insertVector.push_back(newRect2);

            if (newRect3.width && newRect3.height)
              insertVector.push_back(newRect3);
          }
          else if (vertexInCount == 4)
          {
            Rect newRect1 = freeRect;
            Rect newRect2 = freeRect;
            Rect newRect3 = freeRect;
            Rect newRect4 = freeRect;

            newRect1.width = charRect.left - newRect1.left;
            newRect2.height = charRect.top - newRect2.top;
            int dLeft = charRect.left + charRect.width - newRect3.left;
            newRect3.left += dLeft;
            newRect3.width -= dLeft;
            int dTop = charRect.top + charRect.height - newRect4.top;
            newRect4.top += dTop;
            newRect4.height -= dTop;

            if (newRect1.width)
              insertVector.push_back(newRect1);

            if (newRect2.height)
              insertVector.push_back(newRect2);

            if (newRect3.width)
              insertVector.push_back(newRect3);

            if (newRect4.height)
              insertVector.push_back(newRect4);
          }
          else assert(0);

          eraseVector.push_back(freeRectIt);
        } // if (charRect.intersect(freeRect))
      } // freeRects enumeration loop

      for (EraseVector::iterator eraseIt = eraseVector.begin(); eraseIt != eraseVector.end(); ++eraseIt)
        freeRects.erase(*eraseIt);

      for (InsertVector::iterator insertIt = insertVector.begin(); insertIt != insertVector.end(); ++insertIt)
        freeRects.insert(*insertIt);
    }
  }

  bitmap.width = maxRight + 1;
  bitmap.height = maxBottom + 1;

  if (powerOfTwo)
  {
    bitmap.width = (int)glm::pow(2.0f, glm::ceil(glm::log2((float)bitmap.width)));
    bitmap.height = (int)glm::pow(2.0f, glm::ceil(glm::log2((float)bitmap.height)));
  }
  
  bitmap.pixels.resize(bitmap.width * bitmap.height);

  for (CharRectMap::iterator charRectIt = charRects.begin(); charRectIt != charRects.end(); ++charRectIt)
  {
    Rect & charRect = charRectIt->second;
    FontMap::iterator fontIt = fonts.find(charRect.font);

    if (fontIt != fonts.end())
    {
      CharMap::iterator charIt = fontIt->second.chars.find(charRect.charCode);

      if (charIt != fontIt->second.chars.end())
      {
        SDFF_Bitmap & charBitmap = charIt->second;
        copyBitmap(charBitmap, bitmap, charRect.left, charRect.top);
      }
    }
  }

  return SDFF_OK;
}

//-------------------------------------------------------------------------------------------------

void SDFF_Builder::copyBitmap(const SDFF_Bitmap & srcBitmap, SDFF_Bitmap & destBitmap, int xPos, int yPos)
{
  assert(xPos >= 0);
  assert(yPos >= 0);
  assert(xPos + srcBitmap.width <= destBitmap.width);
  assert(yPos + srcBitmap.height <= destBitmap.height);

  int srcIndex = 0;
  int destIndex = yPos * destBitmap.width + xPos;

  for (int y = 0; y < srcBitmap.height; y++)
  {
    for (int x = 0; x < srcBitmap.width; x++)
    {
      destBitmap.pixels[destIndex] = destBitmap.pixels[destIndex] ? destBitmap.pixels[destIndex] : srcBitmap.pixels[srcIndex];
      srcIndex++;
      destIndex++;
    }

    destIndex += destBitmap.width - srcBitmap.width;
  }
}

//-------------------------------------------------------------------------------------------------

float SDFF_Builder::createSdf(const FT_Bitmap & ftBitmap, int falloff, DistanceFieldVector & result)
{
  float maxDist = createDf(ftBitmap, falloff, false, result);

  DistanceFieldVector invResult;
  float maxDistInv = createDf(ftBitmap, falloff, true, invResult);

  for (int i = 0, cnt = result.size(); i < cnt; i++)
  {
    if (result[i] < 0.5f)
      result[i] = 1 - invResult[i];
  }

  return glm::max(maxDist, maxDistInv - 1);
}

//-------------------------------------------------------------------------------------------------
//  Based on "General algorithm for computing distance transforms in linear time"
//  by A.MEIJSTER‚ J.B.T.M.ROERDINK† and W.H.HESSELINK†
//  University of Groningen
//  http://www.rug.nl/research/portal/publications/a-general-algorithm-for-computing-distance-transforms-in-linear-time(15dd2ec9-d221-45da-b2b0-1164978717dc).html

float SDFF_Builder::createDf(const FT_Bitmap & ftBitmap, int falloff, bool invert, DistanceFieldVector & result)
{
  assert(ftBitmap.width > 0);
  assert(ftBitmap.rows > 0);
  assert(falloff >= 0);
  int width = ftBitmap.width + 2 * falloff;
  int height = ftBitmap.rows + 2 * falloff;
  unsigned char * pixels = ftBitmap.buffer;
  int pitch = ftBitmap.pitch;
  result.resize(width * height);

  struct EDT_f
  {
    int result;
    EDT_f(int x, int i, int g_i) : result((x - i) * (x - i) + g_i * g_i) {}
    operator int() { return result; }
  };

  struct EDT_Sep
  {
    int result;
    EDT_Sep(int i, int u, int g_i, int g_u) : result((u*u - i*i + g_u*g_u - g_i*g_i) / (2 * (u - i))) {}
    operator int() { return result; }
  };

  // First stage
  const int inf = width + height;
  std::vector<int> g(width * height);
  int invertValue = (int)invert;

  for (int x = 0; x < width; x++)	
  {
    int pix = invertValue;

    if (!falloff)
    {
      int ind = x / 8;
      int shr = 7 - x % 8;
      unsigned char byte = pixels[ind];
      pix = ((byte >> shr) & 1) ^ invertValue;
    }

    if (pix)
      g[x] = 0;
    else
      g[x] = inf;

    // Scan 1
    for (int y = 1; y < height; y++)	
    {
      int pix = invertValue;

      if (x >= falloff && x < width - falloff && y >= falloff && y < height - falloff)
      {
        int ind = (y - falloff) * pitch + (x - falloff) / 8;
        int shr = 7 - (x - falloff) % 8;
        unsigned char byte = pixels[ind];
        pix = ((byte >> shr) & 1) ^ invertValue;
      }

      if (pix)
        g[x + y * width] = 0;
      else
        g[x + y * width] = 1 + g[x + (y - 1) * width];
    }

    // Scan 2
    for (int y = height - 2; y >= 0; y--)	
    {
      if (g[x + (y + 1) * width] < g[x + y * width])
        g[x + y * width] = 1 + g[x + (y + 1) * width];
    }
  }

  // Second stage
  std::vector<int> s(width);
  std::vector<int> t(width);
  int q = 0;
  int w;
  float maxDistance = 0.0f;

  for (int y = 0; y < height; y++)	
  {
    q = 0;
    s[0] = 0;
    t[0] = 0;

      // Scan 3
    for (int x = 1; x < width; x++)	
    {
      while (q >= 0 && EDT_f(t[q], s[q], g[s[q] + y * width]) > EDT_f(t[q], x, g[x + y * width]))
        q--;

      if (q < 0)	
      {
        q = 0;
        s[0] = x;
      }
      else	
      {
        w = 1 + EDT_Sep(s[q], x, g[s[q] + y * width], g[x + y * width]);

        if (w < width)	
        {
          q++;
          s[q] = x;
          t[q] = w;
        }
      }
    }

    // Scan 4
    for (int x = width - 1; x >= 0; x--)	
    {
      float distance = std::sqrtf((float)EDT_f(x, s[q], g[s[q] + y * width]));
      result[x + y * width] = distance;

      if (distance > maxDistance)
        maxDistance = distance;

      if (x == t[q])
        q--;
    }
  }

  return maxDistance;
}
