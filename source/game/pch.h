// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_PCH_H_
#define _GAME_PCH_H_

#ifdef USE_PCH

#include "../shared_lib/include/pch.h"
#if 0
#include "ai/ai.h"
#include "ai/ai_interface.h"
#include "ai/ai_rule.h"
#include "ai/path_finder.h"
#include "facilities/components.h"
#include "facilities/game_util.h"
#include "facilities/logger.h"
#include "facilities/pos_iterator.h"
#include "game/chat_manager.h"
#include "game/commander.h"
#include "game/console.h"
#include "game/game.h"
#include "game/game_camera.h"
#include "game/game_settings.h"
#include "game/player.h"
#include "game/stats.h"
#include "global/config.h"
#include "global/core_data.h"
#include "global/exceptions.h"
#include "global/lang.h"
#include "global/metrics.h"
#include "graphics/particle_type.h"
#include "graphics/renderer.h"
#include "gui/display.h"
#include "gui/gui.h"
#include "gui/keymap.h"
#include "gui/selection.h"
#include "main/battle_end.h"
#include "main/intro.h"
#include "main/main.h"
#include "main/program.h"
#include "menu/main_menu.h"
#include "menu/menu_background.h"
#include "menu/menu_state_about.h"
#include "menu/menu_state_graphic_info.h"
#include "menu/menu_state_join_game.h"
#include "menu/menu_state_load_game.h"
#include "menu/menu_state_new_game.h"
#include "menu/menu_state_options.h"
#include "menu/menu_state_root.h"
#include "menu/menu_state_scenario.h"
#include "menu/menu_state_start_game_base.h"
#include "network/client_interface.h"
#include "network/file_transfer.h"
//#include "network/network_messenger.h"
#include "network/host.h"
#include "network/network_info.h"
#include "network/network_manager.h"
#include "network/network_message.h"
#include "network/network_status.h"
#include "network/network_types.h"
#include "network/protocol_exception.h"
#include "network/remote_interface.h"
#include "network/server_interface.h"
#include "sound/sound_container.h"
#include "sound/sound_renderer.h"
#include "type_instances/command.h"
#include "type_instances/effect.h"
#include "type_instances/faction.h"
#include "type_instances/object.h"
#include "type_instances/resource.h"
#include "type_instances/unit.h"
#include "type_instances/unit_reference.h"
#include "type_instances/upgrade.h"
#include "types/command_type.h"
#include "types/damage_multiplier.h"
#include "types/effect_type.h"
#include "types/element_type.h"
#include "types/faction_type.h"
#include "types/flags.h"
#include "types/object_type.h"
#include "types/resource_type.h"
#include "types/skill_type.h"
#include "types/tech_tree.h"
#include "types/unit_stats_base.h"
#include "types/unit_type.h"
#include "types/upgrade_type.h"
#include "world/map.h"
#include "world/minimap.h"
#include "world/surface_atlas.h"
#include "world/tileset.h"
#include "world/time_flow.h"
#include "world/unit_updater.h"
#include "world/water_effects.h"
#include "world/world.h"
#endif
#endif // USE_PCH
#endif // _GAME_PCH_H_


