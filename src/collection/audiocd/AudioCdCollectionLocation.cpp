/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#include "AudioCdCollectionLocation.h"

#include "AudioCdMeta.h"
#include "Debug.h"

AudioCdCollectionLocation::AudioCdCollectionLocation( const AudioCdCollection* parentCollection )
    : CollectionLocation( parentCollection )
    , m_collection( parentCollection )
{
}


AudioCdCollectionLocation::~AudioCdCollectionLocation()
{
}

void AudioCdCollectionLocation::getKIOCopyableUrls( const Meta::TrackList & tracks )
{
    DEBUG_BLOCK

    QMap<Meta::TrackPtr, KUrl> resultMap;
    foreach( Meta::TrackPtr trackPtr, tracks ) {
        Meta::AudioCdTrackPtr cdTrack = Meta::AudioCdTrackPtr::staticCast( trackPtr );

        QString path = m_collection->copyableBasePath() + cdTrack->fileNameBase() + '.' + m_collection->encodingFormat();

        debug() << "adding path: " << path;
        
        resultMap.insert( trackPtr, KUrl( path ) );

    }

    slotGetKIOCopyableUrlsDone( resultMap );
}


