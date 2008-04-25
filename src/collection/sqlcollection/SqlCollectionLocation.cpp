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

#include "SqlCollectionLocation.h"

#include "debug.h"
#include "Meta.h"
#include "MetaUtility.h"
#include "mountpointmanager.h"
#include "ScanManager.h"
#include "ScanResultProcessor.h"
#include "SqlCollection.h"
#include "SqlMeta.h"

#include <QFile>

#include <KLocale>
#include <KSharedPtr>

using namespace Meta;

SqlCollectionLocation::SqlCollectionLocation( SqlCollection const *collection )
    : CollectionLocation()
    , m_collection( const_cast<SqlCollection*>( collection ) )
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

bool
SqlCollectionLocation::isWriteable() const
{
    return true;
}

bool
SqlCollectionLocation::isOrganizable() const
{
    return true;
}

bool
SqlCollectionLocation::remove( Meta::TrackPtr track )
{
    KSharedPtr<SqlTrack> sqlTrack = KSharedPtr<SqlTrack>::dynamicCast( track );
    if( sqlTrack && sqlTrack->inCollection() && sqlTrack->collection()->collectionId() == m_collection->collectionId() )
    {
        bool removed = QFile::remove( sqlTrack->playableUrl().path() );
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
        return removed;
    }
    else
    {
        return false;
    }
}

void
SqlCollectionLocation::copyUrlsToCollection( const KUrl::List &sources )
{
    Q_UNUSED( sources );
    m_collection->scanManager()->setBlockScan( true );  //make sure the collection scanner does not run while we are coyping stuff
    //TODO
    m_collection->scanManager()->setBlockScan( false );
    slotCopyOperationFinished();
}

void
SqlCollectionLocation::insertTracks( const QMap<QString, Meta::TrackPtr > &trackMap )
{
    QList<QVariantMap > metadata;
    const QStringList realUrls = trackMap.keys();
    foreach( const QString &url, realUrls )
    {
        QVariantMap trackData = Meta::Field::mapFromTrack( trackMap[ url ].data() );
        trackData.insert( Meta::Field::URL, url );  //store the new url of the file
        metadata.append( trackData );
    }
    ScanResultProcessor processor( m_collection );
    processor.setScanType( ScanResultProcessor::IncrementalScan );
    //TODO: get the new mtime of all updated/created directories
    //TODO: insert all tracks using ScanResultProcessor::processDirectory
    processor.commit();
}

void
SqlCollectionLocation::insertStatistics( const QMap<QString, Meta::TrackPtr > &trackMap )
{
    MountPointManager *mpm = MountPointManager::instance();
    foreach( const QString &url, trackMap.keys() )
    {
        int deviceid = mpm->getIdForUrl( url );
        QString rpath = mpm->getRelativePath( deviceid, url );
        QString sql = QString( "SELECT COUNT(*) FROM statistics LEFT JOIN urls ON statistics.url = urls.id "
                               "WHERE urls.deviceid = %1 AND urls.rpath = '%2';" ).arg( QString::number( deviceid ), m_collection->escape( rpath ) );
        QStringList count = m_collection->query( sql );
        if( count.first().toInt() != 0 )    //crash if the sql is bad
        {
            continue;   //a statistics row already exists for that url, and we cannot merge the statistics
        }
        //the row will exist because this method is called after insertTracks
        QString select = QString( "SELECT id FROM urls WHERE deviceid = %1 AND rpath = '%2';" ).arg( QString::number( deviceid ), m_collection->escape( rpath ) );
        QStringList result = m_collection->query( select );
        QString id = result.first();    //if result is empty something is going very wrong
        //the following sql was copied from SqlMeta.cpp
        QString insert = "INSERT INTO statistics(url,rating,score,playcount,accessdate,createdate) VALUES ( %1 );";
        QString data = "%1,%2,%3,%4,%5,%6";
        Meta::TrackPtr track = trackMap[ url ];
        data = data.arg( id, QString::number( track->rating() ), QString::number( track->score() ) );
        data = data.arg( QString::number( track->playCount() ), QString::number( track->lastPlayed() ), QString::number( track->firstPlayed() ) );
        m_collection->insert( insert.arg( data ), "statistics" );
    }
}

#include "SqlCollectionLocation.moc"
