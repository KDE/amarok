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
#include "EngineController.h"
#include "MainWindow.h"
#include "core/meta/Meta.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModelStack.h"
#include "playlist/view/listview/PrettyListView.h"
#include "playlist/PlaylistDock.h"
#include "ScriptingDefines.h"

#include <QJSEngine>

using namespace AmarokScript;

AmarokPlaylistScript::AmarokPlaylistScript( AmarokScriptEngine *engine )
    : QObject( engine )
    , m_scriptEngine( engine )
{
    QJSValue scriptObject = engine->newQObject( this );
    engine->globalObject().property( QStringLiteral("Amarok") ).setProperty( QStringLiteral("Playlist"), scriptObject );
    connect( The::playlist()->qaim(), &QAbstractItemModel::rowsInserted, this, &AmarokPlaylistScript::slotTrackInserted );
    connect( The::playlist()->qaim(), &QAbstractItemModel::rowsRemoved, this, &AmarokPlaylistScript::slotTrackRemoved );
    engine->registerArrayType< QList<QUrl> >();
}

int
AmarokPlaylistScript::activeIndex()
{
    return The::playlist()->activeRow();
}

int
AmarokPlaylistScript::totalTrackCount()
{
    return The::playlist()->qaim()->rowCount();
}

QString
AmarokPlaylistScript::saveCurrentPlaylist()
{
    QString savePath = Amarok::defaultPlaylistPath();
    The::playlist()->exportPlaylist( savePath );
    return savePath;
}

void
AmarokPlaylistScript::addMedia( const QUrl &url )
{
    The::playlistController()->insertOptioned( url );
}

void
AmarokPlaylistScript::addTrack( const Meta::TrackPtr &track )
{
    The::playlistController()->insertOptioned( track );
}

void
AmarokPlaylistScript::addMediaList( const QList<QUrl> &urls )
{
    QList<QUrl> list;
    for( const QUrl &url : urls )
        list << url;
    The::playlistController()->insertOptioned( list );
}

void
AmarokPlaylistScript::addTrackList( const Meta::TrackList &tracks )
{
    The::playlistController()->insertOptioned( tracks );
}

void
AmarokPlaylistScript::clearPlaylist()
{
    The::playlistController()->clear();
}

void
AmarokPlaylistScript::playByIndex( int index )
{
    The::playlistActions()->play( index );
}

void
AmarokPlaylistScript::playMedia( const QUrl &url )
{
    The::playlistController()->insertOptioned( url, Playlist::OnPlayMediaAction );
}

void
AmarokPlaylistScript::playTrack( const Meta::TrackPtr &track )
{
    The::playlistController()->insertOptioned( track, Playlist::OnPlayMediaAction );
}

void
AmarokPlaylistScript::playMediaList( const QList<QUrl> &urls )
{
    QList<QUrl> list;
    for( const QUrl &url : urls )
        list << url;
    The::playlistController()->insertOptioned( list, Playlist::OnPlayMediaAction );
}

void
AmarokPlaylistScript::playTrackList( const Meta::TrackList &trackList )
{
    The::playlistController()->insertOptioned( trackList, Playlist::OnPlayMediaAction );
}

void
AmarokPlaylistScript::removeCurrentTrack()
{
    The::playlistController()->removeRow( activeIndex() );
}

void
AmarokPlaylistScript::removeByIndex( int index )
{
    if( index < totalTrackCount() )
        The::playlistController()->removeRow( index );
}

void
AmarokPlaylistScript::savePlaylist( const QString& path )
{
    The::playlist()->exportPlaylist( path );
}

void
AmarokPlaylistScript::setStopAfterCurrent( bool on )
{
    if( on )
        The::playlistActions()->stopAfterPlayingTrack();
    else
        The::playlistActions()->stopAfterPlayingTrack( 0 );
}

bool
AmarokPlaylistScript::stopAfterCurrent()
{
    return The::playlistActions()->willStopAfterTrack( The::playlist()->activeId() );
}

void
AmarokPlaylistScript::togglePlaylist()
{
    The::mainWindow()->showHide();
}

QStringList
AmarokPlaylistScript::filenames()
{
    QStringList fileNames;
    for( int i=0; i < The::playlist()->qaim()->rowCount(); i++ )
        fileNames << The::playlist()->trackAt(i)->prettyUrl();
    return fileNames;
}

Meta::TrackPtr
AmarokPlaylistScript::trackAt( int row )
{
    DEBUG_BLOCK
    Meta::TrackPtr track = The::playlist()->trackAt( row );
    return track;
}

QList<int>
AmarokPlaylistScript::selectedIndexes()
{
    DEBUG_BLOCK

    Playlist::PrettyListView* list = qobject_cast<Playlist::PrettyListView*>( The::mainWindow()->playlistDock()->currentView() );
    return list->selectedRows();
}

QStringList
AmarokPlaylistScript::selectedFilenames()
{
    DEBUG_BLOCK

    QStringList fileNames;
    const QList<int> indexes = selectedIndexes();

    for( int i=0; i < indexes.size(); i++ )
        fileNames << The::playlist()->trackAt( indexes[i] )->prettyUrl();

    return fileNames;
}

void
AmarokPlaylistScript::slotTrackInserted( const QModelIndex&, int start, int end )
{
    Q_EMIT trackInserted( start, end );
}

void
AmarokPlaylistScript::slotTrackRemoved( const QModelIndex&, int start, int end )
{
    Q_EMIT trackRemoved( start, end );
}
