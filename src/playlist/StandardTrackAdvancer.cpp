/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "enginecontroller.h"
#include "PlaylistModel.h"
#include "StandardTrackAdvancer.h"

using namespace PlaylistNS;

void
StandardTrackAdvancer::advanceTrack()
{
    int updateRow = m_playlistModel->activeRow() + 1;
    if( (updateRow + 1) <= m_playlistModel->rowCount() )
    {
        setCurrentTrack( updateRow );
    }
}
