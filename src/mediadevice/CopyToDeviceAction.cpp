/***************************************************************************
 *   Copyright (c) 2007  Jamie Faris <farisj@gmail.com>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "CopyToDeviceAction.h"

#include <QAction>

#include <KIcon>

#include "mediabrowser.h"


CopyToDeviceAction::CopyToDeviceAction( QObject *parent, Meta::Track *track )
    : PopupDropperAction( parent )
    , m_tracks()
{
    init();

    m_tracks.append( Meta::TrackPtr( track ) );

    setToolTip( i18n("Copies the track to a portable media device") );
}

CopyToDeviceAction::CopyToDeviceAction( QObject *parent, Meta::Album *album )
    : PopupDropperAction( parent )
    , m_tracks( album->tracks() )
{
    init();

    setToolTip( i18n("Copies all tracks from this album to a portable media device") );
}

CopyToDeviceAction::CopyToDeviceAction( QObject *parent, Meta::Artist *artist )
    : PopupDropperAction( parent )
    , m_tracks( artist->tracks() )
{
    init();

    setToolTip( i18n("Copies all tracks by this artist to a portable media device") );
}

void
CopyToDeviceAction::init()
{
    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );

    setText( i18n("Copy To Media Device") );
    setIcon( KIcon("multimedia-player") );
}

void
CopyToDeviceAction::slotTriggered()
{
    MediaBrowser::queue()->addTracks( m_tracks );
}
