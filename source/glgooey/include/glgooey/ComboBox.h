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
#ifndef COMBOBOX__H
#define COMBOBOX__H

#include "mmgr/mmgr.h"

#include "glgooey/Window.h"
#include "glgooey/RenderDesc.h"

// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    class ListBox;
    class ComboBoxMouseListener;
    class ComboBoxDesc;
    class TextDesc;
}
// *************************************************************************************************




// *************************************************************************************************
namespace Gooey
{



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief A combo box control
//!
//! The GLGooey combo box is fairly typical. If it is clicked, a list of options
//! opens from which the user may select exactly one. The current selection is
//! always visible.
class ComboBox : public Window
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief construction requires the parent and the window rect defining
    //! the position and size of the combo box
    ComboBox(Window* aParent, const Core::Rectangle& rect);

    //! Destructor
    virtual ~ComboBox();


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! Add a string to the end of the list of combo box options
    ComboBox& addString(const std::string& aString);

    //! \brief Returns the string at the specified position. (The returned string is empty
    //! if the passed in position is invalid).
    std::string stringAt(unsigned int position) const;

    //! Returns the string that is currently selected
    std::string selectedString() const;

    //! remove the string at the specified position from the combo box
    ComboBox& removeStringAt(unsigned int position);

    //! removes all strings from the combo box
    ComboBox& removeAllStrings();

    //! \brief Returns the index of string that is currently selected (-1 is returned if
    //! no selection has been made yet)
    int selectedIndex() const;

    //! returns the number of strings maintained by the combo box
    unsigned int numberOfStrings() const;


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief Selects the passed in string - if the string is not yet in the combo
    //! box's list of strings, it is automatically appended to the list.
    ComboBox& selectString(const std::string& aString);

    //!  Selects the string at the passed in position in the string list.
    ComboBox& selectStringAt(unsigned int position);

    //! \brief make the window attempt to process the passed in message
    //! we override this to send messages on to the list box
    virtual bool process(const Message& message);

    //! returns the recommended dimensions of the window in pixels
    virtual Core::Vector2 recommendedSize() const;

    virtual Window& loadAppearance(const std::string& aFileName, const std::string& aWindowName);

    //! Returns true if the drop down list box is currently open
    bool isDropDownOpen() const { return isDropDownOpen_; }


    // ---------------------------------------------------------------------------------------------
    //  Signals
    // ---------------------------------------------------------------------------------------------
public:
    //! Signal emitted when the selected text changes
    Signal1<const std::string&> selectionChanged;

    //! Signal emitted when the drop down list is opened
    Signal0 listOpened;

    //! Signal emitted when the drop down list is closed
    Signal0 listClosed;



    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //! render the window to the current OpenGL context
    virtual Window& draw();

    //! Called when the window loses the focus
    virtual void onLostFocus();

private:
    //!  Opens the combo box's drop down list box
    ComboBox& openDropDown();

    //!  Opens the combo box's drop down list box
    ComboBox& closeDropDown();

    friend class ComboBoxMouseListener;
    ListBox* listBox_;
    ComboBoxMouseListener* comboBoxMouseListener_;
    ComboBoxDesc* renderDesc_;
    bool isDropDownOpen_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The render desc class for combo boxes
class ComboBoxDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    ComboBoxDesc();

    //! Destructor
    virtual ~ComboBoxDesc();


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Draws on the passed in area according to the desc
    virtual void draw(const Core::Rectangle& area) const;

    //! Sets the edit field to be rendered on the next call to draw
    ComboBoxDesc& setComboBox(ComboBox* aComboBox)
    {
        comboBox_ = aComboBox;
        return *this;
    }

    //! Returns a reference to the text desc
    TextDesc& textDesc() { return *textDesc_; }


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    RenderDesc* background_;
    RenderDesc* border_;
    TextDesc*   textDesc_;
    ComboBox*   comboBox_;


    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive & TAGGED_OBJECT(background) & TAGGED_OBJECT(border) & TAGGED_OBJECT(textDesc);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
