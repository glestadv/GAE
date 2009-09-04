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
#ifndef EDITFIELD__H
#define EDITFIELD__H

#include "mmgr/mmgr.h"

#include "glgooey/Window.h"
#include "glgooey/KeyMessage.h"
#include "glgooey/RenderDesc.h"



// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    class EditFieldDesc;
    class EditFieldMouseListener;
    class TextDesc;
}
// *************************************************************************************************




// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Edit fields allow user input from the keyboard in a single line field
class EditField : public Window, public KeyListener
{
    // ---------------------------------------------------------------------------------------------
    //  Data Types
    // ---------------------------------------------------------------------------------------------
public:
    //! struct describing a selection
    struct Selection
    {
        int startIndex;
        int endIndex;
    };


    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief constructor requires the parent and the text with which the field
    //! is to be initialized
    EditField(Window* aParent, const std::string& aText);

    //! destructor
    virtual ~EditField();


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! react to key messages
    virtual bool onCharacterInput(unsigned char aCharacter);

    //! react to key messages
    virtual bool onKeyPressed(unsigned short aKey);



    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! Returns the current position of the caret
    unsigned long caretPosition() const { return caretPosition_; }

    //! Returns the currently selected text (empty if nothing is selected)
    std::string selectedText() const;

    //! Returns the current selection
    Selection currentSelection() const;

    //! Returns the index of the beginning of the current selection
    int currentSelectionStart() const { return selection_.startIndex; }

    //! Returns the index of the end of the current selection
    int currentSelectionEnd() const { return selection_.endIndex; }

    //! Sets the current selection
    EditField& setCurrentSelection(const  Selection& aSelection);

    //! Sets the current selection
    EditField& setCurrentSelection(int startIndex, int endIndex);

    //! Removes the current selection
    EditField& removeSelection();

    //! returns true if a valid non-empty portion of the string is selected
    bool isSelectionValid() const;

    //! \brief Sets the current selection to contain the entire contents of the
    //! edit field
    EditField& selectAll();

    // Commented in base class
    virtual Core::Vector2 recommendedSize() const;
    virtual Window& loadAppearance(const std::string& aFileName, const std::string& aWindowName);


    // ---------------------------------------------------------------------------------------------
    //  Signals
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief This signal is emitted if return is pressed while the edit field
    //! has the focus
    Signal0 returnPressed;

    //!  This signal is emitted when the text in the edit field changes
    Signal1<const std::string&> textChanged;


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    //! render the window to the current OpenGL context
    virtual Window& draw();

private:
    //! react to the user entering a backspace
    EditField& doBackspace();

    //! react to the user pressing the delete key
    EditField& doDelete();

    //! deletes the currently selected text
    EditField& deleteSelection();

    //! returns the sub string up to the caret
    std::string stringUpToCaret() const;

    //! returns the sub string from the caret to the end
    std::string stringFromCaret() const;

    //! sets the position of the caret
    EditField& setCaretPosition(unsigned long pos);

    //! returns the xPos of the caret in app window coords
    float caretXPos() const;

    //! returns the position of the last space character before the passed in position
    int lastSpaceBefore(int pos) const;

    //! returns the position of the beginning of the next word after the passed in position
    int nextWordStartPosAfter(int pos) const;

    //! Returns a pointer to the render desc that will be used to draw this edit field
    EditFieldDesc& renderDesc() const;

    //! Moves the caret maintaining the selection
    void moveCaretMaintainSelection(int anOffset);

    //! Moves the caret discarding the selection
    void moveCaretDiscardSelection(int anOffset);

    //! Returns true if the passed in index is a valid caret position
    bool isValidCaretPosition(int aPosition) const;

    unsigned long caretPosition_;

    friend class EditFieldMouseListener;
    EditFieldMouseListener* mouseListener_;

    Selection selection_;
    int selectionStart_;

    float xOffset_;

    EditFieldDesc* renderDesc_;

    friend class EditFieldDesc;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The render desc class for edit fields
class EditFieldDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    EditFieldDesc();

    //! Destructor
    virtual ~EditFieldDesc();


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Draws on the passed in area according to the desc
    virtual void draw(const Core::Rectangle& area) const;

    //! Sets the edit field to be rendered on the next call to draw
    EditFieldDesc& setEditField(EditField* anEditField)
    {
        editField_ = anEditField;
        return *this;
    }

    //! Returns a reference to the text desc
    TextDesc& textDesc() { return *textDesc_; }


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    //! Draws the text when some of it is selected
    void drawTextWithSelection(const Core::Rectangle& anArea) const;

    //! Draws the text when no text is selected
    void drawTextWithoutSelection(const Core::Rectangle& anArea) const;

    //! \brief Draws the text up to the start of the selection on the passed in area
    //! \returns the width of the text that was drawn
    float drawPreSelectedText(const Core::Rectangle& anArea) const;

    //! \brief Draws the selection mask and the selected text on the passed in area. The width
    //! of the text up until the start of the selection must also be passed in.
    //! \returns the width of the text that was drawn
    float drawSelectedText(const Core::Rectangle& anArea, const Core::Rectangle& aVisibleArea, float aPreSelectedWidth) const;

    //! \brief Draws the text after the end of the selection on the passed in area
    void drawPostSelectedText(const Core::Rectangle& anArea) const;

    //! returns the width of the text currently set in the text desc
    float textDescTextWidth() const;

    //! draws the selection background
    void drawSelectionBackground(const Core::Rectangle& aVisibleArea, float aStartX) const;

    //! draws the caret
    void drawCaret() const;

    RenderDesc* background_;
    RenderDesc* border_;
    TextDesc*   textDesc_;
    Color       selectionColor_;
    Color       selectedTextColor_;
    EditField*  editField_;



    // ---------------------------------------------------------------------------------------------
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
            TAGGED_OBJECT(selectedTextColor);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace

#endif
