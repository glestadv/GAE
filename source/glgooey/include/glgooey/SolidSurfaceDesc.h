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
#ifndef SOLIDSURFACEDESC__H
#define SOLIDSURFACEDESC__H

#include "mmgr/mmgr.h"

#include "glgooey/RenderDesc.h"
#include "glgooey/Color.h"
#include "glgooey/Renderer.h"

#include "glgooey/core/Alignment.h"
#include "glgooey/core/Serialization.h"

#include "glgooey/services/interfaces/Image.h"



// *************************************************************************************************
//  Forward Declarations
namespace Gooey
{
    class TextRenderer;
}
// *************************************************************************************************




// *************************************************************************************************
namespace Gooey
{








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! The available styles for rendering unfilled rectangles
Enum3(RectangleStyle, normal, dropped, raised);
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! The available styles with which a texture may be mapped onto a quad
Enum2(Mapping, tiled, stretched);
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief The frame of a frame window is split into sections. This enum
//!     provides identifiers for each of those sections
Enum9(FrameSection, right, topRight, top, topLeft, left, bottomLeft, bottom, bottomRight, title);
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Used to describe the orientation of various elements
Enum2(Orientation, horizontal, vertical);
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Render desc describing a monochromatic rectangular surface
class SolidSurfaceDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    SolidSurfaceDesc(const Color& aColor = Color(0.35f, 0.35f, 0.35f, 0.5f)) : color_(aColor) {}

    //! Destructor
    virtual ~SolidSurfaceDesc() {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Returns the color of the surface
    Color color() const { return color_; }

    //! sets the color of the surface
    SolidSurfaceDesc& setColor(const Color& aColor)
    {
        color_ = aColor;
        return *this;
    }

    //! Fills the passed in area with the associated color
    virtual void draw(const Core::Rectangle& area) const;


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    Color color_;  //!< The color used to fill areas when drawing


    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive & TAGGED_OBJECT(color);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Render desc describing a surface with a color gradient
class GradientSurfaceDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    GradientSurfaceDesc() :
        color1_(Color(0.35f, 0.35f, 0.35f, 0.5f)),
        color2_(Color(0.35f, 0.35f, 0.35f, 0.5f)),
        orientation_(horizontal) {}

    //! Constructor using parameters
    GradientSurfaceDesc(const Color& aColor1, const Color& aColor2, Orientation anOrientation) :
        color1_(aColor1),
        color2_(aColor2),
        orientation_(anOrientation) {}

    //! Destructor
    virtual ~GradientSurfaceDesc() {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Returns the value of color 1
    Color color1() const { return color1_; }

    //! Sets the value of color 1
    GradientSurfaceDesc& setColor1(const Color& aColor)
    {
        color1_ = aColor;
        return *this;
    }

    //! Returns the value of color 2
    Color color2() const { return color2_; }

    //! Sets the value of color 2
    GradientSurfaceDesc& setColor2(const Color& aColor)
    {
        color2_ = aColor;
        return *this;
    }

    //! sets both colors of the surface
    GradientSurfaceDesc& setColors(const Color& aColor1, const Color& aColor2)
    {
        color1_ = aColor1;
        color2_ = aColor2;
        return *this;
    }

    //! Returns the orientation of the gradient
    Orientation orientation() const { return orientation_; }

    //! Sets the orientation of the gradient
    GradientSurfaceDesc& setOrientation(Orientation anOrientation)
    {
        orientation_ = anOrientation;
        return *this;
    }

    //! \brief Fills the passed in area with a color gradient described by the colors
    //! and the orientation
    virtual void draw(const Core::Rectangle& area) const;


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    Color color1_;  //!< The first color in the gradient
    Color color2_;  //!< The second color in the gradient
    Orientation orientation_; //! The orientation of the gradient (vertical or horizontal)


    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive & TAGGED_OBJECT(color1) & TAGGED_OBJECT(color2) & TAGGED_OBJECT(orientation);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Render desc describing a rectangular frame
//!
//! If the ractangle style is \c normal, the rectangle will be drawn in one color only, so only
//! the \c Color1 field is important. The \c Raised and \c Dropped styles on the other hand
//! require that one coor is set for the highlighted part of the fram and one for the shadowed part.
class RectangleDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    explicit RectangleDesc(RectangleStyle aStyle = normal) :
        style_(aStyle),
        color1_(Color(1, 1, 1, 0.5f)),
        color2_(Color(0, 0, 0, 0.5f)),
        lineWidth_(1.0f) {}

    //! Construction using parameters
    RectangleDesc(RectangleStyle aStyle, const Color& aColor1, const Color& aColor2, float aLineWidth) :
        style_(aStyle),
        color1_(aColor1),
        color2_(aColor2),
        lineWidth_(aLineWidth) {}

    //! Destructor
    virtual ~RectangleDesc() {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Returns the style
    RectangleStyle style() const { return style_; }

    //! Sets the style
    RectangleDesc& setStyle(RectangleStyle aStyle)
    {
        style_ = aStyle;
        return *this;
    }

    //! Returns the value of color 1
    Color color1() const { return color1_; }

    //! Sets the value of color 1
    RectangleDesc& setColor1(const Color& aColor)
    {
        color1_ = aColor;
        return *this;
    }

    //! Returns the value of color 2
    Color color2() const { return color2_; }

    //! Sets the value of color 2
    RectangleDesc& setColor2(const Color& aColor)
    {
        color2_ = aColor;
        return *this;
    }

    //! sets the colors of the rectangle
    RectangleDesc& setColors(const Color& aColor1, const Color& aColor2)
    {
        color1_ = aColor1;
        color2_ = aColor2;
        return *this;
    }

    //! Returns the line width
    float lineWidth() const { return lineWidth_; }

    //! Sets the line width
    RectangleDesc& setLineWidth(float aLineWidth)
    {
        lineWidth_ = aLineWidth;
        return *this;
    }

    //! draws on the passed in area according to the descriptor
    virtual void draw(const Core::Rectangle& area) const;


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    RectangleStyle style_; //!< style may be \c Normal, \c Raised or \c Dropped
    Color color1_;         //!< The first color is used by all three styles
    Color color2_;         //!< The second color is used only by the \c Raised and \c Dropped styles
    float lineWidth_;      //!< The line width to use when drawing the rectangle

    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive &
            TAGGED_OBJECT(style) &
            TAGGED_OBJECT(color1) &
            TAGGED_OBJECT(color2) &
            TAGGED_OBJECT(lineWidth);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Render desc describing text
class TextDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    TextDesc();

    //! Constructor
    TextDesc(const std::string& aName, int aPixelHeight, float anXOffset, float aYOffset, float aMargin,
            const Color& aColor);

    //! Destructor
    virtual ~TextDesc();


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Draws on the passed in area according to the descriptor
    virtual void draw(const Core::Rectangle& anArea) const;

    //! Returns the file name of the font to be used
    std::string fileName() const { return fileName_; }

    //! Sets the file name of the font to be used
    TextDesc& setFileName(const std::string& aFileName) { fileName_ = aFileName; return *this; }

    //! Returns the point size of the font to be used
    int pointSize() const { return pixelHeight_; }

    //! Sets the point size of the font to be used
    TextDesc& setPointSize(int aPointSize) { pixelHeight_ = aPointSize; return *this; }

    //! Returns the x offset of the text relative to the position determined by its alignment
    float xOffset() const { return xOffset_; }

    //! Sets the x offset of the text relative to the position determined by its alignment
    TextDesc& setXOffset(float anOffset) { xOffset_ = anOffset; return *this; }

    //! Returns the y offset of the teyt relative to the position determined by its alignment
    float yOffset() const { return yOffset_; }

    //! Sets the y offset of the teyt relative to the position determined by its alignment
    TextDesc& setYOffset(float anOffset) { yOffset_ = anOffset; return *this; }

    //! Set the text to be rendered
    TextDesc& setText(const std::string& aText) { text_ = aText; return *this; }

    //! Returns the size of the text when rendered with this desc
    Core::Vector2 textSize() const;

    //! Returns the height of a line using this text desc
    float lineHeight() const;

    //! \brief Returns the index of the character at the position determined by the passed in
    //! offset from the beginning of the string
    size_t hitCharacterIndex(const std::string& aString, float anOffset) const;

    //! Returns the color to use when rendering the text
    Color color() const { return color_; }

    //! Sets the color to use when rendering the text
    TextDesc& setColor(const Color& aColor) { color_ = aColor; return *this; }

    //! Returns the size of the margin around the text
    float margin() const { return margin_; }

    //! Sets the size of the margin around the text
    TextDesc& setMargin(float aMargin) { margin_ = aMargin; return *this; }

    //! Sets the horizontal alignment of the text
    TextDesc& setHorizontalAlignment(Core::Alignment::Horizontal aHorizontalAlignment)
    {
        horizontalAlignment_ = aHorizontalAlignment;
        return *this;
    }

    //! \brief Sets the vertical alignment of the text
    //! \note This will only apply to single line text. Multiline text is always top aligned
    TextDesc& setVerticalAlignment(Core::Alignment::Vertical aVerticalAlignment)
    {
        verticalAlignment_ = aVerticalAlignment;
        return *this;
    }

    //! Call this to break the text into multiple lines so that it fits in the render area
    TextDesc& drawAsMultiLine() { isMultiLine_ = true; return *this; }

    //! Call this to draw the text on a single line even if it is too large for the render area
    TextDesc& drawAsSingleLine() { isMultiLine_ = false; return *this; }

    //! Brings the text renderer up to date with the filename and pixel size
    void reloadTextRenderer();



    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    std::string fileName_; //!< The name of the file which stores the font
    int pixelHeight_;      //!< The pixel height (size) of the font
    float xOffset_;        //!< The x offset by which the text is moved relative to its default position.
    float yOffset_;        //!< The y offset by which the text is moved relative to its default position.
    float margin_;         //!< The margin surrounding the text when it is drawn on a rectangular area
    Color color_;          //!< The color in which the text should be rendered
    std::string text_;     //!< The text to be rendered
    bool isMultiLine_;     //!< Set this to true if the text is to be drawn on multiple lines
    Core::Alignment::Horizontal horizontalAlignment_;  //!< How the text is aligned horizontally
    Core::Alignment::Vertical   verticalAlignment_;    //!< How the text is aligned vertically

    TextRenderer* textRenderer_; //!< A pointer to the text renderer

    // Copying and assignment are not allowed
    TextDesc(const TextDesc& aTextDesc);
    TextDesc& operator = (const TextDesc& aTextDesc);

    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive &
            TAGGED_OBJECT(fileName) &
            TAGGED_OBJECT(pixelHeight) &
            TAGGED_OBJECT(xOffset) &
            TAGGED_OBJECT(yOffset) &
            TAGGED_OBJECT(margin) &
            TAGGED_OBJECT(color) &
            TAGGED_OBJECT(horizontalAlignment) &
            TAGGED_OBJECT(verticalAlignment);

        reloadTextRenderer();
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Render desc describing a standard GLGooey check box check mark
class DefaultCheckDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    explicit DefaultCheckDesc(const Color& aColor = Color(1, 1, 1, 0.75f)) : color_(aColor) {}

    //! Destructor
    virtual ~DefaultCheckDesc() {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Draws on the passed in area according to the descriptor
    virtual void draw(const Core::Rectangle& area) const;

    //! Returns the color of the check mark
    Color color() const { return color_; }

    //! sets the color of the check mark
    DefaultCheckDesc& setColor(const Color& aColor)
    {
        color_ = aColor;
        return *this;
    }


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    Color color_; //!< The color in which to draw the check mark


    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive & TAGGED_OBJECT(color);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Render desc describing a section of the standard GLGooey frame window frame.
class DefaultFrameDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    explicit DefaultFrameDesc(FrameSection aSection = top) : section_(aSection),
        backgroundColor_(Color(0.35f, 0.35f, 0.35f, 0.5f)),
        foregroundColor_(Color(1, 1, 1, 0.75f)),
        borderLineWidth_(5) {}

    //! Constructor using parameters
    DefaultFrameDesc(FrameSection aSection, const Color& aBackgroundColor,
                  const Color& aForegroundColor, int aBorderLineWidth) :
        section_(aSection),
        backgroundColor_(aBackgroundColor),
        foregroundColor_(aForegroundColor),
        borderLineWidth_(aBorderLineWidth) {}

    //! Destructor
    virtual ~DefaultFrameDesc() {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Draws on the passed in area according to the descriptor
    virtual void draw(const Core::Rectangle& area) const;

    //! Returns the section this desc applies to
    FrameSection frameSection() const { return section_; }

    //! Sets the frame section this desc applies to
    DefaultFrameDesc& setFrameSection(FrameSection aFrameSection)
    {
        section_ = aFrameSection;
        return *this;
    }

    //! Returns the value of the background color
    Color backgroundColor() const { return backgroundColor_; }

    //! Sets the value of the background color
    DefaultFrameDesc& setBackgroundColor(const Color& aColor)
    {
        backgroundColor_ = aColor;
        return *this;
    }

    //! Returns the value of the foreground color
    Color foregroundColor() const { return foregroundColor_; }

    //! Sets the value of the foreground color
    DefaultFrameDesc& setForegroundColor(const Color& aColor)
    {
        foregroundColor_ = aColor;
        return *this;
    }

    //! Returns the border line width
    int borderLineWidth() const { return borderLineWidth_; }

    //! Sets the border line width
    DefaultFrameDesc& setBorderLineWidth(int aLineWidth)
    {
        borderLineWidth_ = aLineWidth;
        return *this;
    }



    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    FrameSection section_;  //!< The section represented by this object
    Color backgroundColor_; //!< The background color of the section
    Color foregroundColor_; //!< The color of the line running through the section
    int   borderLineWidth_; //!< The width of the line running through the section


    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        RenderDesc::serialize(anArchive);
        anArchive &
            TAGGED_OBJECT(section) &
            TAGGED_OBJECT(backgroundColor) &
            TAGGED_OBJECT(foregroundColor) &
            TAGGED_OBJECT(borderLineWidth);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Render desc describing a surface with a bitmap mapped onto it
class BitmapDesc : public RenderDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    BitmapDesc() : fileName_(""), mapping_(stretched), image_(0), top_(0), bottom_(-1), left_(0), right_(-1) {}

    //! Construction using parameters
    BitmapDesc(const std::string& aFileName, Mapping aMapping,
               int aTopOffset, int aBottomOffset, int aLeftOffset, int aRightOffset) :
        fileName_(aFileName),
        mapping_(aMapping),
        image_(0),
        top_(aTopOffset),
        bottom_(aBottomOffset),
        left_(aLeftOffset),
        right_(aRightOffset)
    {
        image_ = Image::create(fileName_);
        Renderer::registerImage(*image_);
    }

    //! Destructor
    virtual ~BitmapDesc()
    {
        if(image_ != 0) image_->release();
    }


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Draws on the passed in area according to the descriptor
    virtual void draw(const Core::Rectangle& area) const;

    //! Returns the filename of the bitmap to be used
    std::string fileName() const { return fileName_; }

    //! Sets the filename of the bitmap to be used
    void setFileName(const std::string& aFileName) { fileName_ = aFileName; }

    //! Returns the mapping to be used
    Mapping mapping() const { return mapping_; }

    //! Sets the mapping to be used
    void setMapping(Mapping aMapping) { mapping_ = aMapping; }

    //! \brief Sets the offset in the bitmap to the top of the image to be
    //! used (in pixels)
    void setTop(int aTop) { top_ = aTop; }

    //! \brief Sets the offset in the bitmap to the bottom of the image to be
    //! used (in pixels)
    void setBottom(int aBottom) { bottom_ = aBottom; }

    //! \brief Sets the offset in the bitmap to the left of the image to be
    //! used (in pixels)
    void setLeft(int aLeft) { left_ = aLeft; }

    //! \brief Sets the offset in the bitmap to the right of the image to be
    //! used (in pixels)
    void setRight(int aRight) { right_ = aRight; }

    //! \brief Returns the offset in the bitmap to the top of the image to be
    //! used (in pixels)
    int top() const { return top_; }

    //! \brief Returns the offset in the bitmap to the bottom of the image to be
    //! used (in pixels)
    int bottom() const { return bottom_; }

    //! \brief Returns the offset in the bitmap to the left of the image to be
    //! used (in pixels)
    int left() const { return left_; }

    //! \brief Returns the offset in the bitmap to the right of the image to be
    //! used (in pixels)
    int right() const { return right_; }


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
protected:
    std::string fileName_; //!< The name of the file which stores the bitmap
    Mapping mapping_;      //!< How to map the bitmap on to the surface (either \c Tiled or \c Stretched)
    Image* image_;         //!< A pointer to the image

private:
    int top_;              //!< Offset in the bitmap to the top of the image to be used (in pixels)
    int bottom_;           //!< Offset in the bitmap to the bottom of the image to be used (in pixels)
    int left_;             //!< Offset in the bitmap to the left of the image to be used (in pixels)
    int right_;            //!< Offset in the bitmap to the right of the image to be used (in pixels)


    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        std::string oldFileName = fileName_;
        RenderDesc::serialize(anArchive);
        anArchive &
            TAGGED_OBJECT(fileName) &
            TAGGED_OBJECT(mapping) &
            TAGGED_OBJECT(top) &
            TAGGED_OBJECT(bottom) &
            TAGGED_OBJECT(left) &
            TAGGED_OBJECT(right);

        if(fileName_ != oldFileName)
        {
            if(image_ != 0)
            {
                image_->release();
            }
            image_ = Image::create(fileName_);
            Renderer::registerImage(*image_);
        }
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! \brief Render desc describing a surface with a sectioned bitmap mapped onto it
//!
//! The bitmap is considered to be split into three columns, each of which is
//! defined by an offset from the split to the left and right edges of the
//! image respectively.
//!
//! When the Desc is rendered, it renders the two outer columns pixel for
//! pixel and stretches the inner column across the remaining space. With left
//! and right offsets of 0, this desc will do exactly what a normal BitmapDesc
//! does when the mapping is set to stretched.
class Bitmap1x3Desc : public BitmapDesc
{
    // ---------------------------------------------------------------------------------------------
    //  Xstruction
    // ---------------------------------------------------------------------------------------------
public:
    //! Default Constructor
    Bitmap1x3Desc() : leftOffset_(0), rightOffset_(0) {}

    //! Destructor
    virtual ~Bitmap1x3Desc() {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! Draws on the passed in area according to the descriptor
    virtual void draw(const Core::Rectangle& area) const;


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ---------------------------------------------------------------------------------------------
private:
    int leftOffset_;  //!< The offset from the left edge of the bitmap to the left split (>= 0)
    int rightOffset_; //!< The offset from the right split to the right edge of the bitmap (>= 0)


    // ---------------------------------------------------------------------------------------------
    //  Serialization
    // ----------------------------------------------------------------------------------------------
public:
    //! Serialization method
    template<typename Archive> void serialize(Archive& anArchive)
    {
        BitmapDesc::serialize(anArchive);
        anArchive & TAGGED_OBJECT(leftOffset) & TAGGED_OBJECT(rightOffset);
    }
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




} // namespace Gooey

#endif
