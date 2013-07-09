/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2013 Tatjana Gornak <t.gornak@gmail.com>                               *
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

#include "AudioCdCollectionLocation.h"

#include "core/support/Debug.h"
#include "meta/AudioCdTrack.h"
#include "helpers/ParanoiaHelper.h"

#include <ThreadWeaver/Weaver>

using namespace Collections;

AudioCdCollectionLocation::AudioCdCollectionLocation( QWeakPointer<AudioCdCollection> parentCollection )
                         : CollectionLocation( )
                         , m_collection( parentCollection )
                         , m_tracks( 0 )
{
}

Collections::Collection*
AudioCdCollectionLocation::collection() const
{
    return m_collection.data();
}

AudioCdCollectionLocation::~AudioCdCollectionLocation()
{
}

void
AudioCdCollectionLocation::getKIOCopyableUrls( const Meta::TrackList & tracks )
{
    m_tracks = tracks.size();
    m_resultsMap.clear();
    foreach( Meta::TrackPtr track, tracks )
    {
        QString name;
        // create a temporal file with a proper name
        KTemporaryFile* temp = new KTemporaryFile();
        temp->setSuffix( "." % track.data()->type() );
        temp->open(); temp->close();
        debug() << "Temporary file name" << temp->fileName();

        ParanoiaHelper *p = new ParanoiaHelper( m_collection.data()->getDeviceName(), track, temp->fileName() );
        ThreadWeaver::Weaver::instance()->enqueue( p );
        connect( p, SIGNAL(copyingDone(Meta::TrackPtr, const QString&, bool)),
                 this, SLOT(addToMap(Meta::TrackPtr, const QString&, bool)) );
        // delete temporal file when copying is finished or aborted
        connect( this, SIGNAL(finishCopy()), temp, SLOT(deleteLater()) );
        connect( this, SIGNAL(aborted()), temp, SLOT(deleteLater()) );
    }
}

void
AudioCdCollectionLocation::addToMap( Meta::TrackPtr track, const QString &fileName, bool succesfull )
{
    DEBUG_BLOCK

    if ( succesfull )
    {
        m_resultsMap.insert( track, KUrl( fileName ) );
    }
    else
    {
        warning() << "Failed to copy file for track" << track.data()->prettyUrl();
        --m_tracks; // TODO: proper error message
    }

    if ( m_resultsMap.size() == m_tracks )
        slotGetKIOCopyableUrlsDone( m_resultsMap );
}

#include "AudioCdCollectionLocation.moc"


