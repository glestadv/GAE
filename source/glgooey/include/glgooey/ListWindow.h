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
#ifndef LISTWINDOW__H
#define LISTWINDOW__H

#include "mmgr/mmgr.h"

#include "glgooey/Window.h"
#include "glgooey/RenderDesc.h"




// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    class ScrollBar;
    class ListWindowMouseListener;
    class TextDesc;
}
// *************************************************************************************************





// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief A list window maintaining a list of strings
class ListWindow : public Window
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief construction requires the parent and the window rect defining
    //! the position and size of the list window
    ListWindow(Window* aParent, const Core::Rectangle& rect);

    //! destructor
    virtual ~ListWindow();


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! returns the index of the selected item (-1 if no item is selected)
    int selectedIndex() const { return selectedIndex_; }

    //! returns the index of the highlighted item (-1 if no item is selected)
    int highlightedIndex() const { return highlightedIndex_; }

    //! sets the index of the selected item
    ListWindow& select(int index);

    //! Returns the number of entries in the list
    virtual size_t numberOfEntries() const = 0;


    // ---------------------------------------------------------------------------------------------
    //  Signals
    // ---------------------------------------------------------------------------------------------
public:
    //! Emitted when the user clicks with the left mouse button on the list window window
    Signal0 leftClicked;

    //! Emitted when the user clicks with the right mouse button on the list window window
    Signal0 rightClicked;

    //! \brief Emitted when the selection in the list window changes
    //!
    //! Slots connected to this signal must accept one integer parameter accepting
    //! the index of the item selected. E.g.:
    //! \code void onSelectionChanged(int newIndex); \endcode
    Signal1<int> selectionChanged;



    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! The height from the bottom of one line to the bottom of the next
    float lineHeight() const;

    //! Returns the factor by which an entry height is multiplied in order to calculate the line height
    float lineHeightFactor() const { return lineHeightFactor_; }

    //! Sets the factor by which an entry height is multiplied in order to calculate the line height
    ListWindow& setLineHeightFactor(float aFactor) { lineHeightFactor_ = aFactor; return *this; }

    //! \brief Make the window attempt to process the passed in message
    //! we override this to send messages on to the scroll bar
    virtual bool process(const Message& message);

    //! Returns the recommended dimensions of the window in pixels
    virtual Core::Vector2 recommendedSize() const;

    //! Returns a pointer to the vertical scroll bar
    const ScrollBar* verticalScrollBar() const { return _verticalScrollBar; }

    //! the index of the top most visible entry
    unsigned int firstVisibleEntry() const;

    //! Sets the scroll bar parameters according to what is currently visible
    void setScrollBarParameters();

    // Commented in base class
    virtual bool containsFocus() const;
    virtual Window* windowBelow(const Core::Vector2& p);
    virtual bool doesHierarchyContain(Window* aWindow) const;


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //!  called when the window is resized  (resizes the scroll bar)
    virtual void onResize();

    //!  called when the window is moved (repositions the scroll bar)
    virtual void onMove();

    //! Returns the height of text entries
    virtual float entryHeight() const = 0;

    //! Must be called by derived classes when the contents of the list change
    void onContentsChanged();

    //! returns the index of the entry at the passed in (local) point
    unsigned int entryIndexAt(const Core::Vector2& p) const;

    //! Highlights the entry at the passed in index
    void highlight(int anIndex);

    //! positions the scroll bar at the side of the window
    ListWindow& positionScrollBar();

    //! the vertical scroll bar
    ScrollBar* _verticalScrollBar;

    //! returns the total height of the contents of the list window
    float heightOfAllContents() const;

    //! returns the height of the visible contents of the list window
    float heightOfVisibleContents() const;

private:
    friend class ListWindowMouseListener;

    ListWindowMouseListener*  listWindowMouseListener_;

    int selectedIndex_;

    int highlightedIndex_;

    float lineHeightFactor_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<










// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The render desc class for list windows
class ListWindowDesc : public RenderDesc
{
    // ----------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ----------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    ListWindowDesc();

    //! Destructor
    virtual ~ListWindowDesc();


    // ----------------------------------------------------------------------------------------------
    //  Public Interface
    // ----------------------------------------------------------------------------------------------
public:
    //! draws on the passed in area according to the desc
    virtual void draw(const Core::Rectangle& area) const;


    //! Sets the list window to be rendered on the next call to draw
    ListWindowDesc& setListWindow(ListWindow* aListWindow)
    {
        listWindow_ = aListWindow;
        return *this;
    }

    //! Returns a reference to the text desc
    TextDesc& textDesc() { return *textDesc_; }


    // ----------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
protected:
    //! Derived classes override this to draw a row
    virtual void drawRow(size_t aRowIndex, const Core::Rectangle& aRowRectangle) const = 0;

    RenderDesc* background_;
    RenderDesc* border_;
    TextDesc*   textDesc_;
    Color       selectionColor_;
    Color       selectedTextColor_;
    Color       highlightColor_;
    Color       highlightedTextColor_;
    ListWindow* listWindow_;


    // ----------------------------------------------------------------------------------------------
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
            TAGGED_OBJECT(textDesc) &
            TAGGED_OBJECT(selectionColor) &
            TAGGED_OBJECT(selectedTextColor) &
            TAGGED_OBJECT(highlightColor) &
            TAGGED_OBJECT(highlightedTextColor);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// *************************************************************************************************
} // namespace Gooey



#endif
