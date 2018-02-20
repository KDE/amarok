/****************************************************************************************
 * Copyright (c) 2013 Edward Toroshchin <amarok@hades.name>
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

#include "NepomukParser.h"

#include "NepomukCache.h"
#include "NepomukCollection.h"
#include "NepomukSelectors.h"

#include "core/support/Debug.h"

#include "meta/NepomukAlbum.h"
#include "meta/NepomukArtist.h"
#include "meta/NepomukComposer.h"
#include "meta/NepomukGenre.h"
#include "meta/NepomukLabel.h"
#include "meta/NepomukTrack.h"
#include "meta/NepomukYear.h"

namespace Collections {

NepomukParser::NepomukParser( NepomukCollection *coll )
    : m_collection( coll )
{
    Q_ASSERT( coll );
}

bool
NepomukParser::parseOne( Soprano::QueryResultIterator &queryResult,
                         Meta::TrackList &objectList )
{
    QUrl resource = queryResult.binding( NS_track ).uri();
    if( resource.isEmpty() ) return false;

    /* Prevent duplicate results. This may look quite crude, and that's because
     * it is. Actually, this should be handled by SPARQL query's DISTINCT
     * clause, but it doesn't work always. Here's why.
     *
     * We select all the information (title, album, artist, date
     * created/modified, you name it) about the tracks in one go. This allows us
     * to create new Meta* objects without any additional SPARQL queries.
     *
     * But this also prevents DISTINCT from doing what we want, because the
     * "database" "model" of Nepomuk allows tracks to have more than one:
     *
     *  - artist (which is okay),
     *  - title (meh?),
     *  - modification/creation date (lol wut),
     *  - pretty much any other piece of information.
     *
     *  DISTINCT would naturally select all the existing combinations, creating
     *  many duplicates in the query results.
     *
     *  Long story short: TODO improve Nepomuk performance.
     */
    if( m_seen_uri.contains( resource ) ) return false;
    m_seen_uri.insert( resource );

    Meta::TrackPtr track( m_collection->cache()->getTrack( resource ) );
    objectList << track;

    AmarokSharedPointer<Meta::NepomukTrack> ntrack( AmarokSharedPointer<Meta::NepomukTrack>::staticCast( track ) );
    if( !ntrack->isFilled() )
    {
        Meta::ArtistList artist;
        Meta::AlbumList album;
        Meta::ComposerList composer;
        Meta::GenreList genre;
        Meta::YearList year;

        if( parseOne( queryResult, artist ) )
            ntrack->setArtist( artist.first() );
        if( parseOne( queryResult, album ) )
            ntrack->setAlbum( album.first() );
        if( parseOne( queryResult, composer ) )
            ntrack->setComposer( composer.first() );
        if( parseOne( queryResult, genre ) )
            ntrack->setGenre( genre.first() );
        if( parseOne( queryResult, year ) )
            ntrack->setYear( year.first() );

        ntrack->fill( queryResult.binding( NS_trackTitle ).literal().toString(),
                      queryResult.binding( NS_trackUrl ).uri(),
                      m_collection );

        if( !queryResult.binding( NS_trackType ).isEmpty() )
            ntrack->setType( queryResult.binding( NS_trackType ).literal().toString() );
        if( !queryResult.binding( NS_trackLength ).isEmpty() )
            ntrack->setLength( queryResult.binding( NS_trackLength ).literal().toInt() );
        if( !queryResult.binding( NS_trackBitrate ).isEmpty() )
            ntrack->setBitrate( queryResult.binding( NS_trackBitrate ).literal().toInt() );
        if( !queryResult.binding( NS_trackNumber ).isEmpty() )
            ntrack->setTrackNumber( queryResult.binding( NS_trackNumber ).literal().toInt() );
        if( !queryResult.binding( NS_trackBPM ).isEmpty() )
            ntrack->setbpm( queryResult.binding( NS_trackBPM ).literal().toDouble() );
        if( !queryResult.binding( NS_trackComment ).isEmpty() )
            ntrack->setComment( queryResult.binding( NS_trackComment ).literal().toString() );
        if( !queryResult.binding( NS_trackSampleRate ).isEmpty() )
            ntrack->setSampleRate( queryResult.binding( NS_trackSampleRate ).literal().toInt() );
        if( !queryResult.binding( NS_trackFileSize ).isEmpty() )
            ntrack->setFilesize( queryResult.binding( NS_trackFileSize ).literal().toInt() );
        if( !queryResult.binding( NS_trackGain ).isEmpty() )
            ntrack->setTrackGain( queryResult.binding( NS_trackGain ).literal().toDouble() );
        if( !queryResult.binding( NS_trackPeakGain ).isEmpty() )
            ntrack->setTrackPeakGain( queryResult.binding( NS_trackPeakGain ).literal().toDouble() );
        if( !queryResult.binding( NS_albumGain ).isEmpty() )
            ntrack->setAlbumGain( queryResult.binding( NS_albumGain ).literal().toDouble() );
        if( !queryResult.binding( NS_albumPeakGain ).isEmpty() )
            ntrack->setAlbumPeakGain( queryResult.binding( NS_albumPeakGain ).literal().toDouble() );
        if( !queryResult.binding( NS_trackModifyDate ).isEmpty() )
            ntrack->setModifyDate( queryResult.binding( NS_trackModifyDate ).literal().toDateTime() );
        if( !queryResult.binding( NS_trackCreateDate ).isEmpty() )
            ntrack->setCreateDate( queryResult.binding( NS_trackCreateDate ).literal().toDateTime() );
    }

    return true;
}

bool
NepomukParser::parseOne( Soprano::QueryResultIterator &queryResult,
                         Meta::ArtistList &objectList )
{
    QUrl resource = queryResult.binding( NS_artist ).uri();
    if( resource.isEmpty() ) return false;

    Meta::ArtistPtr artist( m_collection->cache()->getArtist( resource ) );
    objectList << artist;

    AmarokSharedPointer<Meta::NepomukArtist> nartist( AmarokSharedPointer<Meta::NepomukArtist>::staticCast( artist ) );
    if( !nartist->isFilled() )
        nartist->fill( queryResult.binding( NS_artistName ).literal().toString() );

    return true;
}

bool
NepomukParser::parseOne( Soprano::QueryResultIterator &queryResult,
                         Meta::AlbumList &objectList )
{
    QUrl resource = queryResult.binding( NS_album ).uri();
    if( resource.isEmpty() ) return false;

    Meta::AlbumPtr album( m_collection->cache()->getAlbum( resource ) );
    objectList << album;

    AmarokSharedPointer<Meta::NepomukAlbum> nalbum( AmarokSharedPointer<Meta::NepomukAlbum>::staticCast( album ) );
    if( !nalbum->isFilled() )
        nalbum->fill( queryResult.binding( NS_albumTitle ).literal().toString() );

    return true;
}

bool
NepomukParser::parseOne( Soprano::QueryResultIterator &queryResult,
                         Meta::GenreList &objectList )
{
    QString genre = queryResult.binding( NS_genre ).literal().toString();

    if( genre.isEmpty() ) return false;

    objectList << m_collection->cache()->getGenre( genre );
    return true;
}

bool
NepomukParser::parseOne( Soprano::QueryResultIterator &queryResult,
                         Meta::ComposerList &objectList )
{
    QUrl resource = queryResult.binding( NS_composer ).uri();
    if( resource.isEmpty() ) return false;

    Meta::ComposerPtr composer( m_collection->cache()->getComposer( resource ) );
    objectList << composer;

    AmarokSharedPointer<Meta::NepomukComposer> ncomposer( AmarokSharedPointer<Meta::NepomukComposer>::staticCast( composer ) );
    if( !ncomposer->isFilled() )
        ncomposer->fill( queryResult.binding( NS_composerName ).literal().toString() );

    return true;
}

bool
NepomukParser::parseOne( Soprano::QueryResultIterator &queryResult,
                         Meta::YearList &objectList )
{
    int year = queryResult.binding( NS_year ).literal().toString().toInt();

    if( !year ) return false;

    objectList << m_collection->cache()->getYear( year );
    return true;
}

bool
NepomukParser::parseOne( Soprano::QueryResultIterator &queryResult,
                         QStringList &objectList )
{
    for( int i = 0; i < queryResult.bindingCount(); ++i )
        objectList << queryResult.binding(i).literal().toString();
    return true;
}

bool
NepomukParser::parseOne( Soprano::QueryResultIterator &queryResult,
                         Meta::LabelList &objectList )
{
    QUrl labelResource = queryResult.binding( NS_tag ).uri();

    if( labelResource.isEmpty() )
        return false;

    objectList << m_collection->cache()->getLabel( labelResource );
    return true;
}

}
