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

#include "MtpCollectionLocation.h"

#include "Debug.h"
#include "Meta.h"
#include "MtpCollection.h"
#include "MtpHandler.h"
#include "MtpMeta.h"
#include "../../statusbar_ng/StatusBar.h"
#include "MediaDeviceCache.h" // for collection refresh hack

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <kjob.h>
#include <KLocale>
#include <KSharedPtr>
#include <KTempDir>
#include <kio/job.h>
#include <kio/jobclasses.h>


using namespace Meta;

MtpCollectionLocation::MtpCollectionLocation( MtpCollection const *collection )
    : CollectionLocation()
    , m_collection( const_cast<MtpCollection*>( collection ) )
    , m_removeSources( false )
    , m_overwriteFiles( false )
{
    //nothing to do
}

MtpCollectionLocation::~MtpCollectionLocation()
{
    DEBUG_BLOCK
    //nothing to do
}

QString
MtpCollectionLocation::prettyLocation() const
{
    return collection()->prettyName();
}

bool
MtpCollectionLocation::isWritable() const
{
    return true;
}

// TODO: implement (use MtpHandler ported method for removing a track)
bool
MtpCollectionLocation::remove( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK

    MtpTrackPtr mtpTrack = MtpTrackPtr::dynamicCast( track );

    if( track )
        return m_collection->deleteTrackFromDevice( mtpTrack );

    return false;
    
}

void
MtpCollectionLocation::slotJobFinished( KJob *job )
{
    DEBUG_BLOCK
    Q_UNUSED(job);
    // TODO: NYI
            /*
    m_jobs.remove( job );
    if( job->error() )
    {
        //TODO: proper error handling
        warning() << "An error occurred when copying a file: " << job->errorString();
    }
    job->deleteLater();
    if( m_jobs.isEmpty() )
    {
        insertTracks( m_destinations );
        insertStatistics( m_destinations );
        //m_collection->scanManager()->setBlockScan( false );
        slotCopyOperationFinished();
    }
            */
}

void
MtpCollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources )
{
    DEBUG_BLOCK

    // iterate through source tracks
    foreach( const Meta::TrackPtr &track, sources.keys() )
    {

        debug() << "copying from " << sources[ track ];
        m_collection->copyTrackToDevice( track );

    }

    m_collection->collectionUpdated();
    
    slotCopyOperationFinished();
}

// pull tracks from device into a KTempDir
void
MtpCollectionLocation::getKIOCopyableUrls( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK

    QMap<Meta::TrackPtr, KUrl> urls;
    
    m_tempdir.setAutoRemove( true );
    
    QString genericError = i18n( "Could not copy track from device." );

    foreach( Meta::TrackPtr trackptr, tracks )
    {
        Meta::MtpTrackPtr track = Meta::MtpTrackPtr::staticCast( trackptr );
        if( !track )
            break;

        QString filename = m_collection->getTempFileName( track, m_tempdir.name() );

        debug() << "Temp Filename: " << filename;

        int ret = m_collection->getTrackToFile( track, filename );
            if( ret != 0 )
            {
                debug() << "Get Track failed: " << ret;
                /*The::statusBar()->shortLongMessage(
                               genericError,
                               i18n( "Could not copy track from device." ),
                                     StatusBar::Error
                                                  );*/
            }
            else
            {
                urls.insert( trackptr, filename );
            }

    }

    slotGetKIOCopyableUrlsDone( urls );

    
}

void
MtpCollectionLocation::insertTracks( const QMap<Meta::TrackPtr, QString> &trackMap )
{
    // NOTE: MtpHandler doing this right now
    Q_UNUSED(trackMap);
#if 0
    DEBUG_BLOCK
    QList<QVariantMap > metadata;
    QStringList urls;
    foreach( const Meta::TrackPtr &track, trackMap.keys() )
    {
        if( m_ignoredDestinations.contains( trackMap[ track ], Qt::CaseSensitive ) )
        {
            continue;
        }
        QVariantMap trackData = Meta::Field::mapFromTrack( track );
        trackData.insert( Meta::Field::URL, trackMap[ track ] );  //store the new url of the file
        metadata.append( trackData );
        urls.append( trackMap[ track ] );
    }
    ScanResultProcessor processor( m_collection );
    processor.setScanType( ScanResultProcessor::IncrementalScan );
    QMap<QString, uint> mtime = updatedMtime( urls );
    foreach( const QString &dir, mtime.keys() )
    {
        processor.addDirectory( dir, mtime[ dir ] );
    }
    if( !metadata.isEmpty() )
    {
        QFileInfo info( metadata.first().value( Meta::Field::URL ).toString() );
        QString currentDir = info.dir().absolutePath();
        QList<QVariantMap > currentMetadata;
        foreach( const QVariantMap &map, metadata )
        {
            debug() << "processing file " << map.value( Meta::Field::URL );
            QFileInfo info( map.value( Meta::Field::URL ).toString() );
            QString dir = info.dir().absolutePath();
            if( dir != currentDir )
            {
                processor.processDirectory( currentMetadata );
                currentDir = dir;
                currentMetadata.clear();
            }
            currentMetadata.append( map );
        }
        if( !currentMetadata.isEmpty() )
        {
            processor.processDirectory( currentMetadata );
        }
    }
    processor.commit();
#endif
}

void
MtpCollectionLocation::insertStatistics( const QMap<Meta::TrackPtr, QString> &trackMap )
{
    DEBUG_BLOCK
    Q_UNUSED(trackMap);
#if 0
    // NOTE: not sure if this is needed
    MountPointManager *mpm = MountPointManager::instance();
    foreach( const Meta::TrackPtr &track, trackMap.keys() )
    {
        if( m_ignoredDestinations.contains( trackMap[ track ], Qt::CaseSensitive ) )
        {
            continue;
        }
        QString url = trackMap[ track ];
        int deviceid = mpm->getIdForUrl( url );
        QString rpath = mpm->getRelativePath( deviceid, url );
        QString mtp = QString( "SELECT COUNT(*) FROM statistics LEFT JOIN urls ON statistics.url = urls.id "
                               "WHERE urls.deviceid = %1 AND urls.rpath = '%2';" ).arg( QString::number( deviceid ), m_collection->escape( rpath ) );
        QStringList count = m_collection->query( mtp );
        if( count.first().toInt() != 0 )    //crash if the mtp is bad
        {
            continue;   //a statistics row already exists for that url, and we cannot merge the statistics
        }
        //the row will exist because this method is called after insertTracks
        QString select = QString( "SELECT id FROM urls WHERE deviceid = %1 AND rpath = '%2';" ).arg( QString::number( deviceid ), m_collection->escape( rpath ) );
        QStringList result = m_collection->query( select );
        QString id = result.first();    //if result is empty something is going very wrong
        //the following mtp was copied from MtpMeta.cpp
        QString insert = "INSERT INTO statistics(url,rating,score,playcount,accessdate,createdate) VALUES ( %1 );";
        QString data = "%1,%2,%3,%4,%5,%6";
        data = data.arg( id, QString::number( track->rating() ), QString::number( track->score() ) );
        data = data.arg( QString::number( track->playCount() ), QString::number( track->lastPlayed() ), QString::number( track->firstPlayed() ) );
        m_collection->insert( insert.arg( data ), "statistics" );
    }
#endif
}

// NOTE: probably unnecessary
/*
void
MtpCollectionLocation::movedByDestination( const Meta::TrackPtr &track, bool removeFromDatabase )
{
    m_tracksRemovedByDestination.insert( track, removeFromDatabase );
}
*/
#include "MtpCollectionLocation.moc"
