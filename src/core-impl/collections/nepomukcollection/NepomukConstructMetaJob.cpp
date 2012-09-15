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

#include "meta/NepomukAlbum.h"
#include "meta/NepomukArtist.h"
#include "meta/NepomukComposer.h"
#include "meta/NepomukGenre.h"
#include "meta/NepomukLabel.h"
#include "meta/NepomukTrack.h"
#include "meta/NepomukYear.h"
#include "NepomukCollection.h"

#include "core-impl/collections/support/MemoryCollection.h"
#include "core-impl/collections/support/MemoryMeta.h"
#include "core/interfaces/Logger.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaKeys.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"

#include <Nepomuk/ResourceManager>
#include <Soprano/Model>
#include <Soprano/Query/QueryLanguage>
#include <Soprano/QueryResultIterator>

using namespace Meta;
using namespace Collections;
using namespace Soprano;

NepomukConstructMetaJob::NepomukConstructMetaJob( NepomukCollection *coll )
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
    if( m_aborted )
        return;

    Model *model = Nepomuk::ResourceManager::instance()->mainModel();

    // a simple query to calculate the number of tracks.
    QString countQuery = QString::fromLatin1( "select count(distinct ?r) where { ?r a nmm:MusicPiece . }" );
    QueryResultIterator its = model->executeQuery( countQuery,
                              Query::QueryLanguageSparql );
    int totalTracks = its.binding( 0 ).toString().toInt();
    QString operationText = i18n( "Updating Nepomuk Collection" );
    Amarok::Components::logger()->newProgressOperation( this, operationText,
            totalTracks, this,
            SLOT( abort() ) );

    QString query
    = QString::fromLatin1( "select distinct ?r ?title ?url ?artist ?composer ?album ?genre "
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
                           "OPTIONAL {"
                           "    ?albumRes nmm:albumGain ?albumGain ."
                           "}"
                           "OPTIONAL {"
                           "    ?albumRes nmm:albumPeakGain ?albumPeakGain ."
                           "}"
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
                           "}" );

    QueryResultIterator it = model->executeQuery( query, Query::QueryLanguageSparql );

    while( it.next() && !m_aborted )
    {
        QUrl trackResUri = it.binding( "r" ).uri();
        NepomukTrackPtr nepTrackPtr( new NepomukTrack( trackResUri , m_coll ) );

        // A few ontologies have the range of float.
        // Hence many properties are first converted to double and
        // then casted to int to be able to parse the real number stored as int.

        KUrl kurl( it.binding( "url" ).toString() );
        QString title = it.binding( "title" ).toString();
        QString type = it.binding( "type" ).toString();
        int length = it.binding( "length" ).toString().toDouble();
        int bitrate = it.binding( "bitrate" ).toString().toFloat();
        int trackNumber = it.binding( "trackNumber" ).toString().toInt();
        //disc number is not yet extracted as there is no explicit ontology for it
        qreal bpm = it.binding( "bpm" ).toString().toInt();
        QString comment = it.binding( "comment" ).toString();
        int sampleRate = it.binding( "sampleRate" ).toString().toFloat();
        int filesize = it.binding( "filesize" ).toString().toInt();
        qreal trackGain = it.binding( "trackGain" ).toString().toFloat();
        qreal trackPeakGain = it.binding( "trackPeakGain" ).toString().toFloat();
        qreal albumGain = it.binding( "albumGain" ).toString().toFloat();
        qreal albumPeakGain = it.binding( "albumPeakGain" ).toString().toFloat();

        QString modifyDate = it.binding( "modifyDate" ).toString();
        QDateTime modidyDateTime = QDateTime::fromString( modifyDate );
        QString createDate = it.binding( "createDate" ).toString();
        QDateTime createDateTime = QDateTime::fromString( createDate );

        // fill all the properties into the NepomukTrack
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

        nepTrackPtr->setModifyDate( modidyDateTime );
        nepTrackPtr->setCreateDate( createDateTime );

        // checking is done on the NepomukTrack side during retrieval
        nepTrackPtr->setPlayableUrl( kurl );
        nepTrackPtr->setUidUrl( trackResUri.toString() );

        // Artist
        ArtistPtr nepArtistPtr;
        QString artistLabel = it.binding( "artist" ).toString();
        if( !artistLabel.isEmpty() )
        {
            nepArtistPtr = new NepomukArtist( artistLabel );
            nepTrackPtr->setArtist( nepArtistPtr );
        }

        //genre
        GenrePtr nepGenrePtr;
        QString genreLabel = it.binding( "genre" ).toString();
        if( !genreLabel.isEmpty() )
        {
            nepGenrePtr = new NepomukGenre( genreLabel ) ;
            nepTrackPtr->setGenre( nepGenrePtr );
        }

        //composer

        ComposerPtr nepComposerPtr;
        QString composerLabel = it.binding( "composer" ).toString();
        if( !composerLabel.isEmpty() )
        {
            nepComposerPtr = new NepomukComposer( composerLabel ) ;
            nepTrackPtr->setComposer( nepComposerPtr );
        }

        //album

        AlbumPtr nepAlbumPtr;
        QString albumLabel = it.binding( "album" ).toString();
        if( !albumLabel.isEmpty() )
        {
            nepAlbumPtr = new NepomukAlbum( albumLabel, nepArtistPtr ) ;
            nepTrackPtr->setAlbum( nepAlbumPtr );
        }

        //labels

        LabelPtr nepLabelPtr;
        QString tagQuery = QString( "select ?tagUri ?tag where "
                                    "{ %1 nao:hasTag ?tagUri . ?tagUri nao:identifier ?tag . }"
                                  ).arg( Node::resourceToN3( trackResUri ) );
        QueryResultIterator its
        = model->executeQuery( tagQuery, Query::QueryLanguageSparql );
        while( its.next() )
        {
            QString label = its.binding( "tag" ).toString();
            if( !label.isEmpty() )
            {
                nepLabelPtr = new NepomukLabel( label );
                nepTrackPtr->addLabel( nepLabelPtr );
            }
        }

        // year

        YearPtr nepYearPtr;
        QString dateAndTime = it.binding( "year" ).toString();
        QDateTime fullDate = QDateTime::fromString( dateAndTime );
        QString yearLabel = QString( fullDate.date().year() );
        if( !yearLabel.isEmpty() )
        {
            nepYearPtr = new NepomukYear( yearLabel ) ;
            nepTrackPtr->setYear( nepYearPtr );
        }

        // the nepomuk track object is by now completely populated with whatever
        // metadata that could be gathered.
        // cast it and assign it to the MapChanger where it weilds its own magic.
        TrackPtr trackPtr =  TrackPtr::staticCast( nepTrackPtr );
        MemoryMeta::MapChanger mapChanger( m_mc.data() );
        mapChanger.addTrack( trackPtr );

        emit incrementProgress();
    }

    emit endProgressOperation( this );
    emit updated();

}
