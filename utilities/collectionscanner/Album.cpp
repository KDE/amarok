/***************************************************************************
 *   Copyright (C) 2010 Ralf Engels <ralf-engels@gmx.de>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "Album.h"
#include "Track.h"

#include <QDebug>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

// constructor is needed to put albums in a hash
CollectionScanner::Album::Album()
{}

CollectionScanner::Album::Album( const QString &name )
    : m_name( name )
{
}

CollectionScanner::Album::Album( QXmlStreamReader *reader )
{
    // improve scanner with skipCurrentElement as soon as Amarok requires Qt 4.6
    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringRef name = reader->name();

            if( name == "name" )
                m_name = reader->readElementText(QXmlStreamReader::SkipChildElements);
            else if( name == "track" )
                m_tracks.append( CollectionScanner::Track( reader ) );
            else if( name == "cover" )
                m_covers.append( reader->readElementText(QXmlStreamReader::SkipChildElements) );
            else
            {
                qDebug() << "Unexpected xml start element"<<name<<"in input";
                reader->skipCurrentElement();
            }
        }

        else if( reader->isEndElement() )
        {
            break;
        }
    }
}

void
CollectionScanner::Album::append( const Track &track )
{
    m_tracks.append( track );
}

void
CollectionScanner::Album::merge( const CollectionScanner::Album &otherAlbum )
{
    m_tracks.append( otherAlbum.m_tracks );
    m_covers.append( otherAlbum.m_covers );
}

void
CollectionScanner::Album::addCovers( const QStringList &covers )
{
    m_covers.append(covers);
}

QString
CollectionScanner::Album::name() const
{
    return m_name;
}

bool
CollectionScanner::Album::isCompilation() const
{
    bool isCompilation = false;
    bool isNoCompilation = false;
    foreach( const Track &track, m_tracks )
    {
        isCompilation   |= track.isCompilation();
        isNoCompilation |= track.isNoCompilation();
    }

    if( isCompilation && !isNoCompilation )
        return true;

    if( !isCompilation && isNoCompilation )
        return false;

    QString strArtist = artist();
    if( strArtist.compare("Various Artists", Qt::CaseInsensitive) == 0 ||
        strArtist.compare(QObject::tr("Various Artists"), Qt::CaseInsensitive) == 0 )
        return true;

    // Compilations where each is track is from a different artist
    // are often stored as one track per directory, e.g.
    // /artistA/compilation/track1
    // /artistB/compilation/track2
    //
    // this is how Amarok 1 (after using Organize Collection) and iTunes are storing
    // these albums on disc
    // the bad thing is that Amarok 1 (as far as I know) didn't set the id3 tags
    if( m_tracks.count() == 1 )
        return true;

    return false; // I still don't know.
}

bool
CollectionScanner::Album::isNoCompilation() const
{
    bool isCompilation = false;
    bool isNoCompilation = false;
    foreach( const Track &track, m_tracks )
    {
        isCompilation   |= track.isCompilation();
        isNoCompilation |= track.isNoCompilation();
    }

    if( !isCompilation && isNoCompilation )
        return true;

    if( isCompilation && !isNoCompilation )
        return false;

    QString strArtist = artist();
    if( strArtist.compare("Various Artists", Qt::CaseInsensitive) == 0 ||
        strArtist.compare(QObject::tr("Various Artists"), Qt::CaseInsensitive) == 0 )
        return false;

    return !strArtist.isEmpty(); // if we have a single album artist, then it's definitely no compilation.
}

QString
CollectionScanner::Album::artist() const
{
    // try to find the albumartist A: tracks must have the artist A or A feat. B (and variants)
    // if no albumartist could be found, it's a compilation
    QString artist;

    foreach( const Track &track, m_tracks )
    {
        // Use the normal artist as album artist if not set.
        QString trackArtist = track.albumArtist();
        if( trackArtist.isEmpty() )
            trackArtist = track.artist();

        // TODO
        if( artist.isEmpty() )
            artist = trackArtist;
        else
        {
            if( artist != trackArtist ) {

                // check for A feat. B //  TODO: There is an ArtistHelper for this
                if( !trackArtist.isEmpty() )
                {
                    if( trackArtist.contains( artist ) )
                        ; // we are still good
                    else if( artist.contains( trackArtist ) )
                        artist = trackArtist; // still ok
                    else
                        return QString();
                }
            }

        }
    }

    return artist;
}


QString
CollectionScanner::Album::cover() const
{
    // we prefere covers included in tracks.
    // At least we know exactly that they really belong to the album
    foreach( const Track &track, m_tracks )
    {
        // IMPROVEMENT: skip covers that have a strange aspect ratio or are
        // unrealistically small, or do not resolve to a valid image
        if( track.hasCover() )
            return track.uniqueid();
    }

    // ok. Now we have to figure out which of the cover images is
    // the best.
    QString bestCover;
    int     bestRating = -1;
    qint64  bestSize = 0;

    foreach( const QString &cover, m_covers )
    {
        int rating = 0;

        if( cover.contains( "front", Qt::CaseInsensitive ) ||
            cover.contains( QObject::tr( "front", "Front cover of an album" ), Qt::CaseInsensitive ) )
            rating += 2;

        if( cover.contains( "cover", Qt::CaseInsensitive ) ||
            cover.contains( QObject::tr( "cover", "(Front) Cover of an album" ), Qt::CaseInsensitive ) )
            rating += 2;

        //next: try "folder" (some applications apparently use this)
        //using compare and not contains to not hit "Folder-Back" or something.
        if( cover.compare( "folder", Qt::CaseInsensitive ) == 0)
            rating += 1;

        QFileInfo info( cover );
        if( (rating == bestRating && info.size() > bestSize) ||
            (rating > bestRating) )
        {
            bestCover = cover;
            bestRating = rating;
            bestSize = info.size();
        }
    }

    return bestCover;
}

QList<CollectionScanner::Track>
CollectionScanner::Album::tracks() const
{
    return m_tracks;
}

#ifdef UTILITIES_BUILD
void
CollectionScanner::Album::toXml( QXmlStreamWriter *writer ) const
{

    writer->writeTextElement( "name", m_name );

    foreach( const CollectionScanner::Track &track, m_tracks )
    {
        writer->writeStartElement( "track" );
        track.toXml( writer );
        writer->writeEndElement();
    }

    foreach( const QString &str, m_covers )
    {
        writer->writeTextElement( "cover", str );
    }
}
#endif // UTILITIES_BUILD

