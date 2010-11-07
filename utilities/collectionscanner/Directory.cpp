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

#include "CollectionScanner.h"
#include "Directory.h"

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

    QStringList covers;

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
        {
            covers += filePath;
        }

        // -- playlist ?
        else if( validPlaylists.contains( suffix ) )
        {
            m_playlists.append( CollectionScanner::Playlist( filePath ) );
        }

        // -- audio track ?
        else
        {
            // remember the last file before it get's dangerous. Before starting taglib
            state->setLastFile( f.absoluteFilePath() );

            CollectionScanner::Track newTrack( filePath );
            if( newTrack.isValid() )
            {
                // remember. the default is that tracks are not in a compilation
                QString aArtist = newTrack.albumArtist();
                if( aArtist.isEmpty() )
                    aArtist = newTrack.artist();
                if( newTrack.isCompilation() )
                    aArtist.clear();

                // hash of a hash
                if( ! m_albums.contains( newTrack.album() ) )
                    m_albums.insert( newTrack.album(),
                                     QHash<QString, CollectionScanner::Album>() );

                if( ! m_albums.value( newTrack.album() ).contains( aArtist ) )
                    m_albums[ newTrack.album() ].insert( aArtist,
                                                         CollectionScanner::Album( newTrack.album() ) );

                m_albums[ newTrack.album() ][aArtist].append( newTrack );
            }
        }
    }

    // -- if all tracks in the directory are in the same album but have different artists,
    //    then it's definitely a collection
    if( m_albums.count() == 1 ) // only one album name
    {
        QHash<QString, CollectionScanner::Album> albums = m_albums.values().first();
        if( albums.count() > 1 ) // several artists
        {
            QHash<QString, Album>::const_iterator j = albums.constBegin();

            // - create the new album and copy it
            CollectionScanner::Album newAlbum( j->name() );
            while (j != albums.constEnd())
            {
                newAlbum.merge( *j );
                ++j;
            }

            // - upated the albums
            m_albums.clear();
            m_albums.insert( newAlbum.name(),
                             QHash<QString, CollectionScanner::Album>() );
            m_albums[ newAlbum.name() ].insert( newAlbum.artist(), newAlbum );
        }
    }

    // -- use the directory name as album name if not a single track in the director
    //    has an album name
    // Note: this is just a heuristic.
    if( m_albums.count() == 1 ) // only one album name
    {
        // remember again. m_albums is a hash of a hash
        QHash<QString, CollectionScanner::Album> albums = m_albums.values().first();
        if( albums.count() == 1 ) // only one artist (or no artist at all)
        {
            CollectionScanner::Album oldAlbum = albums.values().first();
            if( oldAlbum.name().isEmpty() &&
                oldAlbum.tracks().count() < 20 ) // more than 20 songs make it unlikely that it's an album
            {
                // - create the new album and copy it
                CollectionScanner::Album newAlbum( QDir( path ).dirName() );
                newAlbum.merge( oldAlbum );

                // - upated the albums
                m_albums.clear();
                m_albums.insert( newAlbum.name(),
                                 QHash<QString, CollectionScanner::Album>() );
                m_albums[ newAlbum.name() ].insert( newAlbum.artist(), newAlbum );
            }
        }
    }

    // -- add the covers to all albums
    // if the directory contains only one album
    // (or several albums with the same name that did not recognize as a collection)
    // set the images found inside the directory as cover for this album.
    if( m_albums.count() == 1 ) // only one album name
    {
        // again, hash of a has. a little inconvinient.
        QHash<QString, CollectionScanner::Album> albums = m_albums.values().first();

        QHash<QString, Album>::iterator j = albums.begin();
        while (j != albums.end())
        {
            j.value().addCovers( covers );
            ++j;
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
                m_path = reader->readElementText();
            else if( name == "rpath" )
                m_rpath = reader->readElementText();
            else if( name == "mtime" )
                m_mtime = reader->readElementText().toUInt();
            else if( name == "skipped" )
            {
                m_skipped = true;
                reader->readNext(); // this is an empty element and I read over the end element here
            }
            else if( name == "ignored" )
            {
                m_ignored = true;
                reader->readNext(); // this is an empty element and I read over the end element here
            }
            else if( name == "album" )
            {
                CollectionScanner::Album album = CollectionScanner::Album( reader );
                // hash of a hash
                if( ! m_albums.contains( album.name() ) )
                    m_albums.insert( album.name(),
                                     QHash<QString, CollectionScanner::Album>() );
                m_albums[ album.name() ].insert( album.artist(), album );
            }
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

QList<CollectionScanner::Album>
CollectionScanner::Directory::albums() const
{
    QList<CollectionScanner::Album> albums;

    // again, hash of a has. a little inconvinient.
    QHash<QString, QHash<QString, Album> >::const_iterator i = m_albums.constBegin();
    while (i != m_albums.constEnd())
    {
        QHash<QString, Album>::const_iterator j = i.value().constBegin();
        while (j != i.value().constEnd())
        {
            albums.append( j.value() );
            ++j;
        }
        ++i;
    }

    return albums;
}

QList<CollectionScanner::Playlist>
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

    QList<CollectionScanner::Album> allAlbums = albums();
    foreach( const CollectionScanner::Album &album, allAlbums )
    {
        writer->writeStartElement( "album" );
        album.toXml( writer );
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

