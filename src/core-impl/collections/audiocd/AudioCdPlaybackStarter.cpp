/****************************************************************************************
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

#include "AudioCdPlaybackStarter.h"
#include "EngineController.h"
#include "playlist/PlaylistController.h"

#include "core/support/Debug.h"

AudioCdPlaybackStarter::AudioCdPlaybackStarter()
                      : m_deleteTimer()
                      , m_deleteTimeout( 30000 )
{
    DEBUG_BLOCK

    bool audiocd_collection_found = false;
    // check whether AudioCd collection is already available
    QList<Collections::Collection*> collections = CollectionManager::instance()->viewableCollections();
    foreach( Collections::Collection *collection, collections )
    {
        if ( collection->collectionId() == "AudioCd" )
        {
            if ( !collection->memoryCollection().isNull() &&
                 collection->memoryCollection()->trackMap().count() != 0 )
            {
                audiocd_collection_found = true;
                // collection is found, so we start playback
                startPlayback( collection );
                deleteLater();
                return;
            }
        }
    }
    if ( !audiocd_collection_found )
    {
        debug() << "Start watching for newly added collections";
        // connect to CollectionManager to watch for newly added collections
        CollectionManager *manager = CollectionManager::instance();
        connect( manager, SIGNAL(collectionDataChanged(Collections::Collection*)),
                 this, SLOT(slotCollectionChanged(Collections::Collection*)) );
        // setup a suicide timer
        connect( &m_deleteTimer, SIGNAL(timeout()), this, SLOT(deleteLater()) );
        m_deleteTimer.setSingleShot( true );
        m_deleteTimer.start( m_deleteTimeout );
    }
}

void
AudioCdPlaybackStarter::startPlayback( Collections::Collection* audiocd_collection )
{
    if ( !audiocd_collection->memoryCollection().isNull() )
    {
        The::engineController()->stop( true );
        The::playlistController()->clear();
        The::playlistController()->insertOptioned( audiocd_collection->memoryCollection()->trackMap().values(),
                                                   Playlist::DirectPlay );
    }
}

void
AudioCdPlaybackStarter::slotCollectionChanged( Collections::Collection *collection)
{
    m_deleteTimer.stop();
    if ( collection->collectionId() == "AudioCd" )
    {
        startPlayback( collection );
        deleteLater();
    }
    else // restart timer
        m_deleteTimer.start( m_deleteTimeout );
}

