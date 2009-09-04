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
#ifndef __GOOEY_GL_TEXT_RENDERER_H__
#define __GOOEY_GL_TEXT_RENDERER_H__

#include "mmgr/nommgr.h"
#include <cstddef>
#include "mmgr/mmgr.h"

#include "glgooey/services/interfaces/TextRenderer.h"



// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    namespace Font
    {
        class TypeFace;
    }
}
// *************************************************************************************************




// *************************************************************************************************
namespace Gooey
{


// *************************************************************************************************
namespace Font
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Uses OpenGL to render text
class GLTextRenderer : public TextRenderer
{
    // ----------------------------------------------------------------------------------------------
    //  Xstruction
    // ----------------------------------------------------------------------------------------------
protected:
    //! \brief Construction requires the font to use and a reference to the renderer
    GLTextRenderer(const std::string& aFontName, size_t aPointSize);

    //! Destructor
    virtual ~GLTextRenderer();



    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    // Commented in base class
    virtual void render(const std::string& aString, const Core::Rectangle& aRectangle, bool isMultiLine,
        Core::Alignment::Horizontal aHorizontalAlignment, Core::Alignment::Vertical aVerticalAlignment);
    virtual float width(const std::string& aString) const;
    virtual float lineHeight() const;
    virtual size_t hitCharacterIndex(const std::string& aString, float anOffset) const;

    //! Decrements the reference count destroying if necessary - pair this with calls to TextRenderer::create
    virtual void release();

    //! Returns the current value of the reference count
    size_t referenceCount() const { return referenceCount_; }

    //! Increments the reference count
    void incrementReferenceCount();

    //! Decrements the reference count
    void decrementReferenceCount();


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    //! \brief Returns the y coordinate of the base line of a single line of text within the passed
    //! in rectangle if it is to be rendered with the passed in alignment
    float baseLineY(const Core::Rectangle& aRectangle, Core::Alignment::Vertical anAlignment) const;

    //! \brief Returns the initial x coordinate of the pen drawing a single line of text within the passed
    //! in rectangle if it is to be rendered with the passed in alignment
    float penX(const std::string& aString, const Core::Rectangle& aRectangle,
        Core::Alignment::Horizontal anAlignment) const;

    //! Renders multiline text in the passed in rectangle
    void renderMultiline(const std::string& aString, const Core::Rectangle& aRectangle,
        Core::Alignment::Horizontal aHorizontalAlignment);


    TypeFace*  typeFace_; //!< The type face object that is capable of drawing text

    size_t referenceCount_;     //!< The GL text renderers are reference counted

    std::string fontName_; //!< The name of the font

    friend class TextRenderer;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Font

// *************************************************************************************************
} // namespace Gooey



#endif
