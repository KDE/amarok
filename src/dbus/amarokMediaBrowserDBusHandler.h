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

#ifndef AMAROK_MEDIABROWSER_DBUS_HANDLER_H
#define AMAROK_MEDIABROWSER_DBUS_HANDLER_H

#include <QObject>

namespace Amarok
{
    // class DbusMediaBrowserHandler : public QObject
    // {
    //     Q_OBJECT
    //
    //     public:
    //         DbusMediaBrowserHandler();
    //
    //     public /* DBus */ slots:
    //       virtual void deviceConnect();
    //       virtual void deviceDisconnect();
    //       virtual QStringList deviceList();
    //       virtual void deviceSwitch( QString name );
    //       virtual void queue( KUrl url );
    //       virtual void queueList( KUrl::List urls );
    //       virtual void transfer();
    //       virtual void transcodingFinished( QString src, QString dest );
    // };

} // namespace Amarok

#endif

