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
#ifndef READERS__H
#define READERS__H

#include "mmgr/mmgr.h"

#include "glgooey/Color.h"
#include "glgooey/core/StandardException.h"
#include "Enumeration.h"


namespace Gooey
{

// ****************************************************************************
//! The available styles for rendering unfilled rectangles
// ****************************************************************************
Enum3(RectangleStyle, normal, dropped, raised);


// ****************************************************************************
//! The available styles with which a texture may be mapped onto a quad
// ****************************************************************************
Enum2(Mapping, tiled, stretched);


// ****************************************************************************
//! \brief The frame of a frame window is split into sections. This enum
//!     provides identifiers for each of those sections
// ****************************************************************************
Enum9(FrameSection, right, topRight, top, topLeft, left, bottomLeft, bottom, bottomRight, title);



// ****************************************************************************
//! \brief Used to describe the orientation of various elements
// ****************************************************************************
Enum2(Orientation, horizontal, vertical);



// ****************************************************************************
//! \brief Template class used to read in primitive data types
// ****************************************************************************
template<typename T>
class SimpleTypeReader
{
    // ************************************************************************
    //  Construction / Destruction
    // ************************************************************************
public:
    //! \brief Constructor
    //! \param aSimpleType A reference to the object to be read in
    //! \param aString The string which contains the simple type
    SimpleTypeReader(T& aSimpleType, const std::string& aString) :
        simpleType_(aSimpleType), string_(aString) {}

    // ************************************************************************
    //  Overloaded operators
    // ************************************************************************
public:
    //! \brief The operator which does the actual work
    //!
    //! This just casts the string to the required type.
    void operator() ()
    {
        simpleType_ = Core::string_cast<T>(string_);
    }
private:
    T& simpleType_;      //!< reference to the data value to be set
    std::string string_; //!< the string containing the data value

    // Copying and assignment are not allowed
    SimpleTypeReader(const SimpleTypeReader& aReader);
    SimpleTypeReader& operator = (const SimpleTypeReader& aReader);
};



// ****************************************************************************
//! \brief Used to read in colors
//!
//!  Colors are expressed in the xml files in 0xRRGGBBAA format. Internally,
//! colors are in the typical OpenGL floating point format. This class
//! is used to do the reading and provide the conversion between the two
//! formats.
//!
//! \see Color
// ****************************************************************************
class ColorReader
{
    // ************************************************************************
    //  Construction / Destruction
    // ************************************************************************
public:
    //! \brief Constructor
    //! \param aColor A reference to a color object which will receive the read in color
    //! \param aString The string which contains the 0xRRGGBBAA version of the color
    ColorReader(Color& aColor, const std::string& aString) :
        color_(aColor), string_(aString) {}

    // ************************************************************************
    //  Overloaded operators
    // ************************************************************************
public:
    //! \brief The operator which does the actual work
    //!
    //! First the string is converted to an unsigned long value. Then each byte of
    //! the resulting value is divided by 255 to  give the desired floating point
    //! values.
    void operator() ()
    {
        char* p;
        unsigned long val = strtoul(string_.c_str(), &p, 16);
        if(*p) THROW("Conversion Error!");
        unsigned char ucr = static_cast<unsigned char>(val >> 24);
        unsigned char ucg = static_cast<unsigned char>(val >> 16);
        unsigned char ucb = static_cast<unsigned char>(val >> 8);
        unsigned char uca = static_cast<unsigned char>(val);
        color_.setRed  (static_cast<unsigned char>(ucr) / 255.0f);
        color_.setGreen(static_cast<unsigned char>(ucg) / 255.0f);
        color_.setBlue (static_cast<unsigned char>(ucb) / 255.0f);
        color_.setAlpha(static_cast<unsigned char>(uca) / 255.0f);
    }

    // ************************************************************************
    //  Implementation
    // ************************************************************************
private:
    Color& color_;        //!< reference to the color value to be set
    std::string string_;  //!< the string containing the 0xRRGGBBAA version of the color

    // Copying and assignment are not allowed
    ColorReader(const ColorReader& aReader);
    ColorReader& operator = (const ColorReader& aReader);
};



// ****************************************************************************
//! \brief Used to read in available rectangle styles
//!
//!  Core::Rectangle styles are expressed in the xml files in plain text format.
//! Internally, rectangle styles are enumerated values. This class
//! is used to do the reading and provide the conversion between the two
//! formats.
//!
//! \see RectangleStyle
// ****************************************************************************
class RectangleStyleReader
{
    // ************************************************************************
    //  Construction / Destruction
    // ************************************************************************
public:
    //! \brief Constructor
    //! \param aStyle A reference to the enumerated value to be set
    //! \param aString The string which contains the plain text rectangle style
    RectangleStyleReader(RectangleStyle& aStyle, const std::string& aString) :
        style_(aStyle), string_(aString) {}


    // ************************************************************************
    //  Overloaded operators
    // ************************************************************************
public:
    //! \brief The operator which does the actual work
    //!
    //! Here the strings are converted to enumerated values
    void operator() ()
    {
        if(string_ == "Normal")
            style_ = normal;
        else if(string_ == "Dropped")
            style_ = dropped;
        else if(string_ == "Raised")
            style_ = raised;
        else
            THROW("Invalid rectangle style");
    }


    // ************************************************************************
    //  Implementation
    // ************************************************************************
private:
    RectangleStyle& style_;  //!< A reference to the enumerated value to be set
    std::string string_;     //!< A string containing the plain text version of the style

    // Copying and assignment are not allowed
    RectangleStyleReader(const RectangleStyleReader& aReader);
    RectangleStyleReader& operator = (const RectangleStyleReader& aReader);
};



// ****************************************************************************
//! \brief Used to read in available texture mapping styles
//!
//!  Mapping styles are expressed in the xml files in plain text format.
//! Internally, mapping styles are enumerated values. This class
//! is used to do the reading and provide the conversion between the two
//! formats.
//!
//! \see Mapping
// ****************************************************************************
class MappingReader
{
    // ************************************************************************
    //  Construction / Destruction
    // ************************************************************************
public:
    //! \brief Constructor
    //! \param aMapping A reference to the enumerated value to be set
    //! \param aString The string which contains the plain text mapping
    MappingReader(Mapping& aMapping, const std::string& aString) :
        mapping_(aMapping), string_(aString) {}

    // ************************************************************************
    //  Overloaded operators
    // ************************************************************************
public:
    //! \brief The operator which does the actual work
    //!
    //! Here the strings are converted to enumerated values
    void operator() ()
    {
        if(string_ == "Tiled")
            mapping_ = tiled;
        else if(string_ == "Stretched")
            mapping_ = stretched;
        else
            THROW("Invalid bitmap mapping");
    }


    // ************************************************************************
    //  Implementation
    // ************************************************************************
private:
    Mapping& mapping_;    //!< A reference to the enumerated value to be set
    std::string string_;  //!< A string containing the plain text version of the mapping

    // Copying and assignment are not allowed
    MappingReader(const MappingReader& aReader);
    MappingReader& operator = (const MappingReader& aReader);
};



// ****************************************************************************
//! \brief Used to read in frame section identifiers
//!
//!  Frame sections are expressed in the xml files in plain text format.
//! Internally, frame sections are enumerated values. This class
//! is used to do the reading and provide the conversion between the two
//! formats.
//!
//! \see FrameSection
// ****************************************************************************
class FrameSectionReader
{
    // ************************************************************************
    //  Construction / Destruction
    // ************************************************************************
public:
    //! \brief Constructor
    //! \param aFrameSection A reference to the enumerated value to be set
    //! \param aString The string which contains the plain text frame section
    FrameSectionReader(FrameSection& aFrameSection, const std::string& aString) :
        frameSection_(aFrameSection), string_(aString) {}

    // ************************************************************************
    //  Overloaded operators
    // ************************************************************************
public:
    //! \brief The operator which does the actual work
    //!
    //! Here the strings are converted to enumerated values
    void operator() ()
    {
        if(string_ == "Right")
            frameSection_ = right;
        else if(string_ == "TopRight")
            frameSection_ = topRight;
        else if(string_ == "Top")
            frameSection_ = top;
        else if(string_ == "TopLeft")
            frameSection_ = topLeft;
        else if(string_ == "Left")
            frameSection_ = left;
        else if(string_ == "BottomLeft")
            frameSection_ = bottomLeft;
        else if(string_ == "Bottom")
            frameSection_ = bottom;
        else if(string_ == "BottomRight")
            frameSection_ = bottomRight;
        else if(string_ == "Title")
            frameSection_ = title;
        else
            THROW("Invalid frameSection");
    }

    // ************************************************************************
    //  Implementation
    // ************************************************************************
private:
    FrameSection& frameSection_;  //!< A reference to the enumerated value to be set
    std::string string_;          //!< A string containing the plain text version
                                  //!< of the frame section

    // Copying and assignment are not allowed
    FrameSectionReader(const FrameSectionReader& aReader);
    FrameSectionReader& operator = (const FrameSectionReader& aReader);
};



// ****************************************************************************
//! \brief Used to read in orientation identifiers
//!
//!  Orientations are expressed in the xml files in plain text format. 
//! Internally, orientations are enumerated values. This class
//! is used to do the reading and provide the conversion between the two
//! formats.
//!
//! \see Orientation
// ****************************************************************************
class OrientationReader
{
    // ************************************************************************
    //  Construction / Destruction
    // ************************************************************************
public:
    //! \brief Constructor
    //! \param anOrientation A reference to the enumerated value to be set
    //! \param aString The string which contains the plain text orientation
    OrientationReader(Orientation& anOrientation, const std::string& aString) :
        orientation_(anOrientation), string_(aString) {}

    // ************************************************************************
    //  Overloaded operators
    // ************************************************************************
public:
    //! \brief The operator which does the actual work
    //!
    //! Here the strings are converted to enumerated values
    void operator() ()
    {
        if(string_ == "Horizontal")
            orientation_ = horizontal;
        else if(string_ == "Vertical")
            orientation_ = vertical;
        else
            THROW("Invalid orientation");
    }


    // ************************************************************************
    //  Implementation
    // ************************************************************************
private:
    Orientation& orientation_; //!< A reference to the enumerated value to be set
    std::string string_;       //!< A string containing the plain text version 
                               //!< of the orientation

    // Copying and assignment are not allowed
    OrientationReader(const OrientationReader& aReader);
    OrientationReader& operator = (const OrientationReader& aReader);
};



} // namespace

#endif
