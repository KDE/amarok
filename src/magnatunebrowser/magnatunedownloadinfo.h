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


#ifndef MAGNATUNE_DOWNLOAD_INFO_H
#define MAGNATUNE_DOWNLOAD_INFO_H

#include <kurl.h>

#include <qmap.h>
#include <qstring.h>


typedef QMap<QString, QString> DownloadFormatMap;

/**
Class for parsing and storing the info from a download xml file or string 

    @author Nikolaj Hald Nielsen
*/
class MagnatuneDownloadInfo{
public:
    MagnatuneDownloadInfo();
    ~MagnatuneDownloadInfo();

    bool initFromString( QString downloadInfoString );
    bool initFromFile( QString downloadInfoFileName );

    DownloadFormatMap getFormatMap();
    QString getUserName();
    QString getPassword();
    QString getDownloadMessage();
    int getAlbumId();


    void setFormatSelection(QString selectedFormat);
    void setUnpackUrl(QString unpackUrl);
    void setAlbumId(int id);
    bool isReadyForDownload();
    KURL getCompleteDownloadUrl();
    QString getUnpackLocation();



protected:

    DownloadFormatMap m_downloadFormats;
    QString m_userName;
    QString m_password;
    QString m_downloadMessage;

    int m_albumId;

    //the following members are for storing the user selections regarding a download
    QString m_unpackUrl;
    QString m_selectedDownloadFormat;

};

#endif
