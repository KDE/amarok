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

#include "core/support/Amarok.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/FileCollectionLocation.h"
#include "core-impl/meta/file/File.h"
#include "core/meta/support/MetaConstants.h"

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
    , m_eventLoop( 0 )
{
    /* fill in m_collectionFolders.
     * For each collection folder we remember collection location it belongs to. */
    foreach( Collections::Collection *coll , CollectionManager::instance()->queryableCollections() )
    {
        QSharedPointer<Collections::CollectionLocation> location( coll->location() );
        if( location )
        {
            foreach( const QString &path , location->actualLocation() )
            {
                if( m_collectionFolders.contains( path ) )
                {
                    warning() << "Multiple collection locations claim the same path " << path;
                    continue;
                }
                m_collectionFolders.insert( path, location );
                debug() << "Collection folder " << path << " => collection location " << location->prettyLocation();
            }
        }
    }
}

FastForwardWorker::~FastForwardWorker()
{
    QSqlDatabase::removeDatabase( QSqlDatabase::defaultConnection );
}

QSqlDatabase
FastForwardWorker::setupDatabaseConnection()
{
    DEBUG_BLOCK

    const bool isSqlite = m_driver == FastForwardImporter::SQLite;

    QString driver = driverName();
    if( driver.isEmpty() )
    {
        emit importError( i18n( "No database driver was selected" ) );
        return QSqlDatabase(); // no need to set m_failed here, it is in failWithError()
    }

    if( isSqlite && !QFile::exists( m_databaseLocation ) )
    {
        emit importError( i18n( "Database could not be found at: %1", m_databaseLocation ) );
        return QSqlDatabase();
    }

    QSqlDatabase db = QSqlDatabase::addDatabase( driver );
    db.setDatabaseName( isSqlite ? m_databaseLocation : m_database );

    if( !isSqlite )
    {
        db.setHostName( m_hostname );
        db.setUserName( m_username );
        db.setPassword( m_password );
    }

    return db;
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

    QSqlDatabase db = setupDatabaseConnection();
    if( !db.open() )
    {
        failWithError( i18n( "Could not open Amarok 1.4 database: %1", db.lastError().text() ) );
        return;
    }

    QString sql =
      QString( "SELECT lastmountpoint, S.url, S.uniqueid, S.createdate, accessdate, percentage, rating, playcounter, lyrics, title, A.name, R.name, C.name, G.name, Y.name, track, discnumber, filesize "
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
        failWithError( i18n( "Could not execute import query: %1", db.lastError().text() ) );
        return;
    }

    QMap<Collections::CollectionLocation*, QMap<Meta::TrackPtr,QString>* > tracksForInsert;
    ImporterMiscDataStorage dataForInsert;

    for( int c = 0; query.next(); c++ )
    {
        if( m_aborted )
            return;

        int index = 0;
        QString mount    = query.value( index++ ).toString();
        QString url      = query.value( index++ ).toString();
        QString uniqueId = query.value( index++ ).toString();

        QDateTime firstPlayed = QDateTime::fromTime_t(query.value( index++ ).toUInt());
        QDateTime lastPlayed  = QDateTime::fromTime_t(query.value( index++ ).toUInt());
        double score     = query.value( index++ ).toDouble();
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
        url = mount + url.mid( 1 );

        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl( url ) );
        if( track ) // track url exists
        {
            debug() << c << " file exists: " << url;
            Collections::Collection* collection = track->collection(); // try, often will return 0
            if( collection )
            {
                debug() << c << " track already in databse -> update";
                emit trackAdded( track );
            }
            else
            {
                debug() << c << " track not in database";

                Collections::CollectionLocation *location( 0 );
                // go throungh collection locacions and try to find match
                QMapIterator<QString, QSharedPointer<Collections::CollectionLocation> > i( m_collectionFolders );
                while ( i.hasNext() ) {
                    i.next();
                    if( url.startsWith( i.key() ) )
                    {
                        location = i.value().data();
                        break;
                    }
                }

                if( location ) // a collection that can accept this track exists
                {
                    debug() << c << " track found under" << location->prettyLocation() << "-> add";
                    QMap<Meta::TrackPtr,QString> *map;
                    if( tracksForInsert.contains( location ) )
                    {
                        map = tracksForInsert.value( location );
                    }
                    else
                    {
                        map = new QMap<Meta::TrackPtr,QString>;
                        tracksForInsert.insert( location, map );
                    }
                    map->insert( track, track->playableUrl().url() );
                    emit trackAdded( track );
                }
                else // collection that can accept this track does not exist
                {
                    debug() << c << " track is not under configured collection folders -> discard";
                    track = 0;
                    emit trackDiscarded( url );
                    emit showMessage( QString( "<font color='gray'>%1</font>" ).arg(
                            i18n( "(track exists, but does not belong in any of your configured collection folders)" ) ) );
                }
            }
        }
        else // track url does not exist
        {
            debug() << c << " file does not exist: " << url;
            // we need at least a title for a match
            if ( m_smartMatch && !title.isEmpty() ) {
                track = trySmartMatch( url, title, album, artist, composer, genre, year,
                                       trackNr, discNr, filesize );
            }
            else
            {
                debug() << c << " smart matching disabled or too few metadata -> discard";
                emit trackDiscarded( url );
            }
        }

        if( track )
        {
            setTrackMetadata( track, score, rating, firstPlayed, lastPlayed, playCount );
            setTrackMiscData( dataForInsert, track, uniqueId, lyrics, db );
        }
    }

    // iterate over collection locations, add appropriate tracks to each
    {
        QMapIterator<Collections::CollectionLocation*, QMap<Meta::TrackPtr, QString>* > i( tracksForInsert );
        while( i.hasNext() ) {
            i.next();
            Collections::CollectionLocation* location = i.key();
            QMap<Meta::TrackPtr, QString>* tracks = i.value();
            debug() << "Adding new tracks to collection";
            emit showMessage( i18np( "Adding <b>1 new track</b> to Amarok collection <b>%2</b>.",
                                     "Adding <b>%1 new tracks</b> to Amarok collection <b>%2</b>.",
                                     tracks->size(), location->prettyLocation() ) );

            QMapIterator<Meta::TrackPtr, QString> j(*tracks);
            while (j.hasNext()) {
                j.next();
                location->insert( j.key(), j.value() );
            }

            delete tracks; // location is deleted by QSharedPointer
        }
    }

    insertMiscData( dataForInsert ); // this is a hack, see function definition

    if( m_importArtwork )
        importArtwork();
}

void
FastForwardWorker::failWithError( const QString &errorMsg )
{
    debug() << errorMsg;
    emit importError( errorMsg );
    m_failed = true;
}

Meta::TrackPtr
FastForwardWorker::trySmartMatch( const QString url, const QString title, const QString album,
                                  const QString artist, const QString composer, const QString genre,
                                  const uint year, const uint trackNr, const uint discNr,
                                  const uint filesize )
{
    Meta::TrackPtr track( 0 );

    debug() << "    trying to find matching track in collection by tags:" << title << ":" << artist << ":" << album << ": etc...";

    // cleanup from possible previous calls
    m_matchTracks.clear();

    Collections::QueryMaker *trackQueryMaker = CollectionManager::instance()->queryMaker();
    trackQueryMaker->setQueryType( Collections::QueryMaker::Track );

    // set matching criteria to narrow down the corresponding track in
    // the new collection as good as possible
    // NOTE: length: is ruled out, as A1.4 and A2.x sometimes report different
    //       lengths
    //       bitrate: same
    trackQueryMaker->addFilter( Meta::valTitle, title, true, true );
    trackQueryMaker->addFilter( Meta::valAlbum, album, true, true );
    trackQueryMaker->addFilter( Meta::valArtist, artist, true, true );
    trackQueryMaker->addFilter( Meta::valComposer, composer, true, true );
    trackQueryMaker->addFilter( Meta::valGenre, genre, true, true );
    trackQueryMaker->addNumberFilter( Meta::valYear, year, Collections::QueryMaker::Equals );
    trackQueryMaker->addNumberFilter( Meta::valTrackNr, trackNr, Collections::QueryMaker::Equals );
    trackQueryMaker->addNumberFilter( Meta::valDiscNr, discNr, Collections::QueryMaker::Equals );
    trackQueryMaker->addNumberFilter( Meta::valFilesize, filesize, Collections::QueryMaker::Equals );

    connect( trackQueryMaker, SIGNAL(queryDone()), SLOT(queryDone()),
             Qt::QueuedConnection );
    connect( trackQueryMaker, SIGNAL(newResultReady(Meta::TrackList)),
             SLOT(resultReady(Meta::TrackList)),
             Qt::QueuedConnection );
    trackQueryMaker->run();

    m_eventLoop = new QEventLoop();
    m_eventLoop->exec(); // wait for resultReady slot to fire
    delete m_eventLoop;
    m_eventLoop = 0; // avoid dangling pointer

    // evaluate query result
    if ( m_matchTracks.isEmpty() )
    {
        debug() << "    matching done: no track found -> discard";
        emit trackDiscarded( url );
    }
    else if ( m_matchTracks.count() == 1 )
    {
        track = m_matchTracks.first();
        debug() << "    matching done: matching track found -> update";
        emit trackMatchFound( m_matchTracks.first(), url );
    }
    else
    {
        debug() << "    matching done: more than one track found -> discard";
        emit trackMatchMultiple( m_matchTracks, url );
    }

    delete trackQueryMaker;
    return track;
}

void
FastForwardWorker::setTrackMetadata( Meta::TrackPtr track, double score, int rating,
                                        QDateTime firstPlayed, QDateTime lastPlayed, int playCount )
{
    /* import statistics. ec may be different object for different track types.
     * StatisticsCapability for MetaFile::Track only caches info (thus we need to call
     * insertStatistics() afterwards) while Meta::SqlTrack directly saves data to
     * database (thus no insertStatistics() call is necessary)
     */
    Meta::StatisticsPtr statistics = track->statistics();
    statistics->beginUpdate();
    statistics->setScore( score );
    statistics->setRating( rating );
    statistics->setFirstPlayed( firstPlayed );
    statistics->setLastPlayed( lastPlayed );
    statistics->setPlayCount( playCount );
    statistics->endUpdate();
}

void
FastForwardWorker::setTrackMiscData( ImporterMiscDataStorage& dataForInsert, Meta::TrackPtr track,
                                     const QString& uniqueId, QString lyrics, QSqlDatabase db )
{
    QString url = track->playableUrl().url(); // we cannot reuse url, it may have changed in smartMatch

    // lyrics:
    QRegExp lyricsFilter( "<[^>]*>" );
    lyrics.replace( lyricsFilter, QString( "" ) ); // strip html tags
    lyrics.replace( QString( "&apos;" ), QString( "'" ) );
    lyrics.replace( QString( "&quot;" ), QString( "\"" ) );
    lyrics.replace( QString( "&lt;" ), QString( "<" ) );
    lyrics.replace( QString( "&gt;" ), QString( ">" ) );
    if( !lyrics.isEmpty() )
        dataForInsert.insertCachedLyrics( url, lyrics );

    // labels:
    if( !uniqueId.isEmpty() )
    {
        QString labelsSql = QString( "SELECT L.name FROM tags_labels T "
            "LEFT JOIN labels L ON L.id = T.labelid "
            "WHERE L.name != '' AND T.uniqueid = '%1'" ).arg( uniqueId );
        QSqlQuery labelsQuery( labelsSql, db );

        if( labelsQuery.lastError().isValid() )
        {
            failWithError( i18n( "Could not execute labels import query: %1; query was: %2",
                                db.lastError().text(), labelsSql ) );
            return;
        }

        for( int d = 0; labelsQuery.next(); d++ )
        {
            QString label( labelsQuery.value( 0 ).toString() );
            dataForInsert.insertLabel( url,  label );
        }
    }
}

void
FastForwardWorker::insertMiscData( const ImporterMiscDataStorage& dataForInsert )
{
    /* HACK: Meta::Track::setCachedLyrics() and ::addLabel() actually save data only for
     * Meta:SqlTrack.
     * Above call trackForUrl() returns MetaFile:Track if a file is not already in collection.
     * Therefore, we call it again here after all tracks have been added to collection and
     * hope it will return Meta::SqlTrack. */

    debug() << "updating cached lyrics and labels...";
    emit showMessage( i18np( "Updating cached lyrics and labels for 1 track...",
                             "Updating cached lyrics and labels for %1 tracks...",
                             dataForInsert.size() ) );
    int lyricsCount = 0, labelsCount = 0;
    QMapIterator<QString, ImporterMiscData> i( dataForInsert );
    while( i.hasNext() )
    {
        i.next();
        QString url = i.key();
        ImporterMiscData miscData = i.value();

        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl( url ) );
        if( !track ) {
            debug() << "trackForUrl() returned null for url " << url <<
                        ", skipping lyrics and labels update";
            emit showMessage( QString( "<font color='red'>%1</font>" ).arg(
                    i18n( "Failed to update lyrics/labels for track %1", url ) ) );
            continue;
        }

        if( !miscData.cachedLyrics().isEmpty() )
        {
            track->setCachedLyrics( miscData.cachedLyrics() );
            lyricsCount++;
        }
        if( !miscData.labels().isEmpty() )
        {
            foreach( const QString &label, miscData.labels() )
            {
                track->addLabel( label );
            }
            labelsCount++;
        }
    }

    debug() << "lyrics and labels updated";
    QString lyricUpdateMessage = i18np( "Cached lyrics updated for 1 track",
                                        "Cached lyrics updated for %1 tracks",
                                        lyricsCount );

    QString labelUpdateMessage = i18np( "labels added to 1 track",
                                        "labels added to %1 tracks",
                                        labelsCount );


    emit showMessage( i18nc( "%1 is e.g. Cached lyrics updated for 2 tracks, %2 is e.g. labels added to 3 tracks",
                             "%1, %2.",
                             lyricUpdateMessage, labelUpdateMessage ) );
}

void
FastForwardWorker::importArtwork()
{
    emit showMessage( i18n( "Importing downloaded album art..." ) );

    QString newCoverPath = Amarok::saveLocation( "albumcovers/large/" );
    QDir newCoverDir( newCoverPath );
    QDir oldCoverDir( m_importArtworkDir );

    if( newCoverDir.canonicalPath() == oldCoverDir.canonicalPath() )
        return;

    oldCoverDir.setFilter( QDir::Files | QDir::NoDotAndDotDot );

    debug() << "new covers:" << newCoverPath;
    debug() << "old covers:" << m_importArtworkDir;

    int count = 0;
    foreach( const QFileInfo &image, oldCoverDir.entryInfoList() )
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
        else
            count++;
    }

    emit showMessage( i18np( "Copied 1 cover image.", "Copied %1 cover images.", count ) );
}

void
FastForwardWorker::resultReady( const Meta::TrackList &tracks )
{
    m_matchTracks << tracks;
}

void
FastForwardWorker::queryDone()
{
    if( !m_eventLoop )
    {
        error() << "FastForwardWorker::queryDone() was called while m_eventLoop == 0!";
        return;
    }
    if( !m_eventLoop->isRunning() )
    {
        error() << "FastForwardWorker::queryDone() was called while m_eventLoop wasnt running!";
        return;
    }
    m_eventLoop->exit();
}

void
ImporterMiscData::addLabel( const QString &label )
{
    if( !m_labels.contains( label ) )
    {
        m_labels.append( label );
    }
}


void
ImporterMiscDataStorage::insertCachedLyrics( const QString &url, const QString &lyrics )
{
    if( contains( url ) )
    {
        operator[]( url ).setCachedLyrics( lyrics );
    }
    else
    {
        ImporterMiscData data;
        data.setCachedLyrics( lyrics );
        insert( url, data );
    }
}

void
ImporterMiscDataStorage::insertLabel ( const QString &url, const QString &label )
{
    if( contains( url ) )
    {
        operator[]( url ).addLabel( label );
    }
    else
    {
        ImporterMiscData data;
        data.addLabel( label );
        insert( url, data );
    }
}
