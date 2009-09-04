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
#ifndef LISTCONTROL__H
#define LISTCONTROL__H

#include "mmgr/mmgr.h"

#include "ListWindow.h"





// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    class ListControlDesc;
    class TextDesc;
}
// *************************************************************************************************





// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief A list of strings which can be arranged in columns
//! \bug Strings overlap if they are too wide for a column
class ListControl : public ListWindow
{
    // ---------------------------------------------------------------------------------------------
    //  Data Types
    // ---------------------------------------------------------------------------------------------
public:
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //! \brief A row of list control entries
    class Row
    {
        // -----------------------------------------------------------------------------------------
        //  Data Types
        // -----------------------------------------------------------------------------------------
    public:
        typedef std::vector<std::string> Strings;

        // -----------------------------------------------------------------------------------------
        // Construction / Destruction
        // -----------------------------------------------------------------------------------------
    public:
        //! constructor requires reference to associated list control
        Row(ListControl& aListControl) : listControl_(aListControl)
        {
            strings_.resize(aListControl.numberOfColumns());
        }

        //! Copy Constructor
        Row(const Row& aRow) : listControl_(aRow.listControl_)
        {
            strings_ = aRow.strings_;
        }

        // -----------------------------------------------------------------------------------------
        //  Public Interface
        // -----------------------------------------------------------------------------------------
    public:
        //! sets the value of the entry in the specified column
        Row& setEntry(unsigned int aColumn, const std::string& aString)
        {
            ASSERT ( aColumn < listControl_.numberOfColumns() );
            strings_[aColumn] = aString;

            return *this;
        }

        //! returns the string in the passed in column
        const std::string& stringAt(int aColumn) const
        {
            return strings_[aColumn];

        }


        // -----------------------------------------------------------------------------------------
        //  Implementation
        // -----------------------------------------------------------------------------------------
    private:
        ListControl& listControl_;
        Strings strings_;

        // Assignment is not allowed
        Row& operator = (const Row& aRow);
    };
    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



    //! linked list of rows
    typedef std::list<Row> Rows;




    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief construction
    //!
    //! \param aParent the parent window
    //! \param rect the window rectangle defining the position and size
    //!        of the list control.
    //! \param numColumns The number of columns in the list control. This \em
    //!        cannot be altered at run time. The constructor sets all column widths
    //!        to the width of the control divided by the number of columns.
    //!        The widths can be changed at run time using the setColumnWidths
    //!        method.
    //! \sa setColumnWidths
    ListControl(Window* aParent, const Core::Rectangle& rect, unsigned int numColumns);

    //! destructor
    virtual ~ListControl();




    // ---------------------------------------------------------------------------------------------
    //  Signals
    // ---------------------------------------------------------------------------------------------
public:
    //! Emitted when the user clicks with the left mouse button on the list box window
    Signal0 leftClicked;

    //! Emitted when the user clicks with the right mouse button on the list box window
    Signal0 rightClicked;

    //! \brief Emitted when the selection in the list control changes
    //!
    //! Slots connected to this signal must accept one integer parameter
    //! accepting the index of the item selected. E.g.:
    //! \code void onSelectionChanged(int newIndex); \endcode
    Signal1<int> selectionChanged;



    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Returns the number of columns maintained by the control
    unsigned int numberOfColumns() const { return int(columnWidths_.size()); }

    //! \brief Sets the widths of the columns.
    //!
    //!  If the number of widths passed in does not equal the number
    //! of columns, this method will throw an assertion exception
    ListControl& setColumnWidths(const std::vector<float>& widths);

    //! Returns the width of the column with the passed in index
    float columnWidth(unsigned int index) const { return columnWidths_[index]; }

    //! Adds a row containing the passed in strings to the list control
    ListControl& addRow(const std::vector<std::string>& strings);

    //! Remove the row at the specified position from the list control
    ListControl& removeRowAt(unsigned int pos);

    //! Returns the number of strings maintained by the list control
    unsigned int numberOfRows() const { return int(rows_.size()); }

    // COmmented in base class
    size_t numberOfEntries() const { return numberOfRows(); }

    //! \brief Retrieve a row - an exception is thrown if the position is not valid,
    //! so make sure the passed in index less than numberOfRows()
    const Row& rowAt(unsigned int pos) const;

    // Commented in base class
    virtual Window& loadAppearance(const std::string& aFileName, const std::string& aWindowName);


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //! Render the window to the current OpenGL context
    virtual Window& draw();

    //!  called when the window is resized
    virtual void onResize();

    //! Returns the index of the column at the passed in (local) point
    unsigned int columnIndexAt(const Core::Vector2& p) const;

    virtual float entryHeight() const;

    //! The strings maintained by the list control
    Rows rows_;

    //! The widths of the columns
    std::vector<float> columnWidths_;

    //! If the column widths have not been set by the user, this will be true
    bool usingDefaultWidths_;

    //! Sets the column sizes according to the available space
    Window& resizeColumns();

private:
    ListControlDesc* renderDesc_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The render desc class for list controls
class ListControlDesc : public ListWindowDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    ListControlDesc();

    //! Destructor
    virtual ~ListControlDesc();



    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! draws on the passed in area according to the desc
    virtual void draw(const Core::Rectangle& anArea) const;


    // ----------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
protected:
    virtual void drawRow(size_t aRowIndex, const Core::Rectangle& aRowRectangle) const;

private:
    float separatorWidth_;
    Color separatorColor_;



    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        ListWindowDesc::serialize(anArchive);
        anArchive &
            TAGGED_OBJECT(separatorWidth) &
            TAGGED_OBJECT(separatorColor);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey



#endif
