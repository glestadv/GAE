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

#include <algorithm>
#include <numeric>

#include "mmgr/mmgr.h"

#include "glgooey/ListControl.h"

#include "glgooey/MouseMessage.h"
#include "glgooey/SelectionMessage.h"
#include "glgooey/Renderer.h"
#include "glgooey/WindowManager.h"
#include "glgooey/PropertyScheme.h"
#include "glgooey/ScrollBar.h"
#include "glgooey/core/Rectangle.h"
#include "glgooey/SolidSurfaceDesc.h"
#include "glgooey/core/XMLOutputArchive.h"
#include "glgooey/core/XMLInputArchive.h"


// **************************************************************************************************
namespace Gooey
{

REGISTER_CLASS(ListControlDesc);

// **************************************************************************************************
//                            ListControl
// **************************************************************************************************


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListControl::ListControl(Window* aParent, const Core::Rectangle& rect, unsigned int numColumns) :
        ListWindow(aParent, rect),
        usingDefaultWidths_(true),
        renderDesc_(0)
{
    // set the default column widths
    columnWidths_.resize(numColumns);
    for(unsigned i = 0; i < numColumns; ++i)
    {
        columnWidths_[i] = 1;
    }

    setPosition(rect.bottomLeft());
    setSize(rect.size());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListControl::~ListControl()
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListControl&
ListControl::addRow(const std::vector<std::string>& strings)
{
    ASSERT ( strings.size() <= numberOfColumns() );

    Row newRow(*this);
    for(unsigned i = 0; i < strings.size(); ++i)
    {
        newRow.setEntry(i, strings[i]);
    }

    rows_.push_back(newRow);

    onContentsChanged();

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListControl&
ListControl::removeRowAt(unsigned int pos)
{
    Rows::iterator it;
    unsigned int counter = 0;
    for(it = rows_.begin(); it != rows_.end(); ++it)
    {
        if(counter == pos) break;
        counter++;
    }

    if(counter < numberOfRows()) rows_.erase(it);

    onContentsChanged();

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListControl&
ListControl::setColumnWidths(const std::vector<float>& widths)
{
    ASSERT ( widths.size() == numberOfColumns() );

    std::vector<float>::const_iterator it;
    int index = 0;
    for(it = widths.begin(); it != widths.end(); ++it)
    {
        columnWidths_[index++] = *it;
    }

    usingDefaultWidths_ = false;

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
unsigned int
ListControl::columnIndexAt(const Core::Vector2& p) const
{
    unsigned int ret = 0;
    for(unsigned i = 0; i < numberOfColumns()-1; ++i)
    {
        if( (p.x() >= long(columnWidths_[i])) && (p.x() < long(columnWidths_[i+1])) ) break;
        ret++;
    }
    if(ret >= numberOfColumns()) ret = numberOfColumns()-1;

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
ListControl::resizeColumns()
{
    const float clientWidth = (size().x() - 2.0f);
    if(usingDefaultWidths_)
    {
        // set the default column widths
        const float defaultWidth = floorf(clientWidth / numberOfColumns());
        for(unsigned int i = 0; i < numberOfColumns()-1; ++i)
        {
            columnWidths_[i] = defaultWidth;
        }
        columnWidths_[numberOfColumns()-1] = 0.0f;
        columnWidths_[numberOfColumns()-1] = std::accumulate(columnWidths_.begin(), columnWidths_.end(), 0.0f);
    }
    else
    {
        const float totalWidth = std::accumulate(columnWidths_.begin(), columnWidths_.end(), 0.0f);
        columnWidths_[numberOfColumns()-1] = clientWidth - totalWidth;
    }

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
ListControl::draw()
{
    ListControlDesc* renderDesc = renderDesc_;
    if(renderDesc == 0)
        renderDesc = &WindowManager::instance().propertyScheme().listControlDesc_;

    const Core::Vector2 topLeft = position();
    const Core::Vector2 bottomRight = position() + size();
    Core::Rectangle rect(topLeft, bottomRight);

    renderDesc->setListWindow(this);
    renderDesc->draw(rect);

    _verticalScrollBar->render();

    Renderer::setCurrentColor(Color(1, 1, 1, 1));

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
ListControl::entryHeight() const
{
    ListControlDesc* renderDesc = renderDesc_;
    if(renderDesc == 0)
        renderDesc = &WindowManager::instance().propertyScheme().listControlDesc_;
    return renderDesc->textDesc().lineHeight();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const ListControl::Row&
ListControl::rowAt(unsigned int pos) const
{
    if(pos >= numberOfRows())
        THROW("Tried to access the row of a ListControl with an invalid index");

    Rows::const_iterator it = rows_.begin();
    std::advance(it, pos);

    return *it;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ListControl::onResize()
{
    ListWindow::onResize();
    resizeColumns();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
ListControl::loadAppearance(const std::string& aFileName, const std::string& aWindowName)
{
    if(renderDesc_ == 0) renderDesc_ = new ListControlDesc;
    {
        Core::XMLInputArchive archive(aFileName);
        archive & Core::ObjectNamer<ListControlDesc>(aWindowName.c_str(), *renderDesc_);
    }
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// **************************************************************************************************
//                            ListControlDesc
// **************************************************************************************************




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListControlDesc::ListControlDesc() :
    separatorWidth_(1.0f),
    separatorColor_(Color(1, 1, 1, 0.75f))
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ListControlDesc::~ListControlDesc()
{
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ListControlDesc::draw(const Core::Rectangle& anArea) const
{
    ListWindowDesc::draw(anArea);

    // ***********************************************************************
    //  draw the column separators
    // ***********************************************************************

    const Core::Vector2 topLeft     = anArea.topLeft();
    const Core::Vector2 bottomLeft  = anArea.bottomLeft();

    ListControl* listControl = dynamic_cast<ListControl*>(listWindow_);
    Renderer::setCurrentColor(separatorColor_);
    Renderer::setLineWidth(separatorWidth_);
    float curX = listControl->columnWidth(0);
    for(size_t sep = 1; sep < listControl->numberOfColumns(); ++sep)
    {
        Core::Vector2 offs(curX, 0);
        Renderer::drawLine(topLeft + offs, bottomLeft + offs);
        curX += listControl->columnWidth(sep);
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
ListControlDesc::drawRow(size_t aRowIndex, const Core::Rectangle& aRowRectangle) const
{
    ListControl* listControl = dynamic_cast<ListControl*>(listWindow_);

    const ListControl::Row& row = listControl->rowAt(aRowIndex);

    Core::Rectangle textRectangle = aRowRectangle;
    textRectangle.translateBy(Core::Vector2(2.0f, 0.0f));

    bool isNormalText = true;
    Color backgroundColor;
    Color textColor;
    if(static_cast<int>(aRowIndex) == listControl->selectedIndex())
    {
        isNormalText = false;
        backgroundColor = selectionColor_;
        textColor = selectedTextColor_;
    }
    else if(static_cast<int>(aRowIndex) == listControl->highlightedIndex())
    {
        isNormalText = false;
        backgroundColor = highlightColor_;
        textColor = highlightedTextColor_;
    }

    if(!isNormalText)
    {
        Renderer::setCurrentColor(backgroundColor);
        Renderer::drawRectangle(aRowRectangle.topLeft(), aRowRectangle.bottomRight(), true);
        const Color unselectedTextColor = textDesc_->color();
        textDesc_->setColor(textColor);
        for(unsigned j = 0; j < listControl->numberOfColumns(); ++j)
        {
            (*textDesc_)
                .setText(row.stringAt(j))
                .draw(textRectangle)
                ;
            textRectangle.translateBy(Core::Vector2(listControl->columnWidth(j), 0.0f));
        }
        textDesc_->setColor(unselectedTextColor);
    }
    else
    {
        for(unsigned j = 0; j < listControl->numberOfColumns(); ++j)
        {
            (*textDesc_)
                .setText(row.stringAt(j))
                .draw(textRectangle)
                ;
            textRectangle.translateBy(Core::Vector2(listControl->columnWidth(j), 0.0f));
        }
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// **************************************************************************************************
} // namespace

