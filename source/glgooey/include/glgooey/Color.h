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
#ifndef COLOR__H
#define COLOR__H

#include "mmgr/mmgr.h"

#include "glgooey/core/Serialization.h"


// *************************************************************************************************
namespace Gooey
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief RGBA color values
//!
//! This class stores colors as individual r, g, b, a values where \f$ r, g, b, a \in [0,1]\f$ mean
//! the same as in OpenGl.
class Color
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! default constructor - default value is opaque black
    Color();

    //! construct using separate red, green, blue and alpha values 
    Color(float r, float g, float b, float a = 1.0);

    //! copy constructor
    Color(const Color& aColor);

    //! destructor
    virtual ~Color() {}


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    //! returns the red component
    float r() const { return red_; }

    //! returns the green component
    float g() const { return green_; }

    //! returns the blue component
    float b() const { return blue_; }

    //! returns the alpha component
    float a() const { return alpha_; }

    //! set the red component
    Color& setRed(float val) { red_ = val; return *this; }

    //! set the green component
    Color& setGreen(float val) { green_ = val; return *this; }

    //! set the blue component
    Color& setBlue(float val) { blue_ = val; return *this; }

    //! set the alpha component
    Color& setAlpha(float val) { alpha_ = val; return *this; }




    // ---------------------------------------------------------------------------------------------
    //  Overloaded operators
    // ---------------------------------------------------------------------------------------------
public:
    //! assignment
    Color& operator = (const Color& aColor);

    //! equality
    bool operator == (const Color& aColor) const;

    //! inequality
    bool operator != (const Color& aColor) const { return !(*this == aColor); }



    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    float red_;
    float green_;
    float blue_;
    float alpha_;


    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        anArchive &
            TAGGED_OBJECT(red) &
            TAGGED_OBJECT(green) &
            TAGGED_OBJECT(blue) &
            TAGGED_OBJECT(alpha);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// *************************************************************************************************
} // namespace Gooey

#endif
