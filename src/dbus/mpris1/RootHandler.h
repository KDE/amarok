/****************************************************************************************
 * Copyright (c) 2008 Ian Monroe <ian@monroe.nu>                                        *
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

#ifndef MPRIS1_ROOT_HANDLER_H
#define MPRIS1_ROOT_HANDLER_H

#include <QObject>
#include <QDBusArgument>

namespace Mpris1
{
    struct Version
    {
        quint16 major;
        quint16 minor;
    };

    class RootHandler : public QObject
    {
        Q_OBJECT

        public:
            RootHandler();
            QString Identity();
            void Quit();
            Version MprisVersion();
            // NB: Amarok extensions, not part of the mpris spec
            void ShowOSD() const;
            void LoadThemeFile( const QString &path ) const;
    };
}

Q_DECLARE_METATYPE(Mpris1::Version)

// Marshall the DBusVersion data into a D-BUS argument
QDBusArgument &operator<<(QDBusArgument &argument, const Mpris1::Version &version);
// Retrieve the DBusVersion data from the D-BUS argument
const QDBusArgument &operator>>(const QDBusArgument &argument, Mpris1::Version &version);

#endif // MPRIS1_ROOT_HANDLER_H
