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
#ifndef __GOOEY_ENUMERATION_H__
#define __GOOEY_ENUMERATION_H__

#include <string>
#include <cstdarg>

#include "glgooey/core/Serialization.h"

// *************************************************************************************************
namespace Gooey
{


// *************************************************************************************************
namespace Core
{

template<typename EnumType, EnumType rangeStart, EnumType rangeEnd, typename Names>
class Enum
{
public:
    Enum(EnumType aValue = rangeStart) : value_(aValue) {}

public:
    operator EnumType () const { return value_; }

public:
    template<typename Archive> void serialize(Archive& anArchive)
    {
        std::string enum_ = Names::name(int(value_)-int(rangeStart));
        anArchive & TAGGED_OBJECT(enum);
        value_ = valueFromName(enum_);
    }

    bool operator== (EnumType aValue) const { return aValue == value_; }
    bool operator!= (EnumType aValue) const { return aValue != value_; }

private:
    EnumType valueFromName(const std::string& aName)
    {
        int index = int(rangeStart);
        while(std::string(aName) != std::string(Names::name(index-rangeStart)))
        {
            ++index;
            if(index > int(rangeEnd)) throw "Name cannot be converted to enum value";
        }
        return static_cast<EnumType>(index);
    }

    EnumType value_;
};

#define Enum2(Name, e1, e2) \
    enum E##Name { e1 = 0x00, e2 }; \
    class NamesE##Name \
    { \
    public: \
        static const char* name(int i) \
        { \
            static const char* names[] = { #e1, #e2 }; \
            return names[i]; \
        } \
    }; \
    typedef Gooey::Core::Enum<E##Name, e1, e2, NamesE##Name> Name;


#define Enum3(Name, e1, e2, e3) \
    enum E##Name { e1 = 0x00, e2, e3 }; \
    class NamesE##Name \
    { \
    public: \
        static const char* name(int i) \
        { \
            static const char* names[] = { #e1, #e2, #e3 }; \
            return names[i]; \
        } \
    }; \
    typedef Gooey::Core::Enum<E##Name, e1, e3, NamesE##Name> Name;


#define Enum4(Name, e1, e2, e3, e4) \
    enum E##Name { e1 = 0x00, e2, e3, e4 }; \
    class NamesE##Name \
    { \
    public: \
        static const char* name(int i) \
        { \
            static const char* names[] = { #e1, #e2, #e3, #e4 }; \
            return names[i]; \
        } \
    }; \
    typedef Gooey::Core::Enum<E##Name, e1, e4, NamesE##Name> Name;


#define Enum5(Name, e1, e2, e3, e4, e5) \
    enum E##Name { e1 = 0x00, e2, e3, e4, e5 }; \
    class NamesE##Name \
    { \
    public: \
        static const char* name(int i) \
        { \
            static const char* names[] = { #e1, #e2, #e3, #e4, #e5 }; \
            return names[i]; \
        } \
    }; \
    typedef Gooey::Core::Enum<E##Name, e1, e5, NamesE##Name> Name;


#define Enum9(Name, e1, e2, e3, e4, e5, e6, e7, e8, e9) \
    enum E##Name { e1 = 0x00, e2, e3, e4, e5, e6, e7, e8, e9 }; \
    class NamesE##Name \
    { \
    public: \
        static const char* name(int i) \
        { \
            static const char* names[] = { #e1, #e2, #e3, #e4, #e5, #e6, #e7, #e8, #e9 }; \
            return names[i]; \
        } \
    }; \
    typedef Gooey::Core::Enum<E##Name, e1, e9, NamesE##Name> Name;

// *************************************************************************************************
} // namespace Core

// *************************************************************************************************
} // namespace Gooey



#endif
