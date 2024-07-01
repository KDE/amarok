/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "PlaylistFileProvider.h"
#include "App.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "core/logger/Logger.h"
#include "core-impl/playlists/types/file/asx/ASXPlaylist.h"
#include "core-impl/playlists/types/file/m3u/M3UPlaylist.h"
#include "core-impl/playlists/types/file/pls/PLSPlaylist.h"
#include "core-impl/playlists/types/file/xspf/XSPFPlaylist.h"
#include "playlistmanager/PlaylistManager.h"

#include <QDir>
#include <QString>
#include <QTimer>
#include <QUrl>

#include <KIO/Global>
#include <KLocalizedString>

namespace Playlists {

PlaylistFileProvider::PlaylistFileProvider()
 : UserPlaylistProvider()
 , m_playlistsLoaded( false )
 , m_saveLaterTimer( nullptr )
{
    //playlists are lazy loaded but we can count how many we'll load already
    QStringList keys = loadedPlaylistsConfig().keyList();
    for( const QString &key : keys )
    {
        QUrl url( key );
        //Don't load these from the config file, they are read from the directory anyway
        if( KIO::upUrl(url).matches( QUrl::fromUserInput(Amarok::saveLocation(QStringLiteral("playlists"))), QUrl::StripTrailingSlash ) )
            continue;
        m_urlsToLoad << url;
    }
    //also add all files in the $KDEHOME/share/apps/amarok/playlists
    QDir playlistDir = QDir( Amarok::saveLocation( QStringLiteral("playlists") ), QLatin1String(""),
                             QDir::Name,
                             QDir::Files | QDir::Readable );
    for( const QString &file : playlistDir.entryList() )
    {
        QUrl url( playlistDir.path() );
        url = url.adjusted(QUrl::StripTrailingSlash);
        url.setPath(url.path() + QLatin1Char('/') + ( file ));
        if( Playlists::isPlaylist( url ) )
            m_urlsToLoad << url;
    }
}

PlaylistFileProvider::~PlaylistFileProvider()
{
    DEBUG_BLOCK
    //remove all, well add them again soon
    loadedPlaylistsConfig().deleteGroup();
    //Write loaded playlists to config file
    for( Playlists::PlaylistFilePtr playlistFile : m_playlists )
    {
        QUrl url = playlistFile->uidUrl();
        //only save files NOT in "playlists", those are automatically loaded.
        if( KIO::upUrl(url).matches( QUrl::fromUserInput(Amarok::saveLocation( QStringLiteral("playlists") )), QUrl::StripTrailingSlash ) )
            continue;

        //debug() << "storing to rc-file: " << url.url();

        loadedPlaylistsConfig().writeEntry( url.url(), playlistFile->groups() );
    }
    loadedPlaylistsConfig().sync();
}

QString
PlaylistFileProvider::prettyName() const
{
    return i18n( "Playlist Files on Disk" );
}

QIcon PlaylistFileProvider::icon() const
{
    return QIcon::fromTheme( QStringLiteral("folder-documents") );
}

int
PlaylistFileProvider::playlistCount() const
{
    return m_playlists.count() + m_urlsToLoad.count();
}

Playlists::PlaylistList
PlaylistFileProvider::playlists()
{
    Playlists::PlaylistList playlists;

    if( !m_playlistsLoaded )
    {
        //trigger a lazy load the playlists
        QTimer::singleShot(0, this, &PlaylistFileProvider::loadPlaylists );
        return playlists;
    }

    for( const Playlists::PlaylistFilePtr &playlistFile : m_playlists )
    {
        Playlists::PlaylistPtr playlist = Playlists::PlaylistPtr::dynamicCast( playlistFile );
        if( !playlist.isNull() )
            playlists << playlist;
    }
    return playlists;
}

Playlists::PlaylistPtr
PlaylistFileProvider::save( const Meta::TrackList &tracks, const QString &name )
{
    DEBUG_BLOCK

    QString filename = name.isEmpty() ? QDateTime::currentDateTime().toString( QStringLiteral("ddd MMMM d yy hh-mm")) : name;
    filename.replace( QLatin1Char('/'), QLatin1Char('-') );
    filename.replace( QLatin1Char('\\'), QLatin1Char('-') );

    Playlists::PlaylistFormat format = Playlists::getFormat( QUrl::fromUserInput( filename, QString(), QUrl::AssumeLocalFile ) );
    if( format == Playlists::Unknown ) // maybe the name just had a dot in it. We just add .xspf
    {
        format = Playlists::XSPF;
        filename.append( QLatin1String( ".xspf" ) );
    }

    QUrl path = QUrl::fromLocalFile( Amarok::saveLocation( QStringLiteral("playlists") ) );
    path = path.adjusted(QUrl::StripTrailingSlash);
    path.setPath(path.path() + QLatin1Char('/') + ( Amarok::vfatPath( filename ) ));
    if( QFileInfo( path.toLocalFile() ).exists() )
    {
        //TODO:request overwrite
        return Playlists::PlaylistPtr();
    }

    Playlists::PlaylistFile *playlistFile = nullptr;
    switch( format )
    {
        case Playlists::ASX:
            playlistFile = new Playlists::ASXPlaylist( path, this );
            break;
        case Playlists::PLS:
            playlistFile = new Playlists::PLSPlaylist( path, this );
            break;
        case Playlists::M3U:
            playlistFile = new Playlists::M3UPlaylist( path, this );
            break;
        case Playlists::XSPF:
            playlistFile = new Playlists::XSPFPlaylist( path, this );
            break;
        case Playlists::XML:
        case Playlists::RAM:
        case Playlists::SMIL:
        case Playlists::Unknown:
            // this should not happen since we set the format to XSPF above.
            return Playlists::PlaylistPtr();
    }
    playlistFile->setName( filename );
    playlistFile->addTracks( tracks );
    playlistFile->save( true );

    Playlists::PlaylistFilePtr playlistPtr( playlistFile );
    m_playlists << playlistPtr;
    //just in case there wasn't one loaded before.
    m_playlistsLoaded = true;

    Playlists::PlaylistPtr playlist( playlistFile );
    Q_EMIT playlistAdded( playlist );
    return playlist;
}

bool
PlaylistFileProvider::import( const QUrl &path )
{
    DEBUG_BLOCK
    if( !path.isValid() )
    {
        error() << "path is not valid!";
        return false;
    }

    for( Playlists::PlaylistFilePtr playlistFile : m_playlists )
    {
        if( !playlistFile )
        {
            error() << "Could not cast down.";
            error() << "m_playlists got corrupted! " << __FILE__ << ":" << __LINE__;
            continue;
        }
        if( playlistFile->uidUrl() == path )
        {
            debug() << "Playlist " << path.path() << " was already imported";
            return false;
        }
    }

    debug() << "Importing playlist file " << path;
    if( path == QUrl::fromLocalFile(Amarok::defaultPlaylistPath()) )
    {
        error() << "trying to load saved session playlist at %s" << path.path();
        return false;
    }

    Playlists::PlaylistFilePtr playlistFile = Playlists::loadPlaylistFile( path, this );
    if( !playlistFile )
        return false;

    m_playlists << playlistFile;
    //just in case there wasn't one loaded before.
    m_playlistsLoaded = true;

    Q_EMIT playlistAdded( PlaylistPtr( playlistFile.data() ) );
    return true;
}

void
PlaylistFileProvider::renamePlaylist(PlaylistPtr playlist, const QString &newName )
{
    DEBUG_BLOCK
    playlist->setName( newName );
}

bool
PlaylistFileProvider::deletePlaylists( const Playlists::PlaylistList &playlists )
{
    Playlists::PlaylistFileList playlistFiles;
    for( Playlists::PlaylistPtr playlist : playlists )
    {
        Playlists::PlaylistFilePtr playlistFile =
                Playlists::PlaylistFilePtr::dynamicCast( playlist );
        if( !playlistFile.isNull() )
            playlistFiles << playlistFile;
    }
    return deletePlaylistFiles( playlistFiles );
}

bool
PlaylistFileProvider::deletePlaylistFiles( Playlists::PlaylistFileList playlistFiles )
{
    for( Playlists::PlaylistFilePtr playlistFile : playlistFiles )
    {
        m_playlists.removeAll( playlistFile );
        loadedPlaylistsConfig().deleteEntry( playlistFile->uidUrl().url() );
        QFile::remove( playlistFile->uidUrl().path() );
        Q_EMIT playlistRemoved( Playlists::PlaylistPtr::dynamicCast( playlistFile ) );
    }
    loadedPlaylistsConfig().sync();

    return true;
}

void
PlaylistFileProvider::loadPlaylists()
{
    if( m_urlsToLoad.isEmpty() )
        return;

    //arbitrary number of playlists to load during one mainloop run: 5
    for( int i = 0; i < qMin( m_urlsToLoad.count(), 5 ); i++ )
    {
        QUrl url = m_urlsToLoad.takeFirst();
        QString groups = loadedPlaylistsConfig().readEntry( url.url() );
        Playlists::PlaylistFilePtr playlist = Playlists::loadPlaylistFile( url, this );
        if( !playlist )
        {
            Amarok::Logger::longMessage(
                    i18n("The playlist file \"%1\" could not be loaded.", url.fileName() ),
                    Amarok::Logger::Error
                );
            continue;
        }

        if( !groups.isEmpty() && playlist->isWritable() )
            playlist->setGroups( groups.split( QLatin1Char(','),  Qt::SkipEmptyParts ) );

        m_playlists << playlist;
        Q_EMIT playlistAdded( PlaylistPtr( playlist.data() ) );
    }

    //give the mainloop time to run
    if( !m_urlsToLoad.isEmpty() )
        QTimer::singleShot( 0, this, &PlaylistFileProvider::loadPlaylists );
}

void
PlaylistFileProvider::saveLater( Playlists::PlaylistFilePtr playlist )
{
    //WARNING: this assumes the playlistfile uses it's m_url for uidUrl
    if( playlist->uidUrl().isEmpty() )
        return;

    if( !m_saveLaterPlaylists.contains( playlist ) )
        m_saveLaterPlaylists << playlist;

    if( !m_saveLaterTimer )
    {
        m_saveLaterTimer = new QTimer( this );
        m_saveLaterTimer->setSingleShot( true );
        m_saveLaterTimer->setInterval( 0 );
        connect( m_saveLaterTimer, &QTimer::timeout, this, &PlaylistFileProvider::slotSaveLater );
    }

    m_saveLaterTimer->start();
}

void
PlaylistFileProvider::slotSaveLater() //SLOT
{
    for( Playlists::PlaylistFilePtr playlist : m_saveLaterPlaylists )
    {
        playlist->save( true ); //TODO: read relative type when loading
    }

    m_saveLaterPlaylists.clear();
}

KConfigGroup
PlaylistFileProvider::loadedPlaylistsConfig() const
{
    return Amarok::config( QStringLiteral("Loaded Playlist Files") );
}

} //namespace Playlists

