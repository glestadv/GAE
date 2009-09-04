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
#ifndef __GOOEY_CORE_XML_INPUT_ARCHIVE_H__
#define __GOOEY_CORE_XML_INPUT_ARCHIVE_H__

#include <string>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <set>

#include "tinyxml.h"
#include "glgooey/core/Serialization.h"
#include "glgooey/core/Utils.h"



#ifndef _MSC_VER 

// *************************************************************************************************
namespace Gooey
{


// *************************************************************************************************
namespace Core
{



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Archive for reading in xml data
class XMLInputArchive
{
    // ----------------------------------------------------------------------------------------------
    //  Xstruction
    // ----------------------------------------------------------------------------------------------
public:
    //! Construction requires the stream the archive is to operate on
    XMLInputArchive(const std::string& aFileName) : document_(aFileName.c_str()), currentNode_(&document_) { document_.LoadFile(); }

    //! Destructor
    virtual ~XMLInputArchive() {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! The input/output operator
    template<typename T> XMLInputArchive& operator& (const ObjectNamer<T>& aNamedObject);


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    //! \brief Read in a generic object
    //!
    //! when the compiler works out which \c read method to use, this is the one it falls back
    //! on as a last resort. Any objects that end up being read here must support the
    //! \c serialize method. In general user defined classes will be handled here
    template<typename T> void read(T& anObject) { anObject.serialize(*this); }

    //! Used to support serialization of object pointers
    template<typename T> void read(T*& anObject);

    //! Used to support serialization of STL vectors
    template<typename T> void read(std::vector<T>&   aContainer) { readContainer(aContainer); }

    //! Used to support serialization of STL lists
    template<typename T> void read(std::list<T>&     aContainer) { readContainer(aContainer); }

    //! Used to support serialization of STL deques
    template<typename T> void read(std::deque<T>&    aContainer) { readContainer(aContainer); }

    //! Used to support serialization of STL maps
    template<typename T1, typename T2> void read(std::map<T1, T2>& aContainer) { readContainer(aContainer); }

    //! Used to support serialization of std::pair objects as used in maps and multimaps
    template<typename T1, typename T2> void read(std::pair<const T1, T2>& aPair);

    //! Used to support serialization of char values
    void read(char& aChar);

    //! Used to support serialization of unsigned char values
    void read(unsigned char& anUnsignedChar);

    //! Used to support serialization of integer values
    void read(int& anInteger);

    //! Used to support serialization of unsigned integer values
    void read(unsigned int& anUnsignedInteger);

    //! Used to support serialization of boolean values
    void read(bool& aBoolean);

    //! Used to support serialization of floating point values
    void read(float& aFloat);

    //! Used to support serialization of double values
    void read(double& aDouble);

    //! Used to support serialization of std::strings
    void read(std::string& aString);

    //! Generic method for serialization of primitive data types
    template<typename T> void readPrimitive(T& aPrimitive)
    {
        TiXmlNode* textNode = currentNode_->FirstChild();
        if(textNode != 0)
        {
            TiXmlText* text = textNode->ToText();
            if(text != 0) aPrimitive = string_cast<T>(text->Value());
        }
    }

    //! Generic method for serialization of STL containers
    template<typename Container> void readContainer(Container& aContainer);

    //! If we're serializing a dynamic object, this method gets hold of the class name
    void determineClassName(TiXmlElement* anElement);

    TiXmlDocument document_;  //!< The tinyxml document object
    TiXmlNode* currentNode_;  //!< The node that is currently being processed

    std::string className_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<









// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template<typename T> 
XMLInputArchive& 
XMLInputArchive::operator& (const ObjectNamer<T>& aNamedObject)
{
    TiXmlElement* element = currentNode_->FirstChildElement(aNamedObject.name());
    if(element != 0)
    {
        TiXmlNode* thisNode = currentNode_;
        currentNode_ = element;

        determineClassName(element);

        read(aNamedObject.object());

        currentNode_ = thisNode;
    }
    return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template<typename T>
void
XMLInputArchive::read(T*& anObject)
{
    if(anObject != 0) delete anObject;
    if(currentNode_->FirstChild() != 0)
    {
        anObject = reinterpret_cast<T*>(FactoryInterface::createClass(className_));
        FactoryInterface::factoryByNiceName(className_)
            ->serialize(typeid(*this).name(), (void*)(this), (void*&)(anObject));
    }
    else
    {
        anObject = 0;
    }
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template<typename T1, typename T2>
void
XMLInputArchive::read(std::pair<const T1, T2>& aPair)
{
    TiXmlNode* thisNode = currentNode_;
    for(TiXmlNode* child = thisNode->FirstChild(); child != 0; child = child->NextSibling())
    {
        TiXmlElement* element = child->ToElement();
        if(element != 0)
        {
            determineClassName(element);
            currentNode_ = child;
            if(std::string(element->Value()) == "first")
                read(const_cast<T1&>(aPair.first));
            else if(std::string(element->Value()) == "second")
                read(aPair.second);
        }
    }
    currentNode_ = thisNode;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<











// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template<typename Container>
void
XMLInputArchive::readContainer(Container& aContainer)
{
    aContainer.clear();
    TiXmlNode* thisNode = currentNode_;
    for(TiXmlNode* child = thisNode->FirstChild(); child != 0; child = child->NextSibling())
    {
        TiXmlElement* element = child->ToElement();
        if( (element != 0) && (std::string(element->Value()) == "item") )
        {
            determineClassName(element);
            currentNode_ = child;
            typename Container::value_type entry;
            read(entry);
            aContainer.insert(aContainer.end(), entry);
        }
    }
    currentNode_ = thisNode;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// *************************************************************************************************
} // namespace Core

// *************************************************************************************************
} // namespace Gooey


#else



// *************************************************************************************************
namespace Gooey
{


// *************************************************************************************************
namespace Core
{



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//! Archive for reading in xml data
class XMLInputArchive
{
    // ----------------------------------------------------------------------------------------------
    //  Xstruction
    // ----------------------------------------------------------------------------------------------
public:
    //! Construction requires the stream the archive is to operate on
    XMLInputArchive(const std::string& aFileName) : document_(aFileName.c_str()), currentNode_(&document_) { document_.LoadFile(); }

    //! Destructor
    virtual ~XMLInputArchive() {}


    // ---------------------------------------------------------------------------------------------
    //  Public Interface
    // ---------------------------------------------------------------------------------------------
public:
    //! The input/output operator
    template<typename T> XMLInputArchive& operator& (const ObjectNamer<T>& aNamedObject)
    {
        TiXmlElement* element = currentNode_->FirstChildElement(aNamedObject.name());
        if(element != 0)
        {
            TiXmlNode* thisNode = currentNode_;
            currentNode_ = element;

            determineClassName(element);

            read(aNamedObject.object(), TypeFinder::typeOf(aNamedObject.object()));

            currentNode_ = thisNode;
        }
        return *this;
    }


    // ---------------------------------------------------------------------------------------------
    //  Implementation
    // ----------------------------------------------------------------------------------------------
private:
    //! \brief Read in a generic object
    //!
    //! when the compiler works out which \c read method to use, this is the one it falls back
    //! on as a last resort. Any objects that end up being read here must support the
    //! \c serialize method. In general user defined classes will be handled here
    template<typename T> void read(T& anObject, const IsObject&) { anObject.serialize(*this); }

    //! Used to support serialization of object pointers
    template<typename T> void read(T*& anObject, const IsPointer&)
    {
        if(currentNode_->FirstChild() != 0)
        {
            anObject = reinterpret_cast<T*>(FactoryInterface::createClass(className_));
            FactoryInterface::factoryByNiceName(className_)
                ->serialize(typeid(*this).name(), (void*)(this), (void*&)(anObject));
        }
        else
        {
            anObject = 0;
        }
    }

    //! Used to support serialization of STL vectors
    template<typename T> void read(std::vector<T>&   aContainer, const IsObject&) { readContainer(aContainer); }

    //! Used to support serialization of STL lists
    template<typename T> void read(std::list<T>&     aContainer, const IsObject&) { readContainer(aContainer); }

    //! Used to support serialization of STL deques
    template<typename T> void read(std::deque<T>&    aContainer, const IsObject&) { readContainer(aContainer); }

    //! Used to support serialization of STL maps
    template<typename T1, typename T2> void read(std::map<T1, T2>& aContainer, const IsObject&) { readContainer(aContainer); }

    //! Used to support serialization of std::pair objects as used in maps and multimaps
    template<typename T1, typename T2> void read(std::pair<const T1, T2>& aPair, const IsPair&)
    {
        TiXmlNode* thisNode = currentNode_;
        for(TiXmlNode* child = thisNode->FirstChild(); child != 0; child = child->NextSibling())
        {
            TiXmlElement* element = child->ToElement();
            if(element != 0)
            {
                determineClassName(element);
                currentNode_ = child;
                if(std::string(element->Value()) == "first")
                    read(const_cast<T1&>(aPair.first));
                else if(std::string(element->Value()) == "second")
                    read(aPair.second);
            }
        }
        currentNode_ = thisNode;
    }

    //! Used to support serialization of char values
    void read(char& aChar, const IsObject&);

    //! Used to support serialization of unsigned char values
    void read(unsigned char& anUnsignedChar, const IsObject&);

    //! Used to support serialization of integer values
    void read(int& anInteger, const IsObject&);

    //! Used to support serialization of unsigned integer values
    void read(unsigned int& anUnsignedInteger, const IsObject&);

    //! Used to support serialization of boolean values
    void read(bool& aBoolean, const IsObject&);

    //! Used to support serialization of floating point values
    void read(float& aFloat, const IsObject&);

    //! Used to support serialization of double values
    void read(double& aDouble, const IsObject&);

    //! Used to support serialization of std::strings
    void read(std::string& aString, const IsObject&);

    //! Generic method for serialization of primitive data types
    template<typename T> void readPrimitive(T& aPrimitive)
    {
        TiXmlNode* textNode = currentNode_->FirstChild();
        if(textNode != 0)
        {
            TiXmlText* text = textNode->ToText();
            if(text != 0) aPrimitive = string_cast<T>(text->Value());
        }
    }

    //! Generic method for serialization of STL containers
    template<typename Container> void readContainer(Container& aContainer)
    {
        aContainer.clear();
        TiXmlNode* thisNode = currentNode_;
        for(TiXmlNode* child = thisNode->FirstChild(); child != 0; child = child->NextSibling())
        {
            TiXmlElement* element = child->ToElement();
            if( (element != 0) && (std::string(element->Value()) == "item") )
            {
                determineClassName(element);
                currentNode_ = child;
                typename Container::value_type entry;
                read(entry);
                aContainer.insert(aContainer.end(), entry);
            }
        }
        currentNode_ = thisNode;
    }

    //! If we're serializing a dynamic object, this method gets hold of the class name
    void determineClassName(TiXmlElement* anElement);

    TiXmlDocument document_;  //!< The tinyxml document object
    TiXmlNode* currentNode_;  //!< The node that is currently being processed

    std::string className_;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// *************************************************************************************************
} // namespace Core

// *************************************************************************************************
} // namespace Gooey


#endif

#endif
