/****************************************************************************************
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MP3TUNESHARMONYCLIENT_H
#define MP3TUNESHARMONYCLIENT_H

#include "Mp3tunesHarmonyDownload.h"

#include <QObject>
class Mp3tunesHarmonyClient : public QObject {
    Q_OBJECT
    public:
        Mp3tunesHarmonyClient();

    public slots:
        virtual void harmonyError( const QString &error );
        virtual void harmonyWaitingForEmail( const QString &pin );
        virtual void harmonyWaitingForPin();
        virtual void harmonyConnected();
        virtual void harmonyDisconnected();
        virtual void harmonyDownloadReady( const Mp3tunesHarmonyDownload &download );
        virtual void harmonyDownloadPending( const Mp3tunesHarmonyDownload &download );
};

#endif
