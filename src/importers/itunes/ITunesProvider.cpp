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
#include <QTemporaryFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

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
    return Meta::valRating | Meta::valPlaycount;
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
    QFile dbFile( m_config.value( QStringLiteral("dbPath") ).toString() );
    if( dbFile.open( QIODevice::ReadOnly ) )
    {
        QXmlStreamReader xml( &dbFile );
        if( xml.readNextStartElement() )
        {
            if( xml.name() == QStringLiteral("plist") && xml.attributes().value("version") == QStringLiteral("1.0") )
                readPlist( xml, byArtist );
            else
                xml.raiseError( QStringLiteral("the database is ill-formed or version unsupported") );
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
    Q_ASSERT( xml.isStartElement() && xml.name() == QStringLiteral("plist") );
    xml.readNextStartElement();
    Q_ASSERT( xml.isStartElement() && xml.name() == QStringLiteral("dict") );

    while( xml.readNextStartElement() )
    {
        if( xml.name() == QStringLiteral("key") )
        {
            if( xml.readElementText() == QStringLiteral("Tracks") )
                readTracks( xml, byArtist );
        }
        else
            xml.skipCurrentElement();
    }
}

void
ITunesProvider::readTracks( QXmlStreamReader &xml, const QString &byArtist )
{
    Q_ASSERT( xml.isEndElement() && xml.name() == QStringLiteral("key") );
    xml.readNextStartElement();
    Q_ASSERT( xml.isStartElement() && xml.name() == QStringLiteral("dict") );

    while( xml.readNextStartElement() )
        readTrack( xml, byArtist );
}

void
ITunesProvider::readTrack( QXmlStreamReader &xml, const QString &byArtist )
{
    Q_ASSERT( xml.isStartElement() && xml.name() == QStringLiteral("key") );
    xml.skipCurrentElement();
    xml.readNextStartElement();
    Q_ASSERT( xml.isStartElement() && xml.name() == QStringLiteral("dict") );

    Meta::FieldHash metadata;
    QString currentArtist;
    int trackId = -1;

    while( xml.readNextStartElement() )
    {
        // We're only interested in this track if it's by right artist, or if we haven't
        // found the artist yet
        if( xml.name() == QStringLiteral("key")
                 && ( currentArtist.isEmpty() || currentArtist == byArtist ) )
        {
            const QString type = xml.readElementText();

            // If byArtist param is not set, we're only interested in track Artist
            if( byArtist.isEmpty() )
            {
                if( type == QStringLiteral("Artist") )
                    currentArtist = readValue( xml );
            }
            else
            {
                if( type == QStringLiteral("Track ID") )
                {
                    trackId = readValue( xml ).toInt();
                }
                else if( type == QStringLiteral("Name") )
                    metadata.insert( Meta::valTitle, readValue( xml ) );
                else if( type == QStringLiteral("Artist") )
                {
                    currentArtist = readValue( xml );
                    metadata.insert( Meta::valArtist, currentArtist );
                }
                else if( type == QStringLiteral("Album") )
                    metadata.insert( Meta::valAlbum, readValue( xml ) );
                else if( type == QStringLiteral("Composer") )
                    metadata.insert( Meta::valComposer, readValue( xml ) );
                else if( type == QStringLiteral("Year") )
                    metadata.insert( Meta::valYear, readValue( xml ) );
                else if( type == QStringLiteral("Track Number") )
                    metadata.insert( Meta::valTrackNr, readValue( xml ) );
                else if( type == QStringLiteral("Disc Number") )
                    metadata.insert( Meta::valDiscNr, readValue( xml ) );
                else if( type == QStringLiteral("Rating") )
                    metadata.insert( Meta::valRating, readValue( xml ) );
                else if( type == QStringLiteral("Play Date UTC") )
                    metadata.insert( Meta::valLastPlayed, readValue( xml ) );
                else if( type == QStringLiteral("Play Count") )
                    metadata.insert( Meta::valPlaycount, readValue( xml ) );
            }
        }
        else
            xml.skipCurrentElement();
    }

    if( !byArtist.isEmpty() && currentArtist == byArtist && trackId != -1 )
    {
        ITunesTrack *track = new ITunesTrack( trackId, metadata );
        connect( track, &ITunesTrack::commitCalled,
                 this, &ITunesProvider::trackUpdated, Qt::DirectConnection );
        m_artistTracks << TrackPtr( track );
    }
    else if( byArtist.isEmpty() )
        m_artists << currentArtist;
}

QString
ITunesProvider::readValue( QXmlStreamReader &xml )
{
    Q_ASSERT( xml.isEndElement() && xml.name() == QStringLiteral("key") );
    xml.readNextStartElement();
    Q_ASSERT( xml.isStartElement() );
    return xml.readElementText();
}

void
ITunesProvider::writeTracks( QXmlStreamReader &reader, QXmlStreamWriter &writer,
                             const QMap<int, Meta::FieldHash> &dirtyData )
{
    int dictDepth = 0;
    while( !reader.isEndElement() || reader.name() != QStringLiteral("dict") || dictDepth != 0 )
    {
        reader.readNext();

        if( reader.error() )
        {
            warning() << __PRETTY_FUNCTION__ << reader.errorString();
            return;
        }

        writer.writeCurrentToken( reader );

        if( reader.isStartElement() && reader.name() == QStringLiteral("key") && dictDepth == 1 )
        {
            int trackId = reader.readElementText().toInt();
            writer.writeCharacters( QString::number( trackId ) );
            writer.writeCurrentToken( reader );

            if( dirtyData.contains( trackId ) )
                writeTrack( reader, writer, dirtyData.value( trackId ) );
        }
        else if( reader.isStartElement() && reader.name() == QStringLiteral("dict") )
            ++dictDepth;
        else if( reader.isEndElement() && reader.name() == QStringLiteral("dict") )
            --dictDepth;
    }
}

void
ITunesProvider::writeTrack( QXmlStreamReader &reader, QXmlStreamWriter &writer,
                            const Meta::FieldHash &dirtyData )
{
    QString keyWhitespace;
    QString lastWhitespace;

    while( !reader.isEndElement() || reader.name() != QStringLiteral("dict") )
    {
        reader.readNext();

        if( reader.error() )
        {
            warning() << __PRETTY_FUNCTION__ << reader.errorString();
            return;
        }

        if( reader.isWhitespace() ) // control whitespace, we want nicely formatted file
        {
            lastWhitespace = reader.text().toString();
        }
        else if( reader.isStartElement() && reader.name() == QStringLiteral("key") )
        {
            keyWhitespace = lastWhitespace;
            const QString type = reader.readElementText();

            if( type == QStringLiteral("Rating") || type == QStringLiteral("Play Count") )
            {
                reader.readNextStartElement(); // <integer>
                reader.skipCurrentElement();
            }
            else
            {
                writer.writeCharacters( lastWhitespace );
                writer.writeTextElement( "key", type );
            }

            lastWhitespace.clear();
        }
        else if( !reader.isEndElement() || reader.name() != QStringLiteral("dict") )
        {
            writer.writeCharacters( lastWhitespace );
            writer.writeCurrentToken( reader );
            lastWhitespace.clear();
        }
    }

    if( const int rating = dirtyData.value( Meta::valRating ).toInt() )
    {
        writer.writeCharacters( keyWhitespace );
        writer.writeTextElement( "key", "Rating" );
        writer.writeTextElement( "integer", QString::number( rating ) );
    }
    if( const int playCount = dirtyData.value( Meta::valPlaycount ).toInt() )
    {
        writer.writeCharacters( keyWhitespace );
        writer.writeTextElement( "key", "Play Count" );
        writer.writeTextElement( "integer", QString::number( playCount ) );
    }

    writer.writeCharacters( lastWhitespace );
    writer.writeCurrentToken( reader );
    reader.readNext();
}

void
ITunesProvider::trackUpdated( const int trackId, const Meta::FieldHash &statistics )
{
    QMutexLocker lock( &m_dirtyMutex );
    m_dirtyData.insert( trackId, statistics );
}

void
ITunesProvider::commitTracks()
{
    QMutexLocker lock( &m_dirtyMutex );
    if( m_dirtyData.empty() )
        return;

    QMap<int, Meta::FieldHash> dirtyData;
    dirtyData.swap( m_dirtyData );

    QFile dbFile( m_config.value( QStringLiteral("dbPath") ).toString() );
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

    while( !reader.atEnd() )
    {
        reader.readNext();

        if( reader.error() )
        {
            warning() << __PRETTY_FUNCTION__ << "Error reading" << dbFile.fileName();
            return;
        }

        if( reader.isStartElement() && reader.name() == QStringLiteral( "key" ) )
        {
            const QString text = reader.readElementText();
            writer.writeTextElement( "key", text );

            if( text == QStringLiteral("Tracks") )
                writeTracks( reader, writer, dirtyData );
        }
        else if( reader.isStartDocument() )
            writer.writeStartDocument( reader.documentVersion().toString(),
                                       reader.isStandaloneDocument() );
        else
            writer.writeCurrentToken( reader );
    }

    const QString dbName = dbFile.fileName();
    QFile::remove( dbName + QStringLiteral(".bak") );
    dbFile.rename( dbName + QStringLiteral(".bak") );
    tmpFile.copy( dbName );
}
