/****************************************************************************************
 * Copyright (c) 2007-2008 Ian Monroe <ian@monroe.nu>                                   *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DirectoryLoader.h"

#include "Debug.h"
#include "CollectionManager.h"
#include "playlists/impl/file/PlaylistFileSupport.h"
#include "playlist/PlaylistModelStack.h"
#include "playlistmanager/PlaylistManager.h"

#include <kio/job.h> // KIO::listRecursive


DirectoryLoader::DirectoryLoader()
    : QObject( 0 )
        , m_listOperations( 0 )
        , m_localConnection( false )
        , m_row( 0 )
{}

DirectoryLoader::~DirectoryLoader()
{}

void
DirectoryLoader::insertAtRow( int row )
{
    m_row = row;
    m_localConnection = true;
    connect( this, SIGNAL( finished( const Meta::TrackList& ) ), this, SLOT( doInsertAtRow() ) );
}

void
DirectoryLoader::doInsertAtRow()
{
    The::playlistController()->insertTracks( m_row, m_tracks );
    deleteLater();
}

void
DirectoryLoader::init( const QList<QUrl>& qurls )
{
    QList<KUrl> kurls;
    foreach( const QUrl &qurl, qurls )
        kurls << KUrl( qurl );

    init( kurls );
}

void
DirectoryLoader::init( const QList<KUrl>& urls )
{
    //drop from an external source or the file browser
    debug() << "Drop from external source or file browser";
    QList<KIO::ListJob*> jobs;

    foreach( const KUrl &kurl, urls )
    {
        KFileItem kitem( KFileItem::Unknown, KFileItem::Unknown, kurl, true );
        if( kitem.isDir() )
        {
            m_listOperations++;
            KIO::ListJob* lister = KIO::listRecursive( kurl ); //kjob's delete themselves
            connect( lister, SIGNAL( finished( KJob*) ),
                     SLOT( listJobFinished( KJob*) ) );
            connect( lister, SIGNAL( entries( KIO::Job*, const KIO::UDSEntryList& ) ),
                     SLOT( directoryListResults( KIO::Job*, const KIO::UDSEntryList& ) ) );
        }
        else
        {
            if( Meta::isPlaylist( kurl ) )
            {
                Meta::PlaylistFilePtr playlist = Meta::loadPlaylistFile( kurl );
                if( playlist )
                    m_tracks << playlist->tracks();
            }
            else
            {
                Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( kurl );
                if( track )
                    m_tracks << track;
            }
        }
        //else TODO: notify user if can't decode, see also MyDirLister::matchesFilter       
    }
    if( m_listOperations == 0 )
    {
        finishUrlList();
    }
}

void
DirectoryLoader::directoryListResults( KIO::Job *job, const KIO::UDSEntryList &list )
{
    DEBUG_BLOCK
    //dfaure says that job->redirectionUrl().isValid() ? job->redirectionUrl() : job->url(); might be needed
    //but to wait until an issue is actually found, since it might take more work
    const KUrl dir = static_cast<KIO::SimpleJob*>( job )->url();
    foreach( const KIO::UDSEntry &entry, list )
    {
        KFileItem item( entry, dir, true, true );
        m_expanded.append( item );
    }
}

void
DirectoryLoader::listJobFinished(KJob*)
{
    m_listOperations--;
    if( m_listOperations < 1 )
        finishUrlList();
}

void
DirectoryLoader::finishUrlList()
{
    if( !m_expanded.isEmpty() )
    {
        qStableSort( m_expanded.begin(), m_expanded.end(), DirectoryLoader::directorySensitiveLessThan );
        foreach( const KFileItem& item, m_expanded )
        {
            if( !item.isDir() )
            {
                Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( item.url() );
                if( track )
                    m_tracks.append( track );
            }
        }
        qStableSort( m_tracks.begin(), m_tracks.end(), Meta::Track::lessThan );
    }
    emit finished( m_tracks );
    if( !m_localConnection )
        deleteLater();
}

bool
DirectoryLoader::directorySensitiveLessThan( const KFileItem& item1, const KFileItem& item2 )
{
    QString dir1 =  item1.url().directory();
    QString dir2 =  item2.url().directory();

    debug() << "dir1: " << dir1;
    debug() << "dir2: " << dir2;
    if(dir1 == dir2)
    {
        debug() << "dir1==dir2";
        return item1.url().url() < item2.url().url();
    }
    else if( dir1 < dir2 )
    {
        debug() << "dir1<dir2";
        return true;
    } else {
        debug() << "dir1>dir2";
        return false;
    }
}


