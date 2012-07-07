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

#include <Nepomuk/Resource>
#include <Nepomuk/Variant>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Query/ComparisonTerm>
#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/Term>
#include <Nepomuk/Query/Result>
#include <Nepomuk/Query/QueryServiceClient>
#include <Nepomuk/Query/ResourceTypeTerm>
#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NMM>
#include <Nepomuk/Vocabulary/NIE>
#include <Soprano/QueryResultIterator>
#include <Soprano/Query/QueryLanguage>
#include <Soprano/Model>

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
    {
        m_nepomukCollectionReady = true;

        if( buildCollection() )
            debug() << "successful! :)";
        else debug() << "not successful :(";
    }

    else m_nepomukCollectionReady = false;

}

NepomukCollection::~NepomukCollection()
{

}

Collections::QueryMaker*
NepomukCollection::queryMaker()
{
    return new MemoryQueryMaker( m_mc.toWeakRef(), collectionId() );
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
    DEBUG_BLOCK

            m_mc->acquireWriteLock();

    /**
      * First get the meta data like artist, genre, composers and albums
      * And then get the tracks
      * Pass the necessary albums and artists in the track constructor
      * And construct the track map
      */


    //    ArtistMap artistmap = m_mc->artistMap();
    //    setupArtistMap( artistmap );
    //    m_mc->setArtistMap( artistmap );

//    GenreMap genremap = m_mc->genreMap();
//    setupGenreMap( genremap );
//    m_mc->setGenreMap( genremap );

    //    ComposerMap composermap = m_mc->composerMap();
    //    setupComposerMap( composermap );
    //    m_mc->setComposerMap( composermap );

    //    AlbumMap albummap = m_mc->albumMap();
    //    setupAlbumMap( albummap );
    //    m_mc->setAlbumMap( albummap );

    TrackMap trackmap = m_mc->trackMap();
    setupTrackMap( trackmap );
    m_mc->setTrackMap( trackmap );

    m_mc->releaseLock();

    // TODO
    // year??

    // only checking for trackMap now.
    // Should ideally check for more conditions.

    if( m_mc->trackMap().size() == 0 )
        return false;

    else return true;
}



void
NepomukCollection::setupTrackMap( TrackMap &trackmap )
{
    DEBUG_BLOCK
            Query query;

    Term term =  ResourceTypeTerm( Nepomuk::Vocabulary::NFO::Audio() );
    query.setTerm( term );
    QList<Nepomuk::Query::Result> queriedResults = QueryServiceClient::syncQuery( query );

    Q_FOREACH( const Nepomuk::Query::Result & result, queriedResults )
    {
        ArtistPtr artistPtr;
        GenrePtr genrePtr;
        ComposerPtr composerPtr;
        AlbumPtr albumPtr;
        Nepomuk::Resource trackResource = result.resource();
        Nepomuk::Resource tempRes;


        //get and insert artist into artistMap
        QString artistLabel = trackResource.property( Nepomuk::Vocabulary::NMM::performer() ).toString();
        debug()<<"artist is: "<<artistLabel;
//        NepomukArtistPtr nepArtistPtr( new NepomukArtist( artistLabel ) );
//        debug()<<"inserting artist : "<<artistLabel;
//        artistPtr = Meta::ArtistPtr::staticCast( nepArtistPtr );
//        this->m_mc->addArtist(artistPtr);

        //get and insert genre into genreMap
        QString genre = trackResource.property( Nepomuk::Vocabulary::NMM::genre() ).toString();
        NepomukGenrePtr nepGenrePtr( new NepomukGenre( genre ) );
        debug()<<"inserting genre : "<<genre;
        genrePtr = Meta::GenrePtr::staticCast( nepGenrePtr );
        this->m_mc->addGenre(genrePtr);


        NepomukTrackPtr trackPtr( new NepomukTrack( artistPtr,
                                                    genrePtr,
                                                    composerPtr,
                                                    albumPtr,
                                                    trackResource ) );
        //this->m_mc->addTrack( Meta::TrackPtr::staticCast( trackPtr ) );
        debug() << "inserting track with track name : " << trackPtr->name();
        //debug()<<"and artist "<<artistPtr->name();
        trackmap.insert( trackPtr->uidUrl(), Meta::TrackPtr::staticCast( trackPtr ) );

    }

}

void
NepomukCollection::setupArtistMap( ArtistMap &artistmap )
{
    DEBUG_BLOCK
            Query query;

    Term term =  ResourceTypeTerm( Nepomuk::Vocabulary::NMM::performer() );
    query.setTerm( term );

    QList<Nepomuk::Query::Result> queriedResults = QueryServiceClient::syncQuery( query );
    debug() << "artists scanned :" << queriedResults.size();
    Q_FOREACH( const Nepomuk::Query::Result & result, queriedResults )
    {
        QString artist = result.resource().genericLabel();
        NepomukArtistPtr artistPtr( new NepomukArtist( artist ) );
        artistmap.insert( artistPtr->name(), Meta::ArtistPtr::staticCast( artistPtr ) );
    }

}

void
NepomukCollection::setupGenreMap( GenreMap &genremap )
{DEBUG_BLOCK

            Query query;
    //    Soprano::Model* model = Nepomuk::ResourceManager::instance()->mainModel();
    //    QString query = QString( "select ?genre where { ?r a nfo:Audio . ?r nmm:genre ?genre . }" );
    //    Soprano::QueryResultIterator iter = model->executeQuery(
    //                query, Soprano::Query::QueryLanguageSparql );

    //    while( iter.next() )
    //    {
    //        debug() <<"genre"<< Nepomuk::Resource( iter.binding( "genre" ).uri() ).genericLabel();
    //    }

    Term audioTerm = ResourceTypeTerm( Nepomuk::Vocabulary::NFO::Audio() );
    ComparisonTerm term ( Nepomuk::Vocabulary::NMM::genre(), audioTerm );
    term.inverted();
    query.setTerm( term );
    QList<Nepomuk::Query::Result> queriedResults = QueryServiceClient::syncQuery( query );


    Q_FOREACH( const Nepomuk::Query::Result & result, queriedResults )
    {
        QString genre = result.resource().genericLabel();
        NepomukGenrePtr genrePtr( new NepomukGenre( genre ) );
        debug()<<"inserting genre : "<<genre;
        genremap.insert( genrePtr->name(), Meta::GenrePtr::staticCast( genrePtr ) );
    }

}

void
NepomukCollection::setupComposerMap( ComposerMap &composermap )
{
    DEBUG_BLOCK
            Query query;


    Term term =  ResourceTypeTerm( Nepomuk::Vocabulary::NMM::musicAlbum() );
    query.setTerm( term );

    QList<Nepomuk::Query::Result> queriedResults = QueryServiceClient::syncQuery( query );
    debug() << "composers scanned :" << queriedResults.size();

    Q_FOREACH( const Nepomuk::Query::Result & result, queriedResults )
    {
        QString composer = result.resource().genericLabel();
        NepomukComposerPtr composerPtr( new NepomukComposer( composer ) );
        composermap.insert( composerPtr->name(), Meta::ComposerPtr::staticCast( composerPtr ) );
    }

}

void
NepomukCollection::setupAlbumMap( AlbumMap &albummap )
{
    DEBUG_BLOCK
            Query query;

    Term term =  ResourceTypeTerm( Nepomuk::Vocabulary::NMM::musicAlbum() );
    query.setTerm( term );

    QList<Nepomuk::Query::Result> queriedResults = QueryServiceClient::syncQuery( query );
    debug() << "albums scanned :" << queriedResults.size();

    Q_FOREACH( const Nepomuk::Query::Result & result, queriedResults )
    {
        QString album = result.resource().genericLabel();
        NepomukAlbumPtr albumPtr( new NepomukAlbum( album ) );
        albummap.insert( Meta::AlbumPtr::staticCast( albumPtr ) );
    }

}
