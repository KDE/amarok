/***************************************************************************
 * copyright     : (C) 2007 Dan Meltzer <hydrogen@notyetimplemented.com    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "RepeatTrackAdvancer.h"


#include "debug.h"
#include "enginecontroller.h"
#include "PlaylistModel.h"

using namespace Playlist;

void
RepeatTrackAdvancer::advanceTrack()
{
    if( !m_previousTrack || (m_previousTrack != m_playlistModel->activeTrack() ) ) // we need to repeat
    {
        setCurrentTrack( m_playlistModel->activeRow() );
    }
    else {
        if( m_previousTrack == m_playlistModel->activeTrack() ) // We already repeated, advance
        {
            int updateRow = m_playlistModel->activeRow() + 1;
            if( updateRow < m_playlistModel->rowCount() )
            {
                setCurrentTrack( updateRow );
            }
        }
    }
    m_previousTrack = m_playlistModel->activeTrack();
}

void Playlist::RepeatTrackAdvancer::previousTrack()
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
