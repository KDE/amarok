/*
  Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/


#include "magnatunedownloadinfo.h"


#include "debug.h"

#include <qfile.h>

MagnatuneDownloadInfo::MagnatuneDownloadInfo()
{
    m_selectedDownloadFormat = QString::null;
    m_albumId = -1;
}


MagnatuneDownloadInfo::~MagnatuneDownloadInfo()
{}

bool MagnatuneDownloadInfo::initFromFile( QString downloadInfoFileName )
{
    QString xml;

    QFile file( downloadInfoFileName );
    if ( file.open( IO_ReadOnly ) )
    {
        QTextStream stream( &file );
        while ( !stream.atEnd() )
        {
            xml += (stream.readLine() + '\n');
        }
        file.close();
    } else {
        debug() << "Error opening file: '" << downloadInfoFileName << "'" << endl;
        return false;
    }


    //debug() << "XML from file: '" << xml << "'" << endl;
    return initFromString( xml );
}

bool MagnatuneDownloadInfo::initFromString( QString downloadInfoString )
{

    //complete overkill to do a full SAX2 parser for this at the moment... I think....

    // lets make sure that this is actually a valid result

    int testIndex = downloadInfoString.find( "<RESULT>" );
    if ( testIndex == -1 )
    {
        return false;
    };

    int startIndex;
    int endIndex;

    startIndex = downloadInfoString.find( "<DL_USERNAME>", 0, false );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.find( "</DL_USERNAME>", 0, false );
        if ( endIndex != -1 )
        {
            startIndex += 13;

            debug() << "found username: " << downloadInfoString.mid( startIndex, endIndex - startIndex ) << endl;
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


    startIndex = downloadInfoString.find( "<DL_PASSWORD>", 0, false );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.find( "</DL_PASSWORD>", 0, false );
        if ( endIndex != -1 )
        {
            startIndex += 13;
            debug() << "found password: " << downloadInfoString.mid( startIndex, endIndex - startIndex ) << endl;
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


    startIndex = downloadInfoString.find( "<URL_WAVZIP>", 0, false );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.find( "</URL_WAVZIP>", 0, false );
        if ( endIndex != -1 )
        {
            startIndex += 12;
            debug() << "found wav" << endl;
            m_downloadFormats[ "Wav" ] = downloadInfoString.mid( startIndex, endIndex - startIndex );

        }
    }

    startIndex = downloadInfoString.find( "<URL_128KMP3ZIP>", 0, false );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.find( "</URL_128KMP3ZIP>", 0, false );
        if ( endIndex != -1 )
        {
            startIndex += 16;
            debug() << "found 128k mp3" << endl;
            m_downloadFormats[ "128 kbit/s MP3" ] = downloadInfoString.mid( startIndex, endIndex - startIndex );

        }
    }

    startIndex = downloadInfoString.find( "<URL_OGGZIP>", 0, false );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.find( "</URL_OGGZIP>", 0, false );
        if ( endIndex != -1 )
        {
            startIndex += 12;
            debug() << "found ogg-vorbis" << endl;
            m_downloadFormats[ "Ogg-Vorbis" ] = downloadInfoString.mid( startIndex, endIndex - startIndex );

        }
    }

    startIndex = downloadInfoString.find( "<URL_VBRZIP>", 0, false );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.find( "</URL_VBRZIP>", 0, false );
        if ( endIndex != -1 )
        {
            startIndex += 12;
            debug() << "found vbr mp3" << endl;
            m_downloadFormats[ "VBR MP3" ] = downloadInfoString.mid( startIndex, endIndex - startIndex );

        }
    }

    startIndex = downloadInfoString.find( "<URL_FLACZIP>", 0, false );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.find( "</URL_FLACZIP>", 0, false );
        if ( endIndex != -1 )
        {
            startIndex += 13;
            debug() << "found flac" << endl;
            m_downloadFormats[ "FLAC" ] = downloadInfoString.mid( startIndex, endIndex - startIndex );

        }
    }

    startIndex = downloadInfoString.find( "<DL_MSG>", 0, false );
    if ( startIndex != -1 )
    {
        endIndex = downloadInfoString.find( "</DL_MSG>", 0, false );
        if ( endIndex != -1 )
        {
            startIndex += 9;
            debug() << "found dl-message" << endl;
            m_downloadMessage = downloadInfoString.mid( startIndex, endIndex - startIndex );
        }
    }

    return true;
}

QString MagnatuneDownloadInfo::getUserName( )
{
    return m_userName;
}

QString MagnatuneDownloadInfo::getPassword( )
{
    return m_password;
}

QString MagnatuneDownloadInfo::getDownloadMessage( )
{
    return m_downloadMessage;
}

DownloadFormatMap MagnatuneDownloadInfo::getFormatMap()
{
    return m_downloadFormats;
}

void MagnatuneDownloadInfo::setFormatSelection( QString selectedFormat )
{
    m_selectedDownloadFormat = selectedFormat;
}

bool MagnatuneDownloadInfo::isReadyForDownload( )
{
    return !m_selectedDownloadFormat.isEmpty();
}

KURL MagnatuneDownloadInfo::getCompleteDownloadUrl( )
{
   QString url =  m_downloadFormats[ m_selectedDownloadFormat ];
   KURL downloadUrl(url);
   downloadUrl.setUser(m_userName);
   downloadUrl.setPass(m_password);

   return downloadUrl;
}

void MagnatuneDownloadInfo::setUnpackUrl( QString unpackUrl )
{
    m_unpackUrl = unpackUrl;
}

QString MagnatuneDownloadInfo::getUnpackLocation( )
{
    return m_unpackUrl;
}



int MagnatuneDownloadInfo::getAlbumId()
{
    return m_albumId;
}



void MagnatuneDownloadInfo::setAlbumId(int id)
{
    m_albumId = id;
}


