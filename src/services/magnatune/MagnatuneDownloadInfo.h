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

#ifndef MAGNATUNE_DOWNLOAD_INFO_H
#define MAGNATUNE_DOWNLOAD_INFO_H

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
    bool initFromRedownloadXml( const QDomElement &element );

    void setMembershipInfo( const QString &username, const QString &password );
    bool isMembershipDownload();

    DownloadFormatMap formatMap();
    QString userName();
    QString password();
    QString downloadMessage();
    QString albumCode();

    QString albumName();
    QString artistName();
    QString coverUrl();

    void setAlbumName( const QString  &albumName );
    void setArtistName( const QString  &artistName );
    void setCoverUrl( const QString &coverUrl );
    void setFormatSelection( const QString &selectedFormat );
    void setUnpackUrl( const QString &unpackUrl );
    void setAlbumCode( const QString & albumCode );
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
    QString m_albumCode;
    QString m_coverUrl;
    
    bool m_membershipDownload;

    //the following members are for storing the user selections regarding a download
    QString m_unpackUrl;
    QString m_selectedDownloadFormat;

};

#endif
