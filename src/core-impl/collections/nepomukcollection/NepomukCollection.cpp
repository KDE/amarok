/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#define DEBUG_PREFIX "NepomukCollection"

#include "NepomukCollection.h"
#include "NepomukQueryMaker.h"
#include "meta/NepomukTrack.h"
#include "meta/NepomukAlbum.h"
#include "meta/NepomukArtist.h"
#include "meta/NepomukComposer.h"
#include "meta/NepomukGenre.h"

#include "core/meta/Meta.h"
#include "core/meta/support/MetaKeys.h"
#include "core/collections/QueryMaker.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/MemoryCollection.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/Term>
#include <Nepomuk/Query/Result>
#include <Nepomuk/Query/QueryServiceClient>
#include <Nepomuk/Query/ResourceTypeTerm>
#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NMM>
#include <Nepomuk/Vocabulary/NIE>
#include <KIcon>
#include <QString>
#include <QMap>

using namespace Meta;
using namespace Collections;
using namespace Nepomuk::Query;

NepomukCollection::NepomukCollection()
    : Collection()
    , m_mc( new Collections::MemoryCollection() )
{
    debug() << "in nepomukcollection constructor";
    if( Nepomuk::ResourceManager::instance()->initialized() )
        m_nepomukCollectionReady = true;

    if ( buildCollection() )
        debug()<<"successful! :)";
    else debug()<<"not successful :(";

}

NepomukCollection::~NepomukCollection()
{

}

Collections::QueryMaker*
NepomukCollection::queryMaker()
{
    return new MemoryQueryMaker( m_mc.toWeakRef(), collectionId());
}

QString
NepomukCollection::uidUrlProtocol() const
{
    static const QString uid( "amarok-nepomuk" );
    return uid;
}

QString
NepomukCollection::collectionId() const
{
    return QString( "%1://" ).arg( uidUrlProtocol() );
}

QString
NepomukCollection::prettyName() const
{
    return QString( "Nepomuk Collection" );
}

KIcon
NepomukCollection::icon() const
{
    return KIcon( "nepomuk" );
}

bool
NepomukCollection::isWritable() const
{
    // Nepomuk if initialized is always writable
    // A check for nepomuk initialized will suffice
    return m_nepomukCollectionReady;
}

bool
NepomukCollection::buildCollection()
{
    // check if m_mc->acquireReadLock or WriteLock is required
    m_mc->setTrackMap(getTrackMap());
    m_mc->setArtistMap(getArtistMap());
    m_mc->setGenreMap(getGenreMap());
    m_mc->setComposerMap(getComposerMap());
    //m_mc->setAlbumMap(getAlbumMap());
    // TODO
    // year??

    // only checking for trackMap now.
    // Should ideally check for more conditions.
    if ( m_mc->trackMap().size() == 0 )
        return false;

    else return true;
}



TrackMap&
NepomukCollection::getTrackMap() const
{
    DEBUG_BLOCK
    Query query;

    Term term =  ResourceTypeTerm( Nepomuk::Vocabulary::NFO::Audio() );
    query.setTerm(term);
    query.setLimit(100);

    QList<Nepomuk::Query::Result> queriedResults = QueryServiceClient::syncQuery( query );
    TrackMap map;

    Q_FOREACH( const Nepomuk::Query::Result & result, queriedResults )
    {
        NepomukTrackPtr track( new NepomukTrack( result.resource() ) );
        map.insert(track->uidUrl(), Meta::TrackPtr::staticCast(track));
        debug()<<"Inserted track :"<<track->name()<<"into map";
    }

    return map;
}

ArtistMap&
NepomukCollection::getArtistMap() const
{
    DEBUG_BLOCK
    Query query;

    Term term =  ResourceTypeTerm( Nepomuk::Vocabulary::NMM::performer() );
    query.setTerm(term);

    QList<Nepomuk::Query::Result> queriedResults = QueryServiceClient::syncQuery( query );
    ArtistMap map;

    Q_FOREACH( const Nepomuk::Query::Result & result, queriedResults )
    {
        QString artist = result.resource().genericLabel();
        NepomukArtistPtr artistPtr( new NepomukArtist( artist ) );
        map.insert(artistPtr->name(), Meta::ArtistPtr::staticCast(artistPtr));
    }

    return map;
}

GenreMap&
NepomukCollection::getGenreMap() const
{
    DEBUG_BLOCK
    Query query;

    Term term =  ResourceTypeTerm( Nepomuk::Vocabulary::NMM::genre() );
    query.setTerm(term);

    QList<Nepomuk::Query::Result> queriedResults = QueryServiceClient::syncQuery( query );
    GenreMap map;

    Q_FOREACH( const Nepomuk::Query::Result & result, queriedResults )
    {
        QString genre = result.resource().genericLabel();
        NepomukGenrePtr genrePtr( new NepomukGenre( genre ) );
        map.insert(genrePtr->name(), Meta::GenrePtr::staticCast(genrePtr));
    }

    return map;
}

ComposerMap&
NepomukCollection::getComposerMap() const
{
    DEBUG_BLOCK
    Query query;

    Term term =  ResourceTypeTerm( Nepomuk::Vocabulary::NMM::composer() );
    query.setTerm(term);

    QList<Nepomuk::Query::Result> queriedResults = QueryServiceClient::syncQuery( query );
    ComposerMap map;

    Q_FOREACH( const Nepomuk::Query::Result & result, queriedResults )
    {
        QString composer = result.resource().genericLabel();
        NepomukComposerPtr composerPtr( new NepomukComposer( composer ) );
        map.insert(composerPtr->name(), Meta::ComposerPtr::staticCast(composerPtr));
    }

    return map;
}

AlbumMap&
NepomukCollection::getAlbumMap() const
{
    DEBUG_BLOCK
    Query query;

    Term term =  ResourceTypeTerm( Nepomuk::Vocabulary::NMM::MusicAlbum() );
    query.setTerm(term);

    QList<Nepomuk::Query::Result> queriedResults = QueryServiceClient::syncQuery( query );
    AlbumMap map;

    Q_FOREACH( const Nepomuk::Query::Result & result, queriedResults )
    {
        QString album = result.resource().genericLabel();
        NepomukAlbumPtr albumPtr( new NepomukAlbum( album ) );
        map.insert(Meta::AlbumPtr::staticCast(albumPtr));
    }

    return map;
}

void
NepomukCollection::updated()
{
    this->buildCollection();
}
