// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_RESOURCETYPE_H_
#define _GLEST_GAME_RESOURCETYPE_H_

#include "element_type.h"
#include "model.h"
#include "checksum.h"
#include "game_constants.h"

using Shared::Graphics::Model;
using Shared::Util::Checksum;

namespace Glest { namespace ProtoTypes {

// =====================================================
// 	class ResourceType
//
///	A type of resource that can be harvested or not
// =====================================================

class ResourceType : public DisplayableType {
private:
	ResourceClass m_resourceClass;
	int m_tilesetObject;	/**< used only if class == ResourceClass::TILESET, object number in the map */
	int m_resourceNumber;	/**< used only if class == ResourceClass::TECHTREE, resource number in the map */
	int m_interval;		/**< used only if class == ResourceClass::CONSUMABLE */
	int m_defResPerPatch;	/**< used only if class == ResourceClass::TILESET || class == ResourceClass::TECHTREE */
	bool m_recoupCost;	/**< used only if class == ResourceClass::STATIC */
	bool m_infiniteStore; /**< if true storage rules don't apply */
	Modifier m_damageMod; /**< used only if class == ResourceClass::CONSUMABLE, damage to do per interval if no resources to consume */

	Model *m_model;
	/** Wether or not to display this resource at the top of the screen (defaults to true). */
	bool m_display;

public:
	bool load( const string &dir, int id );
	virtual void doChecksum( Checksum &checksum ) const;

	//get
	ResourceClass getClass( ) const { return m_resourceClass; }
	int getTilesetObject( ) const   { return m_tilesetObject; }
	int getResourceNumber( ) const  { return m_resourceNumber; }
	int getInterval( ) const        { return m_interval; }
	int getDefResPerPatch( ) const  { return m_defResPerPatch; }
	bool getRecoupCost( ) const     { return m_recoupCost; }
	bool isInfiniteStore( ) const   { return m_infiniteStore; }
	const Model *getModel( ) const  { return m_model; }
	bool isDisplay( ) const         { return m_display; }
	Modifier getDamageMod( ) const  { return m_damageMod; }

	static ResourceClass strToRc( const string &s );
};

}} //end namespace

#endif
