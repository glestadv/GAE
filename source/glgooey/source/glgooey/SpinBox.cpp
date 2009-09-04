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

#include "glgooey/SpinBox.h"

#include "glgooey/core/Rectangle.h"
#include "glgooey/core/StandardException.h"

#include "glgooey/Renderer.h"
#include "glgooey/RenderDesc.h"
#include "glgooey/PropertyScheme.h"
#include "glgooey/Button.h"
#include "glgooey/EditField.h"
#include "glgooey/MouseMessage.h"
#include "glgooey/WindowManager.h"
#include "glgooey/Timer.h"


// **************************************************************************************************
namespace Gooey
{

class SpinBoxButtonListener;
class SpinBoxFocusListener;






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief An increase or decrease button for a SpinBox
class SpinBoxButton : public Button
{
    // ---------------------------------------------------------------------------------------------
    //  Data Types
    // ---------------------------------------------------------------------------------------------
public:
    //! the directions in which a button can point
    enum Direction { up = 0, down };


    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! construction requires the parent (always a SpinBox) and the direction
    //! in which the button should point
    SpinBoxButton(Window* aParent, Direction aDirection);

    //! destructor
    virtual ~SpinBoxButton();


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








// **************************************************************************************************
//                                      SpinBox
// **************************************************************************************************





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SpinBox::SpinBox(Window* aParent, int initialValue) :
    Slideable(aParent),
    mouseListener_(0)
{
    // create the down button
    incrementButton_ = new SpinBoxButton(this, SpinBoxButton::up);

    // create the up button
    decrementButton_ = new SpinBoxButton(this, SpinBoxButton::down);

    // create the text field
    editField_ = new EditField(this, Core::toString(initialValue));

    // wire up the signals and slots
    incrementButton_->pressed.connect (this, &SpinBox::incrementButtonPressed);
    incrementButton_->released.connect(this, &SpinBox::incrementButtonReleased);
    decrementButton_->pressed.connect (this, &SpinBox::decrementButtonPressed);
    decrementButton_->released.connect(this, &SpinBox::decrementButtonReleased);

    incrementTimer_.trigger.connect(this, &SpinBox::incrementButtonPressed);
    decrementTimer_.trigger.connect(this, &SpinBox::decrementButtonPressed);

    editField_->returnPressed.connect(this, &SpinBox::textEdited);

    setValue(initialValue);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SpinBox::~SpinBox()
{
    delete incrementButton_;
    delete decrementButton_;
    delete editField_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
SpinBox::process(const Message& message)
{
    bool ret = false;
    if(isVisible() && isEnabled())
    {
        if(incrementButton_->process(message)) ret = true;
        if(decrementButton_->process(message)) ret = true;
        if(editField_->process(message)) ret = true;
    }
    if(!ret) ret = Window::process(message);

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window*
SpinBox::windowBelow(const Core::Vector2& p)
{
    Window* ret = Window::windowBelow(p);
    if(ret != 0)
    {
        Window* cw = incrementButton_->windowBelow(p);
        if(cw != 0)
            ret = cw;
        else
        {
            cw = decrementButton_->windowBelow(p);
            if(cw != 0) ret = cw;
        }
    }

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SpinBox&
SpinBox::arrange()
{
    Core::Vector2 editFieldSize(size() - Core::Vector2(size().y()/2, 0));
    Core::Vector2 buttonSize(size().y()/2, size().y() / 2);

    editField_->setPosition(position());
    editField_->setSize(editFieldSize);

    (*incrementButton_)
        .setSize(buttonSize)
        .setPosition(position() + Core::Vector2(editFieldSize.x(), 0))
        ;

    (*decrementButton_)
        .setSize(buttonSize)
        .setPosition(position() + Core::Vector2(editFieldSize.x(), size().y() / 2));
        ;

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
SpinBox::incrementButtonPressed()
{
    textEdited();
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

    updateDisplay();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
SpinBox::decrementButtonPressed()
{
    textEdited();
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

    updateDisplay();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
SpinBox::incrementButtonReleased()
{
    unFocusEditField();
    incrementTimer_.stop();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
SpinBox::decrementButtonReleased()
{
    unFocusEditField();
    decrementTimer_.stop();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
SpinBox::textEdited()
{
    if (isTextNumeric())
    {
        int newValue = Core::string_cast<int>(editField_->text());
        setValue(newValue);
    }

    updateDisplay();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
SpinBox::updateDisplay()
{
    editField_->setText(Core::toString(value()));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
SpinBox::focusEditField()
{
    WindowManager::instance().setFocusedWindow(editField_);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
SpinBox::unFocusEditField()
{
    textEdited();
    editField_->disableFocused();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
SpinBox::draw()
{
    // draw the buttons
    incrementButton_->render();
    decrementButton_->render();

    // draw the edit field.
    editField_->render();

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
SpinBox::isTextNumeric()
{
    const std::string txt = editField_->text();
    bool ret = true;
    for (int i = 0; i < int(txt.length()); ++i)
    {
        if (!isdigit(txt[i]))
        {
            ret = false;
            break;
        }
    }

    return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
SpinBox::loadAppearance(const std::string& aFileName, const std::string& aWindowName)
{
    editField_->loadAppearance(aFileName, aWindowName);

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Vector2
SpinBox::recommendedSize() const
{
    return Core::Vector2(20, 20);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
SpinBox::onGainedFocus()
{
    focusEditField();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
SpinBox::onLostFocus()
{
    unFocusEditField();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
SpinBox::onResize()
{
    arrange();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
SpinBox::onMove()
{
    arrange();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// **************************************************************************************************
//                                      SpinBoxButton
// **************************************************************************************************





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SpinBoxButton::SpinBoxButton(Window* aParent, Direction aDirection) :
    Button(aParent, ""),
    _direction(aDirection)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SpinBoxButton::~SpinBoxButton()
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
SpinBoxButton::draw()
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
    default:
        break;
    }

    Button::draw();

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// **************************************************************************************************
} // namespace Gooey

