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

#include "glgooey/Slider.h"

#include "glgooey/core/Rectangle.h"
#include "glgooey/core/StandardException.h"
#include "glgooey/core/XMLOutputArchive.h"
#include "glgooey/core/XMLInputArchive.h"

#include "glgooey/Renderer.h"
#include "glgooey/PropertyScheme.h"
#include "glgooey/Button.h"
#include "glgooey/MouseMessage.h"
#include "glgooey/WindowManager.h"
#include "glgooey/Timer.h"
#include "glgooey/SolidSurfaceDesc.h"




// **************************************************************************************************
namespace Gooey
{

REGISTER_CLASS(SliderDesc);


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! The slider which runs along the track between the two scroll bar buttons.
class SliderButton : public Button, public MouseListener
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! construction requires a reference to the associated scroll bar
    SliderButton(Slider& aSlider);

    //! destructor
    virtual ~SliderButton() {}


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //! process the moved message by moving the slider if it is currently pressed
    virtual bool moved(MouseMessage& message);

    //! a reference to the associated scroll bar
    Slider& slider_;

    //! we have to keep track of the mouse position
    Core::Vector2 _oldMousePos;

    //! move the slider horizontally as a reaction to being dragged
    SliderButton& moveHorizontally(float dx);

    //! move the slider vertically as a reaction to being dragged
    SliderButton& moveVertically(float dy);

private:
    // Copying and assignment are not allowed
    SliderButton(const SliderButton& aSliderButton);
    SliderButton& operator = (const SliderButton& aSliderButton);
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Handles mouse clicks on the slider window
class SliderMouseListener : public MouseListener
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    SliderMouseListener(Slider& aSlider) : slider_(aSlider) {}
    virtual ~SliderMouseListener() {}

    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    virtual bool clicked(MouseClickMessage& message);

    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    Slider& slider_;

    // Copying and assignment are not allowed
    SliderMouseListener(const SliderMouseListener& aSliderMouseListener);
    SliderMouseListener& operator = (const SliderMouseListener& aSliderMouseListener);
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// **************************************************************************************************
//                                      Slider
// **************************************************************************************************



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Slider::Slider(Window* aParent, Justification aJustification, const Core::Rectangle& aRectangle) :
    Slideable(aParent),
    button_(0),
    justification_(aJustification),
    tickPlacement_(none),
    tickFrequency_(16),
    renderDesc_(0)
{
    // set rectangle
    setWindowRectangle(aRectangle);

    // create the slider
    button_ = new SliderButton(*this);

    // pass the pressed and released signals on
    button_->pressed.connect(this, &Slider::buttonPressed);
    button_->released.connect(this, &Slider::buttonReleased);

    mouseListener_ = new SliderMouseListener(*this);
    addMessageListener(mouseListener_);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Slider::~Slider()
{
    delete button_;

    removeMessageListener(mouseListener_);
    delete mouseListener_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
Slider::process(const Message& message)
{
    bool ret = false;
    if(isVisible() && isEnabled())
        ret = button_->process(message);

    if(!ret) ret = Window::process(message);

    return ret ;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
Slider::trackLength() const
{
    const Core::Vector2 topLeft = position();
    const Core::Vector2 bottomRight = position() + size();

    const Core::Rectangle rect(topLeft, bottomRight);

    const float windowLength = (justification() == horizontal) ? rect.width() : rect.height();

    return (windowLength * 0.9f);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Vector2
Slider::buttonSize() const
{
    const Core::Vector2 topLeft = position();
    const Core::Vector2 bottomRight = position() + size();

    const Core::Rectangle rect(topLeft, bottomRight);

    const bool isHorz = (justification() == horizontal);
    const float l = isHorz ? rect.width()  : rect.height();
    const float h = isHorz ? rect.height() : rect.width();

    float w = ceilf(l * 0.04f);
    if(w < 5.0f) w = 5.0f;

    return Core::Vector2(w, ceilf(h * 0.5f));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Slider&
Slider::arrange()
{
    if(button_ != 0)
        button_->setSize(buttonSize());

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Slider::onResize()
{
    arrange();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Slider::onMove()
{
    arrange();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Vector2
Slider::recommendedSize() const
{
    return (justification_ == horizontal) ? Core::Vector2(150, 32) : Core::Vector2(32, 150);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
Slider::draw()
{
    ASSERT(range() > 0);

    SliderDesc* renderDesc = renderDesc_;
    if(renderDesc == 0)
        renderDesc = &WindowManager::instance().propertyScheme().sliderDesc_;

    const Core::Vector2 topLeft = position();
    const Core::Vector2 bottomRight = position() + size();
    Core::Rectangle rect(topLeft, bottomRight);

    renderDesc->draw(rect);

    // draw the track and the ticks
    Renderer::setCurrentColor(renderDesc->tickColor());
    Renderer::setLineWidth(renderDesc->tickWidth());

    // how many pixels correspond to a unit increment
    const float pixelsPerUnit = trackLength() / static_cast<float>(range());
    const int frequency = (tickFrequency() > 0) ? tickFrequency() : range();

    Core::Vector2 p1;
    Core::Vector2 p2;
    if(justification() == vertical)
    {
        p1 = topLeft + Core::Vector2(rect.halfSize().x(), ((rect.height() - trackLength()) / 2));
        p2 = p1 + Core::Vector2(0, trackLength());

        if( (tickPlacement() == left) || (tickPlacement() == both) )
        {
            const float x1 = topLeft.x();
            const float x2 = x1 + (rect.width() * 0.25f);
            float y = p1.y();
            for(int i = minimum(); i <= maximum(); i += frequency)
            {
                Renderer::drawLine(Core::Vector2(x1, y), Core::Vector2(x2, y));
                y += pixelsPerUnit * static_cast<float>(frequency);
            }
        }
        if( (tickPlacement() == right) || (tickPlacement() == both) )
        {
            const float x2 = bottomRight.x();
            const float x1 = x2 - (rect.width() * 0.25f);
            float y = p1.y();
            for(int i = minimum(); i <= maximum(); i += frequency)
            {
                Renderer::drawLine(Core::Vector2(x1, y), Core::Vector2(x2, y));
                y += pixelsPerUnit * static_cast<float>(frequency);
            }
        }
    }
    else
    {
        p1 = topLeft + Core::Vector2(((rect.width() - trackLength()) * 0.5f), rect.halfSize().y());
        p2 = p1 + Core::Vector2(trackLength(), 0.0f);

        if( (tickPlacement() == below) || (tickPlacement() == both) )
        {
            const float y1 = bottomRight.y();
            const float y2 = y1 - (rect.height() * 0.25f);
            float x = p1.x();
            for(int i = minimum(); i <= maximum(); i += frequency)
            {
                Renderer::drawLine(Core::Vector2(x, y1), Core::Vector2(x, y2));
                x += pixelsPerUnit * static_cast<float>(frequency);
            }
        }
        if( (tickPlacement() == above) || (tickPlacement() == both) )
        {
            const float y2 = topLeft.y();
            const float y1 = y2 + (rect.height() * 0.25f);
            float x = p1.x();
            for(int i = minimum(); i <= maximum(); i += frequency)
            {
                Renderer::drawLine(Core::Vector2(x, y1), Core::Vector2(x, y2));
                x += pixelsPerUnit * static_cast<float>(frequency);
            }
        }
    }
    Renderer::drawLine(p1, p2);

    Renderer::disableClipRectangle();
    Renderer::setLineWidth(1.0f);
    Renderer::setCurrentColor(Color(1, 1, 1, 1));

    // position the button
    const float fraction = float(value() - minimum()) / float(range());
    const float buttonOffset = ceilf((trackLength() * fraction) + 0.5f);
    const float halfButtonWidth  = ceilf(button_->size().x() * 0.5f);
    const float halfButtonHeight = ceilf(button_->size().y() * 0.5f);
    if(justification() == vertical)
    {
        // vertical
        button_->setPosition(p1 + Core::Vector2(-halfButtonWidth, buttonOffset - halfButtonHeight));
    }
    else
    {
        // horizontal
        button_->setPosition(p1 + Core::Vector2(buttonOffset - halfButtonWidth, -halfButtonHeight));
    }

    // draw the slider
    button_->render();

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
Slider::loadAppearance(const std::string& aFileName, const std::string& aWindowName)
{
    if(renderDesc_ == 0) renderDesc_ = new SliderDesc;
    {
        Core::XMLInputArchive archive(aFileName);
        archive & Core::ObjectNamer<SliderDesc>(aWindowName.c_str(), *renderDesc_);
    }
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<









// **************************************************************************************************
//                          SliderButton
// **************************************************************************************************






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SliderButton::SliderButton(Slider& aSlider) :
    Button(&aSlider, ""),
    slider_(aSlider),
    _oldMousePos(-1.0f, -1.0f)
{
    //  make sure a pressed slider remains active (i.e. pressed) even when the
    // mouse wanders away from it
    keepPressedOnExit();

    //  the position gets corrected when the button is drawn, so we don't bother
    // about it here
    setWindowRectangle(Core::Rectangle(slider_.position(), slider_.buttonSize()));

    addMessageListener(this);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
SliderButton::moved(MouseMessage& message)
{
    if(state() == Button::down)
    {
        int value = slider_.value();
        switch(slider_.justification())
        {
        case Slider::horizontal:
            moveHorizontally(message.x());
            break;
        case Slider::vertical:
            moveVertically(message.y());
            break;
        }
        if(value != slider_.value()) slider_.sliderMoved(slider_.value());
    }
    return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SliderButton&
SliderButton::moveHorizontally(float xPos)
{
    // length of the track on which the slider runs
    const float trackLength = slider_.trackLength();

    // how many pixels correspond to a unit increment
    const float pixelsPerUnit = trackLength / static_cast<float>(slider_.range());

    // move the slider to its new position
    setPosition(Core::Vector2(xPos - (size().x() * 0.5f), position().y()));

    // where is that on the track (i.e. distance in pixels from the 'zero' position)
    const float trackOffset = (slider_.size().x() - trackLength) * 0.5f;
    const float positionOnTrack =
        windowRectangle().centerPoint().x() - slider_.position().x() - trackOffset;

    //  use the pixels per unit and the distance in pixels from the 'zero' position
    // to set the new value
    slider_.setValue(slider_.minimum() + static_cast<int>((positionOnTrack / pixelsPerUnit) + 0.5f));

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SliderButton&
SliderButton::moveVertically(float yPos)
{
    // length of the track on which the slider runs
    const float trackLength = slider_.trackLength();

    // how many pixels correspond to a unit increment
    const float pixelsPerUnit = trackLength / static_cast<float>(slider_.range());

    // move the slider to its new position
    setPosition(Core::Vector2(position().x(), yPos));

    // where is that on the track (i.e. distance in pixels from the 'zero' position)
    const float trackOffset = (slider_.size().y() - trackLength) * 0.5f;
    const float positionOnTrack =
        windowRectangle().centerPoint().y() - slider_.position().y() - trackOffset;

    //  use the pixels per unit and the distance in pixels from the 'zero' position
    // to set the new value
    slider_.setValue(slider_.minimum() + static_cast<int>((positionOnTrack / pixelsPerUnit) + 0.5f));

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// **************************************************************************************************
//                          SliderListener
// **************************************************************************************************



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
SliderMouseListener::clicked(MouseClickMessage& message)
{
    bool isHandled = false;
    if(message.event() == MouseClickMessage::released)
    {
        bool isHorz = (slider_.justification() == Slider::horizontal);
        const float positionOnTrack = isHorz ?
            message.x() - slider_.position().x() :
            message.y() - slider_.position().y();
        const float length = isHorz ? slider_.size().x() : slider_.size().y();

        if((positionOnTrack > 0) && (positionOnTrack < length))
        {
            //  We're on the track, check to see if we're infront of or
            // behind the slider
            const float sliderOffset = isHorz ?
                slider_.button_->position().x() - slider_.position().x() :
                slider_.button_->position().y() - slider_.position().y();
            const float sliderSize = isHorz ? slider_.button_->size().x() :
                slider_.button_->size().y();
            if(positionOnTrack < sliderOffset)
                slider_.setValue(slider_.value() - slider_.blockIncrement());
            else if(positionOnTrack > (sliderOffset + sliderSize))
                slider_.setValue(slider_.value() + slider_.blockIncrement());
        }
        isHandled = true;
    }

    return isHandled;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// **************************************************************************************************
//                          SliderDesc
// **************************************************************************************************





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SliderDesc::SliderDesc() :
    background_(new SolidSurfaceDesc()),
    border_(new RectangleDesc()),
    tickColor_(Color(1, 1, 1, 0.75f)),
    tickWidth_(1.0f)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SliderDesc::~SliderDesc()
{
    delete background_;
    delete border_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
SliderDesc::draw(const Core::Rectangle& area) const
{
    if(background_ != 0) background_->draw(area);
    if(border_ != 0)     border_->draw(area);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// **************************************************************************************************
} // namespace Gooey

