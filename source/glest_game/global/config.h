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

#ifndef _GLEST_GAME_CONFIG_H_
#define _GLEST_GAME_CONFIG_H_

#include "properties.h"

namespace Glest{ namespace Game{

using Shared::Util::Properties;

// =====================================================
// 	class Config
//
//	Game configuration
// =====================================================

class Config{
private:
	Properties properties;
	float fastestSpeed;
	float slowestSpeed;

private:
	Config();

public:
	static Config &getInstance(){
		static Config config;
		return config;
	}

	void save(const string &path="glest.ini")				{properties.save(path);}

	int getInt(const string &key) const						{return properties.getInt(key);}
	bool getBool(const string &key) const					{return properties.getBool(key);}
	float getFloat(const string &key) const					{return properties.getFloat(key);}
	const string &getString(const string &key) const		{return properties.getString(key);}

	void setInt(const string &key, int value)				{properties.setInt(key, value);}
	void setBool(const string &key, bool value)				{properties.setBool(key, value);}
	void setFloat(const string &key, float value)			{properties.setFloat(key, value);}
	void setString(const string &key, const string &value)	{properties.setString(key, value);}

	string toString()										{return properties.toString();}

	float getFastestSpeed() const	{return fastestSpeed;}
	float getSlowestSpeed() const	{return slowestSpeed;}

};

}}//end namespace

#endif
