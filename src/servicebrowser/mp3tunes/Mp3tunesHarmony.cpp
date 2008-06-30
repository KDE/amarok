/***************************************************************************
 *   Copyright (c) 2007  Casey Link <unnamedrambler@gmail.com>             *
 *                                                                         *
 *   This program is free software{} you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation{} either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY{} without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program{} if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "Mp3tunesHarmony.h"
#include "Debug.h"
#include <QByteArray>

Mp3tunesHarmonyDownload::Mp3tunesHarmonyDownload()
{
    mp3tunes_harmony_download_init( &m_harmony_download_t );
}

Mp3tunesHarmonyDownload::~Mp3tunesHarmonyDownload()
{
    mp3tunes_harmony_download_deinit( &m_harmony_download_t );
}

QString
Mp3tunesHarmonyDownload::fileKey() const
{
    return QString( m_harmony_download_t->file_key );
}

QString
Mp3tunesHarmonyDownload::fileName() const
{
    return QString( m_harmony_download_t->file_name );
}

QString
Mp3tunesHarmonyDownload::fileFormat() const
{
    return QString( m_harmony_download_t->file_format );
}

unsigned int
Mp3tunesHarmonyDownload::fileSize() const
{
    return m_harmony_download_t->file_size;
}

QString
Mp3tunesHarmonyDownload::artistName() const
{
    return QString( m_harmony_download_t->artist_name );
}

QString
Mp3tunesHarmonyDownload::albumTitle() const
{
    return QString( m_harmony_download_t->album_title );
}

QString
Mp3tunesHarmonyDownload::trackTitle() const
{
    return QString( m_harmony_download_t->track_title );
}

int
Mp3tunesHarmonyDownload::trackNumber() const
{
    return m_harmony_download_t->track_number;
}

QString
Mp3tunesHarmonyDownload::deviceBitrate() const
{
    return QString( m_harmony_download_t->device_bitrate );
}

QString
Mp3tunesHarmonyDownload::fileBitrate() const
{
    return QString( m_harmony_download_t->file_bitrate );
}

QString
Mp3tunesHarmonyDownload::url() const
{
    return QString( m_harmony_download_t->url );
}

Mp3tunesHarmony::Mp3tunesHarmony()
{
    m_harmony = mp3tunes_harmony_new();
    m_err = 0;
}
Mp3tunesHarmony::~Mp3tunesHarmony()
{}

void
Mp3tunesHarmony::setPin( const QString &pin )
{
    mp3tunes_harmony_set_pin( m_harmony, convertToChar( pin ) );
}
void
Mp3tunesHarmony::setEmail( const QString &email )
{
    mp3tunes_harmony_set_email( m_harmony, convertToChar( email ) );
}
void
Mp3tunesHarmony::setIdentifier( const QString &email )
{
    mp3tunes_harmony_set_identifier( m_harmony, convertToChar( email ) );
}
void
Mp3tunesHarmony::setDeviceDescription( const QString &description )
{
    mp3tunes_harmony_set_device_attribute(m_harmony, "device-description",
                                           convertToChar( description ) );
}
void
Mp3tunesHarmony::setDeviceTotalBytes( unsigned long long total )
{
    mp3tunes_harmony_set_device_attribute(m_harmony, "total-bytes", &total );
}
void
Mp3tunesHarmony::setDeviceAvailableBytes( unsigned long long available )
{
    mp3tunes_harmony_set_device_attribute(m_harmony, "available-bytes", &available );
}

QString
Mp3tunesHarmony::pin() const
{
    return QString( mp3tunes_harmony_get_pin(m_harmony) );
}

QString
Mp3tunesHarmony::email() const
{
    return QString( mp3tunes_harmony_get_email(m_harmony) );
}

QString
Mp3tunesHarmony::identifier() const
{
    return QString( mp3tunes_harmony_get_identifier(m_harmony) );
}

QString
Mp3tunesHarmony::jid() const
{
    return QString( mp3tunes_harmony_get_jid(m_harmony) );
}

bool
Mp3tunesHarmony::connect()
{
    mp3tunes_harmony_connect(m_harmony, &m_err);
    if (m_err) {
        /*g_error("Error: %s\n", m_err->message);*/
        return false;
    }
    return true;
}
bool
Mp3tunesHarmony::disconnect()
{
    GError *err;
    mp3tunes_harmony_disconnect(m_harmony, &err);
    if (err) {
       // g_error("Error disconnecting: %s\n", err->message);
        /* If there is an error disconnecting something has probably gone
         * very wrong and reconnection should not be attempted till the user
         * re-initiates it */
        return false;
    }
    return true;
}

/*void
Mp3tunesHarmony::sendDeviceStatus(GError **err){}*/

/*Mp3tunesHarmonyDownload*
Mp3tunesHarmony::downloadQueuePop(){}*/

char *
Mp3tunesHarmony::convertToChar ( const QString &source ) const
{
    QByteArray b = source.toAscii();
    const char *c_tok = b.constData();
    char * ret = ( char * ) malloc ( strlen ( c_tok ) );
    strcpy ( ret, c_tok );
    return ret;
}