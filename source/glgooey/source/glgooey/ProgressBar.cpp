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

#include "glgooey/ProgressBar.h"

#include "glgooey/core/Rectangle.h"
#include "glgooey/core/XMLOutputArchive.h"
#include "glgooey/core/XMLInputArchive.h"

#include "glgooey/Renderer.h"
#include "glgooey/WindowManager.h"
#include "glgooey/Color.h"
#include "glgooey/PropertyScheme.h"
#include "glgooey/SolidSurfaceDesc.h"



// **************************************************************************************************
namespace Gooey
{


REGISTER_CLASS(ProgressBarDesc);



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ProgressBar::ProgressBar(Window* aParent, const Core::Rectangle& aRectangle,
                         const Orientation anOrientation) :
    Window(aParent, ""),
    orientation_(anOrientation),
    percentage_(0),
    renderDesc_(0)
{
    // set the geometry
    setWindowRectangle(aRectangle);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ProgressBar::~ProgressBar()
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Vector2
ProgressBar::recommendedSize() const
{
    return (orientation_ == horizontal) ? Core::Vector2(150, 32) : Core::Vector2(32, 150);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
ProgressBar::draw()
{
    if(percentage() < 0)   setPercentage(0);
    if(percentage() > 100) setPercentage(100);

    ProgressBarDesc* renderDesc = renderDesc_;
    if(renderDesc == 0)
        renderDesc = &WindowManager::instance().propertyScheme().progressBarDesc_;

    renderDesc->setProgressBar(this);

    const Core::Vector2 topLeft = position();
    const Core::Vector2 bottomRight = position() + size();

    renderDesc->draw(Core::Rectangle(topLeft, bottomRight));

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
ProgressBar::loadAppearance(const std::string& aFileName, const std::string& aWindowName)
{
    if(renderDesc_ == 0) renderDesc_ = new ProgressBarDesc;
    {
        Core::XMLInputArchive archive(aFileName);
        archive & Core::ObjectNamer<ProgressBarDesc>(aWindowName.c_str(), *renderDesc_);
    }
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<











// **************************************************************************************************
//                                      ProgressBarDesc
// **************************************************************************************************





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ProgressBarDesc::ProgressBarDesc() :
    horzEmptyDesc_( new SolidSurfaceDesc(Color(1, 0, 0, 0.5f)) ),
    horzFullDesc_ ( new SolidSurfaceDesc(Color(0, 1, 0, 0.5f)) ),
    vertEmptyDesc_( new SolidSurfaceDesc(Color(1, 0, 0, 0.5f)) ),
    vertFullDesc_ ( new SolidSurfaceDesc(Color(0, 1, 0, 0.5f)) ),
    border_(new RectangleDesc())
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ProgressBarDesc::~ProgressBarDesc()
{
    delete horzEmptyDesc_;
    delete horzFullDesc_;
    delete vertEmptyDesc_;
    delete vertFullDesc_;
    delete border_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ProgressBarDesc::draw(const Core::Rectangle& area) const
{
    const Core::Vector2 topLeft = area.topLeft();
    const Core::Vector2 bottomRight = area.bottomRight();

    const Core::Vector2 barTopLeft = topLeft + Core::Vector2(1, 1);
    const Core::Vector2 barBottomRight = bottomRight + Core::Vector2(-1, -1);

    if(progressBar_->orientation() == ProgressBar::horizontal)
    {
        const float entireWidth = float(progressBar_->size().x()) - 2.0f;
        const float f = float(progressBar_->percentage()) * 0.01f;
        const unsigned int fullWidth  = (unsigned int)(entireWidth * f);

        // draw the full block
        if(fullWidth > 0)
        {
            // draw the full portion
            int offs = (fullWidth > 0) ? 1 : 0;
            Renderer::enableClipRectangle(Core::Rectangle(barTopLeft - Core::Vector2(0, 1),
                Core::Vector2(barTopLeft.x() + fullWidth + offs, barBottomRight.y())));
            if(horzFullDesc_ != 0)  horzFullDesc_->draw(area);
        }

        // draw the empty block
        if(entireWidth - fullWidth > 0)
        {
            // draw the empty portion
            Renderer::enableClipRectangle(Core::Rectangle(
                Core::Vector2(barTopLeft.x() + fullWidth, barTopLeft.y() - 1),
                barBottomRight + Core::Vector2(1, 0)));
            if(horzEmptyDesc_ != 0) horzEmptyDesc_->draw(area);
        }
    }
    else
    {
        const float entireHeight = float(progressBar_->size().y()) - 2.0f;
        const float frevent = progressBar_->percentage() * 0.01f;
        const unsigned int fullHeight  = (unsigned int)(entireHeight * frevent);

        // draw the full block
        if(fullHeight > 0)
        {
            // draw the full portion
            int offs = (entireHeight - fullHeight == 0) ? 1 : 0;
            Renderer::enableClipRectangle(Core::Rectangle(
                Core::Vector2(barTopLeft.x(), barBottomRight.y() - fullHeight - offs),
                barBottomRight + Core::Vector2(1, 0)));
            if(vertFullDesc_ != 0) vertFullDesc_->draw(area);
        }

        // draw the empty block
        if(entireHeight - fullHeight > 0)
        {
            // draw the empty portion
            Renderer::enableClipRectangle(Core::Rectangle(
                barTopLeft + Core::Vector2(0, -1), Core::Vector2(barBottomRight.x() + 1,
                barBottomRight.y() - fullHeight)));
            if(vertEmptyDesc_ != 0) vertEmptyDesc_->draw(area);
        }
    }

    Renderer::disableClipRectangle();
    Renderer::setCurrentColor(Color(1, 1, 1, 1));

    if(border_ != 0) border_->draw(area);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// **************************************************************************************************
} // namespace Gooey

