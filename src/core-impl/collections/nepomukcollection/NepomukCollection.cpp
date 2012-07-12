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
    // check if Nepomuk is available, if yes, initialize.
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
    m_nepomukCollectionReady = false;
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
    setupMetaMap();

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
NepomukCollection::setupMetaMap()
{
    DEBUG_BLOCK
            Query query;

    Term term =  ResourceTypeTerm( Nepomuk::Vocabulary::NFO::Audio() );
    query.setTerm( term );
    query.setLimit( 20 );
    QList<Nepomuk::Query::Result> queriedResults = QueryServiceClient::syncQuery( query );

    Q_FOREACH( const Nepomuk::Query::Result & result, queriedResults )
    {
        ArtistPtr artistPtr;
        GenrePtr genrePtr;
        ComposerPtr composerPtr;
        AlbumPtr albumPtr;
        Nepomuk::Resource trackRes = result.resource();
        NepomukArtistPtr nepArtistPtr;
        NepomukGenrePtr nepGenrePtr;
        NepomukComposerPtr nepComposerPtr;
        NepomukAlbumPtr nepAlbumPtr;

        QString artistLabel = trackRes.property( Nepomuk::Vocabulary::NMM::performer() ).toResource().genericLabel();
        debug() << "Artist found :" << artistLabel;

        if ( !( artistLabel.isNull() || artistLabel.isEmpty() ) )
        {
            // see if artist is already present in artistMap
            if( m_mc->artistMap().contains( artistLabel ) )
            {
                debug() << "artist already present!";
                artistPtr = m_mc->artistMap().value( artistLabel );
                nepArtistPtr = NepomukArtistPtr::staticCast( artistPtr );
            }
            else
            {
                nepArtistPtr = new NepomukArtist( artistLabel ) ;
                artistPtr = ArtistPtr::staticCast( nepArtistPtr );
            }
        }

        QString genreLabel = trackRes.property( Nepomuk::Vocabulary::NMM::genre() ).toString();
        debug() << "Genre found :" << genreLabel;

        if ( !( genreLabel.isNull() || genreLabel.isEmpty() ) )
        {
            // see if genre is already present in genreMap
            if( m_mc->genreMap().contains( genreLabel ) )
            {
                debug() << "genre already present!";
                genrePtr = m_mc->genreMap().value( genreLabel );
                nepGenrePtr = NepomukGenrePtr::staticCast( genrePtr );
            }
            else
            {
                nepGenrePtr = new NepomukGenre( genreLabel ) ;
                genrePtr = GenrePtr::staticCast( nepGenrePtr );
            }
        }

        QString composerLabel = trackRes.property( Nepomuk::Vocabulary::NMM::composer() ).toResource().genericLabel();
        debug() << "Composer found :" << composerLabel;

        if ( !( composerLabel.isNull() || composerLabel.isEmpty() ) )
        {
            // see if composer is already present in composerMap
            if( m_mc->composerMap().contains( composerLabel ) )
            {
                debug() << "composer already present!";
                composerPtr = m_mc->composerMap().value( composerLabel );
                nepComposerPtr =  NepomukComposerPtr::staticCast( composerPtr );
            }
            else
            {
                nepComposerPtr = new NepomukComposer( composerLabel ) ;
                composerPtr =  ComposerPtr::staticCast( nepComposerPtr );
            }
        }

        QString albumLabel = trackRes.property( Nepomuk::Vocabulary::NMM::musicAlbum() ).toResource().genericLabel();
        debug() << "Album found :" << albumLabel;

        if ( !( albumLabel.isNull() || albumLabel.isEmpty() ) )
        {

            // see if album is already present in albumMap
            if( m_mc->albumMap().contains( albumLabel, artistLabel ) )
            {
                debug() << "album already present!";
                albumPtr = m_mc->albumMap().value( albumLabel, artistLabel );
                nepAlbumPtr =  NepomukAlbumPtr::staticCast( albumPtr );
            }
            else
            {
                nepAlbumPtr = new NepomukAlbum( albumLabel, artistPtr ) ;
                albumPtr =  AlbumPtr::staticCast( nepAlbumPtr );
            }
        }

        NepomukTrackPtr nepTrackPtr( new NepomukTrack( artistPtr,
                                                       genrePtr,
                                                       composerPtr,
                                                       albumPtr,
                                                       trackRes ) );
        TrackPtr trackPtr =  TrackPtr::staticCast( nepTrackPtr );
        m_mc->addTrack( trackPtr );
        debug() << "inserting track with track name : " << trackPtr->name();

        // add track to artistPtr
        if ( !( artistLabel.isEmpty() || artistLabel.isNull() ) )
        {
            nepArtistPtr->addTrack( trackPtr );
            m_mc->addArtist(  ArtistPtr::staticCast( nepArtistPtr ) );
        }

        // add track to genrePtr
        if ( !( genreLabel.isEmpty() || genreLabel.isNull() ) )
        {
            nepGenrePtr->addTrack( trackPtr );
            m_mc->addGenre(  GenrePtr::staticCast( nepGenrePtr ) );
        }

        // add track to composerPtr
        if ( !( composerLabel.isEmpty() || composerLabel.isNull() ) )
        {
            nepComposerPtr->addTrack( trackPtr );
            m_mc->addComposer(  ComposerPtr::staticCast( nepComposerPtr ) );
        }

        // add track to albumPtr
        if ( !( albumLabel.isEmpty() || albumLabel.isNull() ) )
        {
            nepAlbumPtr->addTrack( trackPtr );
            m_mc->addAlbum(  AlbumPtr::staticCast( nepAlbumPtr ) );
        }
    }

}
