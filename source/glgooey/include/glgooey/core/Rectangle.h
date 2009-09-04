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
#ifndef RECTANGLE__H
#define RECTANGLE__H

#include "mmgr/mmgr.h"

#include "glgooey/core/Vector2.h"


// *************************************************************************************************
namespace Gooey
{

// *************************************************************************************************
namespace Core
{


    
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Axis-aligned rectangle with integer position and extents
//! \note Make sure you take a look at \ref techinfo when using this class
class Rectangle
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! default contructor - both points at (0, 0)
    Rectangle() {}

    //! construction via two points
    Rectangle(const Vector2& p1, const Vector2& p2);

    //! construction from components of the two corner points
    Rectangle(float x1, float y1, float x2, float y2);


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! returns the position (i.e. lowest value corner) of the rectangle
    Vector2 position() const { return position_; }

    //! returns half the rectangle's size
    Vector2 halfSize() const { return size_ * 0.5f; }

    //! the center point
    Vector2 centerPoint() const;

    //! is the passed in point within the rectangle's horizontal edges
    bool isAboveAndBelow(const Vector2& p) const;

    //! is the passed in point within the rectangle's vertical edges
    bool isLeftAndRightOf(const Vector2& p) const;

    //! is the passed in point within the rectangle's edges
    bool surrounds(const Vector2& p) const;

    //! the smallest x coord
    float minX() const { return position_.x(); }

    //! the smallest y coord
    float minY() const { return position_.y(); }

    //! the largest x coord
    float maxX() const { return position_.x() + size_.x(); }

    //! the largest y coord
    float maxY() const { return position_.y() + size_.y(); }

    //! the top right corner point
    Vector2 topRight() const;

    //! the bottom left corner point
    Vector2 bottomLeft() const;

    //! the top left corner point
    Vector2 topLeft() const;

    //! the bottom right corner point
    Vector2 bottomRight() const;

    //! returns the current size
    Vector2 size() const { return size_; }

    //! move to the passed in point
    Rectangle& setPosition(const Vector2& p)
    {
        position_ = p;
        return *this;
    }

    //! resize to the passed in size
    Rectangle& setSize(const Vector2& s)
    {
        size_ = s;
        return *this;
    }


    //! translate through the passed in offset
    Rectangle& translateBy(const Vector2& p);

    //! \brief expands the rectangle in all directions by the passed in amount
    //! \note Accepts negative values
    Rectangle& expandBy(float amount);

    //! \brief expands the rectangle horizontally by the passed in amount
    //! \note Accepts negative values
    Rectangle& expandWidthBy(float amount);

    //! \brief expands the rectangle horizontally by the passed in amount
    //! \note Accepts negative values
    Rectangle& expandHeightBy(float amount);

    //! the height of the rectangle
    float height() const;

    //! the width of the rectangle
    float width() const;

    //! Snaps the size and position of the rectangle to the nearest whole number positions
    Rectangle& snap();


    // ---------------------------------------------------------------------------------------------
    //  Operators
    // ---------------------------------------------------------------------------------------------
public:
    //! equality
    bool operator == (const Rectangle& rect) const;

    //! inequality
    bool operator != (const Rectangle& rect) const;



    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    //! lowest value corner of the rectangle
    Vector2 position_;

    //! extents of the rectangle
    Vector2 size_;


    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        anArchive & TAGGED_OBJECT(position) & TAGGED_OBJECT(size);
    }
};


//! output operator
std::ostream& operator << (std::ostream& os, const Rectangle& rect);


// *************************************************************************************************
} // namespace Core


// *************************************************************************************************
} // namespace Gooey



#endif
