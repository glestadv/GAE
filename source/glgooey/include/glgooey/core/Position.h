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
#ifndef __GOOEY_POSITION__H_
#define __GOOEY_POSITION__H_

#include "mmgr/nommgr.h"
#include <cstddef>
#include <iosfwd>
#include "mmgr/mmgr.h"



// *************************************************************************************************
namespace Gooey
{


// *************************************************************************************************
namespace Core
{


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief integral 2-component tuple class.
template<typename T>
class TwoTuple
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default constructor
    TwoTuple() {}

    //! Construction from individual components
    TwoTuple(T anX, T aY) : x_(anX), y_(aY) {}

    //! Copy constructor
    TwoTuple(const TwoTuple& aTwoTuple) { (*this) = aTwoTuple; }


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Returns the dot product of this vector with the passed in vector
    inline float dot(const TwoTuple& aTwoTuple) const;


    // ---------------------------------------------------------------------------------------------
    //  Operators
    // ---------------------------------------------------------------------------------------------
public:
    inline TwoTuple& operator *= (float aValue); //!< Scalar Multiplication
    inline TwoTuple& operator /= (float aValue); //!< Scalar Division

    inline TwoTuple& operator += (const TwoTuple& aTwoTuple); //!< Component Addition
    inline TwoTuple& operator -= (const TwoTuple& aTwoTuple); //!< Component Subtraction
    inline TwoTuple& operator *= (const TwoTuple& aTwoTuple); //!< Component Multiplication

    inline bool operator == (const TwoTuple& aTwoTuple) const; //!< Equality
    inline bool operator != (const TwoTuple& aTwoTuple) const; //!< Inequality
    inline TwoTuple& operator = (const TwoTuple& aTwoTuple); //!< Assignment

    T  operator [] (size_t anIndex) const; //!< Access
    T& operator [] (size_t anIndex);       //!< Access


    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    T x() const { return x_; } //!< Returns the x_ component.
    T y() const { return y_; } //!< Returns the y_ component.

    void setX(T anX) { x_ = anX; } //!< Sets the x_ component.
    void setY(T aY)  { y_ = aY;  } //!< Sets the y_ component.

    inline void set(T anX, T aY); //!< Sets all components.


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    T x_; //!< X-Component of the vector
    T y_; //!< Y-Component of the vector


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
template<typename T>
std::ostream& operator << (std::ostream& os, const TwoTuple<T>& aTwoTuple);

//! Component addition between two vectors
template<typename T>
inline TwoTuple<T> operator + (const TwoTuple<T>& v1, const TwoTuple<T>& v2);

//! Component subtraction between two vectors
template<typename T>
inline TwoTuple<T> operator - (const TwoTuple<T>& v1, const TwoTuple<T>& v2);

//! Component multiplication between two vectors
template<typename T>
inline TwoTuple<T> operator * (const TwoTuple<T>& v1, const TwoTuple<T>& v2);

//! Scalar multiplication between a vector and a scalar
template<typename T>
inline TwoTuple<T> operator * (const TwoTuple<T>& aTwoTuple, float aScalar);

//! Scalar multiplication between a scalar and a vector
template<typename T>
inline TwoTuple<T> operator * (float aScalar, const TwoTuple<T>& aTwoTuple);

//! Unary minus
template<typename T>
inline TwoTuple<T> operator - (const TwoTuple<T>& aTwoTuple);

#include "Position.inl"

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Two dimensional integer position
class Position : public TwoTuple<int>
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default constructor
    Position() {}

    //! Default constructor
    Position(const TwoTuple<int>& aTwoTuple) : TwoTuple<int>(aTwoTuple.x(), aTwoTuple.y()) {}

    //! Construction from individual components
    Position(int anX, int aY) : TwoTuple<int>(anX, aY) {}

    //! Copy constructor
    Position(const Position& aPosition) : TwoTuple<int>(aPosition) {}
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Two dimensional integer size (non-negative values only)
class Size : public TwoTuple<size_t>
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default constructor
    Size() {}

    //! Construction from individual components
    Size(size_t aWidth, size_t aHeight) : TwoTuple<size_t>(aWidth, aHeight) {}

    //! Copy constructor
    Size(const Size& aSize) : TwoTuple<size_t>(aSize) {}


    // ---------------------------------------------------------------------------------------------
    //  Overloaded operators
    // ---------------------------------------------------------------------------------------------
public:
    //! A size can be implicitly converted to a position
    operator Position () { return Position(static_cast<int>(x()), static_cast<int>(y())); }

    //! A const size can be implicitly converted to a const position
    operator const Position () const { return Position(static_cast<int>(x()), static_cast<int>(y())); }

    // ---------------------------------------------------------------------------------------------
    //  Accessors
    // ---------------------------------------------------------------------------------------------
public:
    size_t width() const { return x(); }  //!< Returns the width
    size_t height() const { return y(); } //!< Returns the height

    size_t area() const { return x() * y(); } //!< Returns the area covered by this size

    void setWidth (size_t aWidth)  { setX(aWidth);  } //!< Sets the width
    void setHeight(size_t aHeight) { setY(aHeight); } //!< Sets the height
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// *************************************************************************************************
} // namespace Core

// *************************************************************************************************
} // namespace Gooey


#endif
