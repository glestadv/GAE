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
#ifndef CHECKBOX__H
#define CHECKBOX__H

#include "mmgr/mmgr.h"

#include "glgooey/Button.h"
#include "glgooey/RenderDesc.h"


// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    class CheckBoxDesc;
}
// *************************************************************************************************




// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief A check box which is basically a button that toggles with support for a "check" mark to
//! show the toggle status
class CheckBox : public BasicButton
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief Constructor
    //!
    //! The constructor requires the parent and the text with which the check box
    //! is to be labeled. If a check box is arranged in a group of
    //! mutually exclusive check boxes (radio buttons), then the associated
    //! group must also be supplied.
    //! \see CheckBoxGroup
    CheckBox(Window* aParent, const std::string& aText, BasicButtonGroup* aGroup = 0);

    //! destructor
    virtual ~CheckBox();


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! returns whether the check box is currently checked
    bool isChecked() const { return isMouseOver(); }

    //! checks the check box
    CheckBox& enableChecked();

    //! "unchecks" the check box
    CheckBox& disableChecked();

    //! toggles the check box
    CheckBox& toggle();

    // Commented in base class
    virtual Core::Vector2 recommendedSize() const;
    virtual Window& loadAppearance(const std::string& aFileName, const std::string& aWindowName);

    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //! render the window to the current OpenGL context
    virtual Window& draw();

private:
    CheckBoxDesc* renderDesc_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief A render desc for the check mark in check boxes
class CheckDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    CheckDesc();

    //! Destructor
    virtual ~CheckDesc();


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! draws on the passed in area according to the descriptor
    virtual void draw(const Core::Rectangle& area) const;

    //! sets the check box object that will be drawn on the next call to draw
    CheckDesc& setCheckBox(CheckBox* aCheckBox)
    {
        checkBox_ = aCheckBox;
        return *this;
    }



    // ----------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    CheckBox*   checkBox_;
    RenderDesc* checked_;
    RenderDesc* unchecked_;



    // ----------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive & TAGGED_OBJECT(checked) & TAGGED_OBJECT(unchecked);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The render desc class for check boxes
class CheckBoxDesc : public ButtonDesc
{
    // ----------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ----------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    CheckBoxDesc() : checkDesc_(new CheckDesc) {}

    //! Destructor
    virtual ~CheckBoxDesc()
    {
        delete checkDesc_;
    }

    // ----------------------------------------------------------------------------------------------
    //  Public Interface
    // ----------------------------------------------------------------------------------------------
public:
    //! draws on the passed in area according to the desc
    virtual void draw(const Core::Rectangle& area) const;


    //! sets the check box object that will be drawn on the next call to draw
    CheckBoxDesc& setCheckBox(CheckBox* aCheckBox)
    {
        checkBox_ = aCheckBox;
        return *this;
    }


    // ----------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    CheckDesc* checkDesc_;
    CheckBox*  checkBox_;



    // ----------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        ButtonDesc::serialize(anArchive);
        anArchive & TAGGED_OBJECT(checkDesc);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
