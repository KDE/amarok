/***************************************************************************
 * copyright            : (C) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "MediaDeviceHandler.h"

MediaDeviceHandler::MediaDeviceHandler( QObject *parent )
: QObject( parent )
{
    DEBUG_BLOCK
}

bool
MediaDeviceHandler::isWritable() const
{
    return false;
}


void
MediaDeviceHandler::getCopyableUrls(const Meta::TrackList &tracks)
{
    QMap<Meta::TrackPtr, KUrl> urls;
    foreach( Meta::TrackPtr track, tracks )
    {
        if( track->isPlayable() )
            urls.insert( track, track->playableUrl() );
    }

    emit gotCopyableUrls( urls );

}

void
MediaDeviceHandler::copyTrackListToDevice(const Meta::TrackList tracklist)
{
    Q_UNUSED( tracklist );
    // TODO: generically implement, abstract from IpodHandler
    return;
}



#include "MediaDeviceHandler.moc"
