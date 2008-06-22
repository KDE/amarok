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

#ifndef AMAROK_COLLECTION_DBUS_HANDLER_H
#define AMAROK_COLLECTION_DBUS_HANDLER_H

#include "amarok_export.h"

#include <QObject>

namespace Amarok
{
	
	class AMAROK_EXPORT amarokCollectionDBusHandler : public QObject
	{
		Q_OBJECT
		
	public:
		amarokCollectionDBusHandler();
		
		public /* DBus */ slots:
		virtual int totalAlbums();
		virtual int totalArtists();
		virtual int totalComposers();
		virtual int totalCompilations();
		virtual int totalGenres();
		virtual int totalTracks();
		virtual bool isDirInCollection( const QString &path );
		virtual QStringList collectionLocation();
		virtual bool moveFile( const QString &oldURL, const QString &newURL, bool overwrite );
		virtual QStringList query(const QString& sql);
		virtual QStringList similarArtists( int artists );
		virtual void migrateFile( const QString &oldURL, const QString &newURL );
		virtual void scanCollection();
		virtual void scanCollectionChanges();
		virtual void disableAutoScoring( bool disable );
		virtual int addLabels( const QString &url, const QStringList &labels );
		virtual void removeLabels( const QString &url, const QStringList &oldLabels );
	};
}

#endif