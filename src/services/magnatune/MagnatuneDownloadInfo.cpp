/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#include "MagnatuneDownloadInfo.h"

#include "core/support/Debug.h"

#include <KLocalizedString>

#include <QFile>
#include <QTextStream>


MagnatuneDownloadInfo::MagnatuneDownloadInfo()
{
    m_selectedDownloadFormat.clear();
}


MagnatuneDownloadInfo::~MagnatuneDownloadInfo()
{}

bool
MagnatuneDownloadInfo::initFromString( const QString &downloadInfoString, bool membershipDownload  )
{

    m_membershipDownload = membershipDownload;
    //complete overkill to do a full SAX2 parser for this at the moment... I think....

    // lets make sure that this is actually a valid result

    int testIndex = downloadInfoString.indexOf( QStringLiteral("<RESULT>") );
    if ( testIndex == -1 )
    {
        return false;
    }

    int startIndex;
    int endIndex;

    if ( membershipDownload == false ) {
        startIndex = downloadInfoString.indexOf( QStringLiteral("<DL_USERNAME>"), 0, Qt::CaseInsensitive );
        if ( startIndex != -1 )
        {
            endIndex = downloadInfoString.indexOf( QStringLiteral("</DL_USERNAME>"), 0, Qt::CaseInsensitive );
            if ( endIndex != -1 )
            {
                startIndex += 13;

                debug() << "found username: " << downloadInfoString.mid( startIndex, endIndex - startIndex );
                m_userName = downloadInfoString.mid( startIndex, endIndex - startIndex );
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }


        startIndex = downloadInfoString.indexOf( QStringLiteral("<DL_PASSWORD>"), 0, Qt::CaseInsensitive );
        if ( startIndex != -1 )
        {
            endIndex = downloadInfoString.indexOf( QStringLiteral("</DL_PASSWORD>"), 0, Qt::CaseInsensitive );
            if ( endIndex != -1 )
            {
                startIndex += 13;
                debug() << "found password: " << downloadInfoString.mid( startIndex, endIndex - startIndex );
                m_password = downloadInfoString.mid( startIndex, endIndex - startIndex );
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }

    } else {
        m_userName.clear();
        m_password.clear();
    }


    startIndex = downloadInfoString.indexOf( QStringLiteral("<URL_WAVZIP>"), 0, Qt::CaseInsensitive );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.indexOf( QStringLiteral("</URL_WAVZIP>"), 0, Qt::CaseInsensitive );
        if ( endIndex != -1 )
        {
            startIndex += 12;
            debug() << "found wav";
            m_downloadFormats[ QStringLiteral("Wav") ] = downloadInfoString.mid( startIndex, endIndex - startIndex ).replace( QStringLiteral("&amp;"), QStringLiteral("&") );

        }
    }

    startIndex = downloadInfoString.indexOf( QStringLiteral("<URL_128KMP3ZIP>"), 0, Qt::CaseInsensitive );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.indexOf( QStringLiteral("</URL_128KMP3ZIP>"), 0, Qt::CaseInsensitive );
        if ( endIndex != -1 )
        {
            startIndex += 16;
            debug() << "found 128k mp3";
            m_downloadFormats[ QStringLiteral("128 kbit/s MP3") ] = downloadInfoString.mid( startIndex, endIndex - startIndex ).replace( QStringLiteral("&amp;"), QStringLiteral("&") );

        }
    }

    startIndex = downloadInfoString.indexOf( QStringLiteral("<URL_OGGZIP>"), 0, Qt::CaseInsensitive );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.indexOf( QStringLiteral("</URL_OGGZIP>"), 0, Qt::CaseInsensitive );
        if ( endIndex != -1 )
        {
            startIndex += 12;
            debug() << "found ogg-vorbis";
            m_downloadFormats[ QStringLiteral("Ogg-Vorbis") ] = downloadInfoString.mid( startIndex, endIndex - startIndex ).replace( QStringLiteral("&amp;"), QStringLiteral("&") );

        }
    }

    startIndex = downloadInfoString.indexOf( QStringLiteral("<URL_VBRZIP>"), 0, Qt::CaseInsensitive );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.indexOf( QStringLiteral("</URL_VBRZIP>"), 0, Qt::CaseInsensitive );
        if ( endIndex != -1 )
        {
            startIndex += 12;
            debug() << "found vbr mp3";
            m_downloadFormats[ QStringLiteral("VBR MP3") ] = downloadInfoString.mid( startIndex, endIndex - startIndex ).replace( QStringLiteral("&amp;"), QStringLiteral("&") );

        }
    }

    startIndex = downloadInfoString.indexOf( QStringLiteral("<URL_FLACZIP>"), 0, Qt::CaseInsensitive );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.indexOf( QStringLiteral("</URL_FLACZIP>"), 0, Qt::CaseInsensitive );
        if ( endIndex != -1 )
        {
            startIndex += 13;
            debug() << "found flac";
            m_downloadFormats[ QStringLiteral("FLAC") ] = downloadInfoString.mid( startIndex, endIndex - startIndex ).replace( QStringLiteral("&amp;"), QStringLiteral("&") );

        }
    }

    startIndex = downloadInfoString.indexOf( QStringLiteral("<DL_MSG>"), 0, Qt::CaseInsensitive );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.indexOf( QStringLiteral("</DL_MSG>"), 0, Qt::CaseInsensitive );
        if ( endIndex != -1 )
        {
            startIndex += 9;
            debug() << "found dl-message";
            m_downloadMessage = downloadInfoString.mid( startIndex, endIndex - startIndex ).replace( QStringLiteral("&amp;"), QStringLiteral("&") );
        }
    }

    return true;
}

bool
MagnatuneDownloadInfo::initFromRedownloadXml( const QDomElement &element )
{


    m_artistName = element.firstChildElement( QStringLiteral("artist") ).text();
    m_albumName = element.firstChildElement( QStringLiteral("album") ).text();
    m_userName = element.firstChildElement( QStringLiteral("username") ).text();
    m_password = element.firstChildElement( QStringLiteral("password") ).text();
    m_albumCode = element.firstChildElement( QStringLiteral("sku") ).text();
    m_coverUrl = element.firstChildElement( QStringLiteral("cover") ).text();

    //get formats
    QDomNode formats = element.firstChildElement( QStringLiteral("formats") );

    QString wav_url = formats.firstChildElement( QStringLiteral("wav_zip") ).text();
    m_downloadFormats[ QStringLiteral("Wav") ] = wav_url;
    QString mp3_url = formats.firstChildElement( QStringLiteral("mp3_zip") ).text();
    m_downloadFormats[ QStringLiteral("128 kbit/s MP3") ] = mp3_url;
    QString vbr_url = formats.firstChildElement( QStringLiteral("vbr_zip") ).text();
    m_downloadFormats[ QStringLiteral("VBR MP3") ] = vbr_url;
    QString ogg_url = formats.firstChildElement( QStringLiteral("ogg_zip") ).text();
    m_downloadFormats[ QStringLiteral("Ogg-Vorbis") ] = ogg_url;
    QString flac_url = formats.firstChildElement( QStringLiteral("flac_zip") ).text();
    m_downloadFormats[ QStringLiteral("FLAC") ] = flac_url;
    

    m_downloadMessage = i18n( "Redownload of a previously purchased album \"%1\" by \"%2\" from Magnatune.com.\n\nUsername: %3\nPassword: %4\n", m_albumName, m_artistName, m_userName, m_password );

    return true;

}

QString
MagnatuneDownloadInfo::userName( ) const
{
    return m_userName;
}

QString
MagnatuneDownloadInfo::password( ) const
{
    return m_password;
}

QString
MagnatuneDownloadInfo::downloadMessage( ) const
{
    return m_downloadMessage;
}

DownloadFormatMap
MagnatuneDownloadInfo::formatMap() const
{
    return m_downloadFormats;
}

void
MagnatuneDownloadInfo::setFormatSelection( const QString &selectedFormat )
{
    m_selectedDownloadFormat = selectedFormat;
}

bool
MagnatuneDownloadInfo::isReadyForDownload( )
{
    return !m_selectedDownloadFormat.isEmpty();
}

QUrl
MagnatuneDownloadInfo::completeDownloadUrl( )
{
   QString url =  m_downloadFormats[ m_selectedDownloadFormat ];
   QUrl downloadUrl(url);
   downloadUrl.setUserName(m_userName);
   downloadUrl.setPassword(m_password);

   return downloadUrl;
}

void
MagnatuneDownloadInfo::setUnpackUrl( const QString &unpackUrl )
{
    m_unpackUrl = unpackUrl;
}

QString
MagnatuneDownloadInfo::unpackLocation( ) const
{
    return m_unpackUrl;
}



QString
MagnatuneDownloadInfo::albumCode() const
{
    return m_albumCode;
}



void
MagnatuneDownloadInfo::setAlbumCode( const QString &albumCode )
{
    m_albumCode = albumCode;
}

bool
MagnatuneDownloadInfo::isMembershipDownload() const
{
    return m_membershipDownload;
}

void
MagnatuneDownloadInfo::setMembershipInfo( const QString &username, const QString &password )
{
    m_userName = username;
    m_password = password;
}


const QString
MagnatuneDownloadInfo::albumName() const
{
    return m_albumName;
}

const QString
MagnatuneDownloadInfo::artistName() const
{
    return m_artistName;
}

const QString
MagnatuneDownloadInfo::coverUrl() const
{
    return m_coverUrl;
}

void
MagnatuneDownloadInfo::setAlbumName( const QString  &albumName )
{
    m_albumName = albumName;
}

void
MagnatuneDownloadInfo::setArtistName( const QString  &artistName )
{
    m_artistName = artistName;
}

void
MagnatuneDownloadInfo::setCoverUrl( const QString &coverUrl )
{
    m_coverUrl = coverUrl;
}

