/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "AudioCdCollectionLocation.h"

#include "AudioCdMeta.h"
#include "Debug.h"
#include "FormatSelectionDialog.h"

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

void AudioCdCollectionLocation::showSourceDialog( const Meta::TrackList &tracks, bool removeSources )
{
    DEBUG_BLOCK
    Q_UNUSED( tracks )
    Q_UNUSED( removeSources )
    FormatSelectionDialog * dlg = new FormatSelectionDialog();

    connect( dlg, SIGNAL( formatSelected( int ) ), this, SLOT( onFormatSelected( int ) ) );
    connect( dlg, SIGNAL( rejected () ), this, SLOT( onCancel() ) );

    dlg->show();
}

void AudioCdCollectionLocation::formatSelected( int format )
{
    Q_UNUSED( format )
}

void AudioCdCollectionLocation::formatSelectionCancelled()
{
}

void AudioCdCollectionLocation::onFormatSelected( int format )
{
    DEBUG_BLOCK
    m_collection->setEncodingFormat( format );
    slotShowSourceDialogDone();
}

void AudioCdCollectionLocation::onCancel()
{
    DEBUG_BLOCK
    abort();
}


#include "AudioCdCollectionLocation.moc"


