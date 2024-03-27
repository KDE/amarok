/***************************************************************************
 *   Copyright (C) 2003-2005 Max Howell <max.howell@methylblue.com>        *
 *             (C) 2003-2010 Mark Kretschmann <kretschmann@kde.org>        *
 *             (C) 2005-2007 Alexandre Oliveira <aleprj@gmail.com>         *
 *             (C) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>         *
 *             (C) 2008-2009 Jeff Mitchell <mitchell@kde.org>              *
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

#include "Directory.h"

#include "collectionscanner/ScanningState.h"
#include "collectionscanner/Track.h"
#include "collectionscanner/utils.h"

#include <QDebug>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QFileInfo>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

CollectionScanner::Directory::Directory( const QString &path,
                                         CollectionScanner::ScanningState *state,
                                         bool skip )
    : m_ignored( false )
{
    m_path = path;
    m_rpath = QDir::current().relativeFilePath( path );
    m_mtime = QFileInfo( path ).lastModified().toSecsSinceEpoch();
    m_skipped = skip;

    if( m_skipped )
        return;

    QDir dir( path );
    if( dir.exists( QStringLiteral("fmps_ignore") ) )
    {
        m_ignored = true;
        return;
    }

    QStringList validImages;
    validImages << QStringLiteral("jpg") << QStringLiteral("png") << QStringLiteral("gif") << QStringLiteral("jpeg") << QStringLiteral("bmp") << QStringLiteral("svg") << QStringLiteral("xpm");
    QStringList validPlaylists;
    validPlaylists << QStringLiteral("m3u") << QStringLiteral("pls") << QStringLiteral("xspf");

    // --- check if we were restarted and failed at a file
    QStringList badFiles;

    if( state->lastDirectory() == path )
    {
        badFiles << state->badFiles();
        QString lastFile = state->lastFile();
        if( !lastFile.isEmpty() )
        {
            badFiles << state->lastFile();
            state->setBadFiles( badFiles );
        }
    }
    else
        state->setLastDirectory( path );
    state->setLastFile( QString() ); // reset so we don't add a leftover file

    dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
    QFileInfoList fileInfos = dir.entryInfoList();

    foreach( const QFileInfo &fi, fileInfos )
    {
        if( !fi.exists() )
            continue;

        const QFileInfo &f = fi.isSymLink() ? QFileInfo( fi.symLinkTarget() ) : fi;

        if( badFiles.contains( f.absoluteFilePath() ) )
            continue;

        const QString suffix  = fi.suffix().toLower();
        const QString filePath = f.absoluteFilePath();

        // -- cover image ?
        if( validImages.contains( suffix ) )
            m_covers.append( filePath );

        // -- playlist ?
        else if( validPlaylists.contains( suffix ) )
            m_playlists.append( CollectionScanner::Playlist( filePath ) );

        // -- audio track ?
        else
        {
            // remember the last file before it get's dangerous. Before starting taglib
            state->setLastFile( f.absoluteFilePath() );

            CollectionScanner::Track *newTrack = new CollectionScanner::Track( filePath, this );
            if( newTrack->isValid() )
                m_tracks.append( newTrack );
            else
                delete newTrack;
        }
    }
}

CollectionScanner::Directory::Directory( QXmlStreamReader *reader )
    : m_mtime( 0 )
    , m_skipped( false )
    , m_ignored( false )
{
    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringRef name = reader->name();
            if( name == QLatin1String("path") )
                m_path = reader->readElementText(QXmlStreamReader::SkipChildElements);
            else if( name == QLatin1String("rpath") )
                m_rpath = reader->readElementText(QXmlStreamReader::SkipChildElements);
            else if( name == QLatin1String("mtime") )
                m_mtime = reader->readElementText(QXmlStreamReader::SkipChildElements).toUInt();
            else if( name == QLatin1String("cover") )
                m_covers.append(reader->readElementText(QXmlStreamReader::SkipChildElements));
            else if( name == QLatin1String("skipped") )
            {
                m_skipped = true;
                reader->skipCurrentElement();
            }
            else if( name == QLatin1String("ignored") )
            {
                m_ignored = true;
                reader->skipCurrentElement();
            }
            else if( name == QLatin1String("track") )
                m_tracks.append( new CollectionScanner::Track( reader, this ) );
            else if( name == QLatin1String("playlist") )
                m_playlists.append( CollectionScanner::Playlist( reader ) );
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

CollectionScanner::Directory::~Directory()
{
    foreach( CollectionScanner::Track *track, m_tracks )
        delete track;
}

QString
CollectionScanner::Directory::path() const
{
    return m_path;
}

QString
CollectionScanner::Directory::rpath() const
{
    return m_rpath;
}

uint
CollectionScanner::Directory::mtime() const
{
    return m_mtime;
}

bool
CollectionScanner::Directory::isSkipped() const
{
    return m_skipped;
}

const QStringList&
CollectionScanner::Directory::covers() const
{
    return m_covers;
}

const QList<CollectionScanner::Track *>&
CollectionScanner::Directory::tracks() const
{
    return m_tracks;
}

const QList<CollectionScanner::Playlist>&
CollectionScanner::Directory::playlists() const
{
    return m_playlists;
}

void
CollectionScanner::Directory::toXml( QXmlStreamWriter *writer ) const
{
    writer->writeTextElement( QStringLiteral("path"), escapeXml10(m_path) );
    writer->writeTextElement( QStringLiteral("rpath"), escapeXml10(m_rpath) );
    writer->writeTextElement( QStringLiteral("mtime"), QString::number( m_mtime ) );
    if( m_skipped )
        writer->writeEmptyElement( QStringLiteral("skipped") );
    if( m_ignored )
        writer->writeEmptyElement( QStringLiteral("ignored") );

    foreach( const QString &cover, m_covers )
    {
        writer->writeTextElement( QStringLiteral("cover"), escapeXml10(cover) );
    }
    foreach( CollectionScanner::Track *track, m_tracks )
    {
        writer->writeStartElement( QStringLiteral("track") );
        track->toXml( writer );
        writer->writeEndElement();
    }

    foreach( const CollectionScanner::Playlist &playlist, m_playlists )
    {
        writer->writeStartElement( QStringLiteral("playlist") );
        playlist.toXml( writer );
        writer->writeEndElement();
    }
}
