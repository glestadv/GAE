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
#ifndef __GOOEY_VECTOR2__H_
#define __GOOEY_VECTOR2__H_

#include "mmgr/nommgr.h"
#include <cstddef>
#include <iosfwd>
#include "mmgr/mmgr.h"

#include "glgooey/core/Serialization.h"

// *************************************************************************************************
namespace Gooey
{


// *************************************************************************************************
namespace Core
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief 2-component vector class.
class Vector2
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default constructor
    Vector2() : x_(0), y_(0) {}

    //! Construction from individual components
    Vector2(float anX, float aY) : x_(anX), y_(aY) {}

    //! Copy constructor
    Vector2(const Vector2& aVector2) { (*this) = aVector2; }


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Returns the dot product of this vector with the passed in vector
    inline float dot(const Vector2& aVector) const;

    //! Returns the squared length of this vector
    inline float squaredLength() const;

    //! Returns the length of this vector
    float length() const;

    //! Normalizes this vector
    Vector2& normalize();

    //! Returns a normalized copy of this vector
    Vector2 normalized() const;

    //! Rounds both components to the nearest whole number value
    Vector2& round();

    //! Returns a copy of this vector with both components rounded to the nearest whole number value
    Vector2 rounded() const;


    // ---------------------------------------------------------------------------------------------
    //  Operators
    // ---------------------------------------------------------------------------------------------
public:
    inline Vector2& operator *= (float aValue); //!< Scalar Multiplication
    inline Vector2& operator /= (float aValue); //!< Scalar Division

    inline Vector2& operator += (const Vector2& aVector); //!< Component Addition
    inline Vector2& operator -= (const Vector2& aVector); //!< Component Subtraction
    inline Vector2& operator *= (const Vector2& aVector); //!< Component Multiplication

    inline bool operator == (const Vector2& aVector) const; //!< Equality
    inline bool operator != (const Vector2& aVector) const; //!< Inequality
    inline Vector2& operator = (const Vector2& aVector); //!< Assignment

    float  operator [] (size_t anIndex) const; //!< Access
    float& operator [] (size_t anIndex);       //!< Access


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    float x() const { return x_; } //!< Returns the x_ component.
    float y() const { return y_; } //!< Returns the y_ component.

    void setX(float anX) { x_ = anX; } //!< Sets the x_ component.
    void setY(float aY) { y_ = aY; } //!< Sets the y_ component.

    inline void set(float anX, float aY); //!< Sets all components.


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    float x_; //!< X-Component of the vector
    float y_; //!< Y-Component of the vector


    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        anArchive & TAGGED_OBJECT(x) & TAGGED_OBJECT(y);
    }
};


//! Writes the passed in vector to the passed in stream
std::ostream& operator << (std::ostream& os, const Vector2& aVector);

//! Component addition between two vectors
inline Vector2 operator + (const Vector2& v1, const Vector2& v2);

//! Component subtraction between two vectors
inline Vector2 operator - (const Vector2& v1, const Vector2& v2);

//! Component multiplication between two vectors
inline Vector2 operator * (const Vector2& v1, const Vector2& v2);

//! Scalar multiplication between a vector and a scalar
inline Vector2 operator * (const Vector2& aVector, float aScalar);

//! Scalar multiplication between a scalar and a vector
inline Vector2 operator * (float aScalar, const Vector2& aVector);

//! Unary minus
inline Vector2 operator - (const Vector2& aVector);

#include "Vector2.inl"

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// *************************************************************************************************
} // namespace Core

// *************************************************************************************************
} // namespace Gooey


#endif
