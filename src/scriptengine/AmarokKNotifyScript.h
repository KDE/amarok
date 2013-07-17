/****************************************************************************************
 * Copyright (c) 2011 Kevin Funk <krf@electrostorm.net>                                 *
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

#ifndef AMAROK_KNOTIFY_SCRIPT_H
#define AMAROK_KNOTIFY_SCRIPT_H

#include <QObject>

class QPixmap;
class QScriptEngine;

namespace AmarokScript
{
    // SCRIPTDOX Amarok.Window.KNotify
    class AmarokKNotifyScript : public QObject
    {
        Q_OBJECT
        Q_PROPERTY( bool kNotifyEnabled READ kNotifyEnabled WRITE setKNotifyEnabled )

        public:
            AmarokKNotifyScript( QScriptEngine* scriptEngine );

        public slots:
            void showCurrentTrack();
            void show(const QString &title, const QString &body);
            void show(const QString &title, const QString &body, const QPixmap &pixmap);

        private:
            void setKNotifyEnabled( bool enable );
            bool kNotifyEnabled();
    };
}

#endif
