/******************************************************************************
 * Copyright (C) 2003 Stanislav Karchebny <berk@inbox.ru>                     *
 *           (C) 2004 Christian Muehlhaeuser <chris@chris.de>                 *
 *           (C) 2005 Ian Monroe <ian@monroe.nu>                              *
 *           (C) 2005 Seb Ruiz <ruiz@kde.org>                                 *
 *           (C) 2006 Alexandre Pereira de Oliveira <aleprj@gmail.com>        *
 *           (C) 2006 2007 Leonardo Franchi <lfranchi@gmail.com>              *
 *           (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
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

#include "amarokPlaylistDBusHandler.h"

#include "App.h" //transferCliArgs
#include "collection/CollectionManager.h"
#include "Debug.h"
#include "MainWindow.h"
#include "playlist/PlaylistModel.h"
#include "StatusBar.h"
#include "TheInstances.h"

#include "amarokPlaylistAdaptor.h"

namespace Amarok
{

	amarokPlaylistDBusHandler::amarokPlaylistDBusHandler()
	: QObject( kapp )
	{
		new amarokPlaylistAdaptor(this);
		QDBusConnection::sessionBus().registerObject("/Playlist", this);
	}

	int amarokPlaylistDBusHandler::getActiveIndex()
	{
		return The::playlistModel()->activeRow();
	}

	int amarokPlaylistDBusHandler::getTotalTrackCount()
	{
		return The::playlistModel()->rowCount();
	}

	QString amarokPlaylistDBusHandler::saveCurrentPlaylist()
	{
		QString savePath = The::playlistModel()->defaultPlaylistPath();
		The::playlistModel()->exportPlaylist( savePath );
		return savePath;
	}

	void amarokPlaylistDBusHandler::addMedia(const KUrl &url)
	{
		Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
		The::playlistModel()->insertOptioned( track, Playlist::Append );
	}

	void amarokPlaylistDBusHandler::addMediaList(const KUrl::List &urls)
	{
		Meta::TrackList tracks = CollectionManager::instance()->tracksForUrls( urls );
		The::playlistModel()->insertOptioned( tracks, Playlist::Append );
	}

	void amarokPlaylistDBusHandler::clearPlaylist()
	{
		The::playlistModel()->clear();
	}

	void amarokPlaylistDBusHandler::playByIndex(int index)
	{
		The::playlistModel()->play( index );
	}

	void amarokPlaylistDBusHandler::playMedia( const KUrl &url )
	{
		Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
		if( track )
		{
			The::playlistModel()->insertOptioned( track, Playlist::DirectPlay | Playlist::Unique );
		}
	}

	void amarokPlaylistDBusHandler::popupMessage( const QString& msg )
	{
		StatusBar::instance()->longMessageThreadSafe( msg );
	}

	void amarokPlaylistDBusHandler::removeCurrentTrack()
	{
		The::playlistModel()->removeRows( getActiveIndex(), 1 );
	}

	void amarokPlaylistDBusHandler::removeByIndex( int index )
	{
		if( index < getTotalTrackCount() )
			The::playlistModel()->removeRows( index, 1 );
	}

	void amarokPlaylistDBusHandler::repopulate()
	{
		AMAROK_NOTIMPLEMENTED
		//PORT 2.0
		//         Playlist::instance()->repopulate();
	}

	void amarokPlaylistDBusHandler::savePlaylist( const QString& path, bool relativePaths )
	{
		Q_UNUSED( relativePaths );
		The::playlistModel()->exportPlaylist( path );
	}

	void amarokPlaylistDBusHandler::setStopAfterCurrent( bool on )
	{
		The::playlistModel()->setStopAfterMode( on ? Playlist::StopAfterCurrent : Playlist::StopNever );
	}

	void amarokPlaylistDBusHandler::shortStatusMessage(const QString& msg)
	{
		StatusBar::instance()->shortMessage( msg );
	}

	void amarokPlaylistDBusHandler::shufflePlaylist()
	{
		AMAROK_NOTIMPLEMENTED
		//PORT 2.0
		//         Playlist::instance()->shuffle();
	}

	void amarokPlaylistDBusHandler::togglePlaylist()
	{
		MainWindow::self()->showHide();
	}

	QStringList amarokPlaylistDBusHandler::filenames()
	{
		QStringList fileNames;
		foreach( Playlist::Item* item, The::playlistModel()->itemList() )
		fileNames << item->track()->prettyUrl();
		return fileNames;
	}
	//     QString amarokPlaylistDBusHandler::currentTrackUniqueId()
	//     {
	//         if( Playlist::instance()->currentItem() )
	//             return Playlist::instance()->currentItem()->uniqueId();
	//         return QString();
	//     }
}

#include "amarokPlaylistDBusHandler.moc"