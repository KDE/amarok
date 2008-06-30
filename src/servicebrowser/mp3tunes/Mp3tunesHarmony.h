/***************************************************************************
 *   Copyright (c) 2007  Casey Link <unnamedrambler@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#ifndef MP3TUNESHARMONY_H
#define MP3TUNESHARMONY_H

#include <QString>

extern "C" {
   // Get libmp3tunes declarations
    #include "libmp3tunes/harmony.h"
    #include "libmp3tunes/locker.h"
}

/**
 * A wrapper class for the libmp3tunes harmony download object.
 * @author Casey Link <unnamedrambler@gmail.com>
 */
class Mp3tunesHarmonyDownload {
    public:
        Mp3tunesHarmonyDownload();
        ~Mp3tunesHarmonyDownload();

        QString fileKey() const;
        QString fileName() const;
        QString fileFormat() const;
        unsigned int fileSize() const;
        QString artistName() const;
        QString albumTitle() const;
        QString trackTitle() const;
        int trackNumber() const;
        QString deviceBitrate() const;
        QString fileBitrate() const;
        QString url() const;
    private:
        mp3tunes_harmony_download_t *m_harmony_download_t;
};

/**
 * A wrapper class for the libmp3tunes harmony object.
 * @author Casey Link <unnamedrambler@gmail.com>
 */
class Mp3tunesHarmony {
    public:
        Mp3tunesHarmony();
        ~Mp3tunesHarmony();

        void setPin( const QString &pin );
        void setEmail( const QString &email );
        void setIdentifier( const QString &email );
        void setDeviceDescription( const QString &description );
        void setDeviceTotalBytes( unsigned long long total );
        void setDeviceAvailableBytes( unsigned long long available );

        QString pin() const;
        QString email() const;
        QString identifier() const;
        QString jid() const;

        bool connect();
        bool disconnect();

        //TODO sendDeviceStatus() and downloadQueuePop() and error handling
        //void sendDeviceStatus(GError **err);
        //Mp3tunesHarmonyDownload* downloadQueuePop();

    private:
        MP3tunesHarmony *m_harmony;
        GError *m_err;

        char *convertToChar( const QString &source ) const;
};

#endif