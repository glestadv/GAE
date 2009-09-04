// __________________________________________________________________________________________________
//    GLGooey Graphical User Interface for OpenGL
//    Copyright (c) 2004 Niel Waldren
//
// This software is provided 'as-is', without any express or implied warranty. In no event will
// the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//     1. The origin of this software must not be misrepresented; you must not claim that you
//        wrote the original software. If you use this software in a product, an acknowledgment
//        in the product documentation would be appreciated but is not required.
//
//     2. Altered source versions must be plainly marked as such, and must not be misrepresented
//        as being the original software.
//
//     3. This notice may not be removed or altered from any source distribution.
//
// __________________________________________________________________________________________________

#include "mmgr/nommgr.h"
#include <cstddef>
#include <vector>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include "glgooey/core/OpenGL.h"

#include "mmgr/mmgr.h"

#include "glfont/TypeFace.h"
#include "glfont/FreeType.h"

#include "glgooey/core/StandardException.h"
#include "glgooey/core/Vector2.h"
#include "glgooey/core/Position.h"



// *************************************************************************************************
namespace Gooey
{

// *************************************************************************************************
namespace Font
{


// *************************************************************************************************
//  Data would be hidden inside an anonymous namespace, but msvc 6 can't handle it
//namespace 
//{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Information on a cached glyph
struct CacheEntry
{
    size_t        textureIndex_;   //!< Index into the TypeFace's texture array
    Core::Vector2 topLeftUV_;      //!< Texcoords for the top left corner of the glyph
    Core::Vector2 bottomRightUV_;  //!< Texcoords for the bottom right corner of the glyph
    Core::Size    renderSize_;     //!< The size of the glyph in pixels on the render target
    Core::Vector2 advance_;        //!< The character advance for the cached glyph
    Core::Vector2 bitmapPosition_; //!< The offset from the pen to the top left corner of the bitmap
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Information on a cache texture
struct TextureInfo
{
    typedef GLuint Handle; //!< typedef for a texture handle for an OpenGL texture
    Handle     handle_;    //!< The OpenGL texture handle
    Core::Size size_;      //!< The size of the texture in texels
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
typedef std::vector<TextureInfo>      Textures;       //!< An array of textures
typedef std::map<FT_UInt, CacheEntry> CharacterCache; //!< The cache maps glyphs to cache entries
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Implementation data for the TypeFace class
struct Data
{
    //! Constructor
    Data(size_t aPointSize, size_t aResolution) :
        pointSize_(aPointSize), resolution_(aResolution) {}

    size_t         pointSize_;      //!< The size of the type face in points
    size_t         resolution_;     //!< The resolution of the type face in dpi
    FT_Face        face_;           //!< A handle to the FreeType face object
    CharacterCache characterCache_; //!< The glyph cache
    Core::Position offset_;         //!< The top left corner of the next glyph to be cached
    Textures       textures_;       //!< The texture sizes and handles obtained from OpenGL
    bool           hasKerning_;     //!< Set to true if the type face supports kerning
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const size_t glyphPadding = 1;     //!< Glyphs have padding around them to prevent filtering issues
const float OneOver64 = 0.015625f; //!< A frequently used constant
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<









// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Returns the value of the next highest power of 2 above \c aValue
size_t
nextHighestPowerOf2(size_t aValue)
{
    size_t v = aValue - 1;
    for(int i = 16; i > 0; i >>= 1) v |= (v >> i);
    return (v + 1);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Returns the maximum texture size permitted by the OpenGL implementation
size_t
maximumTextureSize()
{
    GLint maxSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    if(maxSize > 512) maxSize = 512;
    return static_cast<size_t>(maxSize);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! returns the width to use for a cache texture
size_t
cacheTextureWidth(size_t aGlyphWidth, size_t numGlyphsToCache)
{
    size_t width = nextHighestPowerOf2( ( (aGlyphWidth + glyphPadding) * numGlyphsToCache) + glyphPadding );
    if(width > maximumTextureSize()) width = maximumTextureSize();
    return width;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Returns the size to use for a cache texture
Core::Size
cacheTextureSize(const Core::Size& aGlyphSize, size_t numGlyphsToCache)
{
    size_t textureWidth = cacheTextureWidth(aGlyphSize.width(), numGlyphsToCache);

    const size_t numGlyphsInRow = (textureWidth - (glyphPadding * 2)) / aGlyphSize.width();
    const size_t requiredHeight = ((numGlyphsToCache / numGlyphsInRow) + 1) * aGlyphSize.height();
    size_t textureHeight = nextHighestPowerOf2(requiredHeight);
    if(textureHeight > maximumTextureSize()) textureHeight = maximumTextureSize();

    return Core::Size(textureWidth, textureHeight);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Pushes the pixel storage client attributes and sets them to a standard packing mode
void
setPixelStorage()
{
    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

    // Enforce a standard packing mode
    glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_FALSE );
    glPixelStorei( GL_UNPACK_LSB_FIRST, GL_FALSE );
    glPixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
    glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Pops the pixel storage client attributes off the stack
void
restorePixelStorage()
{
    glPopClientAttrib();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Creates a texture for caching pre-rendered glyphs
TextureInfo
createCacheTexture(const Core::Size& aGlyphSize, size_t numGlyphsToCache)
{
    setPixelStorage();
    
    TextureInfo info;
    glGenTextures(1, &info.handle_);

    glBindTexture(GL_TEXTURE_2D, info.handle_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    info.size_ = cacheTextureSize(aGlyphSize, numGlyphsToCache);

    unsigned char* textureData = new unsigned char[info.size_.area()];
    memset(textureData, 0, info.size_.area());

    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, info.size_.width(), info.size_.height(), 0, GL_ALPHA,
                 GL_UNSIGNED_BYTE, textureData);

    restorePixelStorage();

    delete[] textureData;

    return info;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Blits the glyph represented by the passed in bitmap to the texture described by
//! the passed in texture info.
void
blitGlyphToTexture(FT_Bitmap* aBitmap, const TextureInfo& aTextureInfo,
                   const Core::Position& anOffset, const Core::Size& aGlyphSize)
{
    setPixelStorage();

    const int pitch = aBitmap->pitch;
    unsigned char* data = new unsigned char[aGlyphSize.area()];

    for(size_t y = 0; y < aGlyphSize.height(); ++y)
    {
        memcpy(data + (y * aGlyphSize.width()), aBitmap->buffer + (y * pitch), aGlyphSize.width());
    }

    glBindTexture(GL_TEXTURE_2D, aTextureInfo.handle_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, anOffset.x(), anOffset.y(),
        aGlyphSize.width(), aGlyphSize.height(), GL_ALPHA, GL_UNSIGNED_BYTE, data);

    delete[] data;

    restorePixelStorage();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Functor for destroying OpenGL textures
struct DestroyTexture
{
    //! The function that does the destroying
    void
    operator() (const TextureInfo& info)
    {
        glDeleteTextures(1, &info.handle_);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





//} // namespace
// *************************************************************************************************











// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
TypeFace::TypeFace(const std::string& aFaceName, size_t aPointSize, size_t aResolution) :
    data_(new Data(aPointSize, aResolution))
{
    FreeType::registerUser();

    ASSERT(FT_New_Face(FreeType::library(), aFaceName.c_str(), 0, &data_->face_) == 0);

    ASSERT(FT_IS_SCALABLE(data_->face_));
    ASSERT(FT_IS_SFNT(data_->face_));

    if( !data_->face_->charmap) FT_Set_Charmap(data_->face_, data_->face_->charmaps[0]);

    ASSERT(FT_Set_Char_Size(data_->face_, 0L, static_cast<FT_F26Dot6>(aPointSize << 6), aResolution, aResolution) == 0);

    data_->hasKerning_ = (FT_HAS_KERNING(data_->face_) != 0);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
TypeFace::~TypeFace()
{
    flushCache();
    FT_Done_Face(data_->face_);
    FreeType::unregisterUser();
    delete data_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
size_t
TypeFace::pointSize() const
{
    return data_->pointSize_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::setPointSize(size_t aPointSize)
{
    if(data_->pointSize_ != aPointSize)
    {
        data_->pointSize_ = aPointSize;
        const FT_F26Dot6 sz = static_cast<FT_F26Dot6>(aPointSize << 6);
        ASSERT(FT_Set_Char_Size(data_->face_, sz, sz, data_->resolution_, data_->resolution_) == 0);
        flushCache();
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
size_t
TypeFace::resolution() const
{
    return data_->resolution_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::setResolution(size_t aResolution)
{
    if(data_->resolution_ != aResolution)
    {
        data_->resolution_ = aResolution;
        flushCache();
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float TypeFace::ascent() const
{
    return float(data_->face_->size->metrics.y_ppem) - descent();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float TypeFace::descent() const
{
    return static_cast<float>(-data_->face_->size->metrics.descender) * OneOver64;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float TypeFace::lineHeight() const
{
    return (ascent() + descent());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::flushCache()
{
    data_->characterCache_.clear();
    std::for_each(data_->textures_.begin(), data_->textures_.end(), DestroyTexture());
    data_->textures_.clear();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::addNewTexture(const Core::Size& aGlyphSize)
{
    const size_t numUncachedGlyphs = data_->face_->num_glyphs - data_->characterCache_.size();
    data_->textures_.push_back(createCacheTexture(aGlyphSize, numUncachedGlyphs));
    data_->offset_.set(glyphPadding, glyphPadding);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::render(const std::string& aString, const Core::Vector2& aPosition)
{
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_HINT_BIT | GL_LINE_BIT | GL_PIXEL_MODE_BIT);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();
    glTranslatef(aPosition.x(), aPosition.y(), 0.0f);

    renderGlyphs(aString);

    glPopMatrix();

    glPopAttrib();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
TypeFace::width(const std::string& aString)
{
    std::string::const_iterator it = aString.begin();
    float ret = 0;
    size_t leftChar = 0;
    while(it != aString.end())
    {
        size_t rightChar = FT_Get_Char_Index(data_->face_, *it);
        ret += kerning(leftChar, rightChar).x();
        ret += cachedGlyph(rightChar).advance_.x();
        leftChar = rightChar;
        ++it;
    }
    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
size_t TypeFace::hitCharacterIndex(const std::string& aString, float anOffset)
{
    bool found = false;

    size_t characterIndex = 0;
    if(anOffset >= 0.0f)
    {
        std::string::const_iterator it = aString.begin();
        float oldOffset = 0;
        size_t leftChar = 0;
        while(it != aString.end())
        {
            float newOffset = oldOffset;
            size_t rightChar = FT_Get_Char_Index(data_->face_, *it);
            newOffset += kerning(leftChar, rightChar).x();
            newOffset += cachedGlyph(rightChar).advance_.x();

            if( (newOffset >= anOffset) && (oldOffset <= anOffset) )
            {
                found = true;
                break;
            }

            leftChar = rightChar;
            oldOffset = newOffset;
            ++characterIndex;
            ++it;
        }
    }

    if(!found) characterIndex = std::string::npos;

    return characterIndex;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::addCacheEntry(FT_GlyphRec_* aGlyph, FT_BitmapGlyphRec_* aBitmapGlyph, size_t aGlyphIndex, const Core::Size& aGlyphSize)
{
    const TextureInfo& textureInfo = *(data_->textures_.rbegin());

    const Core::Position bottomRight = data_->offset_ + Core::Position(aGlyphSize);

    const float widthF  = static_cast<float>(textureInfo.size_.width());
    const float heightF = static_cast<float>(textureInfo.size_.height());

    CacheEntry entry;
    entry.topLeftUV_.setX(static_cast<float>(data_->offset_.x()) / widthF);
    entry.topLeftUV_.setY(static_cast<float>(data_->offset_.y()) / heightF);
    entry.bottomRightUV_.setX(static_cast<float>(bottomRight.x()) / widthF);
    entry.bottomRightUV_.setY(static_cast<float>(bottomRight.y()) / heightF);
    entry.textureIndex_ = data_->textures_.size() - 1;
    entry.renderSize_ = aGlyphSize;
    entry.advance_ = Core::Vector2(float(aGlyph->advance.x >> 16), float(aGlyph->advance.y >> 16));
    entry.bitmapPosition_ = Core::Vector2(float(aBitmapGlyph->left), float(-aBitmapGlyph->top));

    data_->characterCache_[aGlyphIndex] = entry;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Size
TypeFace::maximumGlyphSize() const
{
    FT_BBox& bounds = data_->face_->bbox;
    float maxGlyphWidthF = static_cast<float>(bounds.xMax - bounds.xMin);
    maxGlyphWidthF *= ( (float)data_->face_->size->metrics.x_ppem / (float)data_->face_->units_per_EM);

    float maxGlyphHeightF = static_cast<float>(bounds.yMax - bounds.yMin);
    maxGlyphHeightF *= ( (float)data_->face_->size->metrics.y_ppem / (float)data_->face_->units_per_EM);

    return Core::Size(static_cast<size_t>(maxGlyphWidthF), static_cast<size_t>(maxGlyphHeightF));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::ensureTextureIsAvailable()
{
    const Core::Size maxGlyphSize = maximumGlyphSize();

    if(data_->textures_.size() == 0)
    {
        addNewTexture(maxGlyphSize);
    }
    else
    {
        const TextureInfo& textureInfo = *(data_->textures_.rbegin());
        if(data_->offset_.x() > int(textureInfo.size_.width()) - int(maxGlyphSize.width()) - int(glyphPadding))
        {
            data_->offset_.set(glyphPadding, data_->offset_.y() + maxGlyphSize.height());

            if(data_->offset_.y() > int(textureInfo.size_.height()) - int(maxGlyphSize.height()) - int(glyphPadding))
            {
                addNewTexture(maxGlyphSize);
            }
        }
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::cacheGlyph(size_t aGlyphIndex)
{
    ensureTextureIsAvailable();

    const FT_Error loadError = FT_Load_Glyph(data_->face_, aGlyphIndex, FT_LOAD_NO_HINTING);
    ASSERT(loadError == 0);
    if(loadError == 0)
    {
        FT_Glyph glyph;
        const FT_Error getError = FT_Get_Glyph( data_->face_->glyph, &glyph);
        if(getError == 0)
        {
            const FT_Error bitmapError = FT_Glyph_To_Bitmap( &glyph, FT_RENDER_MODE_NORMAL, 0, 1);
            if( (bitmapError == 0) && (glyph->format == FT_GLYPH_FORMAT_BITMAP) )
            {
                FT_BitmapGlyph bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
                FT_Bitmap* bitmap = &bitmapGlyph->bitmap;

                const Core::Size glyphSize(bitmap->width, bitmap->rows);

                if(glyphSize.area() > 0)
                    blitGlyphToTexture(bitmap, *(data_->textures_.rbegin()), data_->offset_, glyphSize);

                addCacheEntry(glyph, bitmapGlyph, aGlyphIndex, glyphSize);
                data_->offset_.setX(data_->offset_.x() + glyphSize.width() + glyphPadding);
            }
        }
        FT_Done_Glyph(glyph);
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
CacheEntry&
TypeFace::cachedGlyph(size_t aGlyphIndex)
{
    CharacterCache::iterator cacheIterator = data_->characterCache_.find(aGlyphIndex);
    if(cacheIterator == data_->characterCache_.end())
    {
        cacheGlyph(aGlyphIndex);
        cacheIterator = data_->characterCache_.find(aGlyphIndex);
        ASSERT(cacheIterator != data_->characterCache_.end());
    }
    return cacheIterator->second;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<









// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Vector2
TypeFace::renderGlyph(size_t aGlyphIndex, const Core::Vector2& aPosition)
{
    CacheEntry& entry = cachedGlyph(aGlyphIndex);

    bindTexture(entry);

    const Core::Vector2 position = aPosition + entry.bitmapPosition_;

    const Core::Vector2 topLeftUV     = entry.topLeftUV_;
    const Core::Vector2 bottomRightUV = entry.bottomRightUV_;
    const float glyphWidth  = static_cast<float>(entry.renderSize_.width());
    const float glyphHeight = static_cast<float>(entry.renderSize_.height());

    glBegin(GL_QUADS);
    {
        glTexCoord2f( topLeftUV.x(), topLeftUV.y() );
        glVertex2f( position.x(), position.y() );

        glTexCoord2f( bottomRightUV.x(), topLeftUV.y() );
        glVertex2f( position.x() + glyphWidth, position.y() );

        glTexCoord2f( bottomRightUV.x(), bottomRightUV.y() );
        glVertex2f( position.x() + glyphWidth, position.y() + glyphHeight );

        glTexCoord2f( topLeftUV.x(), bottomRightUV.y() );
        glVertex2f( position.x(), position.y() + glyphHeight );
    }
    glEnd();

    return entry.advance_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Vector2
TypeFace::kerning(size_t leftGlyphIndex, size_t rightGlyphIndex) const
{
    Core::Vector2 ret;
    if(data_->hasKerning_ && (leftGlyphIndex > 0) && (rightGlyphIndex > 0))
    {
        FT_Vector kerningVector;
        FT_Error ftError = FT_Get_Kerning(data_->face_, leftGlyphIndex, rightGlyphIndex, FT_KERNING_DEFAULT, &kerningVector);
        if(!ftError)
        {
            ret = Core::Vector2(static_cast<float>(kerningVector.x),
                                static_cast<float>(kerningVector.y)) * OneOver64;
        }
    }
    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::bindTexture(const CacheEntry& aCacheEntry) const
{
    ASSERT(aCacheEntry.textureIndex_ < data_->textures_.size());
    const TextureInfo& info = data_->textures_[aCacheEntry.textureIndex_];

    GLint currentTextureID = -1;
    glGetIntegerv(GL_TEXTURE_2D_BINDING_EXT, &currentTextureID);
    if(static_cast<GLuint>(currentTextureID) != info.handle_)
    {
        glBindTexture(GL_TEXTURE_2D, info.handle_);
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TypeFace::renderGlyphs(const std::string& aString)
{
    Core::Vector2 pos(0.0f, 0.0f);
    size_t leftChar = 0;
    std::string::const_iterator it = aString.begin();
    while(it != aString.end())
    {
        size_t rightChar = FT_Get_Char_Index(data_->face_, *it);
        pos += kerning(leftChar, rightChar);
        pos += renderGlyph(rightChar, pos);
        leftChar = rightChar;
        ++it;
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// *************************************************************************************************
} // namespace Font

// *************************************************************************************************
} // namespace Gooey

