/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#define DEBUG_PREFIX "AmarokScript::Playlist"

#include "AmarokPlaylistScript.h"

#include "App.h"
#include "collection/CollectionManager.h"
#include "EngineController.h"
#include "MainWindow.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModelStack.h"
#include "playlist/view/listview/PrettyListView.h"
#include "playlist/PlaylistWidget.h"

#include <QObject>

namespace AmarokScript
{
    AmarokPlaylistScript::AmarokPlaylistScript( QScriptEngine* scriptEngine, QList<QObject*>* wrapperList )
        : QObject( kapp )
        , m_wrapperList( wrapperList )
        , m_scriptEngine( scriptEngine )
    {
        connect( The::playlist(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT ( slotTrackInserted( const QModelIndex&, int, int ) ) );
        connect( The::playlist(), SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT ( slotTrackRemoved( const QModelIndex&, int, int ) ) );
        //connect( The::playlist(), SIGNAL( activeRowChanged( int ) ), this, SIGNAL( activeRowChanged( int ) ) ); // FIXME: still needed?
    }

    AmarokPlaylistScript::~AmarokPlaylistScript()
    {}

    int AmarokPlaylistScript::activeIndex()
    {
        return The::playlist()->activeRow();
    }

    int AmarokPlaylistScript::totalTrackCount()
    {
        return The::playlist()->rowCount();
    }

    QString AmarokPlaylistScript::saveCurrentPlaylist()
    {
        QString savePath = Playlist::ModelStack::instance()->source()->defaultPlaylistPath();
        The::playlist()->exportPlaylist( savePath );
        return savePath;
    }

    void AmarokPlaylistScript::addMedia( const QUrl &url )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
        The::playlistController()->insertOptioned( track, Playlist::Append );
    }

	void AmarokPlaylistScript::addMediaList( const QVariantList &urls )
    {
        KUrl::List list;
        foreach( const QVariant &url, urls )
            list << url.toUrl();
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
        The::playlist()->exportPlaylist( path );
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
        for( int i=0; i < The::playlist()->rowCount(); i++ )
            fileNames << The::playlist()->trackAt(i)->prettyUrl();
        return fileNames;
    }

    QVariant AmarokPlaylistScript::trackAt( int row )
    {
        DEBUG_BLOCK
        Meta::TrackPtr track = The::playlist()->trackAt( row );
        return QVariant::fromValue( track );;
    }

    QList<int> AmarokPlaylistScript::selectedIndexes()
    {
        DEBUG_BLOCK

        Playlist::PrettyListView* list = qobject_cast<Playlist::PrettyListView*>( The::mainWindow()->playlistWidget()->currentView() );
        return list->selectedRows();
    }

    QStringList AmarokPlaylistScript::selectedFilenames()
    {
        DEBUG_BLOCK

        QStringList fileNames;
        const QList<int> indexes = selectedIndexes();

        for( int i=0; i < indexes.size(); i++ )
            fileNames << The::playlist()->trackAt( indexes[i] )->prettyUrl();

        return fileNames;
    }

    void AmarokPlaylistScript::slotTrackInserted( const QModelIndex&, int start, int end )
    {
        emit trackInserted( start, end );
    }

    void AmarokPlaylistScript::slotTrackRemoved( const QModelIndex&, int start, int end )
    {
        emit trackRemoved( start, end );
    }
}

#include "AmarokPlaylistScript.moc"
