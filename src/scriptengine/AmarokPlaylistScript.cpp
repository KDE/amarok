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
#include "playlist/PlaylistModel.h"

#include <QtScript>

namespace AmarokScript
{
    AmarokPlaylistScript::AmarokPlaylistScript( QScriptEngine* ScriptEngine )
    : QObject( kapp )
    {
//        qScriptRegisterMetaType<TrackMeta>( ScriptEngine, toScriptValue, fromScriptValue );
        connect( The::playlistModel(), SIGNAL( playlistCountChanged( int ) ), this, SIGNAL( CountChanged( int ) ) );
        connect( The::playlistModel(), SIGNAL( playlistGroupingChanged() ), this, SIGNAL( GroupingChanged() ) );
        connect( The::playlistModel(), SIGNAL( rowMoved( int, int ) ), this, SIGNAL( rowMoved( int, int ) ) );
        connect( The::playlistModel(), SIGNAL( activeRowChanged( int, int ) ), this, SIGNAL( activeRowChanged( int, int ) ) );
        connect( The::playlistModel(), SIGNAL( activeRowExplicitlyChanged( int, int ) ), this, SIGNAL( activeRowExplicitlyChanged( int, int ) ) );
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

    void AmarokPlaylistScript::addMedia( const KUrl &url )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
        The::playlistModel()->insertOptioned( track, Playlist::Append );
    }

    void AmarokPlaylistScript::addMediaList( const KUrl::List &urls )
    {
        Meta::TrackList tracks = CollectionManager::instance()->tracksForUrls( urls );
        The::playlistModel()->insertOptioned( tracks, Playlist::Append );
    }

    void AmarokPlaylistScript::clearPlaylist()
    {
        The::playlistModel()->clear();
    }

    void AmarokPlaylistScript::playByIndex( int index )
    {
        The::playlistModel()->play( index );
    }

    void AmarokPlaylistScript::playMedia( const KUrl &url )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
        if( track )
            The::playlistModel()->insertOptioned( track, Playlist::DirectPlay | Playlist::Unique );
    }

    void AmarokPlaylistScript::removeCurrentTrack()
    {
        The::playlistModel()->removeRows( activeIndex(), 1 );
    }

    void AmarokPlaylistScript::removeByIndex( int index )
    {
        if( index < totalTrackCount() )
            The::playlistModel()->removeRows( index, 1 );
    }

    void AmarokPlaylistScript::savePlaylist( const QString& path )
    {
        The::playlistModel()->exportPlaylist( path );
    }

    void AmarokPlaylistScript::setStopAfterCurrent( bool on )
    {
        The::playlistModel()->setStopAfterMode( on ? Playlist::StopAfterCurrent : Playlist::StopNever );
    }

    void AmarokPlaylistScript::togglePlaylist()
    {
        The::mainWindow()->showHide();
    }

    QStringList AmarokPlaylistScript::filenames()
    {
        QStringList fileNames;
        foreach( Playlist::Item* item, The::playlistModel()->itemList() )
        fileNames << item->track()->prettyUrl();
        return fileNames;
    }

    TrackMeta AmarokPlaylistScript::TrackInfo( int row )
    {
        TrackMeta info;
        Meta::TrackPtr track;
        if ( index == 0 ) //current playing
            track = The::engineController()->currentTrack();
        else
            track = The::playlistModel()->trackForRow( row );
        if ( track )
        {
            info.isValid = true;
            info.sampleRate = track->sampleRate();
            info.bitrate = track->bitrate();
            info.score = track->score();
            info.rating = track->rating();
            info.inCollection = track->inCollection();
            info.type = track->type();
            info.length = track->length();
            info.fileSize = track->filesize();
            info.trackNumber = track->trackNumber();
            info.discNumber = track->discNumber();
            info.playCount = track->playCount();
            info.playable = track->isPlayable();
            info.album = track->album()->prettyName();
            info.artist = track->artist()->prettyName();
            info.composer = track->composer()->prettyName();
            info.genre = track->genre()->prettyName();
            info.year = track->year()->prettyName();
            info.comment = track->comment();
            info.path = track->playableUrl().path();
        }
        else
            info.isValid = false;
        return info;
    }

    QScriptValue AmarokPlaylistScript::toScriptValue(QScriptEngine *engine, const TrackMeta &in)
    {
        QScriptValue obj = engine->newObject();
        obj.setProperty( "isValid", QScriptValue( engine, in.isValid ) );
        obj.setProperty( "sampleRate", QScriptValue( engine, in.sampleRate ) );
        obj.setProperty( "bitrate", QScriptValue( engine, in.bitrate ) );
        obj.setProperty( "score", QScriptValue( engine, in.score ) );
        obj.setProperty( "rating", QScriptValue( engine, in.rating ) );
        obj.setProperty( "inCollection", QScriptValue( engine, in.inCollection ) );
        obj.setProperty( "type", QScriptValue( engine, in.type ) );
        obj.setProperty( "length", QScriptValue( engine, in.length ) );
        obj.setProperty( "fileSize", QScriptValue( engine, in.fileSize ) );
        obj.setProperty( "trackNumber", QScriptValue( engine, in.trackNumber ) );
        obj.setProperty( "discNumber", QScriptValue( engine, in.discNumber ) );
        obj.setProperty( "playCount", QScriptValue( engine, in.playCount ) );
        obj.setProperty( "playable", QScriptValue( engine, in.playable ) );
        obj.setProperty( "album", QScriptValue( engine, in.album ) );
        obj.setProperty( "artist", QScriptValue( engine, in.artist ) );
        obj.setProperty( "composer", QScriptValue( engine, in.composer ) );
        obj.setProperty( "genre", QScriptValue( engine, in.genre ) );
        obj.setProperty( "year", QScriptValue( engine, in.year ) );
        obj.setProperty( "comment", QScriptValue( engine, in.comment ) );
        obj.setProperty( "path", QScriptValue( engine, in.path ) );
        return obj;
    }

    void AmarokPlaylistScript::fromScriptValue(const QScriptValue &value, TrackMeta &out)
    {
        Q_UNUSED( value );
        Q_UNUSED( out );
    }

}

#include "AmarokPlaylistScript.moc"
