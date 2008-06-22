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

#include "amarokMediaBrowserDBusHandler.h"

#include "App.h"
#include "mediabrowser.h"

namespace Amarok
{
	
	//     DbusMediaBrowserHandler::DbusMediaBrowserHandler()
	//         : QObject( kapp )
	//     {
	//          (void)new MediaBrowserAdaptor(this);
	//          QDBusConnection::sessionBus().registerObject("/MediaBrowser", this);
	//     }
	//
	//     void DbusMediaBrowserHandler::deviceConnect()
	//     {
	//         if ( MediaBrowser::instance()->currentDevice() )
	//             MediaBrowser::instance()->currentDevice()->connectDevice();
	//     }
	//
	//     void DbusMediaBrowserHandler::deviceDisconnect()
	//     {
	//         if ( MediaBrowser::instance()->currentDevice() )
	//             MediaBrowser::instance()->currentDevice()->disconnectDevice();
	//     }
	//
	//     QStringList DbusMediaBrowserHandler::deviceList()
	//     {
	//         return MediaBrowser::instance()->deviceNames();
	//     }
	//
	//     void DbusMediaBrowserHandler::deviceSwitch( QString name )
	//     {
	//         MediaBrowser::instance()->deviceSwitch( name );
	//     }
	//
	//     void DbusMediaBrowserHandler::queue( KUrl url )
	//     {
	//         MediaBrowser::queue()->addUrl( url );
	//         MediaBrowser::queue()->URLsAdded();
	//     }
	//
	//     void DbusMediaBrowserHandler::queueList( KUrl::List urls )
	//     {
	//         MediaBrowser::queue()->addUrls( urls );
	//     }
	//
	//     void DbusMediaBrowserHandler::transfer()
	//     {
	//         if ( MediaBrowser::instance()->currentDevice() )
	//             MediaBrowser::instance()->currentDevice()->transferFiles();
	//     }
	//
	//     void DbusMediaBrowserHandler::transcodingFinished( QString src, QString dest )
	//     {
	//         MediaBrowser::instance()->transcodingFinished( src, dest );
	//     }
	
} //namespace Amarok



#include "amarokMediaBrowserDBusHandler.moc"