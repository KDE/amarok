/******************************************************************************
 * Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "AmarokPlaylistScript.h"

#include "App.h"
#include "collection/CollectionManager.h"
#include "EngineController.h"
#include "MainWindow.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModel.h"

#include <QtScript>

namespace AmarokScript
{
    AmarokPlaylistScript::AmarokPlaylistScript( QScriptEngine* scriptEngine, QList<QObject*>* wrapperList )
    : QObject( kapp )
    , m_wrapperList( wrapperList )
    , m_scriptEngine( scriptEngine )
    {
        m_wrapperList = wrapperList;
        connect( The::playlistModel(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT ( slotCountChanged( ) ) );
        connect( The::playlistModel(), SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT ( slotCountChanged( ) ) );
        connect( The::playlistModel(), SIGNAL( activeRowChanged( int ) ), this, SIGNAL( activeRowChanged( int ) ) );
    }

    AmarokPlaylistScript::~AmarokPlaylistScript()
    {
    }
    int AmarokPlaylistScript::activeIndex()
    {
        return The::playlistModel()->activeRow();
    }

    int AmarokPlaylistScript::totalTrackCount()
    {
        return The::playlistModel()->rowCount();
    }

    QString AmarokPlaylistScript::saveCurrentPlaylist()
    {
        QString savePath = The::playlistModel()->defaultPlaylistPath();
        The::playlistModel()->exportPlaylist( savePath );
        return savePath;
    }

    void AmarokPlaylistScript::addMedia( const QUrl &url )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
        The::playlistController()->insertOptioned( track, Playlist::Append );
    }

    void AmarokPlaylistScript::addMediaList( const QList<QUrl> &urls )
    {
        KUrl::List list;
        foreach( const QUrl &url, urls )
            list << url;
        Meta::TrackList tracks = CollectionManager::instance()->tracksForUrls( list );
        The::playlistController()->insertOptioned( tracks, Playlist::Append );
    }

    void AmarokPlaylistScript::clearPlaylist()
    {
        The::playlistController()->clear();
    }

    void AmarokPlaylistScript::playByIndex( int index )
    {
        The::playlistActions()->play( index );
    }

    void AmarokPlaylistScript::playMedia( const QUrl &url )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
        if( track )
            The::playlistController()->insertOptioned( track, Playlist::DirectPlay | Playlist::Unique );
    }

    void AmarokPlaylistScript::removeCurrentTrack()
    {
        The::playlistController()->removeRow( activeIndex() );
    }

    void AmarokPlaylistScript::removeByIndex( int index )
    {
        if( index < totalTrackCount() )
            The::playlistController()->removeRow( index );
    }

    void AmarokPlaylistScript::savePlaylist( const QString& path )
    {
        The::playlistModel()->exportPlaylist( path );
    }

    void AmarokPlaylistScript::setStopAfterCurrent( bool on )
    {
        The::playlistActions()->setStopAfterMode( on ? Playlist::StopAfterCurrent : Playlist::StopNever );
    }

    void AmarokPlaylistScript::togglePlaylist()
    {
        The::mainWindow()->showHide();
    }

    QStringList AmarokPlaylistScript::filenames()
    {
        QStringList fileNames;
        for (int i=0; i < The::playlistModel()->rowCount(); i++)
            fileNames << The::playlistModel()->trackAt(i)->prettyUrl();
        return fileNames;
    }

    QVariant AmarokPlaylistScript::trackAt( int row )
    {
        DEBUG_BLOCK
        Meta::TrackPtr track = The::playlistModel()->trackAt( row );
        return QVariant::fromValue( track );;
    }

    void AmarokPlaylistScript::slotCountChanged( )
    {
        emit CountChanged ( The::playlistModel()->rowCount() );
    }
}

#include "AmarokPlaylistScript.moc"
