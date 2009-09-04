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
#ifndef RENDERER__H
#define RENDERER__H


#include "mmgr/mmgr.h"


// *************************************************************************************************
//  Forward Declarations
namespace Gooey
{
    class Color;
    class Image;

    namespace Core
    {
        class Vector2;
        class Rectangle;
    }
}
// *************************************************************************************************



// *************************************************************************************************
namespace Gooey
{




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The interface between OpenGL and GLGooey
class Renderer
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! constructor
    Renderer() {}

    //! destructor
    virtual ~Renderer() {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! draw a line
    static void drawLine(const Core::Vector2& start, const Core::Vector2& end);

    //! \brief Draw a rectangle
    //!
    //! \note If the rectangle is filled the drawn rectangle is top-right \b exclusive,
    //! allowing filled rectangles to sit flush against one another without having to
    //! add in a one pixel offset. If the rectangle is not filled, then the
    //! drawn rectangle is top-right \b inclusive. For example:
    //! \code
    //!    drawRectangle(Vector2(10, 10), Vector2(20, 20), true);
    //!    drawRectangle(Vector2(20, 10), Vector2(30, 20), true);
    //! \endcode
    //! will draw two filled neighboring rectangles, that sit flush against one
    //! another with no overlap, whereas:
    //! \code
    //!    drawRectangle(Vector2(10, 10), Vector2(20, 20), false);
    //!    drawRectangle(Vector2(20, 10), Vector2(30, 20), false);
    //! \endcode
    //! will draw two unfilled neighboring rectangles which overlap at x = 20.
    static void drawRectangle(const Core::Vector2& bottomLeft, const Core::Vector2& topRight,
        bool filled);

    //! draws a filled rectangle with a color gradient
    static void drawGradientFilledRectangle(const Core::Vector2& bottomLeft, const Core::Vector2& topRight,
        const Color& color1, const Color& color2, bool horizontal);

    //! \brief Draws a segment of a circle.
    //!
    //! Draws a segment of a circle between t1 and t2 which are angles in radians
    //! between 0 and 2*pi. For instance t1 = 0 and t2 = pi*0.5 would draw the
    //! quarter circle occupying the top right quadrant
    static void drawCircleSegment(const Core::Vector2& center, float radius,
        float t1, float t2, unsigned long numSegments, bool filled);

    //! draw a circle (calls drawCircleSegment with t1 = 0 and t2 = 2*pi
    static void drawCircle(const Core::Vector2& center, float radius,
        unsigned long numSegments, bool filled);

    //! \brief draw a rectangle with rounded corners - the center of the top left circle
    //! segment is at topLeft + Core::Vector2(radius, -radius)
    static void drawRoundedRectangle(const Core::Vector2& topLeft, const Core::Vector2& bottomRight,
        float radius, bool filled);

    //! draw a rectangle that appears to be slightly sunken
    static void drawDroppedRectangle(const Core::Vector2& topLeft, const Core::Vector2& bottomRight,
        const Color& color1, const Color& color2);

    //! draw a rectangle that appears to be slightly raised
    static void drawRaisedRectangle(const Core::Vector2& topLeft, const Core::Vector2& bottomRight,
        const Color& color1, const Color& color2);

    //! \brief enables a clip rectangle, so that only pixels within that rectangle
    //! are drawn
    static void enableClipRectangle(const Core::Rectangle& rectangle);

    //! disables the clip rectangle
    static void disableClipRectangle();

    //! sets the current color to be used in subsequent drawing operations
    static void setCurrentColor(const Color& color);

    //! sets the line width to be used in subsequent drawing operations
    static void setLineWidth(float lineWidthInPixels);

    //! returns the largest line width supported by the OpenGL Implementation
    static float maximumLineWidth();

    //! Returns the currently set line width in pixels
    static float lineWidth();

    //! draws the triangle decribed by the three passed in points
    static void drawTriangle(const Core::Vector2& p1, const Core::Vector2& p2, const Core::Vector2& p3,
        bool filled);

    //! textures the specified rectangle with the passed in image
    //!
    //! \param anImage The image to be rendered
    //! \param rect The rectangle to draw the image in
    //! \param isTiled \c true - The image will be shown 1:1 and repeated if necessary,
    //!                \c false - The image will be stretched to fit in the passed in rectangle
    static void drawImage(const Image& anImage, const Core::Rectangle& rect, bool isTiled,
                float minX, float minY, float maxX, float maxY);

    //! Enables texturing
    static void enableTexturing(const Image& anImage);

    //! Disables texturing
    static void disableTexturing();

    //! registers an image for subsequent use with the renderer
    static void registerImage(const Image& anImage);

    //! \brief Unregisters the passed in image, freeing any resources it might have reserved within
    //! the renderer
    //! \see registerImage
    static void unregisterImage(size_t aHandle);

    //! Does whatever needs to be done before a render pass takes place
    static void preRender();

    //! Does whatever needs to be done before a render pass takes place
    static void postRender();
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// *************************************************************************************************
} // namespace Gooey

#endif
