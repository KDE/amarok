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
#include "core/support/Debug.h"

#include <QFile>
#include <QMutexLocker>
#include <QTemporaryFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

using namespace StatSyncing;

RhythmboxProvider::RhythmboxProvider( const QVariantMap &config,
                                      ImporterManager *importer )
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
    return Meta::valRating | Meta::valLastPlayed | Meta::valPlaycount;
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
    QFile dbFile( m_config.value( "dbPath" ).toString() );
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

    Meta::FieldHash metadata;
    QString currentArtist;
    QString location;

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
                metadata.insert( Meta::valPlaycount, readValue( xml ) );
            else if( xml.name() == "location" )
                location = readValue( xml );
            else
                xml.skipCurrentElement();
        }
        else
            xml.skipCurrentElement();
    }

    if( !byArtist.isEmpty() && currentArtist == byArtist )
    {
        RhythmboxTrack *track = new RhythmboxTrack( location, metadata );
        connect( track, &RhythmboxTrack::commitCalled,
                 this, &RhythmboxProvider::trackUpdated, Qt::DirectConnection );
        m_artistTracks << TrackPtr( track );
    }
    else if( byArtist.isEmpty() )
        m_artists << currentArtist;
}

QString
RhythmboxProvider::readValue( QXmlStreamReader &xml )
{
    return xml.readElementText();
}

void
RhythmboxProvider::writeSong( QXmlStreamReader &reader, QXmlStreamWriter &writer,
                              const QMap<QString, Meta::FieldHash> &dirtyData )
{
    Q_ASSERT( reader.isStartElement() && reader.name() == "entry" );

    Meta::FieldHash metadata;
    QString location;

    writer.writeCurrentToken( reader );
    while( !reader.isEndElement() || reader.name() != "entry" )
    {
        reader.readNext();

        if( reader.error() )
        {
            warning() << __PRETTY_FUNCTION__ << "Error reading song:"
                      << reader.errorString();
            return;
        }

        if( reader.isWhitespace() )
            continue; // autoformat will deal with whitespace for us

        if( reader.isStartElement() )
        {
            if( reader.name() == "rating" )
                metadata.insert( Meta::valRating, readValue( reader ) );
            else if( reader.name() == "last-played" )
                metadata.insert( Meta::valLastPlayed, readValue( reader ) );
            else if( reader.name() == "play-count" )
                metadata.insert( Meta::valPlaycount, readValue( reader ) );
            else if( reader.name() == "location" )
            {
                location = readValue( reader );
                writer.writeTextElement( "location", location );
            }
            else
                writer.writeCurrentToken( reader );
        }
        else if( !reader.isEndElement() || reader.name() != "entry" )
            writer.writeCurrentToken( reader );
    }

    // Override read statistics
    if( dirtyData.contains( location ) )
        metadata = dirtyData.value( location );

    if( metadata.value( Meta::valRating ).toInt() != 0 )
        writer.writeTextElement( "rating", metadata.value( Meta::valRating ).toString() );
    if( metadata.value( Meta::valLastPlayed ).toUInt() != 0 )
        writer.writeTextElement( "last-played",
                                 metadata.value( Meta::valLastPlayed ).toString() );
    if( metadata.value( Meta::valPlaycount ).toInt() != 0 )
        writer.writeTextElement( "play-count",
                                 metadata.value( Meta::valPlaycount ).toString() );

    writer.writeCurrentToken( reader );
}

void
RhythmboxProvider::trackUpdated( const QString &location,
                                 const Meta::FieldHash &statistics )
{
    QMutexLocker lock( &m_dirtyMutex );
    m_dirtyData.insert( location, statistics );
}

void
RhythmboxProvider::commitTracks()
{
    QMutexLocker lock( &m_dirtyMutex );
    if( m_dirtyData.empty() )
        return;

    QMap<QString, Meta::FieldHash> dirtyData;
    dirtyData.swap( m_dirtyData );

    QFile dbFile( m_config.value( "dbPath" ).toString() );
    if( !dbFile.open( QIODevice::ReadOnly ) )
    {
        warning() << __PRETTY_FUNCTION__ << dbFile.fileName() << "is not readable";
        return;
    }

    QTemporaryFile tmpFile;
    if( !tmpFile.open() )
    {
        warning() << __PRETTY_FUNCTION__ << tmpFile.fileName() << "is not writable";
        return;
    }

    QXmlStreamReader reader( &dbFile );
    QXmlStreamWriter writer( &tmpFile );
    writer.setAutoFormatting( true );
    writer.setAutoFormattingIndent( 2 );

    while( !reader.atEnd() )
    {
        reader.readNext();

        if( reader.error() )
        {
            warning() << __PRETTY_FUNCTION__ << "Error reading" << dbFile.fileName();
            return;
        }

        if( reader.isStartElement() && reader.name() == QStringLiteral("entry") &&
            reader.attributes().value( "type" ) == QStringLiteral("song") )
            writeSong( reader, writer, dirtyData );
        else if( reader.isStartDocument() ) // writeCurrentToken doesn't add 'standalone'
            writer.writeStartDocument( reader.documentVersion().toString(),
                                       reader.isStandaloneDocument() );
        else
            writer.writeCurrentToken( reader );
    }

    const QString dbName = dbFile.fileName();
    QFile::remove( dbName + ".bak" );
    dbFile.rename( dbName + ".bak" );
    tmpFile.copy( dbName );
}
