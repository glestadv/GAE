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

#include "glgooey/SolidSurfaceDesc.h"

#include "glgooey/core/Rectangle.h"
#include "glgooey/core/Position.h"
#include "glgooey/core/XMLOutputArchive.h"
#include "glgooey/core/XMLInputArchive.h"

#include "glgooey/services/interfaces/TextRenderer.h"


// **************************************************************************************************
namespace Gooey
{

REGISTER_CLASS(SolidSurfaceDesc);
REGISTER_CLASS(GradientSurfaceDesc);
REGISTER_CLASS(RectangleDesc);
REGISTER_CLASS(TextDesc);
REGISTER_CLASS(DefaultCheckDesc);
REGISTER_CLASS(DefaultFrameDesc);
REGISTER_CLASS(BitmapDesc);
REGISTER_CLASS(Bitmap1x3Desc);




// **************************************************************************************************
//                                   SolidSurfaceDesc
// **************************************************************************************************



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
SolidSurfaceDesc::draw(const Core::Rectangle& area) const
{
    Renderer::setCurrentColor(color_);
    Renderer::drawRectangle(area.topLeft(), area.bottomRight(), true);
    Renderer::setCurrentColor(Color(1, 1, 1, 1));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// **************************************************************************************************
//                                   GradientSurfaceDesc
// **************************************************************************************************




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
GradientSurfaceDesc::draw(const Core::Rectangle& area) const
{
    Renderer::drawGradientFilledRectangle(area.topLeft(), area.bottomRight(),
        color1_, color2_, orientation_ == horizontal);
    Renderer::setCurrentColor(Color(1, 1, 1, 1));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// **************************************************************************************************
//                                   RectangleDesc
// **************************************************************************************************


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
RectangleDesc::draw(const Core::Rectangle& area) const
{
    Renderer::setLineWidth(lineWidth_);
    if(style_ == dropped)
        Renderer::drawDroppedRectangle(area.topLeft(), area.bottomRight(),
            color1_, color2_);
    else if(style_ == raised)
        Renderer::drawRaisedRectangle(area.topLeft(), area.bottomRight(),
            color1_, color2_);
    else
    {
        Renderer::setCurrentColor(color1_);
        Renderer::drawRectangle(area.topLeft(), area.bottomRight(), false);
    }
    Renderer::setLineWidth(1.0f);
    Renderer::setCurrentColor(Color(1, 1, 1, 1));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// **************************************************************************************************
//                                   TextDesc
// **************************************************************************************************




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
TextDesc::TextDesc() :
    fileName_(WindowManager::defaultFont()), pixelHeight_(14), xOffset_(0.0f),
    yOffset_(0.0f), margin_(3.0f), color_(Color(1, 1, 1, 0.75f)),
    isMultiLine_(false),
    horizontalAlignment_(Core::Alignment::hCenter),
    verticalAlignment_(Core::Alignment::vCenter)
{
    textRenderer_ = TextRenderer::create(fileName_, pixelHeight_);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
TextDesc::TextDesc(const std::string& aName, int aPixelHeight, float anXOffset, float aYOffset, float aMargin, const Color& aColor) :
    fileName_(aName), pixelHeight_(aPixelHeight), xOffset_(anXOffset),
    yOffset_(aYOffset), margin_(aMargin), color_(aColor),
    isMultiLine_(false),
    horizontalAlignment_(Core::Alignment::hCenter),
    verticalAlignment_(Core::Alignment::vCenter)
{
    textRenderer_ = TextRenderer::create(fileName_, pixelHeight_);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
TextDesc::~TextDesc()
{
    textRenderer_->release();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TextDesc::reloadTextRenderer()
{
    if(textRenderer_ != 0) textRenderer_->release();
    textRenderer_ = TextRenderer::create(fileName_, pixelHeight_);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
TextDesc::draw(const Core::Rectangle& anArea) const
{
    if(text_ != "")
    {
        Renderer::setCurrentColor(color_);

        Core::Rectangle rectangle = anArea;
        rectangle.expandBy(-margin_);
        rectangle.translateBy(Core::Vector2(xOffset_, yOffset_));

        rectangle.snap();

        const bool isMultiline = isMultiLine_ && textSize().x() >= rectangle.width();
        textRenderer_->render(text_, rectangle, isMultiline, horizontalAlignment_, verticalAlignment_);
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Vector2
TextDesc::textSize() const
{
    Core::Vector2 Result(textRenderer_->width(text_), textRenderer_->lineHeight());

    return Result;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
TextDesc::lineHeight() const
{
    const float height = textRenderer_->lineHeight();

    return height;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
size_t
TextDesc::hitCharacterIndex(const std::string& aString, float anOffset) const
{
    const size_t index = textRenderer_->hitCharacterIndex(aString, anOffset);

    return index;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// **************************************************************************************************
//                                   DefaultCheckDesc
// **************************************************************************************************



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
DefaultCheckDesc::draw(const Core::Rectangle& area) const
{
    // draw the square to be checked
    Renderer::setCurrentColor(color_);
    const float edgeLength = area.size().y();
    const Core::Vector2 squareTopLeft = area.topLeft();
    Renderer::drawRectangle(squareTopLeft, squareTopLeft + Core::Vector2(edgeLength, edgeLength), false);

    GLfloat lineWidth = 1.0f;
    glGetFloatv(GL_LINE_WIDTH, &lineWidth);

    GLboolean isLineSmoothEnabled = false;
    glGetBooleanv(GL_LINE_SMOOTH, &isLineSmoothEnabled);

    glLineWidth(2.0f);
    glEnable(GL_LINE_SMOOTH);
    Renderer::drawLine(squareTopLeft + Core::Vector2(3.0f, 3.0f),  squareTopLeft + Core::Vector2(edgeLength-3.0f, edgeLength-3.0f));
    Renderer::drawLine(squareTopLeft + Core::Vector2(edgeLength-3.0f, 3.0f), squareTopLeft + Core::Vector2(3.0f, edgeLength-3.0f));

    if(lineWidth != 2.0f) glLineWidth(lineWidth);
    if(!isLineSmoothEnabled) glDisable(GL_LINE_SMOOTH);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// **************************************************************************************************
//                                   DefaultFrameDesc
// **************************************************************************************************




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
DefaultFrameDesc::draw(const Core::Rectangle& area) const
{
    // we need pi for the rounded corners
    const float pi = 3.1415926535897932384626433832795f;
    Renderer::setLineWidth(float(borderLineWidth_));
    float w = area.size().x();
    float r = w * 0.5f;

    const int numSegmentsInArc = 7;

    switch(section_)
    {
    case right: // Right
        Renderer::setCurrentColor(backgroundColor_);
        Renderer::drawRectangle(area.bottomLeft(), area.topRight(), true);
        Renderer::setCurrentColor(foregroundColor_);
        Renderer::drawLine(area.bottomLeft() + Core::Vector2(r, 0.0f),  area.topLeft() + Core::Vector2(r, 0.0f));
        break;
    case topRight: // TopRight
        Renderer::setCurrentColor(backgroundColor_);
        Renderer::drawCircleSegment(area.bottomLeft(), w, 0.0f, pi*0.5f, numSegmentsInArc, true);
        Renderer::setCurrentColor(foregroundColor_);
        Renderer::drawCircleSegment(area.bottomLeft(), r, 0.0f, pi*0.5f, numSegmentsInArc, false);
        break;
    case top: // Top
        w = area.size().y();
        r = w * 0.5f;
        Renderer::setCurrentColor(backgroundColor_);
        Renderer::drawRectangle(area.bottomLeft(), area.topRight(), true);
        Renderer::setCurrentColor(foregroundColor_);
        Renderer::drawLine(area.topLeft() + Core::Vector2(0.0f, r),  area.topRight() + Core::Vector2(0.0f,r));
        break;
    case topLeft: // TopLeft
        Renderer::setCurrentColor(backgroundColor_);
        Renderer::drawCircleSegment(area.bottomRight(), w, pi*0.5f, pi, numSegmentsInArc, true);
        Renderer::setCurrentColor(foregroundColor_);
        Renderer::drawCircleSegment(area.bottomRight(), r, pi*0.5f, pi, numSegmentsInArc, false);
        break;
    case left: // Left
        Renderer::setCurrentColor(backgroundColor_);
        Renderer::drawRectangle(area.bottomLeft(), area.topRight(), true);
        Renderer::setCurrentColor(foregroundColor_);
        Renderer::drawLine(area.bottomLeft() + Core::Vector2(r, 0.0f),  area.topLeft() + Core::Vector2(r, 0.0f));
        break;
    case bottomLeft: // BottomLeft
        Renderer::setCurrentColor(backgroundColor_);
        Renderer::drawCircleSegment(area.topRight(), w, pi, pi*1.5f, numSegmentsInArc, true);
        Renderer::setCurrentColor(foregroundColor_);
        Renderer::drawCircleSegment(area.topRight(), r, pi, pi*1.5f, numSegmentsInArc, false);
        break;
    case bottom: // Bottom
        w = area.size().y();
        r = w * 0.5f;
        Renderer::setCurrentColor(backgroundColor_);
        Renderer::drawRectangle(area.bottomLeft(), area.topRight(), true);
        Renderer::setCurrentColor(foregroundColor_);
        Renderer::drawLine(area.topLeft() + Core::Vector2(0.0f, r),  area.topRight() + Core::Vector2(0.0f,r));
        break;
    case bottomRight: // BottomRight
        Renderer::setCurrentColor(backgroundColor_);
        Renderer::drawCircleSegment(area.topLeft(), w, pi*1.5f, pi*2.0f, numSegmentsInArc, true);
        Renderer::setCurrentColor(foregroundColor_);
        Renderer::drawCircleSegment(area.topLeft(), r, pi*1.5f, pi*2.0f, numSegmentsInArc, false);
        break;
    case title: // Title
        Renderer::setCurrentColor(backgroundColor_);
        Renderer::drawRectangle(area.bottomLeft(), area.topRight(), true);
        Renderer::setCurrentColor(foregroundColor_);
        Renderer::drawRoundedRectangle(area.bottomLeft() + Core::Vector2(0.0f, -2.0f), area.topRight() + Core::Vector2(0.0f, 2.0f), 5, true);
        break;
    default:
        THROW("Invalid section specified in default frame");
    }
    Renderer::setLineWidth(1);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// **************************************************************************************************
//                                   BitmapDesc
// **************************************************************************************************



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
BitmapDesc::draw(const Core::Rectangle& area) const
{
    const float width  = static_cast<float>(image_->size().width());
    const float height = static_cast<float>(image_->size().height());

    if( (width > 0.0f) && (height > 0.0f) )
    {
        const float fRight = (right_ < 0) ? width : static_cast<float>(right_);
        const float fBottom = (bottom_ < 0) ? height : static_cast<float>(bottom_);
        const float minX = (static_cast<float>(left_) / width) + 0.001f;
        const float maxY = 1.0f - ((static_cast<float>(top_) / height) + 0.001f);
        const float maxX = (fRight / width) - 0.001f;
        const float minY = 1.0f - ((fBottom / height) - 0.001f);
        Renderer::drawImage(*image_, area, (mapping_ == tiled), minX, minY, maxX, maxY);
    }

}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// **************************************************************************************************
//                                   Bitmap1x3Desc
// **************************************************************************************************



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Bitmap1x3Desc::draw(const Core::Rectangle& area) const
{
    const float width  = static_cast<float>(image_->size().width());
    const float height = static_cast<float>(image_->size().height());

    if( (width > 0.0f) && (height > 0.0f) )
    {
        const int iRight = (right() < 0) ? image_->size().width() : right();
        const int iTop = (top() < 0) ? image_->size().height() : top();

        std::vector<float> xOffsets(4);
        xOffsets.push_back((static_cast<float>(left()) / width) + 0.001f);
        xOffsets.push_back(static_cast<float>(left() + leftOffset_) / width);
        xOffsets.push_back(static_cast<float>(iRight - rightOffset_) / width);
        xOffsets.push_back((static_cast<float>(right()) / width) - 0.001f);

        std::vector<float> rectOffsets(4);
        rectOffsets.push_back(area.minX());
        rectOffsets.push_back(area.minX() + leftOffset_);
        rectOffsets.push_back(area.maxX() - rightOffset_);
        rectOffsets.push_back(area.maxX());
        const float minY = (static_cast<float>(bottom()) / height) + 0.001f;
        const float maxY = (static_cast<float>(iTop) / height) - 0.001f;
        for(int x = 0; x < 3; ++x)
        {
            const float minX = xOffsets[x];
            const float maxX = xOffsets[x+1];
            const Core::Rectangle rect(rectOffsets[x], area.minY(), rectOffsets[x+1], area.maxY());
            Renderer::drawImage(*image_, rect, (mapping_ == tiled), minX, minY, maxX, maxY);
        }
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// **************************************************************************************************
} // namespace Gooey

