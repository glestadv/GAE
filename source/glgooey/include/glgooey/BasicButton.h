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
#ifndef BASICBUTTON__H
#define BASICBUTTON__H

#include "mmgr/mmgr.h"

#include "glgooey/Window.h"
#include "glgooey/RenderDesc.h"


// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    class ButtonMouseListener;
    class ButtonDesc;
    class TextDesc;
    class BasicButtonGroup;
}
// *************************************************************************************************



// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Abstract base class for all buttons
//!
//! This class provides the base functionality for all button objects such as
//! standard buttons, check boxes or multi-text buttons. The BasicButton
//! responds to mouse clicks and roll-overs changing state accordingly. It
//! emits pressed and released signals.
class BasicButton : public Window
{
    // ---------------------------------------------------------------------------------------------
    //  Data Types
    // ---------------------------------------------------------------------------------------------
public:
    //! the different states that can be adopted by a button
    enum State { up = 0,     //!< the default button state
                 down,       //!< the button is pressed
                 rollOver,   //!< the mouse cursor is over the button without pressing
                 rollOverOn  //!< used only for toggle buttons
               };


    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Constructor requires the parent and the text with which the button is to be labeled
    explicit BasicButton(Window* aParent = 0, const std::string& aText = "", BasicButtonGroup* aGroup = 0);

    //! destructor
    virtual ~BasicButton();


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! returns the state of the button
    State state() const { return state_; }

    BasicButton& toggleOn() { isToggleOn_ = true; return *this; }
    BasicButton& toggleOff() { isToggleOn_ = false; return *this; }
    bool isToggleOn() const { return isToggleOn_; }

    BasicButton& setMouseOverStatus(bool isMouseOverButton);
    bool isMouseOver() const    { return isMouseOver_; }

    State drawState() const;
    BasicButton& toggle();

    // Commented in base class
    virtual Core::Vector2 recommendedSize() const;
    virtual Window& loadAppearance(const std::string& aFileName, const std::string& aWindowName);

    //! Returns a reference to the ButtonDesc object responsible for rendering the button
    ButtonDesc& buttonDesc() const;


    // ---------------------------------------------------------------------------------------------
    //  Signals
    // ---------------------------------------------------------------------------------------------
public:
    //! emitted when the button is pressed
    Signal0 pressed;

    //! emitted when the button is released
    Signal0 released;

    //! emitted when the button is clicked
    Signal0 clicked;

    //! emitted when the button changes toggle status
    Signal1<bool> toggled;


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
protected:
    //! \brief Normally a button will be released if the mouse leaves the button while
    //! it is pressed. This is not always the way to go, so we can call this to
    //! keep the button pressed even when the mouse moves far away from the
    //! button.
    BasicButton& keepPressedOnExit();

    //! \brief A function which may be used by derived classes for a standard button
    //! appearance.
    BasicButton& drawStandardButton();

    //! react to the button being pressed with the left mouse button
    BasicButton& onPressedLeft(); // for toggle handling

    //! the mouse listener handling pressing, releasing, roll overs
    ButtonMouseListener* _buttonMouseListener;

    State state_;
    int captureCount_;
    bool keepPressedOnExit_;
    bool isToggleOn_;
    bool isMouseOver_; // used only if isToggleOn_
    BasicButtonGroup* group_;

    ButtonDesc* renderDesc_;

private:
    friend class ButtonMouseListener;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The RenderDesc class for a button state
class ButtonStateDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    ButtonStateDesc();

    //! Constructor
    ButtonStateDesc(RenderDesc* aBackground, RenderDesc* aBorder);

    //! Destructor
    virtual ~ButtonStateDesc();


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ----------------------------------------------------------------------------------------------
public:
    //! sets the button object that will be drawn on the next call to draw
    const ButtonStateDesc& setButton(const BasicButton& aButton) const
    {
        button_ = &aButton;
        return *this;
    }

    //! draws on the passed in area according to the descriptor
    virtual void draw(const Core::Rectangle& area) const;

    //! Returns a reference to the text desc
    TextDesc& textDesc() const { return *textDesc_; }

    //! Returns the color of the text (black if there is no text desc)
    Color textColor() const;


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    RenderDesc* background_;
    RenderDesc* border_;
    TextDesc*   textDesc_;

    mutable const BasicButton* button_;


    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive &
            TAGGED_OBJECT(background) &
            TAGGED_OBJECT(border) &
            TAGGED_OBJECT(textDesc);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The RenderDesc class for a button
class ButtonDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Data Types
    // ---------------------------------------------------------------------------------------------
public:
    enum Arrow { none = 0, up, down, left, right };

    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    ButtonDesc();

    //! Destructor
    virtual ~ButtonDesc();

    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ----------------------------------------------------------------------------------------------
public:
    //! draws on the passed in area according to the desc
    virtual void draw(const Core::Rectangle& area) const;

    //! sets a kind of arrow to be drawn on the next call to draw
    ButtonDesc& setArrow(Arrow anArrow)
    {
        arrow_ = anArrow;
        return *this;
    }

    //! sets the button object that will be drawn on the next call to draw
    ButtonDesc& setButton(const BasicButton& aButton)
    {
        button_ = &aButton;
        return *this;
    }

    //! Returns the size of the text associated with the released (default) state
    Core::Vector2 defaultTextSize() const;

    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    ButtonStateDesc pressedDesc_;
    ButtonStateDesc releasedDesc_;
    ButtonStateDesc rollOverDesc_;
    ButtonStateDesc rollOverOnDesc_;
    mutable Arrow arrow_;
    const BasicButton* button_;
    

    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive &
            TAGGED_OBJECT(pressedDesc) &
            TAGGED_OBJECT(releasedDesc) &
            TAGGED_OBJECT(rollOverDesc) &
            TAGGED_OBJECT(rollOverOnDesc);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


} // namespace

#endif
