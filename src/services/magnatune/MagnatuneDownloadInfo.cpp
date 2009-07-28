/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MagnatuneDownloadInfo.h"


#include "Debug.h"

#include <QFile>
#include <QTextStream>

using namespace Meta;

MagnatuneDownloadInfo::MagnatuneDownloadInfo()
{
    m_selectedDownloadFormat.clear();
    m_album = 0;
}


MagnatuneDownloadInfo::~MagnatuneDownloadInfo()
{}

bool MagnatuneDownloadInfo::initFromFile( const QString &downloadInfoFileName, bool membershipDownload  )
{
    QString xml;

    QFile file( downloadInfoFileName );
    if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        QTextStream stream( &file );
        while ( !stream.atEnd() )
        {
            xml += (stream.readLine() + '\n');
        }
        file.close();
    } else {
        debug() << "Error opening file: '" << downloadInfoFileName << "'";
        return false;
    }


    //debug() << "XML from file: '" << xml << "'";
    return initFromString( xml, membershipDownload );
}

bool MagnatuneDownloadInfo::initFromString( const QString &downloadInfoString, bool membershipDownload  )
{

    m_membershipDownload = membershipDownload;
    //complete overkill to do a full SAX2 parser for this at the moment... I think....

    // lets make sure that this is actually a valid result

    int testIndex = downloadInfoString.indexOf( "<RESULT>" );
    if ( testIndex == -1 )
    {
        return false;
    };

    int startIndex;
    int endIndex;

    if ( membershipDownload == false ) {
        startIndex = downloadInfoString.indexOf( "<DL_USERNAME>", 0, Qt::CaseInsensitive );
        if ( startIndex != -1 )
        {
            endIndex = downloadInfoString.indexOf( "</DL_USERNAME>", 0, Qt::CaseInsensitive );
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


        startIndex = downloadInfoString.indexOf( "<DL_PASSWORD>", 0, Qt::CaseInsensitive );
        if ( startIndex != -1 )
        {
            endIndex = downloadInfoString.indexOf( "</DL_PASSWORD>", 0, Qt::CaseInsensitive );
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


    startIndex = downloadInfoString.indexOf( "<URL_WAVZIP>", 0, Qt::CaseInsensitive );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.indexOf( "</URL_WAVZIP>", 0, Qt::CaseInsensitive );
        if ( endIndex != -1 )
        {
            startIndex += 12;
            debug() << "found wav";
            m_downloadFormats[ "Wav" ] = downloadInfoString.mid( startIndex, endIndex - startIndex ).replace( "&amp;", "&" );

        }
    }

    startIndex = downloadInfoString.indexOf( "<URL_128KMP3ZIP>", 0, Qt::CaseInsensitive );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.indexOf( "</URL_128KMP3ZIP>", 0, Qt::CaseInsensitive );
        if ( endIndex != -1 )
        {
            startIndex += 16;
            debug() << "found 128k mp3";
            m_downloadFormats[ "128 kbit/s MP3" ] = downloadInfoString.mid( startIndex, endIndex - startIndex ).replace( "&amp;", "&" );

        }
    }

    startIndex = downloadInfoString.indexOf( "<URL_OGGZIP>", 0, Qt::CaseInsensitive );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.indexOf( "</URL_OGGZIP>", 0, Qt::CaseInsensitive );
        if ( endIndex != -1 )
        {
            startIndex += 12;
            debug() << "found ogg-vorbis";
            m_downloadFormats[ "Ogg-Vorbis" ] = downloadInfoString.mid( startIndex, endIndex - startIndex ).replace( "&amp;", "&" );

        }
    }

    startIndex = downloadInfoString.indexOf( "<URL_VBRZIP>", 0, Qt::CaseInsensitive );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.indexOf( "</URL_VBRZIP>", 0, Qt::CaseInsensitive );
        if ( endIndex != -1 )
        {
            startIndex += 12;
            debug() << "found vbr mp3";
            m_downloadFormats[ "VBR MP3" ] = downloadInfoString.mid( startIndex, endIndex - startIndex ).replace( "&amp;", "&" );

        }
    }

    startIndex = downloadInfoString.indexOf( "<URL_FLACZIP>", 0, Qt::CaseInsensitive );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.indexOf( "</URL_FLACZIP>", 0, Qt::CaseInsensitive );
        if ( endIndex != -1 )
        {
            startIndex += 13;
            debug() << "found flac";
            m_downloadFormats[ "FLAC" ] = downloadInfoString.mid( startIndex, endIndex - startIndex ).replace( "&amp;", "&" );

        }
    }

    startIndex = downloadInfoString.indexOf( "<DL_MSG>", 0, Qt::CaseInsensitive );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.indexOf( "</DL_MSG>", 0, Qt::CaseInsensitive );
        if ( endIndex != -1 )
        {
            startIndex += 9;
            debug() << "found dl-message";
            m_downloadMessage = downloadInfoString.mid( startIndex, endIndex - startIndex ).replace( "&amp;", "&" );
        }
    }

    return true;
}

bool MagnatuneDownloadInfo::initFromRedownloadXml( const QDomElement &element )
{


    m_artistName = element.firstChildElement( "artist" ).text();
    m_albumName = element.firstChildElement( "album" ).text();
    m_userName = element.firstChildElement( "username" ).text();
    m_password = element.firstChildElement( "password" ).text();

    //get formats
    QDomNode formats = element.firstChildElement( "formats" );

    QString wav_url = formats.firstChildElement( "wav_zip" ).text();
    m_downloadFormats[ "Wav" ] = wav_url;
    QString mp3_url = formats.firstChildElement( "mp3_zip" ).text();
    m_downloadFormats[ "128 kbit/s MP3" ] = mp3_url;
    QString vbr_url = formats.firstChildElement( "vbr_zip" ).text();
    m_downloadFormats[ "VBR MP3" ] = vbr_url;
    QString ogg_url = formats.firstChildElement( "ogg_zip" ).text();
    m_downloadFormats[ "Ogg-Vorbis" ] = ogg_url;
    QString flac_url = formats.firstChildElement( "flac_zip" ).text();
    m_downloadFormats[ "FLAC" ] = flac_url;
    

    m_downloadMessage = i18n( "Redownload of a previously purchased album \"%1\" by \"%2\" from Magnatune.com.\n\nUsername: %3\nPassword: %4\n", m_albumName, m_artistName, m_userName, m_password );

    return true;

}

QString MagnatuneDownloadInfo::userName( )
{
    return m_userName;
}

QString MagnatuneDownloadInfo::password( )
{
    return m_password;
}

QString MagnatuneDownloadInfo::downloadMessage( )
{
    return m_downloadMessage;
}

DownloadFormatMap MagnatuneDownloadInfo::formatMap()
{
    return m_downloadFormats;
}

void MagnatuneDownloadInfo::setFormatSelection( const QString &selectedFormat )
{
    m_selectedDownloadFormat = selectedFormat;
}

bool MagnatuneDownloadInfo::isReadyForDownload( )
{
    return !m_selectedDownloadFormat.isEmpty();
}

KUrl MagnatuneDownloadInfo::completeDownloadUrl( )
{
   QString url =  m_downloadFormats[ m_selectedDownloadFormat ];
   KUrl downloadUrl(url);
   downloadUrl.setUser(m_userName);
   downloadUrl.setPass(m_password);

   return downloadUrl;
}

void MagnatuneDownloadInfo::setUnpackUrl( const QString &unpackUrl )
{
    m_unpackUrl = unpackUrl;
}

QString MagnatuneDownloadInfo::unpackLocation( )
{
    return m_unpackUrl;
}



MagnatuneAlbum * MagnatuneDownloadInfo::album()
{
    return m_album;
}



void MagnatuneDownloadInfo::setAlbum(MagnatuneAlbum * album )
{
    m_album = album;
}

bool MagnatuneDownloadInfo::isMembershipDownload()
{
    return m_membershipDownload;
}

void MagnatuneDownloadInfo::setMembershipInfo( const QString &username, const QString &password )
{
    m_userName = username;
    m_password = password;
}

QString MagnatuneDownloadInfo::albumName()
{
    return m_albumName; 
}

QString MagnatuneDownloadInfo::artistName()
{
    return m_artistName;
}

