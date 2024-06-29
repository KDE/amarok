/****************************************************************************************
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

#define DEBUG_PREFIX "ScriptableServiceQueryMaker"

#include "ScriptableServiceQueryMaker.h"

#include "core/meta/support/MetaConstants.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/MemoryMatcher.h"
#include "scripting/scriptmanager/ScriptManager.h"
#include "services/scriptable/ScriptableServiceMeta.h"

#include <QTimer>

using namespace Collections;

struct ScriptableServiceQueryMaker::Private {
    //don't change the order of items in this enum
    enum QueryType { TRACK=1, ALBUM=2, ARTIST=3, GENRE=4, NONE=5 };
    QueryType type;
    QueryType closestParent;
    int maxsize;
    QString callbackString;
    int parentId;
    AlbumQueryMode albumMode;
    QString filter;
    QString lastFilter;
};

ScriptableServiceQueryMaker::ScriptableServiceQueryMaker( ScriptableServiceCollection * collection, const QString &name )
    : DynamicServiceQueryMaker()
    , d( new Private )
    , m_convertToMultiTracks( false )
{
    setParent( collection );
    m_collection = collection;
    m_name = name;

    connect( collection, &Collections::ScriptableServiceCollection::updateComplete,
             this, &ScriptableServiceQueryMaker::slotScriptComplete );

    d->type = Private::NONE;
    d->closestParent = Private::NONE;
    d->maxsize = -1;
    d->parentId = -1;
    d->albumMode = AllAlbums;
}

ScriptableServiceQueryMaker::~ScriptableServiceQueryMaker()
{
    delete d;
}


void ScriptableServiceQueryMaker::run()
{
    if ( d->albumMode == OnlyCompilations )
        return;

    if ( d->type == Private::NONE )
        //TODO error handling
        return;

    if ( d->callbackString.isEmpty() )
        d->callbackString = QStringLiteral("none");


    if ( d->type == Private::GENRE ) {
        if ( ( m_collection->levels() == 4 ) && (  m_collection->lastFilter() != d->filter ) )
        {
            m_collection->clear();
        }
        QTimer::singleShot( 0, this, &ScriptableServiceQueryMaker::fetchGenre );
    }
    else if ( d->type == Private::ARTIST )
    {
        if ( ( m_collection->levels() == 3 ) && (  m_collection->lastFilter() != d->filter ) )
        {
            m_collection->clear();
        }
        QTimer::singleShot( 0, this, &ScriptableServiceQueryMaker::fetchArtists );
    }
    else if ( d->type == Private::ALBUM )
    {
        if ( ( m_collection->levels() == 2 ) && (  m_collection->lastFilter() != d->filter ) )
        {
            m_collection->clear();
        }
        QTimer::singleShot( 0, this, &ScriptableServiceQueryMaker::fetchAlbums );
    }
    else if ( d->type == Private::TRACK )
    {
        if ( ( m_collection->levels() == 1 ) && (  m_collection->lastFilter() != d->filter ) )
        {
            m_collection->clear();
        }
        QTimer::singleShot( 0, this, &ScriptableServiceQueryMaker::fetchTracks );
    }

}

void ScriptableServiceQueryMaker::abortQuery()
{
}

QueryMaker * ScriptableServiceQueryMaker::setQueryType( QueryType type )
{
    switch( type ) {
    case QueryMaker::Artist:
    case QueryMaker::AlbumArtist:
        d->type = Private::ARTIST;
        return this;

    case QueryMaker::Album:
        d->type = Private::ALBUM;
        return this;

    case QueryMaker::Track:
        d->type = Private::TRACK;
        return this;

    case QueryMaker::Genre:
        d->type = Private::GENRE;
        return this;

    case QueryMaker::Composer:
    case QueryMaker::Year:
    case QueryMaker::Custom:
    case QueryMaker::Label:
    case QueryMaker::None:
        //TODO: Implement.
        return this;
    }

    return this;
}

QueryMaker * ScriptableServiceQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
    if ( d->closestParent > Private::GENRE )
    {
        d->closestParent = Private::GENRE;
        const Meta::ScriptableServiceGenre * scriptableGenre = static_cast< const Meta::ScriptableServiceGenre * >( genre.data() );
        d->callbackString = scriptableGenre->callbackString();
        d->parentId = scriptableGenre->id();
    }
    return this;
}

QueryMaker * ScriptableServiceQueryMaker::addMatch( const Meta::ArtistPtr & artist, QueryMaker::ArtistMatchBehaviour behaviour )
{
    Q_UNUSED( behaviour );
    const Meta::ScriptableServiceArtist *scriptableArtist = dynamic_cast<const Meta::ScriptableServiceArtist *>( artist.data() );
    if ( scriptableArtist && d->closestParent > Private::ARTIST )
    {
        d->closestParent = Private::ARTIST;
        d->callbackString = scriptableArtist->callbackString();
        d->parentId = scriptableArtist->id();
    }
    return this;
}

QueryMaker * ScriptableServiceQueryMaker::addMatch( const Meta::AlbumPtr & album )
{
    if ( d->closestParent > Private::ALBUM )
    {
        d->closestParent = Private::ALBUM;
        debug() << "Here!";
        const Meta::ScriptableServiceAlbum * scriptableAlbum = static_cast< const Meta::ScriptableServiceAlbum * >( album.data() );
        d->callbackString = scriptableAlbum->callbackString();
        d->parentId = scriptableAlbum->id();
    }
    return this;
}

void
ScriptableServiceQueryMaker::setConvertToMultiTracks( bool convert )
{
    m_convertToMultiTracks = convert;
}

void ScriptableServiceQueryMaker::handleResult( const Meta::GenreList & genres )
{
    if ( d->maxsize >= 0 && genres.count() > d->maxsize )
        Q_EMIT newGenresReady( genres.mid( 0, d->maxsize ) );
    else
        Q_EMIT newGenresReady( genres );
}

void ScriptableServiceQueryMaker::handleResult( const Meta::AlbumList & albums )
{
    if ( d->maxsize >= 0 && albums.count() > d->maxsize )
        Q_EMIT newAlbumsReady( albums.mid( 0, d->maxsize ) );
    else
        Q_EMIT newAlbumsReady( albums );
}

void ScriptableServiceQueryMaker::handleResult( const Meta::ArtistList & artists )
{
    if ( d->maxsize >= 0 && artists.count() > d->maxsize )
        Q_EMIT newArtistsReady( artists.mid( 0, d->maxsize ) );
    else
        Q_EMIT newArtistsReady( artists );
}

void ScriptableServiceQueryMaker::handleResult( const Meta::TrackList &tracks )
{
    Meta::TrackList ret;
    if( m_convertToMultiTracks )
    {
        for( const Meta::TrackPtr &track : tracks )
        {
            using namespace Meta;
            const ScriptableServiceTrack *serviceTrack =
                    dynamic_cast<const ScriptableServiceTrack *>( track.data() );
            if( !serviceTrack )
            {
                error() << "failed to convert generic track" << track.data() << "to ScriptableServiceTrack";
                continue;
            }
            ret << serviceTrack->playableTrack();
        }
    }
    else
        ret = tracks;

    if ( d->maxsize >= 0 && ret.count() > d->maxsize )
        Q_EMIT newTracksReady( ret.mid( 0, d->maxsize ) );
    else
        Q_EMIT newTracksReady( ret );
}

void ScriptableServiceQueryMaker::fetchGenre()
{
    DEBUG_BLOCK
    Meta::GenreList genre  = m_collection->genreMap().values();

    if ( genre.count() > 0 )
    {
        handleResult( genre );
        Q_EMIT( queryDone() );
    }
    else
        //this is where we call the script to get it to add more stuff!
        ScriptManager::instance()->ServiceScriptPopulate( m_name, 3, d->parentId, d->callbackString, d->filter );
}

void ScriptableServiceQueryMaker::fetchArtists()
{
    DEBUG_BLOCK
    Meta::ArtistList artists;

    if ( d->parentId != -1 )
    {
        Meta::GenrePtr genrePtr =  m_collection->genreById( d->parentId );
        Meta::ScriptableServiceGenre * scGenre = dynamic_cast<Meta::ScriptableServiceGenre *> ( genrePtr.data() );
        if ( scGenre )
        {
            Meta::ArtistList allArtists = m_collection->artistMap().values();

            for( Meta::ArtistPtr artistPtr : allArtists )
            {
                Meta::ScriptableServiceArtist *scArtist = dynamic_cast<Meta::ScriptableServiceArtist *> ( artistPtr.data() );
                if ( scArtist && scArtist->genreId() == d->parentId )
                    artists.append( artistPtr );
            }
        }
    }

    if ( artists.count() > 0 )
    {
        handleResult( artists );
        Q_EMIT( queryDone() );
    }
    else
        //this is where we call the script to get it to add more stuff!
        ScriptManager::instance()->ServiceScriptPopulate( m_name, 2, d->parentId, d->callbackString, d->filter );
}

void ScriptableServiceQueryMaker::fetchAlbums()
{
    DEBUG_BLOCK
    debug() << "parent id: " << d->parentId;

    if ( d->albumMode == OnlyCompilations)
        return;

    Meta::AlbumList albums;

    if ( d->parentId != -1 )
    {
        albums = matchAlbums( m_collection, m_collection->artistById( d->parentId ) );
    }
    else
        albums = m_collection->albumMap().values();
    if ( albums.count() > 0 )
    {
        handleResult( albums );
        Q_EMIT( queryDone() );
    }
    else
        //this is where we call the script to get it to add more stuff!
        ScriptManager::instance()->ServiceScriptPopulate( m_name, 1, d->parentId, d->callbackString, d->filter );
}

void ScriptableServiceQueryMaker::fetchTracks()
{
    DEBUG_BLOCK

    Meta::TrackList tracks;

    debug() << "parent id: " << d->parentId;

    Meta::AlbumPtr album;
    if ( d->parentId != -1 && ( album = m_collection->albumById( d->parentId ) ) )
    {
        AlbumMatcher albumMatcher( album );
        tracks = albumMatcher.match( m_collection->trackMap().values() );
    }
    else
        tracks = m_collection->trackMap().values();

    if ( tracks.count() > 0 ) {
        handleResult( tracks );
        Q_EMIT( queryDone() );
    }
    else
        //this is where we call the script to get it to add more stuff!
    {
        debug() << "i am sending signals!";
        ScriptManager::instance()->ServiceScriptPopulate( m_name, 0, d->parentId, d->callbackString, d->filter );
    }
}

void ScriptableServiceQueryMaker::slotScriptComplete()
{
    DEBUG_BLOCK

    if ( d->type == Private::GENRE )
    {
        Meta::GenreList genre = m_collection->genreMap().values();
        handleResult( genre );
    }
    else if ( d->type == Private::ARTIST )
    {
        Meta::ArtistList artists;
        if ( d->parentId != -1 )
        {
            Meta::GenrePtr genrePtr =  m_collection->genreById( d->parentId );
            Meta::ScriptableServiceGenre * scGenre = dynamic_cast<Meta::ScriptableServiceGenre *> ( genrePtr.data() );
            if ( scGenre )
            {
                Meta::ArtistList allArtists = m_collection->artistMap().values();

                for( Meta::ArtistPtr artistPtr : allArtists )
                {
                    Meta::ScriptableServiceArtist *scArtist = dynamic_cast<Meta::ScriptableServiceArtist *> ( artistPtr.data() );

                    if ( scArtist && scArtist->genreId() == d->parentId )
                        artists.append( artistPtr );
                }
            }
        }
        else
            artists = m_collection->artistMap().values();
        debug() << "there are " << artists.count() << " artists";
        handleResult( artists );
    }
    else if ( d->type == Private::ALBUM )
    {
       Meta::AlbumList albums;
       if ( d->parentId != -1 )
       {
            albums = matchAlbums( m_collection, m_collection->artistById( d->parentId ) );
       }
       else
            albums = m_collection->albumMap().values();

       debug() << "there are " << albums.count() << " albums";
       handleResult( albums );
    }
    else if ( d->type == Private::TRACK )
    {
        Meta::TrackList tracks;
        if ( d->parentId != -1 )
        {
            Meta::AlbumPtr album = m_collection->albumById( d->parentId );
            if( album )
            {
                AlbumMatcher albumMatcher( album );
                tracks = albumMatcher.match( m_collection->trackMap().values() );
            }
        }
        else
                tracks = m_collection->trackMap().values();
        debug() << "there are " << tracks.count() << " tracks";
        handleResult( tracks );
    }
    Q_EMIT( queryDone() );
}

QueryMaker * ScriptableServiceQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
    d->albumMode = mode;
    return this;
}

QueryMaker * ScriptableServiceQueryMaker::addFilter( qint64 value, const QString & filter, bool matchBegin, bool matchEnd )
{
    Q_UNUSED( matchBegin )
    Q_UNUSED( matchEnd )

    DEBUG_BLOCK

    if ( value == Meta::valTitle )
    {
        //I am sure there is a really good reason to add this space, as nothing works if it is removed, but WHY?!?
        d->filter += filter + QLatin1Char(' ');
        d->filter = d->filter.replace( QLatin1Char(' '), QLatin1String("%20") );
    }

    int level = 0;

    if (  d->type == Private::GENRE )
        level = 4;
    if (  d->type == Private::ARTIST )
        level = 3;
    else if (  d->type == Private::ALBUM )
        level = 2;
    else if (  d->type == Private::TRACK )
        level = 1;

    // should only clear all if we are querying for a top level item
    if ( m_collection->levels() == level )
    {
        //we need to clear everything as we have no idea what the scripts wants to do...
        //TODO: with KSharedPointers in use, does this leak!?

        debug() << "clear all!!!!!!!!!!!!!!";
        m_collection->clear();
    }

    d->lastFilter = d->filter;
    m_collection->setLastFilter( d->filter );
    return this;

}

