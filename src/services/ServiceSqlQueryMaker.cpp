/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#define DEBUG_PREFIX "ServiceSqlQueryMaker"

#include "ServiceSqlQueryMaker.h"

#include <core/storage/SqlStorage.h>
#include "core/meta/support/MetaConstants.h"
#include "core/support/Debug.h"
#include "core-impl/storage/StorageManager.h"
#include "ServiceSqlCollection.h"

#include <QStack>
#include <QSharedPointer>

using namespace Collections;

class ServiceSqlWorkerThread : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        ServiceSqlWorkerThread( ServiceSqlQueryMaker *queryMaker )
            : QObject()
            , ThreadWeaver::Job()
            , m_queryMaker( queryMaker )
            , m_aborted( false )
        {
            //nothing to do
        }

        void requestAbort() override
        {
            m_aborted = true;
        }

    Q_SIGNALS:
        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);

    protected:

        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = nullptr) override
        {
            Q_UNUSED(self);
            Q_UNUSED(thread);
            QString query = m_queryMaker->query();
            QStringList result = m_queryMaker->runQuery( query );
            if( !m_aborted )
                m_queryMaker->handleResult( result );

            if( m_aborted )
                setStatus(Status_Aborted);
            else
                setStatus(Status_Running);
        }

        void defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread) override
        {
            Q_EMIT started(self);
            ThreadWeaver::Job::defaultBegin(self, thread);
        }

        void defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread) override
        {
            ThreadWeaver::Job::defaultEnd(self, thread);
            if (!self->success()) {
                Q_EMIT failed(self);
            }
            Q_EMIT done(self);
        }

    private:
        ServiceSqlQueryMaker *m_queryMaker;
        bool m_aborted;
};

struct ServiceSqlQueryMaker::Private
{
    enum { TRACKS_TABLE = 1, ALBUMS_TABLE = 2, ARTISTS_TABLE = 4, GENRE_TABLE = 8, ALBUMARTISTS_TABLE = 16 };
    int linkedTables;
    QueryMaker::QueryType queryType;
    QString query;
    QString queryReturnValues;
    QString queryFrom;
    QString queryMatch;
    QString queryFilter;
    QString queryOrderBy;
    //bool includedBuilder;
    //bool collectionRestriction;
    AlbumQueryMode albumMode;
    bool withoutDuplicates;
    int maxResultSize;
    QSharedPointer<ServiceSqlWorkerThread> worker;
    QStack<bool> andStack;
};

ServiceSqlQueryMaker::ServiceSqlQueryMaker( ServiceSqlCollection* collection, ServiceMetaFactory * metaFactory, ServiceSqlRegistry * registry )
    : QueryMaker()
    , m_collection( collection )
    , m_registry( registry )
    , m_metaFactory( metaFactory )
    , d( new Private )
{
    //d->includedBuilder = true;
    //d->collectionRestriction = false;
    d->albumMode = AllAlbums;

    d->queryType = QueryMaker::None;
    d->linkedTables = 0;
    d->withoutDuplicates = false;
    d->maxResultSize = -1;
    d->andStack.push( true );
}

ServiceSqlQueryMaker::~ServiceSqlQueryMaker()
{}

void
ServiceSqlQueryMaker::abortQuery()
{
    if( d->worker )
        d->worker->requestAbort();
}

void
ServiceSqlQueryMaker::run()
{
    if( d->queryType == QueryMaker::None )
        return; //better error handling?
    if( d->worker && !d->worker->isFinished() )
    {
        //the worker thread seems to be running
        //TODO: wait for job to complete

    }
    else
    {
        d->worker.reset( new ServiceSqlWorkerThread( this ) );
        connect( d->worker.data(), &ServiceSqlWorkerThread::done, this, &ServiceSqlQueryMaker::done );

        ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(d->worker) );
    }
}

void
ServiceSqlQueryMaker::done( ThreadWeaver::JobPointer job )
{
    Q_UNUSED( job )

    d->worker.clear();
    Q_EMIT queryDone();
}

QueryMaker*
ServiceSqlQueryMaker::setQueryType( QueryType type)
{
    switch( type ) {
    case QueryMaker::Track:
        //make sure to keep this method in sync with handleTracks(QStringList) and the SqlTrack ctor
        if( d->queryType == QueryMaker::None )
        {
            QString prefix = m_metaFactory->tablePrefix();
            //d->queryFrom = ' ' + prefix + "_tracks";

            d->withoutDuplicates = true;
            d->queryFrom = QLatin1Char(' ') + prefix + QStringLiteral("_tracks");
            d->queryType = QueryMaker::Track;
            d->queryReturnValues =  m_metaFactory->getTrackSqlRows() + QLatin1Char(',') +
            m_metaFactory->getAlbumSqlRows() + QLatin1Char(',') +
            m_metaFactory->getArtistSqlRows() + QLatin1Char(',') +
            m_metaFactory->getGenreSqlRows();

            d->linkedTables |= Private::GENRE_TABLE;
            d->linkedTables |= Private::ARTISTS_TABLE;
            d->linkedTables |= Private::ALBUMS_TABLE;

            d->queryOrderBy += QStringLiteral(" GROUP BY ") + prefix + QStringLiteral("_tracks.id"); //fixes the same track being shown several times due to being in several genres

            if ( d->linkedTables & Private::ARTISTS_TABLE )
            {
                d->queryOrderBy += QStringLiteral(" ORDER BY ") + prefix + QStringLiteral("_tracks.album_id"); //make sure items are added as album groups
            }
        }
        return this;

    case QueryMaker::Artist:
        if( d->queryType == QueryMaker::None )
        {
            QString prefix = m_metaFactory->tablePrefix();
            d->queryFrom = QLatin1Char(' ') + prefix + QStringLiteral("_tracks");
            d->linkedTables |= Private::ARTISTS_TABLE;
            d->linkedTables |= Private::ALBUMS_TABLE;
            d->queryType = QueryMaker::Artist;
            d->withoutDuplicates = true;
            d->queryReturnValues = m_metaFactory->getArtistSqlRows();

            d->queryOrderBy += QStringLiteral(" GROUP BY ") + prefix + QStringLiteral("_tracks.id"); //fixes the same track being shown several times due to being in several genres
        }
        return this;

    case QueryMaker::AlbumArtist:
        if( d->queryType == QueryMaker::None )
        {
            QString prefix = m_metaFactory->tablePrefix();
            d->queryFrom = QLatin1Char(' ') + prefix + QStringLiteral("_tracks");
            d->linkedTables |= Private::ALBUMARTISTS_TABLE;
            d->queryType = QueryMaker::AlbumArtist;
            d->withoutDuplicates = true;
            d->queryReturnValues = QStringLiteral( "albumartists.id, " ) +
                                            QStringLiteral( "albumartists.name, " )+
                                            QStringLiteral( "albumartists.description ");
            d->queryOrderBy += QStringLiteral(" GROUP BY ") + prefix + QStringLiteral("_tracks.id"); //fixes the same track being shown several times due to being in several genres
        }
        return this;

    case QueryMaker::Album:
        if( d->queryType == QueryMaker::None )
        {
            QString prefix = m_metaFactory->tablePrefix();
            d->queryFrom = QLatin1Char(' ') + prefix + QStringLiteral("_tracks");
            d->queryType = QueryMaker::Album;
            d->linkedTables |= Private::ALBUMS_TABLE;
            d->linkedTables |= Private::ARTISTS_TABLE;
            d->withoutDuplicates = true;
            d->queryReturnValues = m_metaFactory->getAlbumSqlRows() + QLatin1Char(',') +
            m_metaFactory->getArtistSqlRows();

            d->queryOrderBy += QStringLiteral(" GROUP BY ") + prefix + QStringLiteral("_tracks.id"); //fixes the same track being shown several times due to being in several genres
        }
        return this;

    case QueryMaker::Composer:
        /* if( d->queryType == Private::NONE )
            {
                d->queryType = QueryMaker::Composer;
                d->withoutDuplicates = true;
                d->linkedTables |= QueryMaker::Composer_TAB;
                d->queryReturnValues = "composer.name, composer.id";
            }*/
        return this;

    case QueryMaker::Genre:
        if( d->queryType == QueryMaker::None )
        {
            QString prefix = m_metaFactory->tablePrefix();
            d->queryFrom = QLatin1Char(' ') + prefix + QStringLiteral("_genre");
            d->queryType = QueryMaker::Genre;
            //d->linkedTables |= QueryMaker::Albums_TABLE;
            //d->linkedTables |= QueryMaker::Genre_TABLE;
            d->withoutDuplicates = true;
            d->queryReturnValues = m_metaFactory->getGenreSqlRows();
            d->queryOrderBy = QStringLiteral(" GROUP BY ") + prefix + QStringLiteral("_genre.name"); // HAVING COUNT ( " + prefix +"_genre.name ) > 10 ";
        }
        return this;

    case QueryMaker::Year:
        /*if( d->queryType == Private::NONE )
        {
            d->queryType = Private::YEAR;
            d->withoutDuplicates = true;
            d->linkedTables |= Private::YEAR_TAB;
            d->queryReturnValues = "year.name, year.id";
        }*/
        return this;

    case QueryMaker::Custom:
        /* if( d->queryType == Private::NONE )
        d->queryType = Private::CUSTOM;*/
        return this;
    
    case QueryMaker::Label:
    case QueryMaker::None:
        return this;
    }

    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const Meta::TrackPtr &track )
{
    //DEBUG_BLOCK
    Q_UNUSED( track );
    //TODO still pondering this one...
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const Meta::ArtistPtr &artist, QueryMaker::ArtistMatchBehaviour behaviour )
{
    QString prefix = m_metaFactory->tablePrefix();

    if( !d )
        return this;

    if( behaviour == AlbumArtists || behaviour == AlbumOrTrackArtists )
        d->linkedTables |= Private::ALBUMARTISTS_TABLE;

    //this should NOT be made into a static cast as this might get called with an incompatible type!
    const Meta::ServiceArtist * serviceArtist = dynamic_cast<const Meta::ServiceArtist *>( artist.data() );
    d->linkedTables |= Private::ARTISTS_TABLE;
    if( serviceArtist )
    {
        switch( behaviour )
        {
            case TrackArtists:
                 d->queryMatch += QString( QStringLiteral(" AND ") + prefix + QStringLiteral("_artists.id= '%1'") ).arg( serviceArtist->id() );
                 break;
            case AlbumArtists:
                 d->queryMatch += QStringLiteral( " AND albumartists.id= '%1'" ).arg( serviceArtist->id() );
                 break;
            case AlbumOrTrackArtists:
                 d->queryMatch += QString( QStringLiteral(" AND ( ") + prefix + QStringLiteral("_artists.id= '%1' OR albumartists.id= '%1' )") ).arg( serviceArtist->id() );
                 break;
        }
    }
    else
    {
        switch( behaviour )
        {
            case TrackArtists:
                 d->queryMatch += QString( QStringLiteral(" AND ") + prefix + QStringLiteral("_artists.name= '%1'") ).arg( escape( artist->name() ) );
                 break;
            case AlbumArtists:
                 d->queryMatch += QStringLiteral( " AND albumartists.name= '%1'" ).arg( escape( artist->name() ) );
                 break;
            case AlbumOrTrackArtists:
                 d->queryMatch += QString( QStringLiteral(" AND ( ") + prefix + QStringLiteral("_artists.name= '%1' OR albumartists.name= '%1' )") ).arg( escape( artist->name() ) );
                 break;
        }
    }
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const Meta::AlbumPtr &album )
{
    QString prefix = m_metaFactory->tablePrefix();

    if( !d )
        return this;

    //this should NOT be made into a static cast as this might get called with an incompatible type!
    const Meta::ServiceAlbumPtr serviceAlbum = Meta::ServiceAlbumPtr::dynamicCast( album );

    d->linkedTables |= Private::ALBUMS_TABLE;
    d->linkedTables |= Private::ARTISTS_TABLE;
    if( d->queryType == QueryMaker::Genre )
        d->linkedTables |= Private::GENRE_TABLE;
    if( serviceAlbum )
    {
        d->queryMatch += QStringLiteral(" AND ") + prefix + QStringLiteral("_albums.id = '%1'" ).arg( serviceAlbum->id() );
    }
    else
    {
        d->queryMatch += QStringLiteral(" AND ") + prefix + QStringLiteral("_albums.name='%1'" ).arg( escape( album->name() ) );
    }
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
    QString prefix = m_metaFactory->tablePrefix();

    //this should NOT be made into a static cast as this might get called with an incompatible type!
    const Meta::ServiceGenre* serviceGenre = static_cast<const Meta::ServiceGenre *>( genre.data() );
    if( !d || !serviceGenre )
        return this;

    //genres link to albums in the database, so we need to start from here unless soig a track query

   // if (  d->queryType == Private::TRACK ) {
        //d->queryFrom = ' ' + prefix + "_tracks";
    d->linkedTables |= Private::ALBUMS_TABLE;
    //} else
        //d->queryFrom = ' ' + prefix + "_albums";

        //if ( d->queryType == Private::ARTIST )
    //d->linkedTables |= Private::ARTISTS_TABLE;
    d->linkedTables |= Private::GENRE_TABLE;
    d->queryMatch += QString( QStringLiteral(" AND ") + prefix + QStringLiteral("_genre.name = '%1'") ).arg( serviceGenre->name() );

    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const Meta::ComposerPtr &composer )
{
    Q_UNUSED( composer );
    //TODO
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const Meta::YearPtr &year )
{
    Q_UNUSED( year );
    //TODO
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const Meta::LabelPtr &label )
{
    Q_UNUSED( label );
    //TODO
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    if( !isValidValue( value ) )
    {
        return this;
    }
    //a few hacks needed by some of the speedup code:
    if ( d->queryType == QueryMaker::Genre )
    {
        QString prefix = m_metaFactory->tablePrefix();
        d->queryFrom = QLatin1Char(' ') + prefix + QStringLiteral("_tracks");
        d->linkedTables |= Private::ALBUMS_TABLE;
        d->linkedTables |= Private::ARTISTS_TABLE;
        d->linkedTables |= Private::GENRE_TABLE;
    }
    QString like = likeCondition( filter, !matchBegin, !matchEnd );
    d->queryFilter += QStringLiteral( " %1 %2 %3 " ).arg( andOr(), nameForValue( value ), like );
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{

    if( isValidValue( value ) )
    {
        QString like = likeCondition( filter, !matchBegin, !matchEnd );
        d->queryFilter += QStringLiteral( " %1 NOT %2 %3 " ).arg( andOr(), nameForValue( value ), like );
    }
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    AMAROK_NOTIMPLEMENTED
    Q_UNUSED( value )
    Q_UNUSED( filter )
    Q_UNUSED( compare )
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    AMAROK_NOTIMPLEMENTED
    Q_UNUSED( value )
    Q_UNUSED( filter )
    Q_UNUSED( compare )
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addReturnValue( qint64 value )
{
    Q_UNUSED( value );
    /*if( d->queryType == Private::CUSTOM )
    {
        if ( !d->queryReturnValues.isEmpty() )
            d->queryReturnValues += QLatin1Char(',');
        d->queryReturnValues += nameForValue( value );
    }*/
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
    Q_UNUSED( value )
    Q_UNUSED( function )
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::orderBy( qint64 value, bool descending )
{
    Q_UNUSED( value );
    if ( d->queryOrderBy.isEmpty() )
    d->queryOrderBy = QStringLiteral(" ORDER BY name "); //TODO FIX!!
    d->queryOrderBy += QStringLiteral( " %1 " ).arg( descending ? QStringLiteral("DESC") : QStringLiteral("ASC") );
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::limitMaxResultSize( int size )
{
    d->maxResultSize = size;
    return this;
}

void
ServiceSqlQueryMaker::linkTables()
{
    if( !d->linkedTables )
        return;

    QString prefix = m_metaFactory->tablePrefix();

    //d->queryFrom = ' ' + prefix + "_tracks";

    if( d->linkedTables & Private::ALBUMS_TABLE )
       d->queryFrom += QStringLiteral(" LEFT JOIN ") + prefix + QStringLiteral("_albums ON ") + prefix + QStringLiteral("_tracks.album_id = ") + prefix + QStringLiteral("_albums.id");
    if( d->linkedTables & Private::ARTISTS_TABLE )
       d->queryFrom += QStringLiteral(" LEFT JOIN ") + prefix + QStringLiteral("_artists ON ") + prefix + QStringLiteral("_albums.artist_id = ") + prefix + QStringLiteral("_artists.id");
    if( d->linkedTables & Private::ALBUMARTISTS_TABLE )
        d->queryFrom += QStringLiteral(" LEFT JOIN ") + prefix + QStringLiteral("_artists AS albumartists ON ") + prefix + QStringLiteral("_albums.artist_id = albumartists.id");
    if( d->linkedTables & Private::GENRE_TABLE )
       d->queryFrom += QStringLiteral(" LEFT JOIN ") + prefix + QStringLiteral("_genre ON ") + prefix + QStringLiteral("_genre.album_id = ") + prefix + QStringLiteral("_albums.id");
}

void
ServiceSqlQueryMaker::buildQuery()
{
    if( d->albumMode == OnlyCompilations )
        return;

    linkTables();
    QString query = QStringLiteral("SELECT ");
    if ( d->withoutDuplicates )
        query += QLatin1String("DISTINCT ");
    query += d->queryReturnValues;
    query += QLatin1String(" FROM ");
    query += d->queryFrom;
    query += QLatin1String(" WHERE 1 "); // oh... to not have to bother with the leadig "AND"
                          // that may or may not be needed
    query += d->queryMatch;
    if ( !d->queryFilter.isEmpty() )
    {
        query += QLatin1String(" AND ( 1 ");
        query += d->queryFilter;
        query += QLatin1String(" ) ");
    }
    //query += d->queryFilter;
    query += d->queryOrderBy;
    if ( d->maxResultSize > -1 )
        query += QStringLiteral( " LIMIT %1 OFFSET 0 " ).arg( d->maxResultSize );
    query += QLatin1Char(';');
    d->query = query;
}

QString
ServiceSqlQueryMaker::query()
{
    if ( d->query.isEmpty() )
        buildQuery();
    return d->query;
}

QStringList
ServiceSqlQueryMaker::runQuery( const QString &query )
{
   if( d->albumMode == OnlyCompilations )
       return QStringList();

   return m_collection->query( query );
}

void
ServiceSqlQueryMaker::handleResult( const QStringList &result )
{
    if( !result.isEmpty() )
    {
        switch( d->queryType ) {
        /*case Private::CUSTOM:
            Q_EMIT newResultReady( result );
            break;*/
        case QueryMaker::Track:
            handleTracks( result );
            break;
        case QueryMaker::Artist:
        case QueryMaker::AlbumArtist:
            handleArtists( result );
            break;
        case QueryMaker::Album:
            handleAlbums( result );
            break;
        case QueryMaker::Genre:
            handleGenres( result );
            break;
      /*  case QueryMaker::Composer:
            handleComposers( result );
            break;
        case Private::YEAR:
            handleYears( result );
            break;*/
        case QueryMaker::None:
            debug() << "Warning: queryResult with queryType == NONE";

        default:
            break;
        }
    }
    else
    {
        switch( d->queryType )
        {
            case QueryMaker::Custom:
                Q_EMIT newResultReady( QStringList() );
                break;
            case QueryMaker::Track:
                Q_EMIT newTracksReady( Meta::TrackList() );
                break;
            case QueryMaker::Artist:
                Q_EMIT newArtistsReady( Meta::ArtistList() );
                break;
            case QueryMaker::Album:
                Q_EMIT newAlbumsReady( Meta::AlbumList() );
                break;
            case QueryMaker::AlbumArtist:
                Q_EMIT newArtistsReady( Meta::ArtistList() );
                break;
            case QueryMaker::Genre:
                Q_EMIT newGenresReady( Meta::GenreList() );
                break;
            case QueryMaker::Composer:
                Q_EMIT newComposersReady( Meta::ComposerList() );
                break;
            case QueryMaker::Year:
                Q_EMIT newYearsReady( Meta::YearList() );
                break;
            case QueryMaker::None:
                debug() << "Warning: queryResult with queryType == NONE";
            default:
                break;
        }
    }

    //queryDone will be emitted in done(Job*)
}

bool
ServiceSqlQueryMaker::isValidValue( qint64 value )
{
    return value == Meta::valTitle ||
           value == Meta::valArtist ||
           value == Meta::valAlbum ||
           value == Meta::valGenre;
}

QString
ServiceSqlQueryMaker::nameForValue( qint64 value )
{
    QString prefix = m_metaFactory->tablePrefix();

    switch( value )
    {
        case Meta::valTitle:
            d->linkedTables |= Private::TRACKS_TABLE;
            return prefix + QStringLiteral("_tracks.name");
        case Meta::valArtist:
            d->linkedTables |= Private::ARTISTS_TABLE;
            return prefix + QStringLiteral("_artists.name");
        case Meta::valAlbum:
            d->linkedTables |= Private::ALBUMS_TABLE;
            return prefix + QStringLiteral("_albums.name");
        case Meta::valGenre:
            d->queryFrom = prefix + QStringLiteral("_tracks");
            d->linkedTables |= Private::ALBUMS_TABLE;
            d->linkedTables |= Private::GENRE_TABLE;
            return prefix + QStringLiteral("_genre.name");
        default:
            debug() << "ERROR: unknown value in ServiceSqlQueryMaker::nameForValue( qint64 ): value=" << QString::number( value );
            return QString();
    }
}

void
ServiceSqlQueryMaker::handleTracks( const QStringList &result )
{
    Meta::TrackList tracks;
    //SqlRegistry* reg = m_collection->registry();
    int rowCount = ( m_metaFactory->getTrackSqlRowCount() +
                   m_metaFactory->getAlbumSqlRowCount() +
                   m_metaFactory->getArtistSqlRowCount() +
                   m_metaFactory->getGenreSqlRowCount() );

    int resultRows = result.count() / rowCount;

    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*rowCount, rowCount );

        Meta::TrackPtr trackptr =  m_registry->getTrack( row );
        tracks.append( trackptr );
    }

    Q_EMIT newTracksReady( tracks );
}

void
ServiceSqlQueryMaker::handleArtists( const QStringList &result )
{
    Meta::ArtistList artists;
   // SqlRegistry* reg = m_collection->registry();
    int rowCount = m_metaFactory->getArtistSqlRowCount();
    int resultRows = result.size() / rowCount;
    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*rowCount, rowCount );
        artists.append( m_registry->getArtist( row ) );
    }
    Q_EMIT newArtistsReady( artists );
}

void
ServiceSqlQueryMaker::handleAlbums( const QStringList &result )
{
    Meta::AlbumList albums;
    int rowCount = m_metaFactory->getAlbumSqlRowCount() +  m_metaFactory->getArtistSqlRowCount();
    int resultRows = result.size() / rowCount;

    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*rowCount, rowCount );
        albums.append( m_registry->getAlbum( row ) );
    }
    Q_EMIT newAlbumsReady( albums );
}

void
ServiceSqlQueryMaker::handleGenres( const QStringList &result )
{
    Meta::GenreList genres;

    int rowCount = m_metaFactory->getGenreSqlRowCount();
    int resultRows = result.size() / rowCount;
    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*rowCount, rowCount );
        genres.append( m_registry->getGenre( row ) );
    }
    Q_EMIT newGenresReady( genres );
}

/*void
ServiceSqlQueryMaker::handleComposers( const QStringList &result )
{
    Meta::ComposerList composers;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        composers.append( reg->getComposer( name, id.toInt() ) );
    }
    Q_EMIT newResultReady( composers );
}

void
ServiceSqlQueryMaker::handleYears( const QStringList &result )
{
    Meta::YearList years;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        years.append( reg->getYear( name, id.toInt() ) );
    }
    Q_EMIT newResultReady( years );
}*/

QString
ServiceSqlQueryMaker::escape( const QString &text ) const //krazy2:exclude=constref
{
    auto storage = StorageManager::instance()->sqlStorage();
    if( storage )
        return storage->escape( text );
    else
        return QString();
}

QString
ServiceSqlQueryMaker::likeCondition( const QString &text, bool anyBegin, bool anyEnd ) const
{
    if( anyBegin || anyEnd )
    {
        QString escaped = text;
        escaped = escape( escaped );
        //see comments in SqlQueryMaker::likeCondition
        escaped.replace( QLatin1Char('%'), QLatin1String("/%") ).replace( QLatin1Char('_'), QLatin1String("/_") );

        QString ret = QStringLiteral(" LIKE ");

        ret += QLatin1Char('\'');
        if ( anyBegin )
            ret += QLatin1Char('%');
        ret += escaped;
        if ( anyEnd )
            ret += QLatin1Char('%');
        ret += QLatin1Char('\'');

        //Case insensitive collation for queries
        ret += QLatin1String(" COLLATE utf8_unicode_ci ");

        //Use / as the escape character
        ret += QLatin1String(" ESCAPE '/' ");

        return ret;
    }
    else
    {
        return QStringLiteral( " = '%1' " ).arg( escape( text ) );
    }
}

QueryMaker*
ServiceSqlQueryMaker::beginAnd()
{
    d->queryFilter += andOr();
    d->queryFilter += QLatin1String(" ( 1 ");
    d->andStack.push( true );
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::beginOr()
{
    d->queryFilter += andOr();
    d->queryFilter += QLatin1String(" ( 0 ");
    d->andStack.push( false );
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::endAndOr()
{
    d->queryFilter += QLatin1Char(')');
    d->andStack.pop();
    return this;
}

QString
ServiceSqlQueryMaker::andOr() const
{
    return d->andStack.top() ? QStringLiteral(" AND ") : QStringLiteral(" OR ");
}

QueryMaker *
ServiceSqlQueryMaker::setAlbumQueryMode(AlbumQueryMode mode)
{
    d->albumMode = mode;
    return this;
}

#include "ServiceSqlQueryMaker.moc"
