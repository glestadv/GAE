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
#ifndef PROPERTYSCHEME__H
#define PROPERTYSCHEME__H

#include "mmgr/mmgr.h"

#include "glgooey/Panel.h"
#include "glgooey/StaticText.h"
#include "glgooey/Slider.h"
#include "glgooey/ProgressBar.h"
#include "glgooey/ListControl.h"
#include "glgooey/FrameWindow.h"
#include "glgooey/ComboBox.h"
#include "glgooey/CheckBox.h"
#include "glgooey/ScrollBar.h"
#include "glgooey/ListBox.h"
#include "glgooey/EditField.h"
#include "glgooey/Color.h"
#include "glgooey/RenderDesc.h"

namespace Gooey
{



// ***************************************************************************
//! \class PropertyScheme
//! \brief Appearance schemes for the Gooey UI
//!
//!  GLGooey can load appearance schemes from xml files. The schemes are
//! encapsulated within this class. A default scheme gives GLGooey its
//! characteristic look if no scheme is loaded.
// ***************************************************************************
class PropertyScheme : public RenderDesc
{
    // **********************************************************************
    //  Construction / Destruction
    // **********************************************************************
public:
    //! default constructor
    PropertyScheme();

    //! destructor
    virtual ~PropertyScheme();


    // **********************************************************************
    //  Accessors
    // **********************************************************************
public:
    //! Load a scheme from an xml file
    PropertyScheme& load(const std::string& fileName);

    //! Save a scheme to an xml file
    PropertyScheme& save(const std::string& fileName);

    //! draws on the passed in area according to the descriptor
    virtual void draw(const Core::Rectangle& ) const {}


    // **********************************************************************
    //  Implementation
    // **********************************************************************
public:
    ButtonDesc      buttonDesc_;
    PanelDesc       panelDesc_;
    EditFieldDesc   editFieldDesc_;
    ScrollBarDesc   scrollBarDesc_;
    ListBoxDesc     listBoxDesc_;
    ListControlDesc listControlDesc_;
    ProgressBarDesc progressBarDesc_;
    CheckBoxDesc    checkBoxDesc_;
    SliderDesc      sliderDesc_;
    FrameWindowDesc frameWindowDesc_;
    StaticTextDesc  staticTextDesc_;
    ComboBoxDesc    comboBoxDesc_;


    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive &
            TAGGED_OBJECT(buttonDesc) &
            TAGGED_OBJECT(panelDesc) &
            TAGGED_OBJECT(editFieldDesc) &
            TAGGED_OBJECT(scrollBarDesc) &
            TAGGED_OBJECT(listBoxDesc) &
            TAGGED_OBJECT(listControlDesc) &
            TAGGED_OBJECT(progressBarDesc) &
            TAGGED_OBJECT(checkBoxDesc) &
            TAGGED_OBJECT(sliderDesc) &
            TAGGED_OBJECT(frameWindowDesc) &
            TAGGED_OBJECT(staticTextDesc) &
            TAGGED_OBJECT(comboBoxDesc);
    }
};

} // namespace

#endif
