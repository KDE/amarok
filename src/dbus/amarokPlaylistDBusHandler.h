/******************************************************************************
 * Copyright (C) 2003 Stanislav Karchebny <berk@inbox.ru>                     *
 *           (C) 2005 Ian Monroe <ian@monroe.nu>                              *
 *           (C) 2005 Seb Ruiz <ruiz@kde.org>                                 *
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

#ifndef AMAROK_PLAYLIST_DBUS_HANDLER_H
#define AMAROK_PLAYLIST_DBUS_HANDLER_H

#include <kurl.h>
#include <QObject>

namespace Amarok
{
	class amarokPlaylistDBusHandler : public QObject
		{
			Q_OBJECT
		public:
			amarokPlaylistDBusHandler();
			
		public:
			virtual int     getActiveIndex();
			virtual int     getTotalTrackCount();
			virtual QString saveCurrentPlaylist();
			virtual void    addMedia(const KUrl &);
			virtual void    addMediaList(const KUrl::List &);
			virtual void    clearPlaylist();
			//       virtual QString currentTrackUniqueId();
			virtual void    playByIndex(int);
			virtual void    playMedia(const KUrl &);
			virtual void    popupMessage(const QString&);
			virtual void    removeCurrentTrack();
			virtual void    removeByIndex(int);
			virtual void    repopulate();
			virtual void    savePlaylist(const QString& path, bool relativePaths);
			virtual void    setStopAfterCurrent(bool);
			virtual void    shortStatusMessage(const QString&);
			virtual void    shufflePlaylist();
			virtual void    togglePlaylist();
			virtual QStringList filenames();
		};
}

#endif
