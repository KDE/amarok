/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
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

#include "IpodCollectionLocation.h"

#include "Debug.h"
#include "Meta.h"
#include "IpodCollection.h"
#include "IpodHandler.h"
#include "IpodMeta.h"
#include "../../statusbar/StatusBar.h"
#include "MediaDeviceCache.h" // for collection refresh hack

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <kjob.h>
#include <KLocale>
#include <KSharedPtr>
#include <kio/job.h>
#include <kio/jobclasses.h>


using namespace Meta;

IpodCollectionLocation::IpodCollectionLocation( IpodCollection const *collection )
    : CollectionLocation()
    , m_collection( const_cast<IpodCollection*>( collection ) )
    , m_removeSources( false )
    , m_overwriteFiles( false )
{
    //nothing to do
}

IpodCollectionLocation::~IpodCollectionLocation()
{
    DEBUG_BLOCK
    //nothing to do
}

QString
IpodCollectionLocation::prettyLocation() const
{
    return collection()->prettyName();
}

bool
IpodCollectionLocation::isWritable() const
{
    return true;
}

// TODO: implement (use IpodHandler ported method for removing a track)
bool
IpodCollectionLocation::remove( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK

    IpodTrackPtr ipodTrack = IpodTrackPtr::dynamicCast( track );

    if( track )
        return m_collection->deleteTrackFromDevice( ipodTrack );

    return false;
}

void
IpodCollectionLocation::slotJobFinished( KJob *job )
{
    DEBUG_BLOCK
    Q_UNUSED(job);
}

void
IpodCollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources )
{
    DEBUG_BLOCK

    // iterate through source tracks
    foreach( const Meta::TrackPtr &track, sources.keys() )
    {
        debug() << "copying from " << sources[ track ];
        m_collection->copyTrackToDevice( track );
    }

    m_collection->copyTracksCompleted();

    m_collection->collectionUpdated();
    
    slotCopyOperationFinished();
}

void
IpodCollectionLocation::insertTracks( const QMap<Meta::TrackPtr, QString> &trackMap )
{
    // NOTE: IpodHandler doing this right now
    Q_UNUSED(trackMap);
}

void
IpodCollectionLocation::insertStatistics( const QMap<Meta::TrackPtr, QString> &trackMap )
{
    DEBUG_BLOCK
    Q_UNUSED(trackMap);
}

#include "IpodCollectionLocation.moc"
