/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "enginecontroller.h"
#include "PlaylistModel.h"
#include "TrackAdvancer.h"

using namespace PlaylistNS;

void
TrackAdvancer::setCurrentTrack( int position )
{
    m_playlistModel->setActiveRow( position );
    EngineController::instance()->play( m_playlistModel->activeTrack() );
}