/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "StandardTrackAdvancer.h"


#include "debug.h"
#include "enginecontroller.h"
#include "PlaylistModel.h"

using namespace Playlist;

void
StandardTrackAdvancer::advanceTrack()
{
    int updateRow = m_playlistModel->activeRow() + 1;
    if( updateRow < m_playlistModel->rowCount() )
    {
        setCurrentTrack( updateRow );
    }
}

void Playlist::StandardTrackAdvancer::previousTrack()
{
    DEBUG_BLOCK
    int updateRow = m_playlistModel->activeRow() - 1;
    if ( updateRow < 0 ) 
        return;

    if( updateRow < m_playlistModel->rowCount() )
    {
        setCurrentTrack( updateRow );
    }
}
