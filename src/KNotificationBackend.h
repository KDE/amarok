/****************************************************************************************
 * Copyright (c) 2009-2011 Kevin Funk <krf@electrostorm.net>                            *
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

#ifndef AMAROK_KNOTIFICATIONBACKEND_H
#define AMAROK_KNOTIFICATIONBACKEND_H

#include "core/meta/Meta.h"

#include <QObject>
#include <QPixmap>

class KNotification;

namespace Amarok {

/**
 * Class for accessing KNotify in KDE
 **/
class KNotificationBackend : public QObject
{
    Q_OBJECT

public:
    static KNotificationBackend *instance();
    static void destroy();

    void setEnabled( bool enabled );
    bool isEnabled() const;

    /**
     * Checks if a fullscreen window is currently active.
     */
    bool isFullscreenWindowActive() const;

public slots:
    void show( const QString &title, const QString &body, const QPixmap &pixmap = QPixmap() );
    void showCurrentTrack( bool force = false );

private slots:
    void notificationClosed();

private:
    KNotificationBackend();
    ~KNotificationBackend();

    static KNotificationBackend *s_instance;

    bool m_enabled;
    KNotification* m_notify;
};

}

#endif
