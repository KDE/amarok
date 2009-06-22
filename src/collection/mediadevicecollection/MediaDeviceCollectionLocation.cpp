/*
 *  Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "MediaDeviceCollectionLocation.h"

#include "Debug.h"
#include "Meta.h"
#include "MediaDeviceCollection.h"
#include "MediaDeviceHandler.h"
//#include "MediaDeviceMeta.h"
#include "../../statusbar/StatusBar.h"
#include "MediaDeviceCache.h" // for collection refresh hack


#include <kjob.h>
#include <KLocale>
#include <kio/job.h>
#include <kio/jobclasses.h>


using namespace Meta;

MediaDeviceCollectionLocation::MediaDeviceCollectionLocation( MediaDeviceCollection const *collection )
    : CollectionLocation()
    , m_collection( const_cast<MediaDeviceCollection*>( collection ) )
    , m_removeSources( false )
    , m_overwriteFiles( false )
{
    //nothing to do
}

MediaDeviceCollectionLocation::~MediaDeviceCollectionLocation()
{
    DEBUG_BLOCK
    //nothing to do
}

QString
MediaDeviceCollectionLocation::prettyLocation() const
{
    return collection()->prettyName();
}

bool
MediaDeviceCollectionLocation::isWritable() const
{
    return true;
}

bool
MediaDeviceCollectionLocation::remove( const Meta::TrackPtr &track )
{
    Q_UNUSED( track );
    return false;
}

void
MediaDeviceCollectionLocation::slotJobFinished( KJob *job )
{
    DEBUG_BLOCK
    Q_UNUSED(job);
}

void
MediaDeviceCollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources )
{
    Q_UNUSED( sources );
    DEBUG_BLOCK
/*
    connect( m_collection, SIGNAL( copyTracksCompleted( bool ) ),
             SLOT( copyOperationFinished( bool ) ) );

    // Copy list of tracks
    m_collection->copyTrackListToDevice( sources.keys() );
    */
}

void
MediaDeviceCollectionLocation::copyOperationFinished( bool success )
{
    Q_UNUSED( success );
    /*
    DEBUG_BLOCK
    if( !success )
    {
        QMap<Meta::TrackPtr, QString> failedTracks = m_collection->handler()->tracksFailed();
        debug() << "The following tracks failed to copy";
        foreach( Meta::TrackPtr track, failedTracks.keys() )
            {
                // TODO: better error handling
                debug() << track->artist()->name() << " - " << track->name() << " with error: " << failedTracks[ track ];
                source()->transferError( track, failedTracks[ track ] );
            }
    }

    slotCopyOperationFinished();
    */
}

void
MediaDeviceCollectionLocation::insertTracks( const QMap<Meta::TrackPtr, QString> &trackMap )
{
    // NOTE: MediaDeviceHandler doing this right now
    Q_UNUSED(trackMap);
}

void
MediaDeviceCollectionLocation::insertStatistics( const QMap<Meta::TrackPtr, QString> &trackMap )
{
    DEBUG_BLOCK
    Q_UNUSED(trackMap);
}

#include "MediaDeviceCollectionLocation.moc"

