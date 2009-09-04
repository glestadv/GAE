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
#include "mmgr/mmgr.h"

#include "glgooey/core/OpenGL.h"

#include "glfont/GLTextRenderer.h"
#include "glfont/TypeFace.h"

#include "glgooey/core/Rectangle.h"
#include "glgooey/core/StandardException.h"


// *************************************************************************************************
namespace Gooey
{






// *************************************************************************************************
namespace
{

class FontDetails
{
    std::string _name;
    int _pointSize;
public:
    FontDetails(const std::string& aName, int aPixelHeight) :
        _name(aName), _pointSize(aPixelHeight) {}
    FontDetails(const FontDetails& fd) { (*this) = fd; }
    FontDetails& operator = (const FontDetails& fd)
    { _name = fd._name; _pointSize = fd._pointSize; return *this; }
    bool operator == (const FontDetails& fd) const
    { return (_name == fd._name) && (_pointSize == fd._pointSize); }
    bool operator != (const FontDetails& fd) const
    { return !( (*this) == fd); }
    bool operator < (const FontDetails& fd) const
    {
        bool ret = false;
        if(_pointSize < fd._pointSize)
            ret = true;
        else
        {
            if(_pointSize == fd._pointSize) ret = (_name < fd._name);
        }
        return ret;
    }
};

std::map<FontDetails, Font::GLTextRenderer*> fontMap;

// *************************************************************************************************
} // anonymous namespace




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
TextRenderer*
TextRenderer::create(const std::string& aFileName, size_t aPointSize)
{
    FontDetails fontDetails(aFileName, aPointSize);
    std::map<FontDetails, Font::GLTextRenderer*>::iterator fontIterator = fontMap.find(fontDetails);
    if(fontIterator == fontMap.end())
    {
        fontMap[fontDetails] = new Font::GLTextRenderer(aFileName, aPointSize);
    }

    fontIterator = fontMap.find(fontDetails);
    ASSERT(fontIterator != fontMap.end());

    fontIterator->second->incrementReferenceCount();

    return fontIterator->second;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// *************************************************************************************************
namespace Font
{






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
GLTextRenderer::GLTextRenderer(const std::string& aFontName, size_t aPointSize) :
    TextRenderer(aFontName, aPointSize),
    typeFace_(new TypeFace(aFontName, aPointSize)),
    referenceCount_(0),
    fontName_(aFontName)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
GLTextRenderer::~GLTextRenderer()
{
    ASSERT(referenceCount_ == 0);

    FontDetails fontDetails(fontName_, typeFace_->pointSize());
    fontMap.erase(fontMap.find(fontDetails));

    delete typeFace_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
GLTextRenderer::release()
{
    decrementReferenceCount();
    if(referenceCount() == 0) delete this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
GLTextRenderer::incrementReferenceCount()
{
    ++referenceCount_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
GLTextRenderer::decrementReferenceCount()
{
    ASSERT(referenceCount_ != 0);
    --referenceCount_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
GLTextRenderer::render(const std::string& aString, const Core::Rectangle& aRectangle, bool isMultiLine,
        Core::Alignment::Horizontal aHorizontalAlignment, Core::Alignment::Vertical aVerticalAlignment)
{
    if(typeFace_ != 0)
    {
        using namespace Core;
        if(isMultiLine)
        {
            renderMultiline(aString, aRectangle, aHorizontalAlignment);
        }
        else
        {
            Core::Vector2 position(
                penX(aString, aRectangle, aHorizontalAlignment),
                baseLineY(aRectangle, aVerticalAlignment)
                );
            typeFace_->render(aString, position);
        }
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
GLTextRenderer::width(const std::string& aString) const
{
    return typeFace_->width(aString);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
GLTextRenderer::lineHeight() const
{
    return typeFace_->lineHeight();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
size_t
GLTextRenderer::hitCharacterIndex(const std::string& aString, float anOffset) const
{
    return typeFace_->hitCharacterIndex(aString, anOffset);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<









// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
GLTextRenderer::baseLineY(const Core::Rectangle& aRectangle, Core::Alignment::Vertical anAlignment) const
{
    float ret = 0.0f;
    if(typeFace_ != 0)
    {
        switch(anAlignment)
        {
            case Core::Alignment::top:
            {
                ret = aRectangle.topLeft().y() + typeFace_->ascent();
            }
            break;

            case Core::Alignment::vCenter:
            {
                const float spaceAboveText = floorf((aRectangle.height() - typeFace_->lineHeight()) * 0.5f);
                ret = aRectangle.topLeft().y() + spaceAboveText + typeFace_->ascent();
            }
            break;

            case Core::Alignment::bottom:
            {
                ret = aRectangle.bottomLeft().y() - typeFace_->descent();
            }
            break;
        }
    }
    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
GLTextRenderer::penX(const std::string& aString, const Core::Rectangle& aRectangle,
        Core::Alignment::Horizontal anAlignment) const
{
    float ret = 0.0f;
    if(typeFace_ != 0)
    {
        const float width = typeFace_->width(aString);
        switch(anAlignment)
        {
            case Core::Alignment::left:
            {
                ret = aRectangle.topLeft().x();
            }
            break;

            case Core::Alignment::hCenter:
            {
                ret = floorf(aRectangle.topLeft().x() - (width * 0.5f) + (aRectangle.width() * 0.5f));
            }
            break;

            case Core::Alignment::right:
            {
                ret = aRectangle.bottomRight().x() - width;
            }
            break;
        }
    }
    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
GLTextRenderer::renderMultiline(const std::string& aString, const Core::Rectangle& aRectangle,
        Core::Alignment::Horizontal aHorizontalAlignment)
{
    std::string txt = aString;
    const float availableWidth = aRectangle.width() - 4.0f;

    float y = baseLineY(aRectangle, Core::Alignment::top);
    while(txt.length() > 0)
    {
        // find out where the text is cut off
        size_t pos = hitCharacterIndex(txt, availableWidth);

        if(pos == std::string::npos) pos = txt.size();

        if(pos > 0)
        {
            // make the line the string up to the cut off position
            std::string line = txt.substr(0, pos);
            int p = int(pos);
            // check to see if the cut off position is inside the string and not at the end
            if(pos < txt.length())
            {
                //  if we've cut the string at a space, then the cut is fine where
                // it is. If not, we'll have to find the last space in the line
                // and cut there
                if( (txt[0] != ' ') && (*line.rbegin() != ' ') )
                {
                    std::string::size_type cutPos = line.find_last_of(" ");
                    if(cutPos != std::string::npos)
                    {
                        line = line.substr(0, cutPos);
                        p = int(cutPos);
                    }
                }
            }
            // trim whitespace off the start and end of the line
            line.erase(0, line.find_first_not_of(" "));
            line.erase(line.find_last_not_of(" ")+1, line.length());

            typeFace_->render(line, Core::Vector2(penX(line, aRectangle, aHorizontalAlignment), y));

            // update txt to be whatever remains after taking the line off the front
            txt = txt.substr(p, txt.length());
        }
        else
        {
            txt = "";
        }

        y += typeFace_->lineHeight();
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// *************************************************************************************************
} // namespace Font

// *************************************************************************************************
} // namespace Gooey

