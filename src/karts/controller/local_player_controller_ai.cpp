//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004-2015 Steve Baker <sjbaker1@airmail.net>
//  Copyright (C) 2006-2015 Joerg Henrichs, Steve Baker
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "karts/controller/local_player_controller_ai.hpp"

#include "audio/sfx_base.hpp"
#include "config/player_manager.hpp"
#include "config/stk_config.hpp"
#include "config/user_config.hpp"
#include "graphics/camera.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/particle_emitter.hpp"
#include "graphics/particle_kind.hpp"
#include "input/input_manager.hpp"
#include "items/attachment.hpp"
#include "items/item.hpp"
#include "items/powerup.hpp"
#include "karts/abstract_kart.hpp"
#include "karts/controller/player_controller.hpp"
#include "karts/kart_properties.hpp"
#include "karts/skidding.hpp"
#include "karts/rescue_animation.hpp"
#include "modes/world.hpp"
#include "network/network_config.hpp"
#include "network/race_event_manager.hpp"
#include "race/history.hpp"
#include "states_screens/race_gui_base.hpp"
#include "tracks/track.hpp"
#include "utils/constants.hpp"
#include "utils/log.hpp"
#include "utils/translation.hpp"

/** The constructor for a loca player kart, i.e. a player that is playing
*  on this machine (non-local player would be network clients).
*  \param kart_name Name of the kart.
*  \param position The starting position (1 to n).
*  \param player The player to which this kart belongs.
*  \param init_pos The start coordinates and heading of the kart.
*/
LocalPlayerControllerAI::LocalPlayerControllerAI(AbstractKart *kart,
	StateManager::ActivePlayer *player)
	: SkiddingAI(kart), m_sky_particles_emitter(NULL)
{
	m_player = player;
	if (player)
		player->setKart(kart);

	// Keep a pointer to the camera to remove the need to search for
	// the right camera once per frame later.
	Camera *camera = Camera::createCamera(kart);
	m_camera_index = camera->getIndex();
	m_wee_sound = SFXManager::get()->createSoundSource("wee");
	m_bzzt_sound = SFXManager::get()->getBuffer("bzzt");
	m_ugh_sound = SFXManager::get()->getBuffer("ugh");
	m_grab_sound = SFXManager::get()->getBuffer("grab_collectable");
	m_full_sound = SFXManager::get()->getBuffer("energy_bar_full");

	// Attach Particle System
	Track *track = Track::getCurrentTrack();
#ifndef SERVER_ONLY
	if (UserConfigParams::m_weather_effects &&
		track->getSkyParticles() != NULL)
	{
		track->getSkyParticles()->setBoxSizeXZ(150.0f, 150.0f);

		m_sky_particles_emitter =
			new ParticleEmitter(track->getSkyParticles(),
				core::vector3df(0.0f, 30.0f, 100.0f),
				m_kart->getNode(),
				true);

		// FIXME: in multiplayer mode, this will result in several instances
		//        of the heightmap being calculated and kept in memory
		m_sky_particles_emitter->addHeightMapAffector(track);
	}
#endif
}   // LocalPlayerController

	//-----------------------------------------------------------------------------
	/** Destructor for a player kart.
	*/
LocalPlayerControllerAI::~LocalPlayerControllerAI()
{
	m_wee_sound->deleteSFX();

	if (m_sky_particles_emitter)
		delete m_sky_particles_emitter;
}   // ~LocalPlayerController


	//-----------------------------------------------------------------------------
	/** Resets the player kart for a new or restarted race.
	*/
void LocalPlayerControllerAI::reset()
{
	SkiddingAI::reset();
	m_sound_schedule = false;
}   // reset

	//-----------------------------------------------------------------------------
	/** Called when a kart finishes race.
	*  /param time Finishing time for this kart.
	d*/
void LocalPlayerControllerAI::finishedRace(float time)
{
	// This will implicitely trigger setting the first end camera to be active
	Camera::changeCamera(m_camera_index, Camera::CM_TYPE_END);
}   // finishedRace

bool LocalPlayerControllerAI::canGetAchievements() const
{
	return m_player->getConstProfile() == PlayerManager::getCurrentPlayer();
}   // canGetAchievements

	
