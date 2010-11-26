/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2009-2010 Jeff Mitchell <mitchell@kde.org>                             *
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

#define DEBUG_PREFIX "ScanResultProcessor"

#include "ScanResultProcessor.h"
#include "core/support/Debug.h"
#include "DatabaseCollection.h"
#include "DatabaseMeta.h"
#include "playlistmanager/PlaylistManager.h"
#include "sql/MountPointManager.h"

// include files from the collection scanner utility
#include <collectionscanner/Directory.h>
#include <collectionscanner/Album.h>
#include <collectionscanner/Track.h>
#include <collectionscanner/Playlist.h>

ScanResultProcessor::ScanResultProcessor( QObject *parent )
    : QObject( parent )
    , m_type( PartialUpdateScan )
{
}

ScanResultProcessor::~ScanResultProcessor()
{
    foreach( CollectionScanner::Directory *dir, m_directories )
        delete dir;

    QHash<QString, CollectionScanner::Album*>::const_iterator i = m_albumNames.constBegin();
    while (i != m_albumNames.constEnd()) {
        delete i.value();
        ++i;
    }
 }

#include <core-impl/collections/support/ArtistHelper.h>

void
ScanResultProcessor::addDirectory( CollectionScanner::Directory *dir )
{
    m_directories.append( dir );

    // -- sort the tracks into albums
    QSet<CollectionScanner::Album*> dirAlbums;

    foreach( CollectionScanner::Track track, dir->tracks() )
        dirAlbums.insert( sortTrack( &track ) );

    // -- sort the remainder
    // -- use the directory name as album name
    {
        QString dirAlbumName = QDir( dir->path() ).dirName();
        foreach( CollectionScanner::Track track, dir->tracks() )
            dirAlbums.insert( sortTrack( &track ) );
    }

    // if all the tracks from this directory end up in one album then also add the images
    if( dirAlbums.count() == 1 )
        (*dirAlbums.begin())->addCovers( dir->covers() );
}

void
ScanResultProcessor::commit()
{
    // we are blocking the updated signal for maximum of one second.
    QDateTime blockedTime = QDateTime::currentDateTime();
    blockUpdates();

    // -- commit the directories
    foreach( CollectionScanner::Directory* dir, m_directories )
        commitDirectory( dir );

    // --- add all albums
    QMultiHash<QString, CollectionScanner::Album*>::const_iterator i = m_albumNames.constBegin();
    while (i != m_albumNames.constEnd()) {

        // if we have multiple albums with the same name, check if it
        // might be a compilation

        // commit all albums with a track with the noCompilation flag

        // commit all the remaining albums as a compilation
        commitAlbum( i.value() );

        // release the block every 5 second. Maybe not really needed, but still nice
        if( blockedTime.secsTo( QDateTime::currentDateTime() ) >= 5 )
        {
            unblockUpdates();
            blockedTime = QDateTime::currentDateTime();
            blockUpdates();
        }
        ++i;
    }

    // -- now check if some of the tracks are not longer used and also not moved to another directory
    foreach( CollectionScanner::Directory* dir, m_directories )
        if( !dir->isSkipped() )
        deleteDeletedTracks( dir );

    // -- delete all not-found directories
    if( m_type != PartialUpdateScan )
        deleteDeletedDirectories();

    unblockUpdates();
}

void
ScanResultProcessor::rollback()
{
    // nothing to do
}

void
ScanResultProcessor::commitDirectory( CollectionScanner::Directory *directory )
{
    if( directory->path().isEmpty() )
    {
        warning() << "got directory with no path from the scanner, not adding";
        return;
    }

    if( directory->isSkipped() )
        return;

    // --- add all playlists
    foreach( CollectionScanner::Playlist playlist, directory->playlists() )
        commitPlaylist( &playlist );

    emit directoryCommitted();
}

void
ScanResultProcessor::commitPlaylist( CollectionScanner::Playlist *playlist )
{
    // debug() << "commitPlaylist on " << playlist->path();

    if( The::playlistManager() )
        The::playlistManager()->import( "file:"+playlist->path() );
}

CollectionScanner::Album*
ScanResultProcessor::sortTrack( CollectionScanner::Track *track )
{
    QString albumArtist( track->albumArtist() );
    if( albumArtist.isEmpty() )
        albumArtist = ArtistHelper::realTrackArtist( track->artist() );
    if( track->isCompilation() )
        albumArtist.clear();

    if( track->album().isEmpty() && albumArtist.isEmpty() )
        return false;
    else
        return sortTrack( track, track->album(), albumArtist );
}

/** This will just put the tracks into an album.
    @param album the name of the target album
    @returns true if the track was put into an album
*/
CollectionScanner::Album*
ScanResultProcessor::sortTrack( CollectionScanner::Track *track,
                                const QString &albumName,
                                const QString &albumArtist )
{
    QString newAlbumArtist( albumArtist );
    if( albumName.isEmpty() )
        newAlbumArtist.clear(); // no album, no album artist

    AlbumKey key( albumName, newAlbumArtist );

    CollectionScanner::Album *album;
    if( m_albums.contains( key ) )
        album = m_albums.value( key );
    else
    {
        album = new CollectionScanner::Album( albumName, newAlbumArtist );
        m_albums.insert( key, album );
        m_albumNames.insert( albumName, album );
    }

    album->addTrack( track );
    return album;
}


#include "ScanResultProcessor.moc"

