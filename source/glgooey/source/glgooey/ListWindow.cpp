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

#include "mmgr/mmgr.h"

#include "glgooey/ListWindow.h"

#include "glgooey/MouseMessage.h"
#include "glgooey/SelectionMessage.h"
#include "glgooey/WindowManager.h"
#include "glgooey/ScrollBar.h"
#include "glgooey/core/Rectangle.h"
#include "glgooey/core/StandardException.h"
#include "glgooey/core/Alignment.h"
#include "glgooey/SolidSurfaceDesc.h"
#include "glgooey/core/XMLOutputArchive.h"
#include "glgooey/core/XMLInputArchive.h"


// **************************************************************************************************
namespace Gooey
{


// **************************************************************************************************
//                            ListWindowMouseListener
// **************************************************************************************************


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Listener enabling single selection via the mouse
class ListWindowMouseListener : public MouseListener
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! constructor requires the list window the listener applies to
    ListWindowMouseListener(ListWindow& aListWindow) : listWindow_(aListWindow)
    {
    }


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! react to mouse clicks by selecting the clicked row
    virtual bool clicked(MouseClickMessage& message);

    //! react to mouse move by forgetting the last row under mouse cursor
    virtual bool exited(MouseMessage& message);

    //! react to mouse move by indicating the row under mouse cursor
    virtual bool moved(MouseMessage& message);


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    ListWindow& listWindow_;

    // Copying and assignment are not allowed
    ListWindowMouseListener(const ListWindowMouseListener& aListWindowMouseListener);
    ListWindowMouseListener& operator = (const ListWindowMouseListener& aListWindowMouseListener);
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// **************************************************************************************************
//                                ListWindow
// **************************************************************************************************



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListWindow::ListWindow(Window* aParent, const Core::Rectangle& rect) :
    Window(aParent),
    selectedIndex_(-1),
    highlightedIndex_(-1),
    lineHeightFactor_(1.325f)
{
    _verticalScrollBar = new ScrollBar(this, ScrollBar::vertical,
        rect.bottomRight() - Core::Vector2(0, 0), size().y()
        );

    (*_verticalScrollBar)
        .setMinimum(0)
        .setMaximum(1)
        .setValue(0)
        ;

    // instantiate the listeners
    listWindowMouseListener_ = new ListWindowMouseListener(*this);
    addMessageListener(listWindowMouseListener_);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListWindow::~ListWindow()
{
    removeMessageListener(listWindowMouseListener_);
    delete listWindowMouseListener_;
    delete _verticalScrollBar;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
ListWindow::process(const Message& message)
{
    bool ret = false;
    if(isVisible() && isEnabled())
        ret = _verticalScrollBar->process(message);

    if(ret)
        highlight(-1);
    else
        Window::process(message);

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListWindow&
ListWindow::positionScrollBar()
{
    setScrollBarParameters();

    const float barSize = _verticalScrollBar->width();
    _verticalScrollBar->setPosition(position() + Core::Vector2(size().x() - barSize, 0.0f));
    _verticalScrollBar->setSize(Core::Vector2(barSize, size().y()));

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
unsigned int
ListWindow::firstVisibleEntry() const
{
    float ret = 0.5f + (float(_verticalScrollBar->value()) - (float(lineHeight()) * 0.5f)) / float(lineHeight());
    if(ret < 0.0) ret = 0.0;
    return (unsigned int)(ret);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
unsigned int
ListWindow::entryIndexAt(const Core::Vector2& p) const
{
    ASSERT( p.y() >= 0.0f );
    ASSERT( lineHeight() > 0.0f );
    unsigned int ret = firstVisibleEntry() + static_cast<unsigned int>(p.y() / lineHeight());
    if(ret >= numberOfEntries()) ret = numberOfEntries()-1;
    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Vector2
ListWindow::recommendedSize() const
{
    return Core::Vector2(150.0f, 150.0f);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
ListWindow::lineHeight() const
{
    return ceilf(entryHeight() * lineHeightFactor());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListWindow&
ListWindow::select(int index)
{
    if((index >= 0) && (index < int(numberOfEntries())))
    {
        if(selectedIndex_ != index)
        {
            process(SelectionMessage(Message::deselected, selectedIndex_));
            selectedIndex_ = index;
            process(SelectionMessage(Message::selected, selectedIndex_));
        }
    }
    else
    {
        process(SelectionMessage(Message::deselected, selectedIndex_));
        selectedIndex_ = -1;
    }
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ListWindow::onResize()
{
    positionScrollBar();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ListWindow::onMove()
{
    positionScrollBar();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
ListWindow::heightOfAllContents() const
{
    const float numStrings = (numberOfEntries() > 0) ? static_cast<float>(numberOfEntries()-1) : 0.0f;
    return (lineHeight() * 0.5f) + (numStrings * lineHeight());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
ListWindow::heightOfVisibleContents() const
{
    return (size().y() > heightOfAllContents()) ? heightOfAllContents() : size().y();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ListWindow::setScrollBarParameters()
{
    const int visibleAmount = static_cast<int>(heightOfVisibleContents());
    (*_verticalScrollBar)
        .setVisibleAmount(visibleAmount)
        .setUnitIncrement(static_cast<int>(lineHeight()))
        .setBlockIncrement(visibleAmount - static_cast<int>(lineHeight()))
        ;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ListWindow::onContentsChanged()
{
    _verticalScrollBar->setMaximum(static_cast<int>(heightOfAllContents()));

    setScrollBarParameters();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ListWindow::highlight(int anIndex)
{
    highlightedIndex_ = anIndex;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
ListWindow::containsFocus() const
{
    return Window::containsFocus() || _verticalScrollBar->containsFocus();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
ListWindow::doesHierarchyContain(Window* aWindow) const
{
    return Window::doesHierarchyContain(aWindow) || _verticalScrollBar->doesHierarchyContain(aWindow);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window*
ListWindow::windowBelow(const Core::Vector2& p)
{
    Window* ret = Window::windowBelow(p);

    if(ret != 0)
    {
        Window* cw = _verticalScrollBar->windowBelow(p);
        if (cw)
            ret=cw;
    }

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// **************************************************************************************************
//                            ListWindowMouseListener
// **************************************************************************************************




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
ListWindowMouseListener::clicked(MouseClickMessage& message)
{
    bool isHandled = false;
    if(message.button() == MouseClickMessage::left)
    {
        isHandled = true;
        if(message.event() == MouseClickMessage::pressed)
        {
            if(message.position().x() < listWindow_._verticalScrollBar->position().x())
            {
                const Core::Vector2 p = message.position() - listWindow_.position();
                const int newIndex = listWindow_.entryIndexAt(p);
                const int oldIndex = listWindow_.selectedIndex();
                if(newIndex != oldIndex)

                {
                    listWindow_.select(newIndex);
                    listWindow_.selectionChanged(newIndex);
                }
            }
        }
        else if(message.event() == MouseClickMessage::released)
        {
            listWindow_.leftClicked();
        }
    }
    else if(message.button() == MouseClickMessage::right)
    {
        isHandled = true;
        if(message.event() == MouseClickMessage::released)
        {
            listWindow_.rightClicked();
        }
    }

    return isHandled;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
ListWindowMouseListener::exited(MouseMessage& aMessage)
{
    bool isHandled = false;
    if(aMessage.position().x() < listWindow_._verticalScrollBar->position().x())
    {
        listWindow_.highlight(-1);
        isHandled = true;
    }

    return isHandled;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
ListWindowMouseListener::moved(MouseMessage& aMessage)
{
    bool isHandled = false;
    if(aMessage.position().x() < listWindow_._verticalScrollBar->position().x())
    {
        const Core::Vector2 p = aMessage.position() - listWindow_.position();
        const int index = listWindow_.entryIndexAt(p);
        listWindow_.highlight(index);
        isHandled = true;
    }
    return isHandled;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// --------------------------------------------------------------------------------------------------
//                            ListWindowDesc
// --------------------------------------------------------------------------------------------------





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListWindowDesc::ListWindowDesc() :
    background_(new SolidSurfaceDesc()),
    border_(new RectangleDesc()), textDesc_(new TextDesc()),
    selectionColor_(0.85f, 0.85f, 0.85f, 1.0f),
    selectedTextColor_(0.35f, 0.35f, 0.35f, 1.0f),
    highlightColor_(0.5f, 0.5f, 0.5f, 0.4f),
    highlightedTextColor_(0.85f, 0.85f, 0.85f, 1.0f)
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListWindowDesc::~ListWindowDesc()
{
    delete background_;
    delete border_;
    delete textDesc_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ListWindowDesc::draw(const Core::Rectangle& area) const
{
    textDesc_->setHorizontalAlignment(Core::Alignment::left);

    if(background_ != 0) background_->draw(area);

    Renderer::enableClipRectangle(area);

    const Core::Vector2 topLeft     = area.topLeft();
    const Core::Vector2 bottomRight = area.bottomRight();


    // ***********************************************************************
    //  draw the strings
    // ***********************************************************************

    // which string is at the top of the window
    const size_t topRow = listWindow_->firstVisibleEntry();

    const float lineHeight = listWindow_->lineHeight();
    ASSERT(lineHeight > 0.0f);

    // how many rows are visible
    const size_t numVisibleRows = static_cast<size_t>(area.size().y() / lineHeight);

    const float yOffs = floorf(lineHeight * 0.25f);
    float y = listWindow_->position().y() + yOffs;
    const size_t numRows = listWindow_->numberOfEntries();
    const size_t maxRow = (numRows < topRow+numVisibleRows) ? numRows : topRow+numVisibleRows;
    for(size_t curRow = topRow; curRow < maxRow; ++curRow)
    {
        Core::Rectangle rowRectangle(Core::Vector2(topLeft.x(), y-yOffs),
                Core::Vector2(bottomRight.x() - listWindow_->verticalScrollBar()->size().x(),
                y+lineHeight-yOffs));

        drawRow(curRow, rowRectangle);

        y += lineHeight;
    }

    Renderer::disableClipRectangle();

    if(border_ != 0) border_->draw(area);

    Renderer::setCurrentColor(Color(1, 1, 1, 1));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// **************************************************************************************************
} // namespace Gooey

