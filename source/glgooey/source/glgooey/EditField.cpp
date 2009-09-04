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

#include "glgooey/EditField.h"

#include "glgooey/core/Utils.h"
#include "glgooey/core/XMLOutputArchive.h"
#include "glgooey/core/XMLInputArchive.h"
#include "glgooey/core/Rectangle.h"

#include "glgooey/Renderer.h"
#include "glgooey/WindowManager.h"
#include "glgooey/PropertyScheme.h"
#include "glgooey/KeyMessage.h"
#include "glgooey/MouseMessage.h"
#include "glgooey/EditMessage.h"
#include "glgooey/SolidSurfaceDesc.h"


// *************************************************************************************************
namespace Gooey
{

REGISTER_CLASS(EditFieldDesc);


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Mouse listener for the edit field
class EditFieldMouseListener : public MouseListener
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Construction rquires the edit field
    EditFieldMouseListener(EditField& anEditField) : editField_(anEditField) {}

    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! react to mouse clicks
    virtual bool clicked(MouseClickMessage& aMessage)
    {
        bool isHandled = false;
        if(isLeftClick(aMessage))
        {
            WindowManager::instance().setMouseCapture(&editField_);
            const float xPos = aMessage.position().x() - editField_.position().x();
            if( (xPos > 0.0f) && (xPos < editField_.size().x()) )
            {
                const int hitCharacterIndex = hitCharacter(xPos - editField_.xOffset_);
                editField_.setCaretPosition((unsigned long)(hitCharacterIndex));
                editField_.removeSelection();
                editField_.selectionStart_ = hitCharacterIndex;
            }
            isHandled = true;
        }
        else if(isLeftRelease(aMessage))
        {
            if(WindowManager::instance().captureWindow() == &editField_)
            {
                WindowManager::instance().releaseMouseCapture();
            }
            isHandled = true;
        }
        return isHandled;
    }

    //! react to mouse movement
    virtual bool moved(MouseMessage& message)
    {
        bool isHandled = false;
        if(WindowManager::instance().captureWindow() == &editField_)
        {
            const float xPos = message.position().x() - editField_.position().x();
            if( (xPos > 0.0f) && (xPos < editField_.size().x()) )
            {
                int selectionStart = editField_.selectionStart_;
                int selectionEnd = hitCharacter(xPos - editField_.xOffset_);
                editField_.setCaretPosition(selectionEnd);
                if(selectionStart > selectionEnd) std::swap(selectionStart, selectionEnd);
                editField_.setCurrentSelection(selectionStart, selectionEnd);
            }
            else
            {
                if(xPos <= 0.0f)
                {
                    const int s = editField_.selection_.startIndex;
                    if(s > 0)
                    {
                        editField_.setCurrentSelection(s-1, editField_.selectionStart_);
                        editField_.setCaretPosition(s-1);
                    }
                }
                else if(xPos >= editField_.size().x())
                {
                    unsigned int e = editField_.selection_.endIndex;
                    if(e < editField_.text().length())
                    {
                        editField_.setCurrentSelection(editField_.selectionStart_, e+1);
                        editField_.setCaretPosition(e+1);
                    }
                }
            }
            isHandled = true;
        }
        return isHandled;
    }

    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    bool isLeftClick(MouseClickMessage& aMessage) const
    {
        return
            (aMessage.button() == MouseClickMessage::left) &&
            ((aMessage.event() == MouseClickMessage::pressed) ||
             (aMessage.event() == MouseClickMessage::doubleClicked));
    }

    bool isLeftRelease(MouseClickMessage& aMessage) const
    {
        return
            (aMessage.button() == MouseClickMessage::left) &&
             (aMessage.event() == MouseClickMessage::released);
    }

    int hitCharacter(float aPosition)
    {
        int result = editField_.renderDesc().textDesc().hitCharacterIndex(editField_.text(), aPosition);
        if(result < 0) result = editField_.text().length();
        return result;
    }

    EditField& editField_;

    // Copying and assignment are not allowed
    EditFieldMouseListener(const EditFieldMouseListener& anEditFieldMouseListener);
    EditFieldMouseListener& operator = (const EditFieldMouseListener& anEditFieldMouseListener);
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// **************************************************************************************************
//                                            EditField
// **************************************************************************************************




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EditField::EditField(Window* aParent, const std::string& aText) :
    Window(aParent, aText),
    caretPosition_(int(aText.length())),
    selectionStart_(-1),
    xOffset_(0.0f),
    renderDesc_(0)
{
    selection_.startIndex = -1;
    selection_.endIndex = -1;
    mouseListener_ = new EditFieldMouseListener(*this);
    addMessageListener(mouseListener_);
    addMessageListener(this);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EditField::~EditField()
{
    removeMessageListener(mouseListener_);
    delete mouseListener_;
    removeMessageListener(this);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
EditField::onKeyPressed(unsigned short aKey)
{
    bool isHandled = false;
    if(aKey == KeyMessage::GOOEY_LEFT)
    {
        int offset = -1;
        if(WindowManager::instance().isCtrlPressed())
            offset = lastSpaceBefore(caretPosition()) - caretPosition();
        if(WindowManager::instance().isShiftPressed())
            moveCaretMaintainSelection(offset);
        else
            moveCaretDiscardSelection(offset);
        isHandled = true;
    }
    else if(aKey == KeyMessage::GOOEY_RIGHT)
    {
        int offset = 1;
        if(WindowManager::instance().isCtrlPressed())
            offset = nextWordStartPosAfter(caretPosition()) - caretPosition();
        if(WindowManager::instance().isShiftPressed())
            moveCaretMaintainSelection(offset);
        else
            moveCaretDiscardSelection(offset);
        isHandled = true;
    }
    else if(aKey == KeyMessage::GOOEY_HOME)
    {
        if(WindowManager::instance().isShiftPressed())
        {
            selectionStart_ = caretPosition();
            setCurrentSelection(0, caretPosition());
        }
        else
            removeSelection();

        setCaretPosition(0);
        isHandled = true;
    }
    else if(aKey == KeyMessage::GOOEY_END)
    {
        if(WindowManager::instance().isShiftPressed())
        {
            selectionStart_ = caretPosition();
            setCurrentSelection(int(caretPosition()), int(text().length()));
        }
        else
            removeSelection();
        setCaretPosition((unsigned long)(text().length()));
        isHandled = true;
    }

    return isHandled;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
EditField::onCharacterInput(unsigned char aCharacter)
{
    if(aCharacter < 32)
    {
        // escape codes
        switch(aCharacter)
        {
        case 1: // ctrl-a !?!?!!
            setCaretPosition(int(text().length()));
            selectionStart_ = 0;
            selectAll();
            break;
        case 3: // ctrl-c !?!?!!
            if(selectedText() != "")
                WindowManager::instance().copyToClipboard(selectedText());
            break;
        case 22: // ctrl-v !?!?!!
            {
                if(isSelectionValid()) deleteSelection();
                std::string str = text();
                std::string pasteText = WindowManager::instance().clipboardContents();
                str.insert(caretPosition(), pasteText);
                setText(str);
                textChanged(text());
                setCaretPosition(caretPosition() + pasteText.length());
            }
            break;
        case 24: // ctrl-x !?!?!!
            if(isSelectionValid())
            {
                WindowManager::instance().copyToClipboard(selectedText());
                std::string str = text();
                str.erase(selection_.startIndex, selection_.endIndex - selection_.startIndex);
                setCaretPosition(selection_.startIndex);
                removeSelection();
                setText(str);
                textChanged(text());
            }
            break;
        case 8:
            doBackspace();
            textChanged(text());
            break;
        case 13:
            returnPressed();
        default:
            break;
        }
    }
    else if(aCharacter == 127)
    {
        doDelete();
        textChanged(text());
    }
    else
    {
        if(isSelectionValid()) deleteSelection();
        setText(stringUpToCaret() + Core::toString(aCharacter) + stringFromCaret());
        setCaretPosition(caretPosition()+1);
        textChanged(text());
    }


    return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EditField&
EditField::doBackspace()
{
    if(isSelectionValid())
    {
        deleteSelection();
    }
    else if(caretPosition() > 0)
    {
        std::string s1 = stringUpToCaret();
        s1 = s1.substr(0, s1.length()-1);
        setText(s1 + stringFromCaret());
        setCaretPosition(caretPosition_-1);
    }
    removeSelection();


    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EditField&
EditField::doDelete()
{
    if(isSelectionValid())
    {
        deleteSelection();
    }
    else if(caretPosition() < text().length())
    {
        std::string s = stringFromCaret();
        s = s.substr(1, s.length()-1);
        setText(stringUpToCaret() + s);
    }
    removeSelection();

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EditField&
EditField::deleteSelection()
{
    std::string s = text();
    s = s.substr(0, selection_.startIndex) +
        s.substr(selection_.endIndex, s.length() - selection_.endIndex);
    setCaretPosition(selection_.startIndex);
    removeSelection();
    setText(s);
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
EditField::isSelectionValid() const
{
    const int s = selection_.startIndex;
    const int e = selection_.endIndex;
    return
       ((s >= 0) && (e > 0) && (s < e) &&
        (s < int(text().length())) && (e <= int(text().length())));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EditField&
EditField::setCaretPosition(unsigned long pos)
{
    caretPosition_ = pos;

    //  we'll need to adjust the visible part of the text if the caret has
    // gone off one of the ends
    const float xPos = caretXPos();
    const float leftSideX = position().x() + renderDesc().textDesc().margin();
    const float rightSideX = leftSideX + size().x() - (renderDesc().textDesc().margin() * 2.0f);
    if(xPos < leftSideX)
    {
        xOffset_ += (leftSideX - xPos);
    }
    else if(xPos > rightSideX)
    {
        xOffset_ -= (xPos - rightSideX);
    }

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
EditField::isValidCaretPosition(int aPosition) const
{
    return (aPosition >= 0) && (aPosition <= int(text().length()));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
EditField::moveCaretMaintainSelection(int anOffset)
{
    if(selectionStart_ < 0) selectionStart_ = caretPosition();

    const int newCaretPosition = caretPosition() + anOffset;
    if(isValidCaretPosition(newCaretPosition)) setCaretPosition(newCaretPosition);
    if(selectionStart_ > int(caretPosition()))
        setCurrentSelection(caretPosition(), selectionStart_);
    else
        setCurrentSelection(selectionStart_, caretPosition());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
EditField::moveCaretDiscardSelection(int anOffset)
{
    removeSelection();
    const int newCaretPosition = caretPosition() + anOffset;
    if(isValidCaretPosition(newCaretPosition)) setCaretPosition(newCaretPosition);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string
EditField::stringUpToCaret() const
{
    std::string ret;
    if(caretPosition() > 0) ret = text().substr(0, caretPosition());

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string
EditField::stringFromCaret() const
{
    std::string ret;
    if(caretPosition() < text().length())
        ret = text().substr(caretPosition(), text().length());

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EditField::Selection
EditField::currentSelection() const
{
    return selection_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EditField&
EditField::setCurrentSelection(const  Selection& aSelection)
{
    setCurrentSelection(aSelection.startIndex, aSelection.endIndex);
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EditField&
EditField::setCurrentSelection(int startIndex, int endIndex)
{
    selection_.startIndex = startIndex;
    selection_.endIndex   = endIndex;
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EditField&
EditField::removeSelection()
{
    setCurrentSelection(0, 0);
    selectionStart_ = -1;
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EditField&
EditField::selectAll()
{
    setCurrentSelection(0, int(text().length()));
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
EditField::caretXPos() const
{
    TextDesc& textDesc = renderDesc().textDesc();
    textDesc.setText(stringUpToCaret());
    Core::Vector2 sz = textDesc.textSize();
    return sz.x() + xOffset_ + windowRectangle().bottomLeft().x() + textDesc.margin();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int
EditField::lastSpaceBefore(int pos) const
{
    // get the string up to pos
    std::string str = text().substr(0, pos);

    int ret = 0;
    if(str.length() > 0)
    {
        // strip any trailing spaces
        std::string::size_type n = str.find_last_not_of(' ');
        if(n != std::string::npos) str.erase(n, str.length()-n);

        // return the position of the last space
        std::string::size_type p = str.find_last_of(' ');

        // if we couldn't find a space we return the beginning of the string
        if(p != std::string::npos) ret = int(p+1);
    }
    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int
EditField::nextWordStartPosAfter(int pos) const
{
    // get the string after pos
    std::string str = text().substr(pos, text().length()-pos);

    // find out how many leading spaces there are
    const std::string::size_type firstNonSpacePos = str.find_first_not_of(' ');

    int ret = int(text().length());
    if(firstNonSpacePos != std::string::npos)
    {
        // strip leading spaces
        str.erase(0, firstNonSpacePos);

        // now find the next space
        std::string::size_type p = str.find_first_of(' ');

        if(p != std::string::npos)
        {
            // pass over any leading spaces
            std::string str2 = str.substr(p, str.length());
            std::string::size_type p2 = str2.find_first_not_of(' ');
            if(p2 != std::string::npos)
                ret = int(p2) + int(p) + pos;
        }
    }

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string
EditField::selectedText() const
{
    std::string ret;
    if(isSelectionValid())
        ret = text().substr(selection_.startIndex, selection_.endIndex - selection_.startIndex);

    return ret;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Core::Vector2
EditField::recommendedSize() const
{
    EditFieldDesc* renderDesc = renderDesc_;
    if(renderDesc == 0)
        renderDesc = &WindowManager::instance().propertyScheme().editFieldDesc_;

    renderDesc->textDesc().setText(text());
    return renderDesc->textDesc().textSize() + Core::Vector2(8, 8);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
EditField::draw()
{
    const Core::Vector2 topLeft = position();
    const Core::Vector2 bottomRight = position() + size();
    Core::Rectangle rect(topLeft, bottomRight);

    renderDesc().setEditField(this);
    renderDesc().draw(rect);

    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Window&
EditField::loadAppearance(const std::string& aFileName, const std::string& aWindowName)
{
    if(renderDesc_ == 0) renderDesc_ = new EditFieldDesc;
    {
        Core::XMLInputArchive archive(aFileName);
        archive & Core::ObjectNamer<EditFieldDesc>(aWindowName.c_str(), *renderDesc_);
    }
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EditFieldDesc&
EditField::renderDesc() const
{
    return (renderDesc_ != 0) ? *renderDesc_ : WindowManager::instance().propertyScheme().editFieldDesc_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// **************************************************************************************************
//                                            EditFieldDesc
// **************************************************************************************************




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EditFieldDesc::EditFieldDesc() :
    background_(new SolidSurfaceDesc()),
    border_(new RectangleDesc()),
    textDesc_(new TextDesc()) ,
    selectionColor_(Color(0.85f, 0.85f, 0.85f, 1)),
    selectedTextColor_(Color(0.35f, 0.35f, 0.35f, 1))
{
    textDesc_->setHorizontalAlignment(Core::Alignment::left);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EditFieldDesc::~EditFieldDesc()
{
    delete background_;
    delete border_;
    delete textDesc_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
EditFieldDesc::draw(const Core::Rectangle& anArea) const
{
    textDesc_->setHorizontalAlignment(Core::Alignment::left);

    if(background_ != 0) background_->draw(anArea);

    if(editField_->text() != "")
    {
        Core::Rectangle area = anArea;

        Core::Rectangle clipRectangle(area);
        clipRectangle.expandBy(-textDesc_->margin());
        Renderer::enableClipRectangle(clipRectangle);

        const EditField::Selection& selection = editField_->selection_;
        if(selection.endIndex > selection.startIndex)
            drawTextWithSelection(area);
        else
            drawTextWithoutSelection(area);
    }
    
    drawCaret();

    Renderer::disableClipRectangle();

    if(border_ != 0) border_->draw(anArea);

    Renderer::setCurrentColor(Color(1, 1, 1, 1));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
EditFieldDesc::drawTextWithSelection(const Core::Rectangle& anArea) const
{
    Core::Rectangle area(anArea);

    // We draw in 3 steps and don't want the textDesc's margin to appear each time
    const float margin = textDesc_->margin();
    textDesc_->setMargin(0.0f);
    area.expandWidthBy(-margin);

    // draw pre selected text
    Core::Rectangle textRectangle(area);
    textRectangle.translateBy(Core::Vector2(editField_->xOffset_, 0.0f));
    const float preSelectedWidth = drawPreSelectedText(textRectangle);

    // draw selected text
    textRectangle.translateBy(Core::Vector2(preSelectedWidth, 0.0f));
    const float selectedWidth = drawSelectedText(textRectangle, area, preSelectedWidth);

    // Finally do any text after the selection
    textRectangle.translateBy(Core::Vector2(selectedWidth, 0.0f));
    drawPostSelectedText(textRectangle);

    // Set the margin back
    textDesc_->setMargin(margin);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
EditFieldDesc::drawTextWithoutSelection(const Core::Rectangle& anArea) const
{
    Core::Rectangle textRectangle(anArea);
    textRectangle.translateBy(Core::Vector2(editField_->xOffset_, 0.0f));
    (*textDesc_)
        .setText(editField_->text())
        .draw(textRectangle)
        ;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
EditFieldDesc::drawPreSelectedText(const Core::Rectangle& anArea) const
{
    const std::string text = editField_->text().substr(0, editField_->selection_.startIndex);
    textDesc_->setText(text);

    (*textDesc_).draw(anArea);

    return textDescTextWidth();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
EditFieldDesc::drawSelectedText(const Core::Rectangle& anArea, const Core::Rectangle& aVisibleArea, float aPreSelectedWidth) const
{
    const EditField::Selection& selection = editField_->selection_;
    const int selectionSize = selection.endIndex-selection.startIndex;

    const std::string selected = editField_->text().substr(selection.startIndex, selectionSize);
    textDesc_->setText(selected);

    drawSelectionBackground(aVisibleArea, editField_->xOffset_ + aPreSelectedWidth);

    const Color unselectedColor = textDesc_->color();
    (*textDesc_)
        .setColor(selectedTextColor_)
        .draw(anArea)
        ;

    textDesc_->setColor(unselectedColor);

    return textDescTextWidth();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
EditFieldDesc::drawPostSelectedText(const Core::Rectangle& anArea) const
{
    const int selectionEnd = editField_->selection_.endIndex;
    const std::string& text = editField_->text();
    std::string postSelected = text.substr(selectionEnd, text.length() - selectionEnd);

    (*textDesc_)
        .setText(postSelected)
        .draw(anArea)
        ;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
EditFieldDesc::drawSelectionBackground(const Core::Rectangle& aVisibleArea, float aStartX) const
{
    Renderer::setCurrentColor(selectionColor_);
    const Core::Vector2 topLeft = aVisibleArea.topLeft() + Core::Vector2(aStartX, 1.0f);
    const Core::Vector2 bottomRight(topLeft.x()+textDescTextWidth(), aVisibleArea.bottomRight().y()-1.0f);
    Renderer::drawRectangle(topLeft, bottomRight, true);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
EditFieldDesc::drawCaret() const
{
    if(editField_->isFocused())
    {
        const float x = editField_->caretXPos();
        const float caretHeightOverTwo = (textDesc_->lineHeight() + 4.0f) * 0.5f;
        const float verticalCenter = (editField_->windowRectangle().size().y() * 0.5f) +
                                      editField_->windowRectangle().topLeft().y();

        Renderer::drawLine(Core::Vector2(x, verticalCenter - caretHeightOverTwo),
                 Core::Vector2(x, verticalCenter + caretHeightOverTwo));
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float
EditFieldDesc::textDescTextWidth() const
{
    float width = textDesc_->textSize().x();
    if(width < 0.0f) width = 0.0f;
    return width;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// *************************************************************************************************
} // namespace Gooey
