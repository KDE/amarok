/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "MemoryQueryMakerInternal.h"

#include "MemoryCollection.h"
#include "MemoryCustomValue.h"
#include "MemoryFilter.h"
#include "MemoryMatcher.h"
#include "MemoryQueryMakerHelper.h"

#include "core/meta/Meta.h"

#include <QSharedPointer>

#include <KSortableList>

namespace Collections {

MemoryQueryMakerInternal::MemoryQueryMakerInternal( const QWeakPointer<MemoryCollection> &collection )
    : QObject()
    , m_collection( collection )
    , m_matchers( nullptr )
    , m_filters( nullptr )
    , m_maxSize( 0 )
    , m_type( QueryMaker::None )
    , m_albumQueryMode( QueryMaker::AllAlbums )
    , m_orderDescending( false )
    , m_orderByNumberField( false )
    , m_orderByField( 0 )
{

}

MemoryQueryMakerInternal::~MemoryQueryMakerInternal()
{
    delete m_filters;
    delete m_matchers;
    qDeleteAll( m_returnFunctions );
    qDeleteAll( m_returnValues );
}

void
MemoryQueryMakerInternal::runQuery()
{
    QSharedPointer<MemoryCollection> coll = m_collection.toStrongRef();
    if( coll )
        coll->acquireReadLock();
    //naive implementation, fix this
    if ( m_matchers )
    {
        Meta::TrackList result = coll ? m_matchers->match( coll.data() ) : Meta::TrackList();
        if ( m_filters )
        {
            Meta::TrackList filtered;
            for( const Meta::TrackPtr &track : result )
            {
                if( m_filters->filterMatches( track ) )
                    filtered.append( track );
            }
            handleResult( filtered );
        }
        else
            handleResult( result );
    }
    else if ( m_filters )
    {
        Meta::TrackList tracks = coll ? coll->trackMap().values() : Meta::TrackList();
        Meta::TrackList filtered;
        for( const Meta::TrackPtr &track : tracks )
        {
            if ( m_filters->filterMatches( track ) )
                filtered.append( track );
        }
        handleResult( filtered );
    }
    else
        handleResult();
    if( coll )
        coll->releaseLock();
}

template<typename T>
static inline QList<T> reverse(const QList<T> &l)
{
    QList<T> ret;
    for (int i=l.size() - 1; i>=0; --i)
        ret.append(l.at(i));
    return ret;
}

void
MemoryQueryMakerInternal::handleResult()
{
    QSharedPointer<MemoryCollection> coll = m_collection.toStrongRef();
    //this gets called when we want to return all values for the given query type
    switch( m_type )
    {
        case QueryMaker::Custom :
        {
            QStringList result;
            Meta::TrackList tmpTracks = coll ? coll->trackMap().values() : Meta::TrackList();
            Meta::TrackList tracks;
            for( const Meta::TrackPtr &track : tmpTracks )
            {
                if( ( m_albumQueryMode == QueryMaker::AllAlbums
                    || ( m_albumQueryMode == QueryMaker::OnlyCompilations && track->album()->isCompilation() )
                    || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !track->album()->isCompilation()) ) &&
                    ( m_labelQueryMode == QueryMaker::NoConstraint
                      || ( m_labelQueryMode == QueryMaker::OnlyWithLabels && track->labels().count() > 0 )
                      || ( m_labelQueryMode == QueryMaker::OnlyWithoutLabels && track->labels().isEmpty()) ) )
                {
                    tracks.append( track );
                }
            }

            if( !m_returnFunctions.empty() )
            {
                //no sorting necessary
                for( CustomReturnFunction *function : m_returnFunctions )
                {
                    result.append( function->value( tracks ) );
                }
            }
            else if( !m_returnValues.empty() )
            {
                if( m_orderByField )
                {
                    if( m_orderByNumberField )
                        tracks = MemoryQueryMakerHelper::orderListByNumber( tracks, m_orderByField, m_orderDescending );
                    else
                        tracks = MemoryQueryMakerHelper::orderListByString( tracks, m_orderByField, m_orderDescending );
                }

                int count = 0;
                for( const Meta::TrackPtr &track : tracks )
                {
                    if ( m_maxSize >= 0 && count == m_maxSize )
                        break;

                    for( CustomReturnValue *value : m_returnValues )
                    {
                        result.append( value->value( track ) );
                    }
                    count++;
                }
            }
            Q_EMIT newResultReady( result );
            break;
        }
        case QueryMaker::Track :
        {
            Meta::TrackList tracks;

            Meta::TrackList tmpTracks = coll ? coll->trackMap().values() : Meta::TrackList();
            for( Meta::TrackPtr track : tmpTracks )
            {
                Meta::AlbumPtr album = track->album();
                if( ( m_albumQueryMode == QueryMaker::AllAlbums
                    || ( m_albumQueryMode == QueryMaker::OnlyCompilations && (!album || album->isCompilation()) )
                    || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && (album && !album->isCompilation()) ) ) &&
                    ( m_labelQueryMode == QueryMaker::NoConstraint
                      || ( m_labelQueryMode == QueryMaker::OnlyWithLabels && track->labels().count() > 0 )
                      || ( m_labelQueryMode == QueryMaker::OnlyWithoutLabels && track->labels().isEmpty()) ) )
                {
                    tracks.append( track );
                }
            }

            if( m_orderByField )
            {
                if( m_orderByNumberField )
                    tracks = MemoryQueryMakerHelper::orderListByNumber( tracks, m_orderByField, m_orderDescending );
                else
                    tracks = MemoryQueryMakerHelper::orderListByString( tracks, m_orderByField, m_orderDescending );
            }

            Q_EMIT newTracksReady( tracks );
            break;
        }
        case QueryMaker::Album :
        {
            Meta::AlbumList albums;
            Meta::AlbumList tmp = coll ? coll->albumMap().values() : Meta::AlbumList();
            for( Meta::AlbumPtr album : tmp )
            {
                Meta::TrackList tracks = album->tracks();
                for( Meta::TrackPtr track : tracks )
                {
                    Meta::AlbumPtr album = track->album();
                    if( ( m_albumQueryMode == QueryMaker::AllAlbums
                        || ( m_albumQueryMode == QueryMaker::OnlyCompilations && (!album || album->isCompilation()) )
                        || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && (album && !album->isCompilation()) ) ) &&
                        ( m_labelQueryMode == QueryMaker::NoConstraint
                          || ( m_labelQueryMode == QueryMaker::OnlyWithLabels && track->labels().count() > 0 )
                          || ( m_labelQueryMode == QueryMaker::OnlyWithoutLabels && track->labels().isEmpty()) ) )
                    {
                        albums.append( album );
                        break;
                    }
                }
            }

            albums = MemoryQueryMakerHelper::orderListByName<Meta::AlbumPtr>( albums, m_orderDescending );

            Q_EMIT newAlbumsReady( albums );
            break;
        }
        case QueryMaker::Artist :
        {
            Meta::ArtistList artists;
            Meta::ArtistList tmp = coll ? coll->artistMap().values() : Meta::ArtistList();
            for( Meta::ArtistPtr artist : tmp )
            {
                Meta::TrackList tracks = artist->tracks();
                for( Meta::TrackPtr track : tracks )
                {
                    if( ( m_albumQueryMode == QueryMaker::AllAlbums
                        || ( m_albumQueryMode == QueryMaker::OnlyCompilations && track->album()->isCompilation() )
                        || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !track->album()->isCompilation()) ) &&
                        ( m_labelQueryMode == QueryMaker::NoConstraint
                          || ( m_labelQueryMode == QueryMaker::OnlyWithLabels && track->labels().count() > 0 )
                          || ( m_labelQueryMode == QueryMaker::OnlyWithoutLabels && track->labels().isEmpty()) ) )
                    {
                        artists.append( artist );
                        break;
                    }
                }
            }
            artists = MemoryQueryMakerHelper::orderListByName<Meta::ArtistPtr>( artists, m_orderDescending );
            Q_EMIT newArtistsReady( artists );
            break;
        }
        case QueryMaker::AlbumArtist :
        {
            Meta::ArtistList artists;
            Meta::AlbumList tmp = coll ? coll->albumMap().values() : Meta::AlbumList();
            for( Meta::AlbumPtr album : tmp )
            {
                if( !album->hasAlbumArtist() )
                    continue;

                Meta::TrackList tracks = album->tracks();
                for( Meta::TrackPtr track : tracks )
                {
                    if( ( m_albumQueryMode == QueryMaker::AllAlbums
                        || ( m_albumQueryMode == QueryMaker::OnlyCompilations && album->isCompilation() )
                        || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !album->isCompilation()) ) &&
                        ( m_labelQueryMode == QueryMaker::NoConstraint
                          || ( m_labelQueryMode == QueryMaker::OnlyWithLabels && track->labels().count() > 0 )
                          || ( m_labelQueryMode == QueryMaker::OnlyWithoutLabels && track->labels().isEmpty()) ) )
                    {
                        artists.append( album->albumArtist() );
                        break;
                    }
                }
            }
            artists = MemoryQueryMakerHelper::orderListByName<Meta::ArtistPtr>( artists, m_orderDescending );
            Q_EMIT newArtistsReady( artists );
            break;
        }
        case QueryMaker::Composer :
        {
            Meta::ComposerList composers;
            Meta::ComposerList tmp = coll ? coll->composerMap().values() : Meta::ComposerList();
            for( Meta::ComposerPtr composer : tmp )
            {
                Meta::TrackList tracks = composer->tracks();
                for( Meta::TrackPtr track : tracks )
                {
                    if( ( m_albumQueryMode == QueryMaker::AllAlbums
                        || ( m_albumQueryMode == QueryMaker::OnlyCompilations && track->album()->isCompilation() )
                        || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !track->album()->isCompilation()) ) &&
                        ( m_labelQueryMode == QueryMaker::NoConstraint
                          || ( m_labelQueryMode == QueryMaker::OnlyWithLabels && track->labels().count() > 0 )
                          || ( m_labelQueryMode == QueryMaker::OnlyWithoutLabels && track->labels().isEmpty()) ) )
                    {
                        composers.append( composer );
                        break;
                    }
                }
            }
            composers = MemoryQueryMakerHelper::orderListByName<Meta::ComposerPtr>( composers, m_orderDescending );

            Q_EMIT newComposersReady( composers );
            break;
        }
        case QueryMaker::Genre :
        {
            Meta::GenreList genres;
            Meta::GenreList tmp = coll ? coll->genreMap().values() : Meta::GenreList();
            for( Meta::GenrePtr genre : tmp )
            {
                Meta::TrackList tracks = genre->tracks();
                for( Meta::TrackPtr track : tracks )
                {
                    if( ( m_albumQueryMode == QueryMaker::AllAlbums
                        || ( m_albumQueryMode == QueryMaker::OnlyCompilations && track->album()->isCompilation() )
                        || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !track->album()->isCompilation()) ) &&
                        ( m_labelQueryMode == QueryMaker::NoConstraint
                          || ( m_labelQueryMode == QueryMaker::OnlyWithLabels && track->labels().count() > 0 )
                          || ( m_labelQueryMode == QueryMaker::OnlyWithoutLabels && track->labels().isEmpty()) ) )
                    {
                        genres.append( genre );
                        break;
                    }
                }
            }

            genres = MemoryQueryMakerHelper::orderListByName<Meta::GenrePtr>( genres, m_orderDescending );

            Q_EMIT newGenresReady( genres );
            break;
        }
        case QueryMaker::Year :
        {
            Meta::YearList years;
            Meta::YearList tmp = coll ? coll->yearMap().values() : Meta::YearList();
            for( Meta::YearPtr year : tmp )
            {
                Meta::TrackList tracks = year->tracks();
                for( Meta::TrackPtr track : tracks )
                {
                    if( ( m_albumQueryMode == QueryMaker::AllAlbums
                        || ( m_albumQueryMode == QueryMaker::OnlyCompilations && track->album()->isCompilation() )
                        || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !track->album()->isCompilation()) ) &&
                        ( m_labelQueryMode == QueryMaker::NoConstraint
                          || ( m_labelQueryMode == QueryMaker::OnlyWithLabels && track->labels().count() > 0 )
                          || ( m_labelQueryMode == QueryMaker::OnlyWithoutLabels && track->labels().isEmpty()) ) )
                    {
                        years.append( year );
                        break;
                    }
                }
            }

            //this a special case which requires a bit of code duplication
            //years have to be ordered as numbers, but orderListByNumber does not work for Meta::YearPtrs
            if( m_orderByField == Meta::valYear )
            {
                years = MemoryQueryMakerHelper::orderListByYear( years, m_orderDescending );
            }

            Q_EMIT newYearsReady( years );
            break;
        }
        case QueryMaker::Label:
        {
            Meta::LabelList labels;
            Meta::LabelList tmp = coll ? coll->labelMap().values() : Meta::LabelList();
            for( const Meta::LabelPtr &label : tmp )
            {
                Meta::TrackList tracks = coll ? coll->labelToTrackMap().value( label ) : Meta::TrackList();
                for( const Meta::TrackPtr &track : tracks )
                {
                    if( ( m_albumQueryMode == QueryMaker::AllAlbums
                        || ( m_albumQueryMode == QueryMaker::OnlyCompilations && track->album()->isCompilation() )
                        || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !track->album()->isCompilation()) ) &&
                        ( m_labelQueryMode == QueryMaker::NoConstraint
                          || ( m_labelQueryMode == QueryMaker::OnlyWithLabels && track->labels().count() > 0 )
                          || ( m_labelQueryMode == QueryMaker::OnlyWithoutLabels && track->labels().isEmpty()) ) )
                    {
                        labels.append( label );
                        break;
                    }
                }
            }

            labels = MemoryQueryMakerHelper::orderListByName<Meta::LabelPtr>( labels, m_orderDescending );

            Q_EMIT newLabelsReady( labels );
            break;
        }
        case QueryMaker::None :
            //nothing to do
            break;
    }
}

void
MemoryQueryMakerInternal::handleResult( const Meta::TrackList &tmpTracks )
{
    Meta::TrackList tracks;
    for( const Meta::TrackPtr &track : tmpTracks )
    {
        if( ( m_albumQueryMode == QueryMaker::AllAlbums
            || ( m_albumQueryMode == QueryMaker::OnlyCompilations && track->album()->isCompilation() )
            || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !track->album()->isCompilation()) ) &&
            ( m_labelQueryMode == QueryMaker::NoConstraint
              || ( m_labelQueryMode == QueryMaker::OnlyWithLabels && track->labels().count() > 0 )
              || ( m_labelQueryMode == QueryMaker::OnlyWithoutLabels && track->labels().isEmpty()) ) )
        {
            tracks.append( track );
        }
    }

    switch( m_type )
    {
        case QueryMaker::Custom :
        {
            QStringList result;
            if( !m_returnFunctions.empty() )
            {
                //no sorting necessary
                for( const CustomReturnFunction *function : m_returnFunctions )
                {
                    result.append( function->value( tracks ) );
                }
            }
            else if( !m_returnValues.empty() )
            {
                Meta::TrackList resultTracks = tracks;
                if( m_orderByField )
                {
                    if( m_orderByNumberField )
                        resultTracks = MemoryQueryMakerHelper::orderListByNumber( resultTracks, m_orderByField, m_orderDescending );
                    else
                        resultTracks = MemoryQueryMakerHelper::orderListByString( resultTracks, m_orderByField, m_orderDescending );
                }

                int count = 0;
                for( const Meta::TrackPtr &track : resultTracks )
                {
                    if ( m_maxSize >= 0 && count == m_maxSize )
                        break;

                    for( const CustomReturnValue *value : m_returnValues )
                    {
                        result.append( value->value( track ) );
                    }
                    count++;
                }
            }
            Q_EMIT newResultReady( result );
            break;
        }
        case QueryMaker::Track :
        {
            Meta::TrackList newResult;

            if( m_orderByField )
            {
                if( m_orderByNumberField )
                    newResult = MemoryQueryMakerHelper::orderListByNumber( tracks, m_orderByField, m_orderDescending );
                else
                    newResult = MemoryQueryMakerHelper::orderListByString( tracks, m_orderByField, m_orderDescending );
            }
            else
                newResult = tracks;

            Q_EMIT newTracksReady( newResult );
            break;
        }
        case QueryMaker::Album :
        {
            QSet<Meta::AlbumPtr> albumSet;
            for( Meta::TrackPtr track : tracks )
            {
                albumSet.insert( track->album() );
            }
            Meta::AlbumList albumList = albumSet.values();
            albumList = MemoryQueryMakerHelper::orderListByName<Meta::AlbumPtr>( albumList, m_orderDescending );
            Q_EMIT newAlbumsReady( albumList );
            break;
        }
        case QueryMaker::Artist :
        {
            QSet<Meta::ArtistPtr> artistSet;
            for( Meta::TrackPtr track : tracks )
            {
                artistSet.insert( track->artist() );
            }
            Meta::ArtistList list = artistSet.values();
            list = MemoryQueryMakerHelper::orderListByName<Meta::ArtistPtr>( list, m_orderDescending );
            Q_EMIT newArtistsReady( list );
            break;
        }
        case QueryMaker::AlbumArtist :
        {
            QSet<Meta::ArtistPtr> artistSet;
            for( Meta::TrackPtr track : tracks )
            {
                if( !track->album().isNull() && track->album()->hasAlbumArtist() )
                    artistSet.insert( track->album()->albumArtist() );
            }
            Meta::ArtistList list = artistSet.values();
            list = MemoryQueryMakerHelper::orderListByName<Meta::ArtistPtr>( list, m_orderDescending );
            Q_EMIT newArtistsReady( list );
            break;
        }
        case QueryMaker::Genre :
        {
            QSet<Meta::GenrePtr> genreSet;
            for( Meta::TrackPtr track : tracks )
            {
                genreSet.insert( track->genre() );
            }
            Meta::GenreList list = genreSet.values();
            list = MemoryQueryMakerHelper::orderListByName<Meta::GenrePtr>( list, m_orderDescending );
            Q_EMIT newGenresReady( list );
            break;
        }
        case QueryMaker::Composer :
        {
            QSet<Meta::ComposerPtr> composerSet;
            for( Meta::TrackPtr track : tracks )
            {
                composerSet.insert( track->composer() );
            }
            Meta::ComposerList list = composerSet.values();
            list = MemoryQueryMakerHelper::orderListByName<Meta::ComposerPtr>( list, m_orderDescending );
            Q_EMIT newComposersReady( list );
            break;
        }
        case QueryMaker::Year :
        {
            QSet<Meta::YearPtr> yearSet;
            for( Meta::TrackPtr track : tracks )
            {
                yearSet.insert( track->year() );
            }
            Meta::YearList years = yearSet.values();
            if( m_orderByField == Meta::valYear )
            {
                years = MemoryQueryMakerHelper::orderListByYear( years, m_orderDescending );
            }

            Q_EMIT newYearsReady( years );
            break;
        }
        case QueryMaker::Label:
        {
            QSet<Meta::LabelPtr> labelSet;
            for( const Meta::TrackPtr &track : tracks )
            {
                Meta::LabelList tracklabels=track->labels();
                QSet<Meta::LabelPtr> addLabelsSet(tracklabels.begin(), tracklabels.end());
                labelSet.unite( addLabelsSet );
            }
            Meta::LabelList labels = labelSet.values();
            if( m_orderByField == Meta::valLabel )
            {
                labels = MemoryQueryMakerHelper::orderListByName<Meta::LabelPtr>( labels, m_orderDescending );
            }
            Q_EMIT newLabelsReady( labels );
            break;
        }
        case QueryMaker::None:
            //should never happen, but handle error anyway
            break;
    }
}

void
MemoryQueryMakerInternal::setMatchers( MemoryMatcher *matchers )
{
    m_matchers = matchers;
}

void
MemoryQueryMakerInternal::setFilters( MemoryFilter *filters )
{
    m_filters = filters;
}

void
MemoryQueryMakerInternal::setMaxSize( int maxSize )
{
    m_maxSize = maxSize;
}

void
MemoryQueryMakerInternal::setType( QueryMaker::QueryType type )
{
    m_type = type;
}

void
MemoryQueryMakerInternal::setCustomReturnFunctions( const QList<CustomReturnFunction *> &functions )
{
    m_returnFunctions = functions;
}

void
MemoryQueryMakerInternal::setCustomReturnValues( const QList<CustomReturnValue *> &values )
{
    m_returnValues = values;
}

} //namespace Collections

