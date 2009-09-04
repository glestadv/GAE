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
//
// !! NOTE !! The SpinBox window is largely the work of Mark Harris
// __________________________________________________________________________________________________

#ifndef SPINBOX__H
#define SPINBOX__H

#include "mmgr/mmgr.h"

#include "glgooey/Slideable.h"
#include "glgooey/Timer.h"


// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    class EditField;
    class SpinBoxButton;
}
// *************************************************************************************************





// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! A "typical" GUI integer SpinBox with up and down buttons.
class SpinBox : public Slideable
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! constructor
    //! \param aParent the parent window
    //! \param initialValue the initial value in the spin box' edit field
    SpinBox(Window* aParent, int initialValue = 0);

    //! destructor
    virtual ~SpinBox();


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    // Commented in base class
    virtual bool process(const Message& message);
    virtual Core::Vector2 recommendedSize() const;
    virtual Window& loadAppearance(const std::string& aFileName, const std::string& aWindowName);


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //! render the window to the current OpenGL context
    virtual Window& draw();

    //! called when the window is resized (rearranges the children)
    virtual void onResize();

    //! called when the window is moved (rearranges the children)
    virtual void onMove();

    //! Called when the window receives the focus
    virtual void onGainedFocus();

    //! Called when the window loses the focus
    virtual void onLostFocus();

    //! Returns a pointer to the window below the passed in point
    virtual Window* windowBelow(const Core::Vector2& p);

    //!  put all the bits in the right place after the geometry of the bar has
    //! been changed
    SpinBox& arrange();

    //! slot reacts to the increment button being pressed
    void incrementButtonPressed();

    //! slot reacts to the decrement button being pressed
    void decrementButtonPressed();

    //! slot reacts to the increment button being released
    void incrementButtonReleased();

    //! slot reacts to the decrement button being released
    void decrementButtonReleased();

    //! slot reacts to a return press after text is edited in the edit field.
    void textEdited();

    //! Called by the button press slots to update the value display
    void updateDisplay();

    //! Called by the FocusListener to focus the edit field when clicked.
    void focusEditField();

    //! Called by the FocusListener to unfocus the edit field when something
    //! else is clicked.
    void unFocusEditField();

    //! the scroll bar button for incrementing the value
    SpinBoxButton* incrementButton_;

    //! the scroll bar button for decrementing the value
    SpinBoxButton* decrementButton_;

    //! the text field that displays the current value
    EditField*     editField_;

    //! The timer which allows us to keep moving the bar when the increment button is held pressed
    Timer incrementTimer_;

    //! The timer which allows us to keep moving the bar when the decrement button is held pressed
    Timer decrementTimer_;

private:
    bool isTextNumeric();

    friend class SpinBoxMouseListener;
    SpinBoxMouseListener* mouseListener_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
