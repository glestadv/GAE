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
#include <png.h>

#include "mmgr/mmgr.h"

#include "pngImage/PNGImage.h"

#include "glgooey/core/StandardException.h"



// *************************************************************************************************
namespace Gooey
{




// *************************************************************************************************
namespace
{

//! The number of bytes used to check whether a file is actually a PNG file
const unsigned int numBytesForPNGCheck = 4;

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
isValidPNGFile(FILE *fp)
{
    png_byte buf[numBytesForPNGCheck];

    for(size_t i = 0; i < numBytesForPNGCheck; i++)
    {
        if (!(buf[i] = fgetc(fp))) return false; // ### RETURN ###
    }
    return (!png_sig_cmp(buf, 0, numBytesForPNGCheck));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



std::map<std::string, PNGImage*> imageMap;

// *************************************************************************************************
} // anonymous namespace





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Image*
Image::create(const std::string& aFileName)
{
    std::map<std::string, PNGImage*>::iterator imageIterator = imageMap.find(aFileName);
    if(imageIterator == imageMap.end())
    {
        imageMap[aFileName] = new PNGImage(aFileName);
    }

    imageIterator = imageMap.find(aFileName);
    ASSERT(imageIterator != imageMap.end());

    imageIterator->second->incrementReferenceCount();

    return imageIterator->second;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
user_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    voidp read_io_ptr = png_get_io_ptr(png_ptr) ;
    fread( (unsigned char*) data, length, 1, (FILE*) read_io_ptr ) ;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PNGImage::PNGImage(const std::string& aFileName) :
    ReferenceCountedImage(aFileName),
    data_(0)
{
    FILE *fp = fopen(aFileName.c_str(), "rb");
    if(fp == 0)
    {
        THROW(aFileName + " could not be opened.");
    }
    if(!isValidPNGFile(fp))
    {
        fclose(fp);
        THROW(aFileName + " is not a valid png file");
    }


    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if(png_ptr == 0)
    {
        fclose(fp);
        THROW("png_create_read_struct failed for " + aFileName);
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == 0)
    {
        png_destroy_read_struct(&png_ptr, png_infopp(0), png_infopp(0));
        fclose(fp);
        THROW("png_create_info_struct failed for " + aFileName);
    }


    if(setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, png_infopp(0), png_infopp(0));
        fclose(fp);
        THROW("setjmp failed for " + aFileName);
    }

    png_set_read_fn( png_ptr, fp, (png_rw_ptr)user_read_data ) ;	

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, numBytesForPNGCheck);

    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, 0);
    const unsigned int width = png_get_image_width(png_ptr, info_ptr);
    const unsigned int height = png_get_image_height(png_ptr, info_ptr);
    const unsigned int bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    if(bit_depth != 8)
    {
        fclose(fp);
        png_destroy_info_struct(png_ptr, png_infopp(&info_ptr));
        png_destroy_read_struct(&png_ptr, png_infopp(0), png_infopp(0));
        THROW(aFileName + " has an unsupported bit depth. The bit depth must be 8.");
    }
    const png_byte colorType = png_get_color_type(png_ptr, info_ptr);
    if((colorType != PNG_COLOR_TYPE_RGB) && (colorType != PNG_COLOR_TYPE_RGB_ALPHA))
    {
        fclose(fp);
        png_destroy_info_struct(png_ptr, png_infopp(&info_ptr));
        png_destroy_read_struct(&png_ptr, png_infopp(0), png_infopp(0));
        THROW(aFileName + " has an unsupported color type. RGB and RGBA images are supported.");
    }

    const int bytesPerPixel = (colorType == PNG_COLOR_TYPE_RGB) ? 3 : 4;
    const int stride = bytesPerPixel * width;

    unsigned char* data = new unsigned char[stride * height];
    png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);
    for(unsigned int i = 0; i < height; ++i)
    {
        const unsigned int row = height - i - 1;
        memcpy(data + (row * stride), row_pointers[i], stride);
    }

    size_.set(width, height);
    numberOfBitsPerPixel_ = bytesPerPixel * 8;
    data_ = data;

    fclose(fp);
    png_destroy_info_struct(png_ptr, png_infopp(&info_ptr));
    png_destroy_read_struct(&png_ptr, png_infopp(0), png_infopp(0));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<








// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PNGImage::~PNGImage()
{
    ASSERT(referenceCount() == 0);

    imageMap.erase(imageMap.find(fileName()));

    delete[] data_;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// *************************************************************************************************
} // namespace Gooey

