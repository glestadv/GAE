// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa,
//				  2008 Daniel Santos <daniel.santos@pobox.com>,
//				  2009 Nathan Turner <hailstone3>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

//#include "pch.h"
#include "xml_parser.h"

#include <fstream>
#include <stdexcept>

#include "conversion.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Shared { namespace Xml {

const string defaultIndent = string("  ");

// =====================================================
//	class XmlIo
// =====================================================

bool XmlIo::initialized= false;

XmlIo &XmlIo::getInstance(){
	static XmlIo XmlIo;
	return XmlIo;
}

XmlNode *XmlIo::load(const string &path){
	// creates a document from file

	TiXmlDocument document( path.c_str() );

	if ( !document.LoadFile() )	{
		char message[strSize];
		sprintf(message, "Error parsing XML, file: %s, line %i, %s", path.c_str(), document.ErrorRow(), document.ErrorDesc());
		throw runtime_error(message);
	}

	XmlNode *rootNode = new XmlNode(document.RootElement());

	return rootNode;
}

XmlNode *XmlIo::parseString(const char *doc, size_t size) {
	// creates a document from string

	TiXmlDocument document;

	document.Parse(doc); // returns const char* but not sure why

	if ( document.Error() ) {
		char message[strSize];
		sprintf(message, "Error parsing XML text: line %i, %s", document.ErrorRow(), document.ErrorDesc());
		throw runtime_error(message);
	}

	XmlNode *rootNode = new XmlNode(document.RootElement());

	return rootNode;
}

void XmlIo::save(const string &path, const XmlNode *node){		
	// doesn't keep: space chars, doc type declaration (although a generic one 
	//	can be added), any other text like comments

	TiXmlDocument document;

	TiXmlElement *rootElement = new TiXmlElement(node->getName().c_str());
	
	if ( !document.LinkEndChild(rootElement) ) { // TinyXML owns pointer
		throw runtime_error("Problem adding xml child element to: document");
	}

	node->populateElement(rootElement);

	if ( !document.SaveFile( path.c_str() ) ) {
		throw runtime_error("Unable to save xml file: " + path);
	}
}

// =====================================================
//	class XmlNode
// =====================================================
		
XmlNode::XmlNode(TiXmlNode *node) : text() {
	//no node
	if ( !node ) {
		name = "";
		return;
	}

	//get name
	name = node->ValueStr();

	//check document
	if (node->Type() == TiXmlNode::DOCUMENT) {
		name = "document";
	}

	//add children to node
	TiXmlElement *childElement = node->FirstChildElement();

	while ( childElement ) {
		XmlNode *xmlNode = new XmlNode(childElement); // recursive, null childElement as base
		
		children.push_back(xmlNode);
		childElement = childElement->NextSiblingElement();
	}

	//add attributes to node
	TiXmlElement *element = node->ToElement();
	TiXmlAttribute *attribute = element->FirstAttribute();

	while ( attribute ) {
		XmlAttribute *xmlAttribute = new XmlAttribute(attribute);
		attributes.push_back(xmlAttribute);

		attribute = attribute->Next();
	}


	// text
	if ( element->FirstChild() ) {
		TiXmlText *script = element->FirstChild()->ToText ();
		if ( script ) {
			script->SetCondenseWhiteSpace ( false );
			text = script->Value();
		}
	}
}

XmlNode::XmlNode(const string &name) {
	this->name = name;
}

XmlNode::~XmlNode(){
	for (int i = 0; i < children.size(); ++i) {
		delete children[i];
	}
	for (int i = 0; i < attributes.size(); ++i) {
		delete attributes[i];
	}
}

XmlAttribute *XmlNode::getAttribute(int i) const{
	if (i >= attributes.size()) {
		throw runtime_error(getName() + " node doesn't have " + intToStr(i) + " attributes");
	}
	return attributes[i];
}

XmlAttribute *XmlNode::getAttribute(const string &name, bool required) const{
	for (int i = 0; i < attributes.size(); ++i) {
		if (attributes[i]->getName() == name) {
			return attributes[i];
		}
	}
	if (!required) {
		return NULL;
	}
	throw runtime_error("\"" + getName() + "\" node doesn't have a attribute named \"" + name + "\"");
}

XmlNode *XmlNode::getChild(int i) const {
	if (i >= children.size()) {
		throw runtime_error("\"" + getName() + "\" node doesn't have "
				+ intToStr(i + 1) + " children");
	}
	return children[i];
}

XmlNode *XmlNode::getChild(const string &childName, int i, bool required) const{
	int count = 0;
	if (i < children.size()) {
		for (int j = 0; j < children.size(); ++j) {
			if (children[j]->getName() == childName) {
				if (count == i) {
					return children[j];
				}
				count++;
			}
		}
	}

	if (!required) {
		return NULL;
	}
	throw runtime_error("Node \"" + getName() + "\" doesn't have "
			+ intToStr(i + 1) + " children named  \"" + childName
			+ "\"\n\nTree: " + getTreeString());
}

XmlNode *XmlNode::addChild(const string &name){
	XmlNode *node= new XmlNode(name);
	children.push_back(node);
	return node;
}

XmlAttribute *XmlNode::addAttribute(const char *name, const char *value){
	XmlAttribute *attr= new XmlAttribute(name, value);
	attributes.push_back(attr);
	return attr;
}

auto_ptr<string> XmlNode::toString(bool pretty, const string &indentSingle) const {
	stringstream str;

	if (pretty) {
		string indent;
		toStringPretty(str, indent, indentSingle);
	} else {
		toStringSimple(str);
	}
	return auto_ptr<string>(new string(str.str()));
}

void XmlNode::toStringSimple(stringstream &str) const {
	bool needsClosingTag = children.size() || text.size();

	str << "<" << name;

	for (Attributes::const_iterator a = attributes.begin(); a != attributes.end(); ++a) {
		str << " ";
		(*a)->toString(str);
	}

	if (needsClosingTag) {
		str << "/>";
	} else {
		str << ">";
		if (text.size()) {
			str << text;
		}

		for (Nodes::const_iterator n = children.begin(); n != children.end(); ++n) {
			(*n)->toStringSimple(str);
		}

		// closing tag
		str << "</" << name << ">";
	}
}

void XmlNode::toStringPretty(stringstream &str, string &indent, const string &indentSingle) const {
	str << indent << "<" << name;

	for (Attributes::const_iterator a = attributes.begin(); a != attributes.end(); ++a) {
		str << " ";
		(*a)->toString(str);
	}

	if (children.size() || text.size()) {
		str << "/>" << endl;
	} else {
		str << ">" << endl;
		indent.append(indentSingle);

		// FIXME: All CR or CR/LF should have indentation appended to them
		if (text.size()) {
			str << text << endl;
		}

		for (Nodes::const_iterator n = children.begin(); n != children.end(); ++n) {
			(*n)->toStringPretty(str, indent, indentSingle);
		}

		// closing tag
		indent.erase(indent.length() - indentSingle.length());
		str << indent << "</" << name << ">" << endl;
	}
}

/*
char *XmlNode::toString() const {
	string xmlString = "<" + name;
	
	//add attributes to string
	for (int i = 0; i < attributes.size(); ++i) {
		xmlString += " " + attributes[i]->toString();
	}
	
	if (children.size() <= 0) {
		xmlString += "/>";
	} else {
		xmlString += ">";

		//add children nodes to string
		for (int i = 0; i < children.size(); ++i) {
			xmlString += children[i]->toString(); // recursive, base when no children
		}

		//closing tag
		xmlString += "</" + name + ">";
	}

	char *cstr = new char [xmlString.size()+1];
	strcpy(cstr, xmlString.c_str());

	return cstr;
}
*/
void XmlNode::populateElement(TiXmlElement *node) const {
	//add all the attributes to the element node
	for (int i = 0; i < attributes.size(); ++i) {
		node->SetAttribute(attributes[i]->getName(), attributes[i]->getValue().c_str());
	}

	//add all the children to the element node
	for (int i = 0; i < children.size(); ++i) {
		TiXmlElement *childElement = new TiXmlElement(children[i]->getName().c_str());
		
		if ( !(node->LinkEndChild(childElement)) ) { // TinyXML owns pointer
			throw runtime_error("Problem adding xml child element to: " + name);
		}

		children[i]->populateElement(childElement); // recursive, base when no children
	}
}

string XmlNode::getTreeString() const{
	stringstream str;

	str << getName();

	if (!children.empty()) {
		str << " (";
		for (int i = 0; i < children.size(); ++i) {
			str << children[i]->getTreeString() << " ";
		}
		str << ") ";
	}

	return str.str();
}

// =====================================================
//	class XmlAttribute
// =====================================================

XmlAttribute::XmlAttribute(TiXmlAttribute *attribute){
	name = attribute->Name();
	value = attribute->ValueStr();
}

bool XmlAttribute::getBoolValue() const {
	if(value == "true") {
		return true;
	} else if (value == "false") {
		return false;
	} else {
		throw runtime_error("Not a valid bool value (true or false): " + getName() + ": " + value);
	}
}


int XmlAttribute::getIntValue(int min, int max) const{
	int i= strToInt(value);
	if(i<min || i>max){
		throw runtime_error("Xml Attribute int out of range: " + getName() + ": " + value);
	}
	return i;
}

float XmlAttribute::getFloatValue(float min, float max) const{
	float f= strToFloat(value);
	if(f<min || f>max){
		throw runtime_error("Xml attribute float out of range: " + getName() + ": " + value);
	}
	return f;
}

const string &XmlAttribute::getRestrictedValue() const
{
	const string allowedCharacters = "abcdefghijklmnopqrstuvwxyz1234567890._-/";

	for(int i= 0; i<value.size(); ++i){
		if(allowedCharacters.find(value[i])==string::npos){
			throw runtime_error(
				string("The string \"" + value + "\" contains a character that is not allowed: \"") + value[i] +
				"\"\nFor portability reasons the only allowed characters in this field are: " + allowedCharacters);
		}
	}

	return value;
}

}}//end namespace
