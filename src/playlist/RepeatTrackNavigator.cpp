/***************************************************************************
 * copyright     : (C) 2007 Dan Meltzer <hydrogen@notyetimplemented.com    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "RepeatTrackNavigator.h"


#include "EngineController.h"
#include "PlaylistModel.h"

using namespace Playlist;

Meta::TrackPtr
RepeatTrackNavigator::nextTrack()
{
    if( !m_previousTrack || (m_previousTrack != m_playlistModel->activeTrack() ) ) // we need to repeat
    {
        Meta::TrackPtr nextTrack = m_playlistModel->activeTrack();
        m_previousTrack = nextTrack;
        return nextTrack;
    }
    else {
        if( m_previousTrack == m_playlistModel->activeTrack() ) // We already repeated, advance
        {
            int updateRow = m_playlistModel->activeRow() + 1;
            if( updateRow < m_playlistModel->rowCount() )
            {
                return m_playlistModel->itemList().at( updateRow )->track();
            }
        }
    }

	return Meta::TrackPtr();
}

Meta::TrackPtr
RepeatTrackNavigator::userNextTrack()
{
    int updateRow = m_playlistModel->activeRow() + 1;
    if( updateRow < m_playlistModel->rowCount() )
    {
        setCurrentTrack( updateRow );
    }
	return Meta::TrackPtr();
}


