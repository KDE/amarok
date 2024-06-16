/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MediaDeviceCollectionLocation.h"

#include "MediaDeviceCache.h" // for collection refresh hack
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "core-impl/collections/mediadevicecollection/MediaDeviceCollection.h"

#include <KJob>
#include <KLocalizedString>
#include <kio/job.h>
#include <kio/jobclasses.h>

using namespace Collections;

MediaDeviceCollectionLocation::MediaDeviceCollectionLocation( MediaDeviceCollection *collection )
    : CollectionLocation( collection )
    , m_collection( collection )
    , m_handler( m_collection->handler() )
{
    //nothing to do
}

MediaDeviceCollectionLocation::~MediaDeviceCollectionLocation()
{
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
    return m_handler->isWritable();
}

void
MediaDeviceCollectionLocation::getKIOCopyableUrls( const Meta::TrackList &tracks )
{
    connect( m_handler, &Meta::MediaDeviceHandler::gotCopyableUrls, this, &MediaDeviceCollectionLocation::slotGetKIOCopyableUrlsDone );
    m_handler->getCopyableUrls( tracks );
}


void
MediaDeviceCollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, QUrl> &sources,
                                                     const Transcoding::Configuration &configuration )
{
    DEBUG_BLOCK
    Q_UNUSED( configuration )

    connect( m_handler, &Meta::MediaDeviceHandler::copyTracksDone,
             this, &MediaDeviceCollectionLocation::copyOperationFinished,
             Qt::QueuedConnection );
    m_handler->copyTrackListToDevice( sources.keys() );

/*
    connect( m_collection, SIGNAL(copyTracksCompleted(bool)),
             SLOT(copyOperationFinished(bool)) );

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
    {
        m_handler->writeDatabase();
    }
    // TODO: will be replaced with a more powerful method
    // which deals with particular reasons for failed copies
    /*
    DEBUG_BLOCK
    if( !success )
    {
        QMap<Meta::TrackPtr, QString> failedTracks = m_collection->handler()->tracksFailed();
        debug() << "The following tracks failed to copy";
        for( Meta::TrackPtr track : failedTracks.keys() )
            {
                // TODO: better error handling
                debug() << track->artist()->name() << " - " << track->name() << " with error: " << failedTracks[ track ];
                source()->transferError( track, failedTracks[ track ] );
            }
    }
    */
    slotCopyOperationFinished();
}

void
MediaDeviceCollectionLocation::removeUrlsFromCollection( const Meta::TrackList &sources )
{
    DEBUG_BLOCK
    connect( m_handler, &Meta::MediaDeviceHandler::removeTracksDone,
             this, &MediaDeviceCollectionLocation::removeOperationFinished );

    m_handler->removeTrackListFromDevice( sources );
}

void
MediaDeviceCollectionLocation::removeOperationFinished()
{
    DEBUG_BLOCK

    m_handler->writeDatabase();

    slotRemoveOperationFinished();
}


