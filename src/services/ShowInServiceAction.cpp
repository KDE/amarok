/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ShowInServiceAction.h"

#include "amarokurls/AmarokUrl.h"

ShowInServiceAction::ShowInServiceAction( ServiceBase * service, Meta::ServiceTrack *track )
    : QAction( service )
    , m_track( track )
    , m_service( service )
{
    setIcon ( KIcon( "system-search" ) );
    setText( i18n( "Go to artist in %1 service", service->name() ) );

    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );
}

ShowInServiceAction::~ShowInServiceAction()
{
}

void ShowInServiceAction::slotTriggered()
{
    DEBUG_BLOCK

    //artist or album?

    if ( m_service == 0 || !m_track || !m_track->artist() )
        return;

    QString urlString = QString( "amarok://navigate/service/%1/artist-album/artist:\"%2\"" )
                        .arg( m_service->name() )
                        .arg( m_track->artist()->prettyName() );

    AmarokUrl url( urlString );
    url.run();
}

#include "ShowInServiceAction.moc"

