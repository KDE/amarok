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

#ifdef UTILITIES_BUILD

CollectionScanner::Directory::Directory( const QString &path,
                                         CollectionScanner::ScanningState *state,
                                         bool skip )
    : m_ignored( false )
{
    m_path = path;
    m_rpath = QDir::current().relativeFilePath( path );
    m_mtime = QFileInfo( path ).lastModified().toTime_t();
    m_skipped = skip;

    if( m_skipped )
        return;

    QDir dir( path );
    if( dir.exists( "fmps_ignore" ) )
    {
        m_ignored = true;
        return;
    }

    QStringList validImages;
    validImages << "jpg" << "png" << "gif" << "jpeg" << "bmp" << "svg" << "xpm";
    QStringList validPlaylists;
    validPlaylists << "m3u" << "pls" << "xspf";

    // --- check if we were restarted and failed at a file
    QStringList badFiles;

    if( state->lastDirectory() == path )
    {
        badFiles << state->badFiles();
        badFiles << state->lastFile();

        state->setBadFiles( badFiles );
    }
    else
    {
        state->setLastDirectory( path );
        state->setLastFile( QString() );
        state->setBadFiles( badFiles );
    }

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
#endif // UTILITIES_BUILD

    CollectionScanner::Directory::Directory( QXmlStreamReader *reader )
    : m_mtime( 0 )
    , m_skipped( false )
    , m_ignored( false )
{
    // improve scanner with skipCurrentElement as soon as Amarok requires Qt 4.6
    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringRef name = reader->name();
            if( name == "path" )
                m_path = reader->readElementText(QXmlStreamReader::SkipChildElements);
            else if( name == "rpath" )
                m_rpath = reader->readElementText(QXmlStreamReader::SkipChildElements);
            else if( name == "mtime" )
                m_mtime = reader->readElementText(QXmlStreamReader::SkipChildElements).toUInt();
            else if( name == "cover" )
                m_covers.append(reader->readElementText(QXmlStreamReader::SkipChildElements));
            else if( name == "skipped" )
            {
                m_skipped = true;
                reader->skipCurrentElement();
            }
            else if( name == "ignored" )
            {
                m_ignored = true;
                reader->skipCurrentElement();
            }
            else if( name == "track" )
                m_tracks.append( new CollectionScanner::Track( reader, this ) );
            else if( name == "playlist" )
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

#ifdef UTILITIES_BUILD
void
CollectionScanner::Directory::toXml( QXmlStreamWriter *writer ) const
{
    writer->writeTextElement( "path", m_path );
    writer->writeTextElement( "rpath", m_rpath );
    writer->writeTextElement( "mtime", QString::number( m_mtime ) );
    if( m_skipped )
        writer->writeEmptyElement( "skipped" );
    if( m_ignored )
        writer->writeEmptyElement( "ignored" );

    foreach( const QString &cover, m_covers )
    {
        writer->writeTextElement( "cover", cover );
    }
    foreach( CollectionScanner::Track *track, m_tracks )
    {
        writer->writeStartElement( QLatin1String("track") );
        track->toXml( writer );
        writer->writeEndElement();
    }

    foreach( const CollectionScanner::Playlist &playlist, m_playlists )
    {
        writer->writeStartElement( "playlist" );
        playlist.toXml( writer );
        writer->writeEndElement();
    }
}
#endif // UTILITIES_BUILD
