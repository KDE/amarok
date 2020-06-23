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
#include <QPixmap>

class QJSEngine;

namespace AmarokScript
{
    // SCRIPTDOX Amarok.Window.KNotify
    /**
     * KNotify is the notification subsystem within KDE, which alerts users to
     * user-configured events within the KDE system.
     */
    class AmarokKNotifyScript : public QObject
    {
        Q_OBJECT
        /**
         * Whether KNotify is enabled
         */
        Q_PROPERTY( bool kNotifyEnabled READ kNotifyEnabled WRITE setKNotifyEnabled )

        public:
            explicit AmarokKNotifyScript( QJSEngine* scriptEngine );

            /**
             * Show notifications for the currently playing track.
             */
            Q_INVOKABLE void showCurrentTrack();

            /**
             * Show a custom KNotify notification
             */
            Q_INVOKABLE void show(const QString &title, const QString &body, const QPixmap &pixmap = QPixmap() );

        private:
            void setKNotifyEnabled( bool enable );
            bool kNotifyEnabled();
    };
}

#endif
