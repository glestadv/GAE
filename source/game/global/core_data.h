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

#ifndef _GLEST_GAME_COREDATA_H_
#define _GLEST_GAME_COREDATA_H_

#include <string>

#include "sound.h"
#include "font.h"
#include "texture.h"
#include "sound_container.h"

namespace Glest{ namespace Game{

using Shared::Graphics::Texture2D;
using Shared::Graphics::Texture3D;
using Shared::Graphics::Font2D;
using Shared::Sound::StrSound;
using Shared::Sound::StaticSound;

class Renderer;
class Config;

// =====================================================
// 	class CoreData
//
/// Data shared ammongst all the GuiProgramStates
// =====================================================

class CoreData {
private:
    StrSound introMusic;
    StrSound menuMusic;
	StaticSound clickSoundA;
    StaticSound clickSoundB;
    StaticSound clickSoundC;
	SoundContainer waterSounds;

	Texture2D *logoTexture;
    Texture2D *backgroundTexture;
    Texture2D *fireTexture;
    Texture2D *snowTexture;
	Texture2D *waterSplashTexture;
    Texture2D *customTexture;
	Texture2D *buttonSmallTexture;
	Texture2D *buttonBigTexture;
	Texture2D *textEntryTexture;

    Font2D *displayFont;
	Font2D *menuFontNormal;
	Font2D *menuFontSmall;
	Font2D *menuFontBig;
	Font2D *menuFontVeryBig;
	Font2D *consoleFont;

public:
	CoreData(const Config &config, Renderer &renderer);
	~CoreData();

	static const CoreData &getInstance();

	const Texture2D *getBackgroundTexture() const	{return backgroundTexture;}
	const Texture2D *getFireTexture() const			{return fireTexture;}
	const Texture2D *getSnowTexture() const			{return snowTexture;}
	const Texture2D *getLogoTexture() const			{return logoTexture;}
	const Texture2D *getWaterSplashTexture() const	{return waterSplashTexture;}
	const Texture2D *getCustomTexture() const		{return customTexture;}
	const Texture2D *getButtonSmallTexture() const	{return buttonSmallTexture;}
	const Texture2D *getButtonBigTexture() const	{return buttonBigTexture;}
	const Texture2D *getTextEntryTexture() const	{return textEntryTexture;}

	const StrSound &getIntroMusic() const			{return introMusic;}
	const StrSound &getMenuMusic() const			{return menuMusic;}
    const StaticSound &getClickSoundA() const		{return clickSoundA;}
    const StaticSound &getClickSoundB() const		{return clickSoundB;}
    const StaticSound &getClickSoundC() const		{return clickSoundC;}
	const StaticSound &getWaterSound() const		{return *waterSounds.getRandSound();}

	const Font2D *getDisplayFont() const			{return displayFont;}
    const Font2D *getMenuFontSmall() const			{return menuFontSmall;}
    const Font2D *getMenuFontNormal() const			{return menuFontNormal;}
    const Font2D *getMenuFontBig() const			{return menuFontBig;}
	const Font2D *getMenuFontVeryBig() const		{return menuFontVeryBig;}
    const Font2D *getConsoleFont() const			{return consoleFont;}

private:
	int computeFontSize(int size);
};

}} //end namespace

#endif
