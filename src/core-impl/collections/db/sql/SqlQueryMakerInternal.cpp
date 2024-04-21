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

#include <core/storage/SqlStorage.h>
#include "core/support/Debug.h"
#include "SqlCollection.h"
#include "SqlMeta.h"
#include "SqlRegistry.h"

#include <QStringList>

using namespace Collections;

SqlQueryMakerInternal::SqlQueryMakerInternal( SqlCollection *collection )
    : QObject()
    , m_collection( collection )
    , m_queryType( QueryMaker::None )
    , m_aborted( false )
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
    if( !m_collection.isNull() )
    {
        if( !m_aborted )
        {
            QStringList result = m_collection->sqlStorage()->query( m_query );
            if( !m_aborted ) // Abort might be called while waiting for query to finish
                handleResult( result );
        }
    }
    else
    {
        deleteLater();
    }

}

void
SqlQueryMakerInternal::requestAbort()
{
    m_aborted = true;
}

void
SqlQueryMakerInternal::setQuery( const QString &query )
{
    //qDebug() << query;
    m_query = query;
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
            Q_EMIT newResultReady( result );
            break;
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
        case QueryMaker::Composer:
            handleComposers( result );
            break;
        case QueryMaker::Year:
            handleYears( result );
            break;
        case QueryMaker::Label:
            handleLabels( result );
            break;

        case QueryMaker::None:
            debug() << "Warning: queryResult with queryType == NONE";
        }
    }
    else
    {
        switch( m_queryType ) {
            case QueryMaker::Custom:
                Q_EMIT newResultReady( QStringList() );
                break;
            case QueryMaker::Track:
                Q_EMIT newTracksReady( Meta::TrackList() );
                break;
            case QueryMaker::Artist:
            case QueryMaker::AlbumArtist:
                Q_EMIT newArtistsReady( Meta::ArtistList() );
                break;
            case QueryMaker::Album:
                Q_EMIT newAlbumsReady( Meta::AlbumList() );
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
            case QueryMaker::Label:
                Q_EMIT newLabelsReady( Meta::LabelList() );
                break;

            case QueryMaker::None:
                debug() << "Warning: queryResult with queryType == NONE";
        }
    }

    //queryDone will be emitted in done(Job*)
}

void
SqlQueryMakerInternal::handleTracks( const QStringList &result )
{
    Meta::TrackList tracks;
    SqlRegistry* reg = m_collection->registry();
    int returnCount = Meta::SqlTrack::getTrackReturnValueCount();
    int resultRows = result.size() / returnCount;
    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*returnCount, returnCount );
        tracks.append( reg->getTrack( row[Meta::SqlTrack::returnIndex_trackId].toInt(), row ) );
    }
    Q_EMIT newTracksReady( tracks );
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
        if( id.toInt() > 0 )
            artists.append( reg->getArtist( id.toInt(), name ) );
    }
    Q_EMIT newArtistsReady( artists );
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
        albums.append( reg->getAlbum( id.toInt(), name, artist.toInt() ) );
    }
    Q_EMIT newAlbumsReady( albums );
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
        genres.append( reg->getGenre( id.toInt(), name ) );
    }
    Q_EMIT newGenresReady( genres );
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
        composers.append( reg->getComposer( id.toInt(), name ) );
    }
    Q_EMIT newComposersReady( composers );
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
        years.append( reg->getYear( id.toInt(), name.toInt() ) );
    }
    Q_EMIT newYearsReady( years );
}

void
SqlQueryMakerInternal::handleLabels( const QStringList &result )
{
    Meta::LabelList labels;
    SqlRegistry *reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString label = iter.next();
        QString id = iter.next();
        labels.append( reg->getLabel( id.toInt(), label ) );
    }

    Q_EMIT newLabelsReady( labels );
}

