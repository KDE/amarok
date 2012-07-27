/****************************************************************************************
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

#include "NepomukConstructMetaJob.h"

#include "NepomukCollection.h"
#include "meta/NepomukAlbum.h"
#include "meta/NepomukArtist.h"
#include "meta/NepomukGenre.h"
#include "meta/NepomukComposer.h"
#include "meta/NepomukYear.h"
#include "meta/NepomukTrack.h"

#include "core/meta/Meta.h"
#include "core/meta/support/MetaKeys.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/MemoryCollection.h"
#include "core-impl/collections/support/MemoryMeta.h"

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

using namespace MemoryMeta;
using namespace Collections;
using namespace Nepomuk::Query;

NepomukConstructMetaJob::NepomukConstructMetaJob(NepomukCollection* coll)
    : Job()
    , m_mc( coll->m_mc )
    , m_aborted( false )
    , m_coll( coll )
{

}

void NepomukConstructMetaJob::abort()
{
    m_aborted = true;
}

void
NepomukConstructMetaJob::run()
{

    Query query;
    Term term =  ResourceTypeTerm( Nepomuk::Vocabulary::NFO::Audio() );
    query.setTerm( term );
    QList<Nepomuk::Query::Result> queriedResults = QueryServiceClient::syncQuery( query );

    Q_FOREACH( const Nepomuk::Query::Result & result, queriedResults )
    {
        Nepomuk::Resource trackRes = result.resource();
        NepomukArtistPtr nepArtistPtr;
        NepomukGenrePtr nepGenrePtr;
        NepomukComposerPtr nepComposerPtr;
        NepomukAlbumPtr nepAlbumPtr;

        // check if track doesn't already exist in TrackMap
        NepomukTrackPtr nepTrackPtr( new NepomukTrack( trackRes, m_coll ) );

        QString artistLabel = trackRes.property( Nepomuk::Vocabulary::NMM::performer() ).toResource().genericLabel();
        if ( !artistLabel.isEmpty() )
        {
            debug() << "Artist found :" << artistLabel;
            nepArtistPtr = new NepomukArtist( artistLabel );
            nepTrackPtr->setArtist( nepArtistPtr );
        }

        QString genreLabel = trackRes.property( Nepomuk::Vocabulary::NMM::genre() ).toString();
        if ( !genreLabel.isEmpty() )
        {
            debug() << "Genre found :" << genreLabel;
            nepGenrePtr = new NepomukGenre( genreLabel ) ;
            nepTrackPtr->setGenre( nepGenrePtr );
        }

        QString composerLabel = trackRes.property( Nepomuk::Vocabulary::NMM::composer() ).toResource().genericLabel();
        if ( !composerLabel.isEmpty() )
        {
            debug() << "Composer found :" << composerLabel;
            nepComposerPtr = new NepomukComposer( composerLabel ) ;
            nepTrackPtr->setComposer( nepComposerPtr );
        }

        QString albumLabel = trackRes.property( Nepomuk::Vocabulary::NMM::musicAlbum() ).toResource().genericLabel();
        if ( !albumLabel.isEmpty() )
        {
            debug() << "Album found :" << albumLabel;
            nepAlbumPtr = new NepomukAlbum( albumLabel, ArtistPtr::staticCast( nepArtistPtr ) ) ;
            nepTrackPtr->setAlbum( nepAlbumPtr );
        }

        TrackPtr trackPtr =  TrackPtr::staticCast( nepTrackPtr );

        MemoryMeta::MapChanger mapChanger( m_mc.data() );
        mapChanger.addTrack(trackPtr);
        debug() << "inserting track with track name : " << trackPtr->name();

        emit incrementProgress();
    }

    emit endProgressOperation( this );
    emit m_coll->collectionUpdated();

}

