/***********************************************************************
 * Copyright 2012  Eike Hein <hein@kde.org>
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

#ifndef AMAROK_MEDIAPLAYER2_H
#define AMAROK_MEDIAPLAYER2_H

#include "DBusAbstractAdaptor.h"
#include <QStringList>

namespace Amarok {
    class MediaPlayer2 : public DBusAbstractAdaptor
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2") // Docs: http://www.mpris.org/2.1/spec/Root_Node.html

        Q_PROPERTY(bool CanRaise READ CanRaise)
        Q_PROPERTY(bool CanQuit READ CanQuit)
        Q_PROPERTY(bool CanSetFullscreen READ CanSetFullscreen)
        Q_PROPERTY(bool Fullscreen READ Fullscreen)

        Q_PROPERTY(bool HasTrackList READ HasTrackList)

        Q_PROPERTY(QString Identity READ Identity)
        Q_PROPERTY(QString DesktopEntry READ DesktopEntry)

        Q_PROPERTY(QStringList SupportedUriSchemes READ SupportedUriSchemes)
        Q_PROPERTY(QStringList SupportedMimeTypes READ SupportedMimeTypes)

        public:
            explicit MediaPlayer2(QObject* parent);
            ~MediaPlayer2();

            bool CanRaise() const;
            bool CanQuit() const;
            bool CanSetFullscreen() const;
            bool Fullscreen() const;

            bool HasTrackList() const;

            QString Identity() const;
            QString DesktopEntry() const;

            QStringList SupportedUriSchemes() const;
            QStringList SupportedMimeTypes() const;

        public Q_SLOTS:
            void Raise() const;
            void Quit() const;
    };
}

#endif // AMAROK_MEDIAPLAYER2_H
