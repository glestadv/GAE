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

#include "glgooey/ScrollBar.h"

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


REGISTER_CLASS(ScrollBarDesc);


class ScrollBarButtonListener;


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! The slider which runs along the track between the two scroll bar buttons.
class ScrollBarSlider : public Button, public MouseListener
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! construction requires a reference to the associated scroll bar
    ScrollBarSlider(ScrollBar& aScrollBar);

    //! destructor
    virtual ~ScrollBarSlider() {}

    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    float offsetToDiscreetPos() const { return _offsetToDiscreetPos; }


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //! process the moved message by moving the slider if it is currently pressed
    virtual bool moved(MouseMessage& message);

    //! react to the button being released
    virtual Button& onReleased() { _offsetToDiscreetPos = 0; return *this; }

    //! a reference to the associated scroll bar
    ScrollBar& _scrollBar;

    //! we have to keep track of the mouse position
    Core::Vector2 _oldMousePos;

    //! move the slider horizontally as a reaction to being dragged
    ScrollBarSlider& moveHorizontally(float dx);

    //! move the slider vertically as a reaction to being dragged
    ScrollBarSlider& moveVertically(float dy);


private:
    float _offsetToDiscreetPos;

    //! \brief how many screen units (pixels) correspond to one unit of the scroll bar's range
    float screenUnitsPerScrollBarUnit() const;

    // Copying and assignment are not allowed
    ScrollBarSlider(const ScrollBarSlider& aScrollBarSlider);
    ScrollBarSlider& operator = (const ScrollBarSlider& aScrollBarSlider);
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! An increase or decrease button for the ends of the scroll bar
class ScrollBarButton : public Button
{
    // ---------------------------------------------------------------------------------------------
    //  Public Data Types
    // ---------------------------------------------------------------------------------------------
public:
    //! the directions in which a button can point
    enum Direction { up = 0, down, left, right };


    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Construction requires the parent (always a scroll bar) and the direction
    //! in which the button should point
    ScrollBarButton(Window* aParent, Direction aDirection);

    //! destructor
    virtual ~ScrollBarButton();


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! the direction in which the button's arrow points
    Direction direction() const { return _direction; }


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //! render the window to the current OpenGL context
    virtual Window& draw();

private:
    Direction _direction;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Handles mouse clicks on the scroll bar
class ScrollBarMouseListener : public MouseListener
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    ScrollBarMouseListener(ScrollBar& aScrollBar) : _scrollBar(aScrollBar) {}
    virtual ~ScrollBarMouseListener() {}

    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    virtual bool clicked(MouseClickMessage& message);

    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    ScrollBar& _scrollBar;

    // Copying and assignment are not allowed
    ScrollBarMouseListener(const ScrollBarMouseListener& aScrollBarMouseListener);
    ScrollBarMouseListener& operator = (const ScrollBarMouseListener& aScrollBarMouseListener);
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<










// **************************************************************************************************
//                                      ScrollBar
// **************************************************************************************************





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ScrollBar::ScrollBar(Window* aParent, Justification aJustification,
                     const Core::Vector2& topLeft, float extent) :
    Slideable(aParent),
    mouseListener_(0),
    justification_(aJustification),
    visibleAmount_(1),
    renderDesc_(0)
{

    // create the slider
    slider_ = new ScrollBarSlider(*this);
    // pass the pressed and released signals on
    slider_->pressed.connect(this, &ScrollBar::buttonPressed);
    slider_->released.connect(this, &ScrollBar::buttonReleased);

    Core::Vector2 sz;

    if(justification() == vertical)
    {
        // create the down button
        incrementButton_ = new ScrollBarButton(this, ScrollBarButton::down);

        // create the up button
        decrementButton_ = new ScrollBarButton(this, ScrollBarButton::up);

        // set size
        sz = Core::Vector2(width(), extent);
    }
    else
    {

        // create the right button
        incrementButton_ = new ScrollBarButton(this, ScrollBarButton::right);

        // create the left button
        decrementButton_ = new ScrollBarButton(this, ScrollBarButton::left);

        // set size
        sz = Core::Vector2(extent, width());
    }
    // wire up the signals and slots
    incrementButton_->pressed.connect (this, &ScrollBar::incrementButtonPressed);
    incrementButton_->released.connect(this, &ScrollBar::incrementButtonReleased);
    decrementButton_->pressed.connect (this, &ScrollBar::decrementButtonPressed);
    decrementButton_->released.connect(this, &ScrollBar::decrementButtonReleased);

    incrementTimer_.trigger.connect(this, &ScrollBar::incrementButtonPressed);
    decrementTimer_.trigger.connect(this, &ScrollBar::decrementButtonPressed);

    // set position
    setPosition(topLeft);
    setSize(sz);

    mouseListener_ = new ScrollBarMouseListener(*this);

    addMessageListener(mouseListener_);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ScrollBar::~ScrollBar()
{
    removeMessageListener(mouseListener_);
    delete mouseListener_;

    delete incrementButton_;
    delete decrementButton_;
    delete slider_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
ScrollBar::process(const Message& message)
{
    bool ret = false;
    if(isVisible() && isEnabled())
    {
        if(incrementButton_->process(message)) ret = true;
        if(decrementButton_->process(message)) ret = true;
        if(slider_->process(message)) ret = true;
    }
    if(!ret) ret = Window::process(message);
    return ret ;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
ScrollBar::containsFocus() const
{
    return Window::containsFocus() || 
        incrementButton_->containsFocus() ||
        decrementButton_->containsFocus() ||
        slider_->containsFocus();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
ScrollBar::doesHierarchyContain(Window* aWindow) const
{
    return Window::doesHierarchyContain(aWindow) || 
        incrementButton_->doesHierarchyContain(aWindow) ||
        decrementButton_->doesHierarchyContain(aWindow) ||
        slider_->doesHierarchyContain(aWindow);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
ScrollBar::trackLength() const
{
    const Core::Vector2 topLeft = position();
    const Core::Vector2 bottomRight = position() + size();

    const Core::Rectangle rect(topLeft, bottomRight);

    float ret = 0;
    if(justification() == horizontal)
    {
        ret = rect.size().x() -
            incrementButton_->size().x() - decrementButton_->size().x();
    }
    else
    {
        ret = rect.size().y() -
            incrementButton_->size().y() - decrementButton_->size().y();
    }

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
ScrollBar::computeSliderLength() const
{
    const float length = trackLength();
    const int l = range();
    const float visibleFraction = float(visibleAmount()) / static_cast<float>(l);

    float ret = static_cast<float>(length) * visibleFraction;

    if(ret < decrementButton_->size().x()) ret = decrementButton_->size().x();

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
ScrollBar::discreetSliderOffset() const
{
    const float slideLength = trackLength() - computeSliderLength();

    const float valueFraction = static_cast<float>(value()) / static_cast<float>(range());

    return width() + (slideLength * valueFraction);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ScrollBar&
ScrollBar::arrange()
{
    const int l = range();
    if(visibleAmount() > l) setVisibleAmount(l);
    const Core::Vector2 buttonPoint(width(), width());
    if(justification() == vertical)
    {
        (*incrementButton_)
            .setSize(buttonPoint)
            .setPosition(position() + Core::Vector2(0, size().y() - width()))
            ;

        (*decrementButton_)
            .setSize(buttonPoint)
            .setPosition(position())
            ;
    }
    else
    {
        (*incrementButton_)
            .setSize(buttonPoint)
            .setPosition(position() + Core::Vector2(size().x() - width(), 0))

            ;

        (*decrementButton_)
            .setSize(buttonPoint)
            .setPosition(position())
            ;
    }

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ScrollBar::incrementButtonPressed()
{
    int newValue = value() + unitIncrement();
    setValue(newValue);

    // start the timer if it's not already on, quicken it if it is
    if(!incrementTimer_.isStarted())
    {
        incrementTimer_.setInterval(0.25f);
        incrementTimer_.start();
    }
    else
        incrementTimer_.setInterval(0.05f);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ScrollBar::decrementButtonPressed()
{
    int newValue = value() - unitIncrement();
    setValue(newValue);

    // start the timer if it's not already on, quicken it if it is
    if(!decrementTimer_.isStarted())
    {
        decrementTimer_.setInterval(0.25f);
        decrementTimer_.start();
    }
    else
        decrementTimer_.setInterval(0.05f);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ScrollBar::incrementButtonReleased()
{
    incrementTimer_.stop();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ScrollBar::decrementButtonReleased()
{
    decrementTimer_.stop();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ScrollBar::onResize()
{
    arrange();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ScrollBar::onMove()
{
    arrange();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Vector2
ScrollBar::recommendedSize() const
{
    return (justification_ == horizontal) ? Core::Vector2(150.0f, width()) : Core::Vector2(width(), 150.0f);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
ScrollBar::width() const
{
    ScrollBarDesc* renderDesc = renderDesc_;
    if(renderDesc == 0)
        renderDesc = &WindowManager::instance().propertyScheme().scrollBarDesc_;

    return renderDesc->width();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
ScrollBar::draw()
{
    ScrollBarDesc* renderDesc = renderDesc_;
    if(renderDesc == 0)
        renderDesc = &WindowManager::instance().propertyScheme().scrollBarDesc_;

    const Core::Vector2 topLeft = position();
    const Core::Vector2 bottomRight = position() + size();
    Core::Rectangle rect(topLeft, bottomRight);

    renderDesc->draw(rect);

    Renderer::disableClipRectangle();
    Renderer::setCurrentColor(Color(1, 1, 1, 1));

    // size and position the slider
    ASSERT ( (maximum() > minimum()) );
    if(justification() == vertical)
    {
        // vertical
        const float width = decrementButton_->size().x();
        (*slider_)
            .setSize(Core::Vector2(width, computeSliderLength()))
            .setPosition(topLeft + Core::Vector2(0.0f, discreetSliderOffset() + slider_->offsetToDiscreetPos()))
            ;
    }
    else
    {
        // horizontal
        const float height = decrementButton_->size().y();
        (*slider_)
            .setSize(Core::Vector2(computeSliderLength(), height))
            .setPosition(topLeft + Core::Vector2(discreetSliderOffset() + slider_->offsetToDiscreetPos(), 0.0f))
            ;
    }

    // draw the slider
    slider_->render();

    // draw the buttons
    incrementButton_->render();
    decrementButton_->render();

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
ScrollBar::loadAppearance(const std::string& aFileName, const std::string& aWindowName)
{
    if(renderDesc_ == 0) renderDesc_ = new ScrollBarDesc;
    {
        Core::XMLInputArchive archive(aFileName);
        archive & Core::ObjectNamer<ScrollBarDesc>(aWindowName.c_str(), *renderDesc_);
    }
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// **************************************************************************************************
//                                     ScrollBarButton
// **************************************************************************************************





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ScrollBarButton::ScrollBarButton(Window* aParent, Direction aDirection) :
    Button(aParent, ""),
    _direction(aDirection)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ScrollBarButton::~ScrollBarButton()
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
ScrollBarButton::draw()
{
    ButtonDesc* buttonDesc = renderDesc_;
    if(buttonDesc == 0)
        buttonDesc = &WindowManager::instance().propertyScheme().buttonDesc_;

    switch(direction())
    {
    case up:
        buttonDesc->setArrow(ButtonDesc::up);
        break;
    case down:
        buttonDesc->setArrow(ButtonDesc::down);
        break;
    case left:
        buttonDesc->setArrow(ButtonDesc::left);
        break;
    case right:
        buttonDesc->setArrow(ButtonDesc::right);
        break;
    default:
        break;
    }

    Button::draw();

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// **************************************************************************************************
//                                       ScrollBarSlider
// **************************************************************************************************






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ScrollBarSlider::ScrollBarSlider(ScrollBar& aScrollBar) :
    Button(&aScrollBar, ""),
    _scrollBar(aScrollBar),
    _oldMousePos(-1.0f, -1.0f),
    _offsetToDiscreetPos(0.0f)
{
    //  make sure a pressed slider remains active (i.e. pressed) even when the
    // mouse wanders away from it
    keepPressedOnExit();

    const float sz = _scrollBar.width();
    switch(_scrollBar.justification())
    {
    case ScrollBar::horizontal:
        setPosition(_scrollBar.position() + Core::Vector2(sz, 0.0f));
        setSize(Core::Vector2(sz, sz));
        break;
    case ScrollBar::vertical:
        setPosition(_scrollBar.position() + Core::Vector2(0.0f, sz));
        setSize(Core::Vector2(sz, sz));
        break;
    }


    addMessageListener(this);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
ScrollBarSlider::moved(MouseMessage& message)
{
    if(_oldMousePos.x() >= 0.0f)
    {
        if(state() == Button::down)
        {
            int value = _scrollBar.value();
            switch(_scrollBar.justification())
            {
            case ScrollBar::horizontal:
                {
                    const float delta = message.x() - _oldMousePos.x();
                    if(delta != 0) moveHorizontally(delta);
                }
                break;
            case ScrollBar::vertical:
                {
                    const float delta = message.y() - _oldMousePos.y();
                    if(delta != 0) moveVertically(delta);
                }
                break;
            }
            if(value != _scrollBar.value()) _scrollBar.sliderMoved(_scrollBar.value());
        }
    }
    _oldMousePos.setX(message.x());
    _oldMousePos.setY(message.y());
    return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ScrollBarSlider&
ScrollBarSlider::moveHorizontally(float dx)
{
    // move the slider to its new position
    setPosition(position() + Core::Vector2(dx, 0.0f));

    // where is that on the track (i.e. distance in pixels from the decrement button)
    const float positionOnTrack = position().x() - _scrollBar.position().x() - _scrollBar.width();

    //  use the pixels per unit and the distance in pixels from the decrement button
    // to set the new value
    _scrollBar.setValue(static_cast<int>(positionOnTrack / screenUnitsPerScrollBarUnit()));

    // where does the scroll bar reckon the slider should now be
    const float properPosition = _scrollBar.position().x() + _scrollBar.discreetSliderOffset();

    // where the slider actually is
    const float actualPosition = position().x();

    // remember the offset between the two
    _offsetToDiscreetPos = actualPosition - properPosition;

    // check for the ends
    if( (actualPosition < _scrollBar.position().x() + _scrollBar.width()) && (offsetToDiscreetPos() < 0) )
    {
        _offsetToDiscreetPos = 0.0f;
    }
    const float endOfSlider = actualPosition + _scrollBar.computeSliderLength();
    const float endOfTrack  = _scrollBar.position().x() + _scrollBar.width() + _scrollBar.trackLength();
    if( (endOfSlider > endOfTrack) && (offsetToDiscreetPos() > 0.0f) )
        _offsetToDiscreetPos = 0.0f;


    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ScrollBarSlider&
ScrollBarSlider::moveVertically(float dy)
{
    // move the slider to its new position
    setPosition(position() + Core::Vector2(0.0f, dy));

    // where is that on the track (i.e. distance in pixels from the decrement button)
    const float positionOnTrack = position().y() - _scrollBar.position().y() - _scrollBar.width();

    //  use the pixels per unit and the distance in pixels from the decrement button
    // to set the new value
    _scrollBar.setValue(static_cast<int>(positionOnTrack / screenUnitsPerScrollBarUnit()));

    // where does the scroll bar reckon the slider should now be
    const float properPosition = _scrollBar.position().y() + _scrollBar.discreetSliderOffset();

    // where the slider actually is
    const float actualPosition = position().y();

    // remember the offset between the two
    _offsetToDiscreetPos = actualPosition - properPosition;

    // check for the ends
    if( (actualPosition < _scrollBar.position().y() + _scrollBar.width()) && (offsetToDiscreetPos() < 0.0f) )
    {
        _offsetToDiscreetPos = 0.0f;
    }
    const float endOfSlider = actualPosition + _scrollBar.computeSliderLength();
    const float endOfTrack  = _scrollBar.position().y() + _scrollBar.width() + _scrollBar.trackLength();
    if( (endOfSlider > endOfTrack) && (offsetToDiscreetPos() > 0.0f) )
        _offsetToDiscreetPos = 0.0f;

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
ScrollBarSlider::screenUnitsPerScrollBarUnit() const
{
    // length of available track (i.e. without the slider)
    const float trackLength = _scrollBar.trackLength() - _scrollBar.computeSliderLength();

    // what fraction of the scroll bar's range is a unit increment
    const float unitFraction = 1.0f / float(_scrollBar.range());

    return trackLength * unitFraction;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// **************************************************************************************************
//                                   ScrollBarListener
// **************************************************************************************************



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
ScrollBarMouseListener::clicked(MouseClickMessage& message)
{
    bool isHandled = false;
    if(message.event() == MouseClickMessage::released)
    {
        const float positionOnTrack = (_scrollBar.justification() == ScrollBar::horizontal) ?
            static_cast<float>(message.x()) - _scrollBar.position().x() - _scrollBar.width() :
            static_cast<float>(message.y()) - _scrollBar.position().y() - _scrollBar.width();

        if((positionOnTrack > 0.0f) && (positionOnTrack < _scrollBar.trackLength()))
        {
            //  We're on the track, check to see if we're infront of or
            // behind the slider
            const float sliderOffset = _scrollBar.discreetSliderOffset() - _scrollBar.width();
            if(positionOnTrack < sliderOffset)
                _scrollBar.setValue(_scrollBar.value() - _scrollBar.blockIncrement());
            else if(positionOnTrack > (sliderOffset + _scrollBar.computeSliderLength()))
                _scrollBar.setValue(_scrollBar.value() + _scrollBar.blockIncrement());
        }
        isHandled = true;
    }

    return isHandled;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// **************************************************************************************************
//                                    ScrollBarDesc
// **************************************************************************************************





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ScrollBarDesc::ScrollBarDesc() :
    background_(new SolidSurfaceDesc()),
    border_(new RectangleDesc()),
    width_(16.0f)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ScrollBarDesc::~ScrollBarDesc()
{
    delete background_;
    delete border_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// **************************************************************************************************
} // namespace Gooey

