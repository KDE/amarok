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

#ifndef MAGNATUNE_DOWNLOAD_INFO_H
#define MAGNATUNE_DOWNLOAD_INFO_H

#include "MagnatuneMeta.h"

#include <kurl.h>

#include <QDomElement>
#include <qmap.h>
#include <QString>


typedef QMap<QString, QString> DownloadFormatMap;

/**
Class for parsing and storing the info from a download xml file or string

    @author Nikolaj Hald Nielsen
*/
class MagnatuneDownloadInfo{
public:
    MagnatuneDownloadInfo();
    ~MagnatuneDownloadInfo();

    bool initFromString( const QString &downloadInfoString, bool membershipDownload );
    bool initFromFile( const QString &downloadInfoFileName, bool membershipDownload );
    bool initFromRedownloadXml( const QDomElement &element );

    void setMembershipInfo( const QString &username, const QString &password );
    bool isMembershipDownload();

    DownloadFormatMap formatMap();
    QString userName();
    QString password();
    QString downloadMessage();
    Meta::MagnatuneAlbum * album();

    QString albumName();
    QString artistName();


    void setFormatSelection( const QString &selectedFormat );
    void setUnpackUrl( const QString &unpackUrl );
    void setAlbum( Meta::MagnatuneAlbum * album );
    bool isReadyForDownload();
    KUrl completeDownloadUrl();
    QString unpackLocation();



protected:

    DownloadFormatMap m_downloadFormats;
    QString m_userName;
    QString m_password;
    QString m_downloadMessage;

    QString m_artistName;
    QString m_albumName;

    Meta::MagnatuneAlbum * m_album;
    bool m_membershipDownload;

    //the following members are for storing the user selections regarding a download
    QString m_unpackUrl;
    QString m_selectedDownloadFormat;

};

#endif
