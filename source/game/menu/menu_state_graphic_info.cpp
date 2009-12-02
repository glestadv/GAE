// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "menu_state_graphic_info.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "menu_state_options.h"

#include "leak_dumper.h"


namespace Glest { namespace Game {

// =====================================================
//  class MenuStateGraphicInfo
// =====================================================

MenuStateGraphicInfo::MenuStateGraphicInfo(GuiProgram &program, MainMenu &mainMenu)
        : MenuState(program, mainMenu, "info") {
    buttonReturn.init(387, 100, 125);
    labelInfo.init(100, 700);
    labelMoreInfo.init(100, 500);
    labelMoreInfo.setFont(getCoreData().getMenuFontSmall());

    Renderer &renderer = getRenderer();
    glInfo = renderer.getGlInfo();
    glMoreInfo = renderer.getGlMoreInfo();
}

void MenuStateGraphicInfo::mouseClick(int x, int y, MouseButton mouseButton) {
    const CoreData &coreData = getCoreData();
    SoundRenderer &soundRenderer = getSoundRenderer();

    if (buttonReturn.mouseClick(x, y)) {
        soundRenderer.playFx(coreData.getClickSoundA());
        changeState<MenuStateOptions>();
    }
}

void MenuStateGraphicInfo::mouseMove(int x, int y, const MouseState &ms) {
    buttonReturn.mouseMove(x, y);
}

void MenuStateGraphicInfo::render() {
    Renderer &renderer = getRenderer();
    const Lang &lang = getLang();

    buttonReturn.setText(lang.get("Return"));
    labelInfo.setText(glInfo);
    labelMoreInfo.setText(glMoreInfo);

    renderer.renderButton(&buttonReturn);
    renderer.renderLabel(&labelInfo);
    renderer.renderLabel(&labelMoreInfo);
}

}}//end namespace
