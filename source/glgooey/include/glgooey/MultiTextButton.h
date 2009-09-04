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
#ifndef MULTITEXTBUTTON__H
#define MULTITEXTBUTTON__H

#include "mmgr/mmgr.h"

#include "glgooey/BasicButton.h"
#include "glgooey/core/StandardException.h"




// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    class MTButtonMouseListener;
    class MTBLeftButtonListener;
}
// *************************************************************************************************



// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief A button control allowing single selection
//!
//! A button into which multiple strings can be added. By clicking on the button the list of
//! strings is traversed one by one. The currently active string ist the window text (accessible
//! via the text() method).
class MultiTextButton : public BasicButton
{
    // ---------------------------------------------------------------------------------------------
    //  Internal Data types
    // ---------------------------------------------------------------------------------------------
private:
    typedef std::list<std::string> Strings;


    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief Constructor
    //!
    //! Requires the parent window and the text which initially
    //! labels the button
    MultiTextButton(Window* aParent, const std::string& aText);

    //! destructor
    virtual ~MultiTextButton();


    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! add a string to the button's list of strings
    MultiTextButton& addString(const std::string& aString)
    {
        strings_.push_back(aString);
        return *this;
    }

    //! the number of strings that the button contains
    unsigned int numberOfStrings() const { return int(strings_.size()); }

    //! returns the string at the passed in index
    std::string stringAt(int index) const
    {
        Strings::const_iterator it = strings_.begin();
        std::advance(it, index);
        return *it;
    }



    // ---------------------------------------------------------------------------------------------
    //! \name Signals
    // ---------------------------------------------------------------------------------------------
public:
    //! emitted when the selected text changes
    Signal1<const std::string&> selectionChanged;


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //! render the window to the current OpenGL context
    virtual Window& draw();

    //! react to the button being pressed with the right mouse button
    MultiTextButton& onPressedRight();

    //! react to the button being pressed with the left mouse button
    MultiTextButton& onPressedLeft();

    //! the index of the currently active text
    unsigned long stringIndex_;

    friend class MTButtonMouseListener;

    MTButtonMouseListener* mtButtonMouseListener_;

private:
    Strings strings_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
