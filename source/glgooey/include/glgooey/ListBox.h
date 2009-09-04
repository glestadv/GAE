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
#ifndef LISTBOX__H
#define LISTBOX__H

#include "mmgr/mmgr.h"

#include "ListWindow.h"
#include "glgooey/MouseMessage.h"






// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    class ListBoxDesc;
    class TextDesc;
}
// *************************************************************************************************





// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! A list box maintaining a list of strings
class ListBox : public ListWindow
{
    // ----------------------------------------------------------------------------------------------
    //  Data Types
    // ----------------------------------------------------------------------------------------------
public:
    typedef std::list<std::string> Strings;


    // ----------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ----------------------------------------------------------------------------------------------
public:
    //! \brief construction requires the parent and the window rect defining
    //! the position and size of the list box
    ListBox(Window* aParent, const Core::Rectangle& rect);

    //! destructor
    virtual ~ListBox();



    // ----------------------------------------------------------------------------------------------
    //  Public Interface
    // ----------------------------------------------------------------------------------------------
public:
    //! Add a string to the list box
    ListBox& addString(const std::string& aString);

    //! Remove the string at the specified position from the list box
    ListBox& removeStringAt(size_t pos);

    //! Removes all strings from the list box
    ListBox& removeAllStrings();

    //! \brief Returns the string at the specified position. (The returned string is empty
    //! if the passed in position is invalid).
    std::string stringAt(size_t position) const;

    //! Returns the number of strings maintained by the list box
    size_t numberOfStrings() const { return strings_.size(); }

    // Commented in base class
    virtual size_t numberOfEntries() const { return numberOfStrings(); }

    //! Returns \c true if the list box contains the passed in string
    bool contains(const std::string& aString) const;

    //! Returns the index of the passed in string
    unsigned int indexOf(const std::string& aString) const;

    // Commented in base class
    virtual Window& loadAppearance(const std::string& aFileName, const std::string& aWindowName);


    // ----------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
protected:
    //! render the window to the current OpenGL context
    virtual Window& draw();

    // COmmented in base class
    virtual float entryHeight() const;


private:
    Strings strings_;  //!< the strings maintained by the list box

    ListBoxDesc* renderDesc_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The render desc class for list windows
class ListBoxDesc : public ListWindowDesc
{
    // ----------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ----------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    ListBoxDesc() {}

    //! Destructor
    virtual ~ListBoxDesc() {}


    // ----------------------------------------------------------------------------------------------
    //  Public Interface
    // ----------------------------------------------------------------------------------------------
public:
    //! draws on the passed in area according to the desc
    virtual void draw(const Core::Rectangle& anArea) const
    {
        ListWindowDesc::draw(anArea);
    }



    // ----------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
protected:
    virtual void drawRow(size_t aRowIndex, const Core::Rectangle& aRowRectangle) const;


    // ----------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        ListWindowDesc::serialize(anArchive);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// *************************************************************************************************
} // namespace Gooey



#endif
