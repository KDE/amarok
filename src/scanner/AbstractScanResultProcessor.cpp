/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2009-2010 Jeff Mitchell <mitchell@kde.org>                             *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "AbstractScanResultProcessor"

#include "AbstractScanResultProcessor.h"

#include "collectionscanner/Album.h"
#include "collectionscanner/Directory.h"
#include "collectionscanner/Playlist.h"
#include "collectionscanner/Track.h"
#include "core/logger/Logger.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "core-impl/collections/support/ArtistHelper.h"
#include "playlistmanager/PlaylistManager.h"
#include "scanner/GenericScanManager.h"

AbstractScanResultProcessor::AbstractScanResultProcessor( GenericScanManager* manager, QObject* parent )
    : QObject( parent )
    , m_manager( manager )
    , m_type( GenericScanManager::PartialUpdateScan )
{
    connect( manager, &GenericScanManager::started,
             this, &AbstractScanResultProcessor::scanStarted );
    connect( manager, &GenericScanManager::directoryCount,
             this, &AbstractScanResultProcessor::scanDirectoryCount );
    connect( manager, &GenericScanManager::directoryScanned,
             this, &AbstractScanResultProcessor::scanDirectoryScanned );
    connect( manager, &GenericScanManager::succeeded,
             this, &AbstractScanResultProcessor::scanSucceeded );
    connect( manager, &GenericScanManager::failed,
             this, &AbstractScanResultProcessor::scanFailed );
}

AbstractScanResultProcessor::~AbstractScanResultProcessor()
{
    cleanupMembers();
}

void
AbstractScanResultProcessor::scanStarted( GenericScanManager::ScanType type )
{
    DEBUG_BLOCK;

    m_type = type;

    // -- show the progress operation for the job
    Amarok::Logger::newProgressOperation( this,
                                          i18n( "Scanning music" ),
                                          100,
                                          this,
                                          &AbstractScanResultProcessor::abort );

}

void
AbstractScanResultProcessor::scanDirectoryCount( int count )
{
    // message( i18np("Found one directory", "Found %1 directories", count ) );
    debug() << "got" << count << "directories";
    Q_EMIT totalSteps( count * 2 );
}

void
AbstractScanResultProcessor::scanDirectoryScanned( QSharedPointer<CollectionScanner::Directory> dir )
{
    m_directories.append( dir );
    Q_EMIT incrementProgress();
}

void
AbstractScanResultProcessor::scanSucceeded()
{
    DEBUG_BLOCK;

    // the default for albums with several artists is that it's a compilation
    // however, some album names are unlikely to be a compilation
    static QStringList nonCompilationAlbumNames;
    if( nonCompilationAlbumNames.isEmpty() )
    {
        nonCompilationAlbumNames
            << QLatin1String("") // don't throw together albums without name. At least not here
            << QStringLiteral("Best Of")
            << QStringLiteral("Anthology")
            << QStringLiteral("Hit collection")
            << QStringLiteral("Greatest Hits")
            << QStringLiteral("All Time Greatest Hits")
            << QStringLiteral("Live");
    }

    // -- commit the directories
    for( QSharedPointer<CollectionScanner::Directory> dir : m_directories )
    {
        commitDirectory( dir );

        // -- sort the tracks into albums
        QSet<CollectionScanner::Album*> dirAlbums;
        QSet<QString> dirAlbumNames;
        QList<CollectionScanner::Track*> tracks = dir->tracks();

        // check what album names we have
        for( CollectionScanner::Track* track : dir->tracks() )
        {
            if( !track->album().isEmpty() )
                dirAlbumNames.insert( track->album() );
        }

        // use the directory name as album name
        QString fallbackAlbumName = ( dirAlbumNames.isEmpty() ?
                                      QDir( dir->path() ).dirName() :
                                      QString() );

        for( CollectionScanner::Track* track : dir->tracks() )
        {
            CollectionScanner::Album *album = sortTrack( track, fallbackAlbumName );

            dirAlbums.insert( album );
            dirAlbumNames.insert( track->album() );
        }

        // if all the tracks from this directory end up in one album
        // (or they have at least the same name) then it's likely that an image
        // from this directory could be a cover
        if( dirAlbumNames.count() == 1 )
            (*dirAlbums.begin())->setCovers( dir->covers() );
    }

    // --- add all albums
    QList<QString> keys = m_albumNames.uniqueKeys();
    Q_EMIT totalSteps( m_directories.count() + keys.count() ); // update progress bar
    for( const QString &key : keys )
    {
        // --- commit the albums as compilation or normal album

        QList<CollectionScanner::Album*> albums = m_albumNames.values( key );

        // if we have multiple albums with the same name, check if it
        // might be a compilation
        for( int i = albums.count() - 1; i >= 0; --i )
        {
            CollectionScanner::Album *album = albums.at( i );
            // commit all albums with a track with the noCompilation flag
            if( album->isNoCompilation() ||
                nonCompilationAlbumNames.contains( album->name(), Qt::CaseInsensitive ) )
                commitAlbum( albums.takeAt( i ) );
        }

        // only one album left. (that either means all have the same album artist tag
        // or none has one. In the last case we try to guess an album artist)
        if( albums.count() == 1 )
        {
            CollectionScanner::Album *album = albums.takeFirst();

            // look if all the tracks have the same (guessed) artist.
            // It's no compilation.
            bool isCompilation = false;
            bool firstTrack = true;
            QString albumArtist;
            for( CollectionScanner::Track *track : album->tracks() )
            {
                QString trackAlbumArtist =
                    ArtistHelper::bestGuessAlbumArtist( track->albumArtist(),
                                                        track->artist(),
                                                        track->genre(),
                                                        track->composer() );
                if( firstTrack )
                    albumArtist = trackAlbumArtist;
                firstTrack = false;

                if( trackAlbumArtist.isEmpty() || track->isCompilation() ||
                    albumArtist != trackAlbumArtist )
                    isCompilation = true;
            }

            if( !isCompilation )
                album->setArtist( albumArtist );
            commitAlbum( album );
        }

        // several albums with different album artist.
        else if( albums.count() > 1 )
        {
            while( !albums.isEmpty() )
                commitAlbum( albums.takeFirst() );
        }

        Q_EMIT incrementProgress();
    }

    // -- now check if some of the tracks are not longer used and also not moved to another directory
    for( QSharedPointer<CollectionScanner::Directory> dir : m_directories )
        if( !dir->isSkipped() )
            deleteDeletedTracksAndSubdirs( dir );

    // -- delete all not-found directories (special handling for PartialUpdateScan):
    deleteDeletedDirectories();

    cleanupMembers();
    Q_EMIT endProgressOperation( this );
}

void
AbstractScanResultProcessor::scanFailed( const QString& text )
{
    message( text );

    cleanupMembers();
    Q_EMIT endProgressOperation( this );
}

void
AbstractScanResultProcessor::abort()
{
    m_manager->abort();
}

void
AbstractScanResultProcessor::commitDirectory( QSharedPointer<CollectionScanner::Directory> dir )
{
    if( dir->path().isEmpty() )
    {
        warning() << "got directory with no path from the scanner, not adding";
        return;
    }

    // --- add all playlists
    for( const CollectionScanner::Playlist &playlist : dir->playlists() )
        commitPlaylist( playlist );
}

void
AbstractScanResultProcessor::commitPlaylist( const CollectionScanner::Playlist &playlist )
{
    // debug() << "commitPlaylist on " << playlist->path();

    if( The::playlistManager() )
        The::playlistManager()->import( QUrl::fromLocalFile( playlist.path() ) );
}

CollectionScanner::Album*
AbstractScanResultProcessor::sortTrack( CollectionScanner::Track *track, const QString &dirName )
{
    QString albumName = track->album();

    // we allow albums with empty name and nonempty artist, see bug 272471
    QString albumArtist = track->albumArtist();
    if( track->isCompilation() )
        albumArtist.clear();  // no album artist denotes a compilation
    if( track->isNoCompilation() && albumArtist.isEmpty() )
        albumArtist = ArtistHelper::bestGuessAlbumArtist( track->albumArtist(),
                                                          track->artist(),
                                                          track->genre(),
                                                          track->composer() );

    if( albumName.isEmpty() && albumArtist.isEmpty() ) // try harder to set at least one
        albumName = dirName;

    AlbumKey key( albumName, albumArtist );

    CollectionScanner::Album *album;
    if( m_albums.contains( key ) )
        album = m_albums.value( key );
    else
    {
        album = new CollectionScanner::Album( albumName, albumArtist );
        m_albums.insert( key, album );
        m_albumNames.insert( albumName, album );
    }

    album->addTrack( track );
    return album;
}

void
AbstractScanResultProcessor::cleanupMembers()
{
    // note: qt had problems with CLEAN_ALL macro
    QHash<QString, CollectionScanner::Album*>::const_iterator i = m_albumNames.constBegin();
    while (i != m_albumNames.constEnd()) {
        delete i.value();
        ++i;
    }
    m_albumNames.clear();
    m_albums.clear();
    m_directories.clear();
}


