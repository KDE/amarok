/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#include "FastForwardWorker.h"

#include "Amarok.h"
#include "CollectionManager.h"
#include "CollectionLocation.h"
#include "Debug.h"
#include "collection/support/FileCollectionLocation.h"
#include "StatisticsCapability.h"
#include "meta/file/File.h"
#include "MetaConstants.h"

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <threadweaver/Thread.h>

#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QSqlError>
#include <QSqlQuery>

FastForwardWorker::FastForwardWorker()
    : ThreadWeaver::Job()
    , m_aborted( false )
    , m_failed( false )
    , m_driver( FastForwardImporter::SQLite )
    , m_databaseLocation()
    , m_database()
    , m_hostname()
    , m_username()
    , m_password()
    , m_smartMatch( false )
    , m_importArtwork( false )
    , m_queryRunning( false )
{
}

QSqlDatabase
FastForwardWorker::databaseConnection()
{
    DEBUG_BLOCK

    const bool isSqlite = m_driver == FastForwardImporter::SQLite;

    QString driver = driverName();
    if( driver.isEmpty() )
    {
        emit importError( i18n("No database driver was selected") );
        m_failed = true;
        return QSqlDatabase();
    }

    if( isSqlite && !QFile::exists( m_databaseLocation ) )
    {
        emit importError( i18n("Database could not be found at: %1", m_databaseLocation ) );
        m_failed = true;
        return QSqlDatabase();
    }

    QSqlDatabase connection = QSqlDatabase::addDatabase( driver );
    connection.setDatabaseName( isSqlite ? m_databaseLocation : m_database );

    if( !isSqlite )
    {
        connection.setHostName( m_hostname );
        connection.setUserName( m_username );
        connection.setPassword( m_password );
    }
    return connection;
}

const QString
FastForwardWorker::driverName() const
{
    switch( m_driver )
    {
        case FastForwardImporter::SQLite:     return "QSQLITE";
        case FastForwardImporter::MySQL:      return "QMYSQL";
        case FastForwardImporter::PostgreSQL: return "QPSQL";
    }
    return QString();
}

void
FastForwardWorker::run()
{
    DEBUG_BLOCK

    QSqlDatabase db = databaseConnection();
    if( !db.open() )
    {
        QString errorMsg = i18n( "Could not open Amarok 1.4 database: %1", db.lastError().text() );
        debug() << errorMsg;
        emit importError( errorMsg );
        m_failed = true;
        return;
    }


    QString sql =
      QString( "SELECT lastmountpoint, S.url, S.createdate, accessdate, percentage, rating, playcounter, lyrics, title, A.name, R.name, C.name, G.name, Y.name, track, discnumber, filesize "
        "FROM statistics S "
        "LEFT OUTER JOIN devices D "
        "  ON S.deviceid = D.id "
        "LEFT OUTER JOIN lyrics L "
        "  ON L.deviceid = S.deviceid "
        "  AND L.url = S.url "
        "LEFT OUTER JOIN tags T "
        "  ON T.deviceid = S.deviceid "
        "  AND T.url = S.url "
        "LEFT OUTER JOIN album A "
        "  ON T.album = A.id "
        "LEFT OUTER JOIN artist R "
        "  ON T.artist = R.id "
        "LEFT OUTER JOIN composer C "
        "  ON T.composer = C.id "
        "LEFT OUTER JOIN genre G "
        "  ON T.genre = G.id "
        "LEFT OUTER JOIN year Y "
        "  ON T.year = Y.id "
        "ORDER BY lastmountpoint, S.url" );
    QSqlQuery query( sql, db );

    if( query.lastError().isValid() )
    {
        QString errorMsg = i18n( "Could not execute import query: %1", db.lastError().text() );
        error() << "Error executing import query:" << errorMsg;
        emit importError( errorMsg );
        m_failed = true;
        return;
    }

    debug() << "active?" << query.isActive() << "size:" << query.size();

    QMap<Meta::TrackPtr,QString> tracksForInsert;

    for( int c = 0; query.next(); c++ )
    {
        if( m_aborted )
            return;

        int index = 0;
        QString mount = query.value( index++ ).toString();
        QString url   = query.value( index++ ).toString();

        uint firstPlayed = query.value( index++ ).toUInt();
        uint lastPlayed  = query.value( index++ ).toUInt();
        uint score       = query.value( index++ ).toDouble();
        int rating       = query.value( index++ ).toInt();
        int playCount    = query.value( index++ ).toInt();
        QString lyrics   = query.value( index++ ).toString();
        QString title    = query.value( index++ ).toString();
        QString album    = query.value( index++ ).toString();
        QString artist   = query.value( index++ ).toString();
        QString composer = query.value( index++ ).toString();
        QString genre    = query.value( index++ ).toString();
        uint year        = query.value( index++ ).toUInt();
        uint trackNr     = query.value( index++ ).toUInt();
        uint discNr      = query.value( index++ ).toUInt();
        uint filesize    = query.value( index++ ).toUInt();
        
        // remove the relative part of the url, and make the url absolute
        url = mount + url.mid(1);


        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl( url ) );
        if( track )
        {
            debug() << c << " found track by URL: " << url;

            if( !track->inCollection() )
            {
                tracksForInsert.insert( track, track->playableUrl().url() );
                debug() << c << " inserting track:" << track->playableUrl();
            }
            else {
                Amarok::Collection* collection = track->collection();
                if (collection)
                    debug() << c << " track in collection (" << track->collection()->location()->prettyLocation() << "):" << track->playableUrl();
            }

            emit trackAdded( track );

        }
        else
        {
            debug() << c << " no track found by URL: " << url;

            // we need at least a title for a match
            if ( m_smartMatch && !title.isEmpty() ) {

                debug() << c << " trying to find matching track in collection by tags:" << title << ":" << artist << ":" << album << ": etc...";

                Amarok::Collection *coll = CollectionManager::instance()->primaryCollection();
                if( !coll )
                    continue;

                // setup query
                m_matchTracks.clear();
                // state var to make the query synchronous (not exactly elegant, but'll do..)
                m_queryRunning = true;

                QueryMaker *qm_track = coll->queryMaker()->setQueryType( QueryMaker::Track );

                // set matching criteria to narrow down the corresponding track in
                // the new collection as good as possible
                // NOTE: length: is ruled out, as A1.4 and A2.x sometimes report different
                //       lengths
                //       bitrate: same
                qm_track->addFilter( Meta::valTitle, title, true, true );
                qm_track->addFilter( Meta::valAlbum, album, true, true );
                qm_track->addFilter( Meta::valArtist, artist, true, true );
                qm_track->addFilter( Meta::valComposer, composer, true, true );
                qm_track->addFilter( Meta::valGenre, genre, true, true );
                qm_track->addNumberFilter( Meta::valYear, year, QueryMaker::Equals );
                qm_track->addNumberFilter( Meta::valTrackNr, trackNr, QueryMaker::Equals );
                qm_track->addNumberFilter( Meta::valDiscNr, discNr, QueryMaker::Equals );
                qm_track->addNumberFilter( Meta::valFilesize, filesize, QueryMaker::Equals );

                connect( qm_track, SIGNAL( queryDone() ), SLOT( queryDone() ) );
                connect( qm_track, SIGNAL( newResultReady( QString, Meta::TrackList ) ), SLOT( resultReady( QString, Meta::TrackList ) ), Qt::QueuedConnection );
                qm_track->run();

                // block until query is finished
                QCoreApplication::processEvents();
                while ( m_queryRunning == true ) {
                    thread()->msleep( 10 );
                    QCoreApplication::processEvents();
                }

                // evaluate query result
                if ( m_matchTracks.isEmpty() )
                {

                    debug() << c << " matching done: no track found for url " << url;

                    emit trackDiscarded( url );

                }
                else if ( m_matchTracks.count() == 1 )
                {

                    track = m_matchTracks.first();

                    debug() << c << " matching done: matching track found for url " << url;
                    debug() << c << "  (" << track->collection()->location()->prettyLocation() << "):" << track->playableUrl();

                    emit trackMatchFound( m_matchTracks.first(), url );

                }
                else
                {
                    debug() << c << " matching done: more than one track found for url " << url;

                    foreach( Meta::TrackPtr d_track, m_matchTracks )
                    {
                        if ( d_track && d_track->artist() && d_track->album() )
                            debug() << c << "   found track: " << d_track->name() << " : " << d_track->artist()->name() << " : " << d_track->album()->name() << " - " << d_track->playableUrl();
                        ;
                    }

                    emit trackMatchMultiple( m_matchTracks, url );
                }


                delete qm_track;

            } else {
                debug() << c << " no track produced for URL " << url;

                emit trackDiscarded( url );
            }
        }

        if( track )
        {
            // import statistics
            Meta::StatisticsCapability *ec = track->create<Meta::StatisticsCapability>();
            if( !ec )
                continue;

            ec->beginStatisticsUpdate();
            ec->setScore( score );
            ec->setRating( rating );
            ec->setFirstPlayed( firstPlayed );
            ec->setLastPlayed( lastPlayed );
            ec->setPlayCount( playCount );
            ec->endStatisticsUpdate();
            
            if( !lyrics.isEmpty() )
                track->setCachedLyrics( lyrics );
            }
    }

    if( tracksForInsert.size() > 0 )
    {
        emit showMessage( i18n( "Synchronizing Amarok database..." ) );
        CollectionLocation *location = CollectionManager::instance()->primaryCollection()->location();
        location->insertTracks( tracksForInsert );
        location->insertStatistics( tracksForInsert );
    }

    if( m_importArtwork )
    {
        const QString message = i18n( "Importing downloaded album art" );
        emit showMessage( message );

        QString newCoverPath = Amarok::saveLocation( "albumcovers/large/" );
        QDir newCoverDir( newCoverPath );
        QDir oldCoverDir( m_importArtworkDir ); 

        if( newCoverDir.canonicalPath() == oldCoverDir.canonicalPath() )
            return;

        oldCoverDir.setFilter( QDir::Files | QDir::NoDotAndDotDot );

        debug() << "new covers:" << newCoverPath;
        debug() << "old covers:" << m_importArtworkDir;

        foreach( QFileInfo image, oldCoverDir.entryInfoList() )
        {
            if( m_aborted )
                return;

            debug() << "image copy:" << image.fileName() << " : " << image.absoluteFilePath();
            QString newPath = newCoverDir.absoluteFilePath( image.fileName() );

            KUrl src( image.absoluteFilePath() );
            KUrl dst( newPath );

            //TODO: should this be asynchronous?
            KIO::FileCopyJob *job = KIO::file_copy( src, dst, -1 /*no special perms*/ , KIO::HideProgressInfo );
            if( !job->exec() ) // job deletes itself
                error() << "Couldn't copy image" << image.fileName();
        }
    }
}

void FastForwardWorker::resultReady( const QString &collectionId, const Meta::TrackList &tracks )
{
    Q_UNUSED( collectionId )

    m_matchTracks << tracks;
}

void FastForwardWorker::queryDone()
{
    m_queryRunning = false;
}

