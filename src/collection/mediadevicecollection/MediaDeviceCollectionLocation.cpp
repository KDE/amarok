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
    , m_handler( m_collection->handler() )
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

// NOTE: must be overridden by child class if
// it is writeable
bool
MediaDeviceCollectionLocation::isWritable() const
{
    return false;
}

bool
MediaDeviceCollectionLocation::remove( const Meta::TrackPtr &track )
{
    // TODO: call handler's method to delete a track
    // that doesn't write to database.  Writing will
    // be handled at the end of the removal of a list
    // of tracks, instead of just one
    Q_UNUSED( track );
    return false;
}

void
MediaDeviceCollectionLocation::getKIOCopyableUrls( const Meta::TrackList &tracks )
{
    //    CollectionLocation::getKIOCopyableUrls(tracks);
    connect( m_handler, SIGNAL( gotCopyableUrls( const QMap<Meta::TrackPtr, KUrl> & ) ),SLOT(slotGetKIOCopyableUrlsDone(const QMap<Meta::TrackPtr,KUrl> &)) );

    m_handler->getCopyableUrls( tracks );
}


void
MediaDeviceCollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources )
{
    DEBUG_BLOCK

    connect( m_handler, SIGNAL( copyTracksDone( bool  ) ),
             SLOT( copyOperationFinished( bool ) ),
             Qt::QueuedConnection );
    m_handler->copyTrackListToDevice( sources.keys() );

/*
    connect( m_collection, SIGNAL( copyTracksCompleted( bool ) ),
             SLOT( copyOperationFinished( bool ) ) );

    // Copy list of tracks
    m_collection->copyTrackListToDevice( sources.keys() );
    */

    // TODO: call handler's method for copying a list of
    // tracks to the device.  At the end, if successful,
    // write to database, and any unsuccessful track
    // copies will generate warning/error messages
}

void
MediaDeviceCollectionLocation::copyOperationFinished( bool success )
{
    // TODO: should connect database written signal to
    // to this slot
    if( success )
        m_handler->writeDatabase();
    // TODO: will be replaced with a more powerful method
    // which deals with particular reasons for failed copies
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
    */
    slotCopyOperationFinished();

}

#include "MediaDeviceCollectionLocation.moc"

