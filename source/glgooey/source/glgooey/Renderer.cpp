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

#include "glgooey/core/OpenGL.h"

#include "glgooey/Renderer.h"

#include "glgooey/services/interfaces/Image.h"

#include "glgooey/core/Vector2.h"
#include "glgooey/core/Rectangle.h"
#include "glgooey/core/StandardException.h"
#include "glgooey/core/Position.h"

#include "glgooey/WindowManager.h"
#include "glgooey/Color.h"

// **************************************************************************************************
namespace Gooey
{


UnregisterImageFunction* Image::unregisterImage_ = Renderer::unregisterImage;



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::drawLine(const Core::Vector2& start, const Core::Vector2& end)
{
    glBegin(GL_LINES);
    glVertex2f( start.x(), start.y() );
    glVertex2f( end.x(),   end.y()   );
    glEnd( );
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::drawRectangle(const Core::Vector2& bottomLeft, const Core::Vector2& topRight, bool filled)
{
    if(filled)
    {
        glBegin(GL_TRIANGLE_STRIP);
        glVertex2f( bottomLeft.x(), topRight.y()   );
        glVertex2f( bottomLeft.x(), bottomLeft.y() );
        glVertex2f( topRight.x(),   topRight.y()   );
        glVertex2f( topRight.x(),   bottomLeft.y() );
    }
    else
    {
        const float penWidth = lineWidth();
        const float halfPenWidth = penWidth * 0.5f;

        glBegin(GL_LINES);
        glVertex2f( bottomLeft.x(),  topRight.y() - halfPenWidth );     // top left -> top right
        glVertex2f( topRight.x(),    topRight.y() - halfPenWidth );

        glVertex2f( topRight.x() - halfPenWidth, topRight.y() - penWidth   ); // top right -> bottom right
        glVertex2f( topRight.x() - halfPenWidth, bottomLeft.y() + penWidth );

        glVertex2f( topRight.x(),   bottomLeft.y() + halfPenWidth ); // bottom right -> bottom left
        glVertex2f( bottomLeft.x(), bottomLeft.y() + halfPenWidth );

        glVertex2f( bottomLeft.x() + halfPenWidth, bottomLeft.y() + penWidth );     // bottom left -> top left
        glVertex2f( bottomLeft.x() + halfPenWidth, topRight.y() - penWidth   );
    }
    glEnd();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::drawCircleSegment(const Core::Vector2& center, float radius,
                       float t1, float t2, unsigned long numSegments, bool filled)
{
    const float radToDeg = 57.295779513082320876798154814105f;
    const float a1 = t1 * radToDeg + 90.0f;
    const float a2 = t2 * radToDeg + 90.0f;
    float xtra = 0.0f;
    if(!filled)
    {
        float lineWidth;
        glGetFloatv(GL_LINE_WIDTH, &lineWidth);
        xtra = lineWidth * 0.5f;
    }
    const float outerRadius = filled ? radius : radius + xtra;
    const float innerRadius = filled ? 0.0f : radius - xtra;

    const float cx = float(center.x());
    const float cy = float(center.y());

    glTranslatef(cx, cy, 0.0f);

    static GLUquadricObj* quadric = gluNewQuadric();
    gluPartialDisk(quadric, innerRadius, outerRadius, numSegments, 1, a1, a2-a1);

    glTranslatef(-cx, -cy, 0.0f);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::drawCircle(const Core::Vector2& center, float radius,
                       unsigned long numSegments, bool filled)
{
    const float pi = 3.1415926535897932384626433832795f;
    drawCircleSegment(center, radius, 0, 2.0f*pi, numSegments, filled);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::drawRoundedRectangle(const Core::Vector2& bottomLeft, const Core::Vector2& topRight,
                          float radius, bool filled)
{
    const float pi = 3.1415926535897932384626433832795f;

    // get the centers of the four rounded corners
    const float r = radius;
    const Core::Vector2 bottomLeftCenter  = bottomLeft + Core::Vector2(r, -r);
    const Core::Vector2 topRightCenter    = topRight + Core::Vector2(-r, r);
    const Core::Vector2 topLeftCenter     = Core::Vector2(bottomLeft.x() + r, topRight.y() + r);
    const Core::Vector2 bottomRightCenter = Core::Vector2(topRight.x() - r, bottomLeft.y() - r);

    // now draw the four rounded corners
    drawCircleSegment( topLeftCenter,     radius, pi*0.5f, pi,      7, filled );
    drawCircleSegment( bottomRightCenter, radius, pi*1.5f, pi*2.0f, 7, filled );
    drawCircleSegment( bottomLeftCenter,  radius, pi,      pi*1.5f, 7, filled );
    drawCircleSegment( topRightCenter,    radius, 0,       pi*0.5f, 7, filled );

    if(filled)
    {
        // fill the rest in with rectangles
        drawRectangle(topLeftCenter + Core::Vector2(-r, 0), bottomRightCenter + Core::Vector2(r, 0), filled);
        drawRectangle(topLeftCenter + Core::Vector2(0, -r), topRightCenter, filled);
        drawRectangle(bottomLeftCenter, bottomRightCenter + Core::Vector2(0, r), filled);
    }
    else
    {
        // fill the gaps in with lines
        drawLine(bottomLeftCenter  + Core::Vector2(-r,  0), topLeftCenter     + Core::Vector2(-r,  0));
        drawLine(bottomLeftCenter  + Core::Vector2( 0, -r), bottomRightCenter + Core::Vector2( 0, -r));
        drawLine(bottomRightCenter + Core::Vector2( r,  0), topRightCenter    + Core::Vector2( r,  0));
        drawLine(topLeftCenter     + Core::Vector2( 0,  r), topRightCenter    + Core::Vector2( 0,  r));
    }

}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::drawDroppedRectangle(const Core::Vector2& topLeft, const Core::Vector2& bottomRight,
                                 const Color& color1, const Color& color2)
{
    const float penWidth = lineWidth();
    const float halfPenWidth = penWidth * 0.5f;


    glBegin(GL_LINES);
        glColor4f(color1.r(), color1.g(), color1.b(), color1.a());
        glVertex2f( topLeft.x(),        bottomRight.y() - halfPenWidth );     // top left -> top right
        glVertex2f( bottomRight.x(),    bottomRight.y() - halfPenWidth );

        glVertex2f( bottomRight.x() - halfPenWidth, bottomRight.y() - penWidth   ); // top right -> bottom right
        glVertex2f( bottomRight.x() - halfPenWidth, topLeft.y() + penWidth       );

        glColor4f(color2.r(), color2.g(), color2.b(), color2.a());
        glVertex2f( bottomRight.x(), topLeft.y() + halfPenWidth ); // bottom right -> bottom left
        glVertex2f( topLeft.x(),     topLeft.y() + halfPenWidth );

        glVertex2f( topLeft.x() + halfPenWidth, topLeft.y() + penWidth       );     // bottom left -> top left
        glVertex2f( topLeft.x() + halfPenWidth, bottomRight.y() - penWidth   );
    glEnd();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::drawRaisedRectangle(const Core::Vector2& topLeft, const Core::Vector2& bottomRight,
                                const Color& color1, const Color& color2)
{
    drawDroppedRectangle(topLeft, bottomRight, color2, color1);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::enableClipRectangle(const Gooey::Core::Rectangle& rectangle)
{
    GLfloat vp[4];
    glGetFloatv(GL_VIEWPORT, vp);

    glEnable(GL_SCISSOR_TEST);
    glScissor(static_cast<int>(rectangle.topLeft().x()),
              static_cast<int>(vp[3] - vp[1] - rectangle.bottomLeft().y()),
              static_cast<int>(rectangle.size().x())+1,
              static_cast<int>(rectangle.size().y())+1);

}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::disableClipRectangle()
{
    glDisable(GL_SCISSOR_TEST);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::setCurrentColor(const Color& color)
{
    glColor4f(color.r(), color.g(), color.b(), color.a());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::setLineWidth(float lineWidthInPixels)
{
    glLineWidth(lineWidthInPixels);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
Renderer::maximumLineWidth()
{
    GLfloat ret[2];
    glGetFloatv(GL_LINE_WIDTH_RANGE, ret);
    return float(ret[1]);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
Renderer::lineWidth()
{
    GLfloat ret;
    glGetFloatv(GL_LINE_WIDTH, &ret);
    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::drawTriangle(const Core::Vector2& p1, const Core::Vector2& p2, const Core::Vector2& p3, bool filled)
{
    if(filled)
        glBegin(GL_TRIANGLES);
    else
        glBegin(GL_LINE_LOOP);

    glVertex2f(p1.x(), p1.y());
    glVertex2f(p2.x(), p2.y());
    glVertex2f(p3.x(), p3.y());

    glEnd();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::drawGradientFilledRectangle(const Core::Vector2& bottomLeft, const Core::Vector2& topRight,
            const Color& color1, const Color& color2, bool horizontal)
{
    glBegin(GL_TRIANGLE_STRIP);
    if(horizontal)
    {
        glColor4f(color1.r(), color1.g(), color1.b(), color1.a());
        glVertex2f( bottomLeft.x(), topRight.y()   );
        glVertex2f( bottomLeft.x(), bottomLeft.y() );
        glColor4f(color2.r(), color2.g(), color2.b(), color2.a());
        glVertex2f( topRight.x(),   topRight.y()   );
        glVertex2f( topRight.x(),   bottomLeft.y() );
    }
    else
    {
        glColor4f(color1.r(), color1.g(), color1.b(), color1.a());
        glVertex2f( bottomLeft.x(), topRight.y()   );
        glColor4f(color2.r(), color2.g(), color2.b(), color2.a());
        glVertex2f( bottomLeft.x(), bottomLeft.y() );
        glColor4f(color1.r(), color1.g(), color1.b(), color1.a());
        glVertex2f( topRight.x(),   topRight.y()   );
        glColor4f(color2.r(), color2.g(), color2.b(), color2.a());
        glVertex2f( topRight.x(),   bottomLeft.y() );
    }
    glEnd();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::drawImage(const Image& anImage, const Gooey::Core::Rectangle& anArea,
            bool isTiled, float minX, float minY, float maxX, float maxY)
{
    Renderer::enableTexturing(anImage);

    Core::Vector2 topLeft = anArea.topLeft();
    Core::Vector2 bottomRight = anArea.bottomRight();

//     if(isTiled)
//     {
//         maxX = float(anArea.width())  / float(anImage.width());
//         const float height = float(anArea.height()) / float(anImage.height());
//         maxY = ceilf(height) - 0.01f;
//         minY = maxY - height;
//     }

    glColor3f(1.0f,1.0f,1.0f);

    glBegin(GL_QUADS);
        glTexCoord2f(minX, minY);
        glVertex2f(topLeft.x(), bottomRight.y());
        glTexCoord2f(minX, maxY);
        glVertex2f(topLeft.x(), topLeft.y());
        glTexCoord2f(maxX, maxY);
        glVertex2f(bottomRight.x(), topLeft.y());
        glTexCoord2f(maxX, minY);
        glVertex2f(bottomRight.x(), bottomRight.y());
    glEnd();

    Renderer::disableTexturing();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::enableTexturing(const Image& anImage)
{
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if(anImage.handle() == 0) registerImage(anImage);

    ASSERT(anImage.handle() != 0);

    glBindTexture(GL_TEXTURE_2D, anImage.handle());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::disableTexturing()
{
    glPopAttrib();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::registerImage(const Image& anImage)
{
    GLuint glName = 0;
    glGenTextures(1, &glName);

    if(glName != 0)
    {
        glBindTexture(GL_TEXTURE_2D, glName);

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        ASSERT( (anImage.numberOfBitsPerPixel() == 24) || (anImage.numberOfBitsPerPixel() == 32) );
        const GLuint format = (anImage.numberOfBitsPerPixel() == 24) ? GL_RGB : GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, anImage.size().width(), anImage.size().height(), 0, format, GL_UNSIGNED_BYTE, anImage.data());

        anImage.setHandle(glName);
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::unregisterImage(size_t aHandle)
{
    const GLuint glName = static_cast<GLuint>(aHandle);
    glDeleteTextures(1, &glName);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::preRender()
{
    // Save the current state
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    // Enforce a standard packing mode
    glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_FALSE );
    glPixelStorei( GL_UNPACK_LSB_FIRST, GL_FALSE );
    glPixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
    glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    // set the orthographic projection
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    const Core::Vector2 applicationSize = WindowManager::instance().applicationSize();
    glOrtho( 0.0, applicationSize.x(), applicationSize.y(), 0.0, -10, 10 );

    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();


    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Renderer::postRender()
{
    // return to the original projection
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);


    // Restore the current state
    glPopAttrib();
    glPopClientAttrib();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// **************************************************************************************************
} //namespace Gooey
