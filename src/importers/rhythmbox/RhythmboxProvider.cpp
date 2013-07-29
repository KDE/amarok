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

#include "RhythmboxProvider.h"

#include "RhythmboxTrack.h"
#include "MetaValues.h"
#include "core/support/Debug.h"

#include <QFile>
#include <QXmlStreamReader>

using namespace StatSyncing;

RhythmboxProvider::RhythmboxProvider( const QVariantMap &config, ImporterManager *importer )
    : ImporterProvider( config, importer )
{
}

RhythmboxProvider::~RhythmboxProvider()
{
}

qint64
RhythmboxProvider::reliableTrackMetaData() const
{
    return Meta::valTitle | Meta::valArtist | Meta::valAlbum | Meta::valTrackNr
            | Meta::valDiscNr;
}

qint64
RhythmboxProvider::writableTrackStatsData() const
{
    //TODO: Write capabilities
    return 0;
}

QSet<QString>
RhythmboxProvider::artists()
{
    readXml( QString() );
    QSet<QString> result;
    result.swap( m_artists );
    return result;
}

TrackList
RhythmboxProvider::artistTracks( const QString &artistName )
{
    readXml( artistName );
    TrackList result;
    result.swap( m_artistTracks );
    return result;
}

void
RhythmboxProvider::readXml( const QString &byArtist )
{
    QFile dbFile( m_config["dbPath"].toString() );
    if( dbFile.open( QIODevice::ReadOnly ) )
    {
        QXmlStreamReader xml( &dbFile );
        if( xml.readNextStartElement() )
        {
            if( xml.name() == "rhythmdb" )
            {
                if( xml.attributes().value("version") != "1.8" )
                    warning() << __PRETTY_FUNCTION__ << "unsupported database version";

                readRhythmdb( xml, byArtist );
            }
            else
                xml.raiseError( "the database file is ill-formatted" );
        }

        if( xml.hasError() )
            warning() << "There was an error reading" << dbFile.fileName() << ":"
                      << xml.errorString();
    }
    else
        warning() << __PRETTY_FUNCTION__ << "couldn't open" << dbFile.fileName();
}

void
RhythmboxProvider::readRhythmdb( QXmlStreamReader &xml, const QString &byArtist )
{
    Q_ASSERT( xml.isStartElement() && xml.name() == "rhythmdb" );

    while( xml.readNextStartElement() )
    {
        if( xml.name() == "entry" && xml.attributes().value( "type" ) == "song" )
            readSong( xml, byArtist );
        else
            xml.skipCurrentElement();
    }
}

void
RhythmboxProvider::readSong( QXmlStreamReader &xml, const QString &byArtist )
{
    Q_ASSERT( xml.isStartElement() && xml.name() == "entry" );

    QMap<qint64, QString> metadata;
    QString currentArtist;

    while( xml.readNextStartElement() )
    {
        if( byArtist.isEmpty() && currentArtist.isEmpty() )
        {
            if( xml.name() == "artist" )
                currentArtist = readValue( xml );
            else
                xml.skipCurrentElement();
        }
        else if( currentArtist.isEmpty() || currentArtist == byArtist )
        {
            if( xml.name() == "title" )
                metadata.insert( Meta::valTitle, readValue( xml ) );
            else if( xml.name() == "artist" )
            {
                currentArtist = readValue( xml );
                metadata.insert( Meta::valArtist, currentArtist );
            }
            else if( xml.name() == "album" )
                metadata.insert( Meta::valAlbum, readValue( xml ) );
            else if( xml.name() == "track-number" )
                metadata.insert( Meta::valTrackNr, readValue( xml ) );
            else if( xml.name() == "disc-number" )
                metadata.insert( Meta::valDiscNr, readValue( xml ) );
            else if( xml.name() == "rating" )
                metadata.insert( Meta::valRating, readValue( xml ) );
            else if( xml.name() == "last-played" )
                metadata.insert( Meta::valLastPlayed, readValue( xml ) );
            else if( xml.name() == "play-count" )
                metadata.insert( Meta::valLastPlayed, readValue( xml ) );
            else
                xml.skipCurrentElement();
        }
        else
            xml.skipCurrentElement();
    }

    if( !byArtist.isEmpty() && currentArtist == byArtist )
        m_artistTracks << TrackPtr( new RhythmboxTrack( metadata ) );
    else if( byArtist.isEmpty() )
        m_artists << currentArtist;
}

QString
RhythmboxProvider::readValue( QXmlStreamReader &xml )
{
    return xml.readElementText();
}
