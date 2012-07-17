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
#include "core-impl/collections/support/MemoryMeta.h"
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

using namespace MemoryMeta;
using namespace Collections;
using namespace Nepomuk::Query;

NepomukCollection::NepomukCollection()
    : Collection()
    , m_mc( new Collections::MemoryCollection() )
{
    // check if Nepomuk is available, if yes, initialize.
    if( Nepomuk::ResourceManager::instance()->initialized() )
        m_nepomukCollectionReady = true;

    else m_nepomukCollectionReady = false;

    if( buildCollection() )
        debug() << "loaded some tracks";
    else debug() << "didn't load any tracks";
}

NepomukCollection::~NepomukCollection()
{
    delete m_mc;
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
        NepomukTrackPtr nepTrackPtr( new NepomukTrack( trackRes ) );

        QString artistLabel = trackRes.property( Nepomuk::Vocabulary::NMM::performer() ).toResource().genericLabel();
        debug() << "Artist found :" << artistLabel;
        nepArtistPtr = new NepomukArtist( artistLabel );
        nepTrackPtr->setArtist( nepArtistPtr );

        QString genreLabel = trackRes.property( Nepomuk::Vocabulary::NMM::genre() ).toString();
        debug() << "Genre found :" << genreLabel;
        nepGenrePtr = new NepomukGenre( genreLabel ) ;
        nepTrackPtr->setGenre( nepGenrePtr );

        QString composerLabel = trackRes.property( Nepomuk::Vocabulary::NMM::composer() ).toResource().genericLabel();
        debug() << "Composer found :" << composerLabel;
        nepComposerPtr = new NepomukComposer( composerLabel ) ;
        nepTrackPtr->setComposer( nepComposerPtr );

        QString albumLabel = trackRes.property( Nepomuk::Vocabulary::NMM::musicAlbum() ).toResource().genericLabel();
        debug() << "Album found :" << albumLabel;
        nepAlbumPtr = new NepomukAlbum( albumLabel, ArtistPtr::staticCast( nepArtistPtr ) ) ;
        nepTrackPtr->setAlbum( nepAlbumPtr );

        TrackPtr trackPtr =  TrackPtr::staticCast( nepTrackPtr );

        MemoryMeta::MapChanger mapChanger( m_mc.data() );
        mapChanger.addTrack(trackPtr);
        debug() << "inserting track with track name : " << trackPtr->name();

    }

    if ( m_mc->trackMap().size() > 0 )
        return true;
    else return false;
}
