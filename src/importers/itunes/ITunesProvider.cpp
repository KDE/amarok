/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "ITunesProvider.h"

#include "ITunesTrack.h"
#include "core/support/Debug.h"

#include <QFile>
#include <QXmlStreamReader>

using namespace StatSyncing;

ITunesProvider::ITunesProvider( const QVariantMap &config, ImporterManager *importer )
    : ImporterProvider( config, importer )
{
}

ITunesProvider::~ITunesProvider()
{
}

qint64
ITunesProvider::reliableTrackMetaData() const
{
    return Meta::valTitle | Meta::valArtist | Meta::valAlbum | Meta::valComposer
            | Meta::valYear | Meta::valTrackNr | Meta::valDiscNr;
}

qint64
ITunesProvider::writableTrackStatsData() const
{
    //TODO: Write capabilities
    return 0;
}

QSet<QString>
ITunesProvider::artists()
{
    readXml( QString() );
    QSet<QString> result;
    result.swap( m_artists );
    return result;
}

TrackList
ITunesProvider::artistTracks( const QString &artistName )
{
    readXml( artistName );
    TrackList result;
    result.swap( m_artistTracks );
    return result;
}

void
ITunesProvider::readXml( const QString &byArtist )
{
    QFile dbFile( m_config.value( "dbPath" ).toString() );
    if( dbFile.open( QIODevice::ReadOnly ) )
    {
        QXmlStreamReader xml( &dbFile );
        if( xml.readNextStartElement() )
        {
            if( xml.name() == "plist" && xml.attributes().value("version") == "1.0" )
                readPlist( xml, byArtist );
            else
                xml.raiseError( "the database is ill-formed or version unsupported" );
        }

        if( xml.hasError() )
            warning() << "There was an error reading" << dbFile.fileName() << ":"
                      << xml.errorString();
    }
    else
        warning() << __PRETTY_FUNCTION__ << "couldn't open" << dbFile.fileName();
}

void
ITunesProvider::readPlist( QXmlStreamReader &xml, const QString &byArtist )
{
    Q_ASSERT( xml.isStartElement() && xml.name() == "plist" );
    xml.readNextStartElement();
    Q_ASSERT( xml.isStartElement() && xml.name() == "dict" );

    while( xml.readNextStartElement() )
    {
        if( xml.name() == "key" )
        {
            if( xml.readElementText() == "Tracks" )
                readTracks( xml, byArtist );
        }
        else
            xml.skipCurrentElement();
    }
}

void
ITunesProvider::readTracks( QXmlStreamReader &xml, const QString &byArtist )
{
    Q_ASSERT( xml.isEndElement() && xml.name() == "key" );
    xml.readNextStartElement();
    Q_ASSERT( xml.isStartElement() && xml.name() == "dict" );

    while( xml.readNextStartElement() )
        readTrack( xml, byArtist );
}

void
ITunesProvider::readTrack( QXmlStreamReader &xml, const QString &byArtist )
{
    Q_ASSERT( xml.isStartElement() && xml.name() == "key" );
    xml.skipCurrentElement();
    xml.readNextStartElement();
    Q_ASSERT( xml.isStartElement() && xml.name() == "dict" );

    Meta::FieldHash metadata;
    QString currentArtist;

    while( xml.readNextStartElement() )
    {
        // We're only interested in this track if it's by right artist, or if we haven't
        // found the artist yet
        if( xml.name() == "key"
                 && ( currentArtist.isEmpty() || currentArtist == byArtist ) )
        {
            const QString type = xml.readElementText();

            // If byArtist param is not set, we're only interested in track Artist
            if( byArtist.isEmpty() )
            {
                if( type == "Artist" )
                    currentArtist = readValue( xml );
            }
            else
            {
                if( type == "Name" )
                    metadata.insert( Meta::valTitle, readValue( xml ) );
                else if( type == "Artist" )
                {
                    currentArtist = readValue( xml );
                    metadata.insert( Meta::valArtist, currentArtist );
                }
                else if( type == "Album" )
                    metadata.insert( Meta::valAlbum, readValue( xml ) );
                else if( type == "Composer" )
                    metadata.insert( Meta::valComposer, readValue( xml ) );
                else if( type == "Year" )
                    metadata.insert( Meta::valYear, readValue( xml ) );
                else if( type == "Track Number" )
                    metadata.insert( Meta::valTrackNr, readValue( xml ) );
                else if( type == "Disc Number" )
                    metadata.insert( Meta::valDiscNr, readValue( xml ) );
                else if( type == "Rating" )
                    metadata.insert( Meta::valRating, readValue( xml ) );
                else if( type == "Play Date UTC" )
                    metadata.insert( Meta::valLastPlayed, readValue( xml ) );
                else if( type == "Play Count" )
                    metadata.insert( Meta::valPlaycount, readValue( xml ) );
            }
        }
        else
            xml.skipCurrentElement();
    }

    if( !byArtist.isEmpty() && currentArtist == byArtist )
        m_artistTracks << TrackPtr( new ITunesTrack( metadata ) );
    else if( byArtist.isEmpty() )
        m_artists << currentArtist;
}

QString
ITunesProvider::readValue( QXmlStreamReader &xml )
{
    Q_ASSERT( xml.isEndElement() && xml.name() == "key" );
    xml.readNextStartElement();
    Q_ASSERT( xml.isStartElement() );
    return xml.readElementText();
}
