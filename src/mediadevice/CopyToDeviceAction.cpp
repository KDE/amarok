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

#include "mediabrowser.h"


CopyToDeviceAction::CopyToDeviceAction( QObject *parent, Meta::Track *track )
    : QAction( parent )
    , m_track( track )
{
    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );

    setText( i18n("Copy To Media Device") );
    setToolTip( i18n("Copies the track to a portable media device") );
}

void
CopyToDeviceAction::slotTriggered()
{
    MediaBrowser::queue()->addTrack( m_track );
}
