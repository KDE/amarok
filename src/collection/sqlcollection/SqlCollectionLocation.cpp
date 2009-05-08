/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *  Copyright (c) 2008 Jason A. Donenfeld <Jason@zx2c4.com>
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

#include "SqlCollectionLocation.h"

#include "Debug.h"
#include "Meta.h"
#include "MetaUtility.h"
#include "MountPointManager.h"
#include "OrganizeCollectionDialog.h"
#include "ScanManager.h"
#include "ScanResultProcessor.h"
#include "SqlCollection.h"
#include "SqlMeta.h"
#include "../../statusbar/StatusBar.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <kjob.h>
#include <KLocale>
#include <KSharedPtr>
#include <kio/job.h>
#include <kio/jobclasses.h>


using namespace Meta;

SqlCollectionLocation::SqlCollectionLocation( SqlCollection const *collection )
    : CollectionLocation()
    , m_collection( const_cast<SqlCollection*>( collection ) )
    , m_overwriteFiles( false )
{
    //nothing to do
}

SqlCollectionLocation::~SqlCollectionLocation()
{
    //nothing to do
}

QString
SqlCollectionLocation::prettyLocation() const
{
    return i18n( "Local Collection" );
}

QStringList
SqlCollectionLocation::actualLocation() const
{
    return MountPointManager::instance()->collectionFolders();
}
bool
SqlCollectionLocation::isWritable() const
{
    return true;
}

bool
SqlCollectionLocation::isOrganizable() const
{
    return true;
}

bool
SqlCollectionLocation::remove( const Meta::TrackPtr &track )
{
    KSharedPtr<SqlTrack> sqlTrack = KSharedPtr<SqlTrack>::dynamicCast( track );
    if( sqlTrack && sqlTrack->inCollection() && sqlTrack->collection()->collectionId() == m_collection->collectionId() )
    {
        bool removed;
        if( !consideredByDestination( track ) )
        {
            removed = QFile::remove( sqlTrack->playableUrl().path() );
        }
        else
        {
            removed = movedByDestination( track );
        }
        if( removed )
        {

            QString query = QString( "SELECT id FROM urls WHERE deviceid = %1 AND rpath = '%2';" )
                                .arg( QString::number( sqlTrack->deviceid() ), m_collection->escape( sqlTrack->rpath() ) );
            QStringList res = m_collection->query( query );
            if( res.isEmpty() )
            {
                warning() << "Tried to remove a track from SqlCollection which is not in the collection";
            }
            else
            {
                int id = res[0].toInt();
                QString query = QString( "DELETE FROM tracks where id = %1;" ).arg( id );
                m_collection->query( query );
            }
        }
        if( removed )
        {
            QFileInfo file( sqlTrack->playableUrl().path() );
            QDir dir = file.dir();
            const QStringList collectionFolders = MountPointManager::instance()->collectionFolders();
            while( !collectionFolders.contains( dir.absolutePath() ) && !dir.isRoot() && dir.count() == 0 )
            {
                const QString name = dir.dirName();
                dir.cdUp();
                if( !dir.rmdir( name ) )
                    break;
            }

        }
        return removed;
    }
    else
    {
        return false;
    }
}

void
SqlCollectionLocation::showDestinationDialog( const Meta::TrackList &tracks, bool removeSources )
{
    setGoingToRemoveSources( removeSources );
    OrganizeCollectionDialog *dialog = new OrganizeCollectionDialog( tracks );
    connect( dialog, SIGNAL( accepted() ), SLOT( slotDialogAccepted() ) );
    connect( dialog, SIGNAL( rejected() ), SLOT( slotDialogRejected() ) );
    dialog->show();
}

void
SqlCollectionLocation::slotDialogAccepted()
{
    sender()->deleteLater();
    OrganizeCollectionDialog *dialog = qobject_cast<OrganizeCollectionDialog*>( sender() );
    m_destinations = dialog->getDestinations();
    m_overwriteFiles = dialog->overwriteDestinations();
    slotShowDestinationDialogDone();
}

void
SqlCollectionLocation::slotDialogRejected()
{
    DEBUG_BLOCK
    sender()->deleteLater();
    abort();
}

void
SqlCollectionLocation::slotJobFinished( KJob *job )
{
    if( job->error() )
    {
        //TODO: proper error handling
        warning() << "An error occurred when copying a file: " << job->errorString();
        source()->transferError(m_jobs.value( job ), KIO::buildErrorString( job->error(), job->errorString() ) );
        m_destinations.remove( m_jobs.value( job ) );
    }
    m_jobs.remove( job );
    job->deleteLater();
    if( m_jobs.isEmpty() )
    {
        insertTracks( m_destinations );
        insertStatistics( m_destinations );
        m_collection->scanManager()->setBlockScan( false );
        slotCopyOperationFinished();
    }
}

void
SqlCollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources )
{
    m_collection->scanManager()->setBlockScan( true );  //make sure the collection scanner does not run while we are coyping stuff
    bool jobsCreated = false;
    foreach( const Meta::TrackPtr &track, sources.keys() )
    {
        KIO::FileCopyJob *job = 0;
        KUrl dest = m_destinations[ track ];
        dest.cleanPath();
        KUrl src = sources[ track ];
        src.cleanPath();
        debug() << "copying from " << src << " to " << dest;
        KIO::JobFlags flags = KIO::HideProgressInfo;
        if( m_overwriteFiles )
        {
            flags |= KIO::Overwrite;
        }
        QFileInfo info( dest.pathOrUrl() );
        QDir dir = info.dir();
        if( !dir.exists() )
        {
            if( !dir.mkpath( "." ) )
            {
                warning() << "Could not create directory " << dir;
                //TODO: might be shown to the user at some point
                //i18n-ify
                source->transferError( track, "Could not create directory " + dir.absolutePath() );
                continue;
            }
        }
        if( src == dest) {
            //no changes, so leave the database alone, and don't erase anything
            source()->setMovedByDestination( track, false );
            continue;
        }
        //we should only move it directly if we're moving within the same collection
        else if( isGoingToRemoveSources() && source()->collection() == collection() )
        {
            job = KIO::file_move( src, dest, -1, flags );
            source()->setMovedByDestination( track, true );  //remove old location from tracks table
        }
        else
        {
            //later on in the case that remove is called, the file will be deleted because we didn't apply moveByDestination to the track
            job = KIO::file_copy( src, dest, -1, flags );
        }
        if( job )   //just to be safe
        {
            connect( job, SIGNAL( result(KJob*) ), SLOT( slotJobFinished(KJob*) ) );
            QString name = track->prettyName();
            if( track->artist() )
                name = QString( "%1 - %2" ).arg( track->artist()->name(), track->prettyName() );

            The::statusBar()->newProgressOperation( job, i18n( "Transferring: %1", name ) );
            m_jobs.insert( job, track );
            job->start();
            jobsCreated = true;
        }
    }
    if( !jobsCreated ) //this signal needs to be called no matter what, even if there are no job finishes to call it
        slotCopyOperationFinished();
}

void
SqlCollectionLocation::insertTracks( const QMap<Meta::TrackPtr, QString> &trackMap )
{
    QList<QVariantMap > metadata;
    QStringList urls;
    foreach( const Meta::TrackPtr &track, trackMap.keys() )
    {
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
}

QMap<QString, uint>
SqlCollectionLocation::updatedMtime( const QStringList &urls )
{
    QMap<QString, uint> mtime;
    foreach( const QString &url, urls )
    {
        QFileInfo fileInfo( url );
        QDir parentDir = fileInfo.dir();
        while( !mtime.contains( parentDir.absolutePath() ) && m_collection->isDirInCollection( parentDir.absolutePath() ) )
        {
            QFileInfo dir( parentDir.absolutePath() );
            mtime.insert( parentDir.absolutePath(), dir.lastModified().toTime_t() );
            if( !parentDir.cdUp() )
            {
                break;  //we arrived at the root of the filesystem
            }
        }
    }
    return mtime;
}

void
SqlCollectionLocation::insertStatistics( const QMap<Meta::TrackPtr, QString> &trackMap )
{
    MountPointManager *mpm = MountPointManager::instance();
    foreach( const Meta::TrackPtr &track, trackMap.keys() )
    {
        QString url = trackMap[ track ];
        int deviceid = mpm->getIdForUrl( url );
        QString rpath = mpm->getRelativePath( deviceid, url );
        QString sql = QString( "SELECT COUNT(*) FROM statistics LEFT JOIN urls ON statistics.url = urls.id "
                               "WHERE urls.deviceid = %1 AND urls.rpath = '%2';" ).arg( QString::number( deviceid ), m_collection->escape( rpath ) );
        QStringList count = m_collection->query( sql );
        if( count.isEmpty() || count.first().toInt() != 0 )    //crash if the sql is bad
        {
            continue;   //a statistics row already exists for that url, and we cannot merge the statistics
        }
        //the row will exist because this method is called after insertTracks
        QString select = QString( "SELECT id FROM urls WHERE deviceid = %1 AND rpath = '%2';" ).arg( QString::number( deviceid ), m_collection->escape( rpath ) );
        QStringList result = m_collection->query( select );
        if( result.isEmpty() )
        {
            warning() << "SQL Query returned no results:" << select;
            continue;
        }
        QString id = result.first();    //if result is empty something is going very wrong
        //the following sql was copied from SqlMeta.cpp
        QString insert = "INSERT INTO statistics(url,rating,score,playcount,accessdate,createdate) VALUES ( %1 );";
        QString data = "%1,%2,%3,%4,%5,%6";
        data = data.arg( id, QString::number( track->rating() ), QString::number( track->score() ),
                    QString::number( track->playCount() ), QString::number( track->lastPlayed() ), QString::number( track->firstPlayed() ) );
        m_collection->insert( insert.arg( data ), "statistics" );
    }
}

#include "SqlCollectionLocation.moc"
