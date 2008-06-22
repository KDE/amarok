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

#include "amarokPlaylistBrowserDBusHandler.h"

#include "App.h" //transferCliArgs
#include "Debug.h"

#include "amarokPlaylistBrowserAdaptor.h"

namespace Amarok
{

    amarokPlaylistBrowserDBusHandler::amarokPlaylistBrowserDBusHandler()
    :QObject( kapp )
    {
        new amarokPlaylistBrowserAdaptor(this);
        QDBusConnection::sessionBus().registerObject("/PlaylistBrowser", this);
    }

    void amarokPlaylistBrowserDBusHandler::addPodcast( const QString &url )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( url );
        //PORT 2.0
        //         PlaylistBrowser::instance()->addPodcast( url );
    }

    void amarokPlaylistBrowserDBusHandler::scanPodcasts()
    {
        AMAROK_NOTIMPLEMENTED
        //PORT 2.0
        //         PlaylistBrowser::instance()->scanPodcasts();
    }

    void amarokPlaylistBrowserDBusHandler::addPlaylist( const QString &url )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( url );
        //PORT 2.0
        //         PlaylistBrowser::instance()->addPlaylist( url );
    }

    int amarokPlaylistBrowserDBusHandler::loadPlaylist( const QString &playlist )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( playlist ); return -1;
        //PORT 2.0
        //         return PlaylistBrowser::instance()->loadPlaylist( playlist );
    }
}

#include "amarokPlaylistBrowserDBusHandler.moc"
