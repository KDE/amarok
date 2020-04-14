/***********************************************************************
 * Copyright 2012  Alex Merry <alex.merry@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************/

#ifndef AMAROK_MEDIAPLAYER2AMAROKEXTENSIONS_H
#define AMAROK_MEDIAPLAYER2AMAROKEXTENSIONS_H

#include "DBusAbstractAdaptor.h"

#include <QDBusObjectPath>
#include <QVariantMap>

namespace Amarok
{
    class MediaPlayer2AmarokExtensions : public DBusAbstractAdaptor
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "org.kde.amarok.Mpris2Extensions.Player")

        Q_PROPERTY( bool Muted READ Muted WRITE setMuted )

        public:
            explicit MediaPlayer2AmarokExtensions( QObject* parent );
            ~MediaPlayer2AmarokExtensions() override;

            bool Muted() const;
            void setMuted( bool muted );

        public Q_SLOTS:
            void StopAfterCurrent();
            void AdjustVolume( double IncreaseBy );

        private Q_SLOTS:
            void mutedChanged( bool newValue );
    };
}

#endif // AMAROK_MEDIAPLAYER2AMAROKEXTENSIONS_H
