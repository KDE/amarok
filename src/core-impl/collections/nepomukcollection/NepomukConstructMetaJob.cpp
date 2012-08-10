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

#define DEBUG_PREFIX "NepomukConstructMetaJob"

#include "NepomukConstructMetaJob.h"

#include "NepomukCollection.h"
#include "meta/NepomukAlbum.h"
#include "meta/NepomukArtist.h"
#include "meta/NepomukGenre.h"
#include "meta/NepomukComposer.h"
#include "meta/NepomukLabel.h"
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
#include <Nepomuk/Tag>
#include <Nepomuk/Query/Result>
#include <Nepomuk/Query/QueryServiceClient>
#include <Nepomuk/Query/ResourceTypeTerm>
#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NMM>
#include <Nepomuk/Vocabulary/NIE>

#include <Soprano/QueryResultIterator>
#include <Soprano/Query/QueryLanguage>
#include <Soprano/Model>

using namespace Meta;
using namespace Collections;
using namespace Nepomuk::Query;

NepomukConstructMetaJob::NepomukConstructMetaJob( NepomukCollection* coll )
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
    Soprano::Model* model = Nepomuk::ResourceManager::instance()->mainModel();

    QString query
            = QString( "select distinct ?r ?title ?url ?artist ?composer ?album ?genre "
                       "?artistRes ?composerRes ?albumRes "
                       "?year ?bpm ?rating ?length ?sampleRate ?trackNumber ?type "
                       " ?bitrate ?modifyDate ?createDate ?comment ?filesize "
                       " ?trackGain ?trackPeakGain ?albumGain ?albumPeakGain "
                       " ?tags "
                       "{"
                       "?r a nfo:Audio ."
                       "?r nie:title ?title ."
                       "?r nie:url ?url ."
                       "OPTIONAL {"
                       "   ?r nmm:performer ?artistRes ."
                       "   ?artistRes nco:fullname ?artist ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nmm:composer ?composerRes ."
                       "    ?composerRes nco:fullname ?composer ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nmm:musicAlbum ?albumRes ."
                       "    ?albumRes nie:title ?album ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nmm:genre ?genre ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nmm:releaseDate ?year ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nmm:beatsPerMinute ?bpm ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nao:numericRating ?rating ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nfo:duration ?length ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nfo:sampleRate ?sampleRate ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nmm:trackNumber ?trackNumer ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nfo:codec ?type ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nfo:averageBitrate ?bitrate ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nie:modified ?modifyDate ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nie:created ?createDate ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nie:comment ?comment ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nie:contentSize ?filesize ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nmm:trackGain ?trackGain ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nmm:trackPeakGain ?trackPeakGain ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nmm:albumGain ?albumGain ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nmm:albumPeakGain ?albumPeakGain ."
                       "}"
                       "OPTIONAL {"
                       "    ?r nao:has ?albumPeakGain ."
                       "}"
                       "}" );

    Soprano::QueryResultIterator it
            = model->executeQuery( query,
                                   Soprano::Query::QueryLanguageSparql );
    while( it.next() )
    {
        QUrl trackResUri = it.binding( "r" ).uri();
        NepomukArtistPtr nepArtistPtr;
        NepomukGenrePtr nepGenrePtr;
        NepomukComposerPtr nepComposerPtr;
        NepomukAlbumPtr nepAlbumPtr;
        NepomukLabelPtr nepLabelPtr;
        NepomukYearPtr nepYearPtr;

        // check if track doesn't already exist in TrackMap
        if( m_trackHash.contains( trackResUri ) )
            continue;
        // not present, construct the nepomuk track object and insert it into HashMap

        NepomukTrackPtr nepTrackPtr( new NepomukTrack( trackResUri , m_coll ) );

        // many properties are first converted to double and then casted to int
        // because without this widening conversion in the beginning, it was leading to
        // erraneous values due to the size limitation of int.

        KUrl kurl( it.binding( "url" ).toString() );
        QString title = it.binding( "title" ).toString();
        QString type = it.binding( "type" ).toString();
        int length = ( int ) it.binding( "length" ).toString().toDouble();
        int bitrate = ( int )it.binding( "bitrate" ).toString().toDouble();
        int trackNumber = ( int )it.binding( "trackNumber" ).toString().toDouble();
        //disc number is not yet extracted as there is no explicit ontology for the same
        qreal bpm = it.binding( "bpm" ).toString().toDouble();
        QString comment = it.binding( "comment" ).toString();
        int sampleRate = ( int )it.binding( "sampleRate" ).toString().toDouble();
        int filesize = ( int )it.binding( "filesize" ).toString().toDouble();
        double trackGain = it.binding( "trackGain" ).toString().toDouble();
        double trackPeakGain = it.binding( "trackPeakGain" ).toString().toDouble();
        double albumGain = it.binding( "albumGain" ).toString().toDouble();
        double albumPeakGain = it.binding( "albumPeakGain" ).toString().toDouble();

        QString modifyDate = it.binding("modifyDate").toString();
        QDateTime modidyDateTime = QDateTime::fromString(modifyDate);
        QString createDate = it.binding("createDate").toString();
        QDateTime createDateTime = QDateTime::fromString(createDate);

        // populate all the properties into the NepomukTrack based on the availability

        nepTrackPtr->setName( title );
        nepTrackPtr->setType( type );
        nepTrackPtr->setLength( length );
        nepTrackPtr->setBitrate( bitrate );
        nepTrackPtr->setTrackNumber( trackNumber );
        nepTrackPtr->setbpm( bpm );
        nepTrackPtr->setComment( comment );
        nepTrackPtr->setSampleRate( sampleRate );
        nepTrackPtr->setFilesize( filesize );
        nepTrackPtr->setTrackGain( trackGain );
        nepTrackPtr->setTrackPeakGain( trackPeakGain );
        nepTrackPtr->setAlbumGain( albumGain );
        nepTrackPtr->setAlbumPeakGain( albumPeakGain );

        nepTrackPtr->setModifyDate(modidyDateTime);
        nepTrackPtr->setCreateDate(createDateTime);

        // checking is done on the NepomukTrack side during retrieval
        nepTrackPtr->setKUrl( kurl );

        m_trackHash.insert( trackResUri, Meta::TrackPtr::staticCast( nepTrackPtr ) );

        // Artist

        QUrl artistResUri = it.binding( "artistRes" ).uri();

        // check if artist doesn't already exist in HashMap
        if( m_artistHash.contains( artistResUri ) )
        {
            ArtistPtr artistPtr = m_artistHash.value( artistResUri );
            nepTrackPtr->setArtist( Meta::NepomukArtistPtr::staticCast( artistPtr ) );
        }
        // not present, construct the nepomuk artist object and insert it into HashMap
        else
        {
            QString artistLabel = it.binding( "artist" ).toString();
            if( !artistLabel.isEmpty() )
            {
                debug() << "Artist found :" << artistLabel;
                nepArtistPtr = new NepomukArtist( artistLabel );
                nepTrackPtr->setArtist( nepArtistPtr );
                m_artistHash.insert( artistResUri, Meta::ArtistPtr::staticCast( nepArtistPtr ) );
            }
        }

        QString genreLabel = it.binding( "genre" ).toString();

        // check if genre doesn't already exist in HashMap
        if( m_genreHash.contains( genreLabel ) )
        {
            GenrePtr genrePtr = m_genreHash.value( genreLabel );
            nepTrackPtr->setGenre( Meta::NepomukGenrePtr::staticCast( genrePtr ) );
        }
        // not present, construct the nepomuk genre object and insert it into HashMap
        else
        {
            if( !genreLabel.isEmpty() )
            {
                debug() << "Genre found :" << genreLabel;
                nepGenrePtr = new NepomukGenre( genreLabel ) ;
                nepTrackPtr->setGenre( nepGenrePtr );
                m_genreHash.insert( genreLabel, Meta::GenrePtr::staticCast( nepGenrePtr ) );
            }
        }

        QUrl composerResUri = it.binding( "composerRes" ).uri();

        // check if composer doesn't already exist in HashMap
        if( m_composerHash.contains( composerResUri ) )
        {
            ComposerPtr composerPtr = m_composerHash.value( composerResUri );
            nepTrackPtr->setComposer( Meta::NepomukComposerPtr::staticCast( composerPtr ) );
        }
        // not present, construct the nepomuk composer object and insert it into HashMap
        else
        {
            QString composerLabel = it.binding( "composer" ).toString();
            if( !composerLabel.isEmpty() )
            {
                debug() << "Composer found :" << composerLabel;
                nepComposerPtr = new NepomukComposer( composerLabel ) ;
                nepTrackPtr->setComposer( nepComposerPtr );
                m_composerHash.insert( composerResUri, Meta::ComposerPtr::staticCast( nepComposerPtr ) );
            }
        }

        QUrl albumResUri = it.binding( "albumRes" ).uri();
        // check if album doesn't already exist in HashMap
        if( m_albumHash.contains( albumResUri ) )
        {
            AlbumPtr albumPtr = m_albumHash.value( albumResUri );
            nepTrackPtr->setAlbum( Meta::NepomukAlbumPtr::staticCast( albumPtr ) );
        }
        // not present, construct the nepomuk album object and insert it into HashMap
        else
        {
            QString albumLabel = it.binding( "album" ).toString();
            if( !albumLabel.isEmpty() )
            {
                debug() << "Album found :" << albumLabel;
                nepAlbumPtr = new NepomukAlbum( albumLabel, ArtistPtr::staticCast( nepArtistPtr ) ) ;
                nepTrackPtr->setAlbum( nepAlbumPtr );
                m_albumHash.insert( albumResUri, Meta::AlbumPtr::staticCast( nepAlbumPtr ) );
            }
        }

        QString tagQuery
                = QString( "select distinct ?tag where { %1 nao:hasTag ?tag . }" )
                .arg( Soprano::Node::resourceToN3( trackResUri ) );

        Soprano::QueryResultIterator its
                = model->executeQuery( tagQuery, Soprano::Query::QueryLanguageSparql );
        while( its.next() )
        {
            debug() << "TAG : " << its.binding( "tag" ).toString();

            QUrl labelResUri = its.binding( "tag" ).uri();

            if( m_labelHash.contains( labelResUri ) )
            {
                LabelPtr labelPtr = m_labelHash.value( labelResUri );
                nepTrackPtr->addLabel( Meta::LabelPtr::staticCast( labelPtr ) );
            }

            else
            {
                QString label = its.binding( "tag" ).toString();
                if( !label.isEmpty() )
                {
                    debug() << "Label found : " << label;
                    nepLabelPtr = new NepomukLabel( label );
                    nepTrackPtr->addLabel( Meta::LabelPtr::staticCast( nepLabelPtr ) );
                    m_labelHash.insert( labelResUri, Meta::LabelPtr::staticCast( nepLabelPtr ) );

                }
            }
        }

        // year


        QString dateAndTime = it.binding("year").toString();
        QDateTime fullDate = QDateTime::fromString(dateAndTime);
        QString yearLabel = QString( fullDate.date().year() );

        // check if year doesn't already exist in YearMap
        if( m_yearHash.contains( yearLabel ) )
        {
            YearPtr yearPtr = m_yearHash.value( yearLabel );
            nepTrackPtr->setYear( Meta::NepomukYearPtr::staticCast( yearPtr ) );
        }
        // not present, construct the nepomuk year object and insert it into HashMap
        else
        {
            if( !yearLabel.isEmpty() )
            {
                debug() << "Year found :" << yearLabel;
                nepYearPtr = new NepomukYear( yearLabel ) ;
                nepTrackPtr->setYear( nepYearPtr );
                m_yearHash.insert( yearLabel, Meta::YearPtr::staticCast( nepYearPtr ) );
            }
        }

        // the nepomuk track object is by now completely populated with whatever
        // metadata that could be gathered.
        // cast it and assign it to the MapChanger where it weilds its own magic.
        TrackPtr trackPtr =  TrackPtr::staticCast( nepTrackPtr );

        MemoryMeta::MapChanger mapChanger( m_mc.data() );
        mapChanger.addTrack( trackPtr );
        debug() << "inserting track with track name : " << trackPtr->name();
        debug() << "-------------------------------";

        emit incrementProgress();
    }

    emit endProgressOperation( this );
    emit m_coll->collectionUpdated();
}
