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

#ifndef MP3TUNESHARMONYDOWNLOAD_H
#define MP3TUNESHARMONYDOWNLOAD_H

#include <QString>
#include <QVariantMap>
extern "C" {
   // Get libmp3tunes declarations
    #include "../libmp3tunes/harmony.h"
}

/**
 * A wrapper class for the libmp3tunes harmony download object.
 * It contains metadata for new tracks.
 * @author Casey Link <unnamedrambler@gmail.com>
 */
class Mp3tunesHarmonyDownload {
    public:
        /**
         * Default constructor does nothing.
         */
        Mp3tunesHarmonyDownload();
	Mp3tunesHarmonyDownload( const QVariantMap &map );
        Mp3tunesHarmonyDownload( mp3tunes_harmony_download_t *download );
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
	QVariantMap serialize() const;

    private:
        QString m_fileKey;
        QString m_fileName;
        QString m_fileFormat;
        unsigned int m_fileSize;
        QString m_artistName;
        QString m_albumTitle;
        QString m_trackTitle;
        int m_trackNumber;
        QString m_deviceBitrate;
        QString m_fileBitrate;
        QString m_url;
};

#endif

