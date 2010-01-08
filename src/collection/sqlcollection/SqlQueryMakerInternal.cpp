/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "SqlQueryMakerInternal.h"

#include "collection/SqlStorage.h"
#include "Debug.h"
#include "SqlCollection.h"
#include "SqlMeta.h"
#include "SqlRegistry.h"

#include <QStringList>

SqlQueryMakerInternal::SqlQueryMakerInternal( SqlCollection *collection )
    : QObject()
    , m_collection( collection )
    , m_resultAsDataPtrs( false )
    , m_queryType( QueryMaker::None )
{
}

SqlQueryMakerInternal::~ SqlQueryMakerInternal()
{
    disconnect();
}

void
SqlQueryMakerInternal::run()
{
    Q_ASSERT( !m_query.isEmpty() );
    if( m_collection )
    {
        QStringList result = m_collection->sqlStorage()->query( m_query );
        handleResult( result );
    }
    else
    {
        deleteLater();
    }

}

void
SqlQueryMakerInternal::setQuery( const QString &query )
{
    //qDebug() << query;
    m_query = query;
}

void
SqlQueryMakerInternal::setResultAsDataPtrs( bool value )
{
    m_resultAsDataPtrs = value;
}

void
SqlQueryMakerInternal::setQueryType( QueryMaker::QueryType type )
{
    m_queryType = type;
}

void
SqlQueryMakerInternal::handleResult( const QStringList &result )
{
    if( !result.isEmpty() )
    {
        switch( m_queryType ) {
        case QueryMaker::Custom:
            emit newResultReady( m_collection->collectionId(), result );
            break;
        case QueryMaker::Track:
            handleTracks( result );
            break;
        case QueryMaker::Artist:
            handleArtists( result );
            break;
        case QueryMaker::Album:
            handleAlbums( result );
            break;
        case QueryMaker::Genre:
            handleGenres( result );
            break;
        case QueryMaker::Composer:
            handleComposers( result );
            break;
        case QueryMaker::Year:
            handleYears( result );
            break;

        case QueryMaker::None:
            debug() << "Warning: queryResult with queryType == NONE";
        }
    }
    else
    {
        if( m_resultAsDataPtrs )
        {
            emit newResultReady( m_collection->collectionId(), Meta::DataList() );
        }
        else
        {
            switch( m_queryType ) {
                case QueryMaker::Custom:
                    emit newResultReady( m_collection->collectionId(), QStringList() );
                    break;
                case QueryMaker::Track:
                    emit newResultReady( m_collection->collectionId(), Meta::TrackList() );
                    break;
                case QueryMaker::Artist:
                    emit newResultReady( m_collection->collectionId(), Meta::ArtistList() );
                    break;
                case QueryMaker::Album:
                    emit newResultReady( m_collection->collectionId(), Meta::AlbumList() );
                    break;
                case QueryMaker::Genre:
                    emit newResultReady( m_collection->collectionId(), Meta::GenreList() );
                    break;
                case QueryMaker::Composer:
                    emit newResultReady( m_collection->collectionId(), Meta::ComposerList() );
                    break;
                case QueryMaker::Year:
                    emit newResultReady( m_collection->collectionId(), Meta::YearList() );
                    break;

            case QueryMaker::None:
                debug() << "Warning: queryResult with queryType == NONE";
            }
        }
    }

    //queryDone will be emitted in done(Job*)
}

// What's worse, a bunch of almost identical repeated code, or a not so obvious macro? :-)
// The macro below will emit the proper result signal. If m_resultAsDataPtrs is true,
// it'll emit the signal that takes a list of DataPtrs. Otherwise, it'll call the
// signal that takes the list of the specific class.
// If qm is used blocking it only stores the result ptrs into data as DataPtrs

#define emitOrStoreProperResult( PointerType, list ) { \
            if ( m_resultAsDataPtrs ) { \
                Meta::DataList data; \
                foreach( PointerType p, list ) { \
                    data << Meta::DataPtr::staticCast( p ); \
                } \
                emit newResultReady( m_collection->collectionId(), data ); \
            } \
            else { \
                emit newResultReady( m_collection->collectionId(), list ); \
            } \
       }

void
SqlQueryMakerInternal::handleTracks( const QStringList &result )
{
    Meta::TrackList tracks;
    SqlRegistry* reg = m_collection->registry();
    //there are 29 columns in the result set as generated by startTrackQuery()
    int returnCount = Meta::SqlTrack::getTrackReturnValueCount();
    int resultRows = result.size() / returnCount;
    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*returnCount, returnCount );
        tracks.append( reg->getTrack( row ) );
    }
    emitOrStoreProperResult( Meta::TrackPtr, tracks );
}

void
SqlQueryMakerInternal::handleArtists( const QStringList &result )
{
    Meta::ArtistList artists;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        artists.append( reg->getArtist( name, id.toInt() ) );
    }
    emitOrStoreProperResult( Meta::ArtistPtr, artists );
}

void
SqlQueryMakerInternal::handleAlbums( const QStringList &result )
{
    Meta::AlbumList albums;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        QString artist = iter.next();
        albums.append( reg->getAlbum( name, id.toInt(), artist.toInt() ) );
    }
    emitOrStoreProperResult( Meta::AlbumPtr, albums );
}

void
SqlQueryMakerInternal::handleGenres( const QStringList &result )
{
    Meta::GenreList genres;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        genres.append( reg->getGenre( name, id.toInt() ) );
    }
    emitOrStoreProperResult( Meta::GenrePtr, genres );
}

void
SqlQueryMakerInternal::handleComposers( const QStringList &result )
{
    Meta::ComposerList composers;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        composers.append( reg->getComposer( name, id.toInt() ) );
    }
    emitOrStoreProperResult( Meta::ComposerPtr, composers );
}

void
SqlQueryMakerInternal::handleYears( const QStringList &result )
{
    Meta::YearList years;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        years.append( reg->getYear( name, id.toInt() ) );
    }
    emitOrStoreProperResult( Meta::YearPtr, years );
}

#include "SqlQueryMakerInternal.moc"
