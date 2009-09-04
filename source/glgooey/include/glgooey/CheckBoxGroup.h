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
#ifndef CHECKBOXGROUP__H
#define CHECKBOXGROUP__H


#include "mmgr/mmgr.h"

#include "glgooey/BasicButtonGroup.h"

// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    class CheckBox;
}
// *************************************************************************************************


// *************************************************************************************************
namespace Gooey
{

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief forces the checkboxes contained within the group to be mutually exclusive (radio-button
//!  behavior)
//!
//! Use an instance of this class to implement radio buttons. Construction
//! of an instance of CheckBox accepts a parameter which is a pointer to
//! an instance of CheckBoxGroup. By passing the same group to a number
//! of check boxes, the check boxes act as radio buttons. E.g.:
//! \code
//! CheckBoxGroup* radioButtons = new CheckBoxGroup;
//!
//! CheckBox* radioButton1 = new CheckBox(0, "Radio Button 1", radioButtons);
//! CheckBox* radioButton2 = new CheckBox(0, "Radio Button 2", radioButtons);
//! CheckBox* radioButton3 = new CheckBox(0, "Radio Button 3", radioButtons);
//! CheckBox* radioButton4 = new CheckBox(0, "Radio Button 4", radioButtons);
//! \endcode
//! will result in four mutually exclusive check boxes.
class CheckBoxGroup : public BasicButtonGroup
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! default constructor
    CheckBoxGroup() {}

    //! destructor
    virtual ~CheckBoxGroup() {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! returns a pointer to the selected check box
    CheckBox* selectedCheckBox() const;

    //! selects the passed in check box
    CheckBoxGroup& select(CheckBox* checkBox);

    //! returns the index of the passed in check box
    unsigned int indexOf(CheckBox* checkBox) const;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace

#endif
