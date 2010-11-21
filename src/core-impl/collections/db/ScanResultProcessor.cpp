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

#include "ScanResultProcessor.h"
#include "DatabaseCollection.h"
#include "DatabaseMeta.h"
#include "sql/MountPointManager.h"

#include "playlistmanager/PlaylistManager.h"

#include "core/support/Debug.h"

// include files from the collection scanner utility
#include <collectionscanner/Directory.h>
#include <collectionscanner/Album.h>
#include <collectionscanner/Track.h>
#include <collectionscanner/Playlist.h>

ScanResultProcessor::ScanResultProcessor( ScanType type, QObject *parent )
    : QObject( parent )
    , m_type( type )
{
}

ScanResultProcessor::~ScanResultProcessor()
{
    foreach( CollectionScanner::Directory *dir, m_directories )
        delete dir;
}


void
ScanResultProcessor::addDirectory( CollectionScanner::Directory *dir )
{
    m_directories.append( dir );
}


void
ScanResultProcessor::commit()
{
    // we are blocking the updated signal for maximum of one second.
    QDateTime blockedTime = QDateTime::currentDateTime();
    blockUpdates();

    // -- now commit the directories
    foreach( const CollectionScanner::Directory* dir, m_directories )
    {
        commitDirectory( dir );

        // release the block every 5 second. Maybe not really needed, but still nice
        if( blockedTime.secsTo( QDateTime::currentDateTime() ) >= 5 )
        {
            unblockUpdates();
            blockedTime = QDateTime::currentDateTime();
            blockUpdates();
        }
    }

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
ScanResultProcessor::commitDirectory( const CollectionScanner::Directory *dir )
{
    if( dir->path().isEmpty() )
    {
        warning() << "got directory with no path from the scanner, not adding";
        return;
    }

    // selects and inserts should be done atomar, in theory

    // --- updated the directory entry
    int dirId = getDirectory( dir->path(), dir->mtime() );

    m_foundDirectories.insert(dirId, dir->path());
    if( dir->isSkipped() )
        return;

    // --- add all albums
    foreach( const CollectionScanner::Album &album, dir->albums() )
        commitAlbum( &album, dirId );

    // --- add all playlists
    foreach( const CollectionScanner::Playlist &playlist, dir->playlists() )
        commitPlaylist( &playlist );

    deleteDeletedTracks( dirId );

    emit directoryCommitted();
}

void
ScanResultProcessor::commitPlaylist( const CollectionScanner::Playlist *playlist )
{
    // debug() << "SRP::commitPlaylist on " << playlist->path();

    if( The::playlistManager() )
        The::playlistManager()->import( "file:"+playlist->path() );
}

#include "ScanResultProcessor.moc"

