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

#include "Mp3tunesHarmonyDownload.h"

Mp3tunesHarmonyDownload::Mp3tunesHarmonyDownload()
    : m_fileKey()
    , m_fileName()
    , m_fileFormat()
    , m_fileSize( 0 )
    , m_artistName()
    , m_albumTitle()
    , m_trackTitle()
    , m_trackNumber( 0 )
    , m_deviceBitrate()
    , m_fileBitrate()
    , m_url()
{}

Mp3tunesHarmonyDownload::Mp3tunesHarmonyDownload( mp3tunes_harmony_download_t *download )
{

    m_fileKey = download->file_key;
    m_fileName = download->file_name;
    m_fileFormat = download->file_format;
    m_fileSize = download->file_size;
    m_artistName = download->artist_name ;
    if( download->album_title )
        m_albumTitle = download->album_title;
    else
        m_albumTitle.clear();

    m_trackTitle = download->track_title;
    m_trackNumber = download->track_number;
    m_deviceBitrate = download->device_bitrate;
    m_fileBitrate = download->file_bitrate;
    if( download->url )
        m_url = download->url;
    else
        m_url.clear();
}

Mp3tunesHarmonyDownload::Mp3tunesHarmonyDownload( const QVariantMap &map )
{
    m_fileKey = map["fileKey"].toString();
    m_fileName = map["fileName"].toString();
    m_fileFormat = map["fileFormat"].toString();
    m_fileSize = map["fileSize"].toInt();
    m_artistName = map["artistName"].toString();
    m_albumTitle = map["albumTitle"].toString();
    m_trackTitle = map["trackTitle"].toString();
    m_trackNumber = map["trackNumber"].toInt();
    m_deviceBitrate = map["deviceBitrate"].toString();
    m_fileBitrate = map["fileBitrate"].toString();
    m_url = map["url"].toString();
}

Mp3tunesHarmonyDownload::~Mp3tunesHarmonyDownload()
{}

QString
Mp3tunesHarmonyDownload::fileKey() const
{
    return m_fileKey;
}

QString
Mp3tunesHarmonyDownload::fileName() const
{
    return m_fileName;
}

QString
Mp3tunesHarmonyDownload::fileFormat() const
{
    return m_fileFormat;
}

unsigned int
Mp3tunesHarmonyDownload::fileSize() const
{
    return m_fileSize;
}

QString
Mp3tunesHarmonyDownload::artistName() const
{
    return m_artistName;
}

QString
Mp3tunesHarmonyDownload::albumTitle() const
{
    return m_albumTitle;
}

QString
Mp3tunesHarmonyDownload::trackTitle() const
{
    return m_trackTitle;
}

int
Mp3tunesHarmonyDownload::trackNumber() const
{
    return m_trackNumber;
}

QString
Mp3tunesHarmonyDownload::deviceBitrate() const
{
    return m_deviceBitrate;
}

QString
Mp3tunesHarmonyDownload::fileBitrate() const
{
    return m_fileBitrate;
}

QString
Mp3tunesHarmonyDownload::url() const
{
    return m_url;
}

QVariantMap Mp3tunesHarmonyDownload::serialize() const
{
  QVariantMap map;
  map["fileKey"] = m_fileKey;
  map["fileName"] = m_fileName;
  map["fileFormat"] = m_fileFormat;
  map["fileSize"] = m_fileSize;
  map["artistName"] = m_artistName;
  map["albumTitle"] = m_albumTitle;
  map["trackTitle"] = m_trackTitle;
  map["trackNumber"] = m_trackNumber;
  map["deviceBitrate"] = m_deviceBitrate;
  map["fileBitrate"] = m_fileBitrate;
  map["url"] = m_url;
  return map;
}
