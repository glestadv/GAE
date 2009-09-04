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
#ifndef STATICTEXT__H
#define STATICTEXT__H

#include "mmgr/mmgr.h"

#include "glgooey/Window.h"
#include "glgooey/RenderDesc.h"

#include "glgooey/core/Alignment.h"



// *************************************************************************************************
//  Forward declarations
namespace Gooey
{
    class StaticTextDesc;
    class TextDesc;
}
// *************************************************************************************************



// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Use this class to add non-editable text elements to the user interface
class StaticText : public Window
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! \brief Constructor requires the parent window and the string to be displayed.
    //! Optionally, the default left-alignment may be altered.
    StaticText(Window* aParent, const std::string& aText);

    //! destructor
    virtual ~StaticText();



    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
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
    StaticTextDesc* renderDesc_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The RenderDesc class for a static text control
class StaticTextDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Construction / Destruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    StaticTextDesc();

    //! Destructor
    virtual ~StaticTextDesc();


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Draws on the passed in area according to the desc
    virtual void draw(const Core::Rectangle& area) const;

    //! Sets the static text control to be rendered on the next call to draw
    StaticTextDesc& setStaticText(StaticText* aStaticText)
    {
        staticText_ = aStaticText;
        return *this;
    }

    //! Returns a reference to the TextDesc object used to draw the text
    TextDesc& textDesc() { return *textDesc_; }

    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    StaticText* staticText_;
    RenderDesc* background_;
    RenderDesc* border_;
    TextDesc*   textDesc_;



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
