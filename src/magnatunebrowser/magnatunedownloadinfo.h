//
// C++ Interface: magnatune_download_info
//
// Description: 
//
//
// Author: Mark Kretschmann <markey@web.de>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
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


    void setFormatSelection(QString selectedFormat);
    void setUnpackUrl(QString unpackUrl);
    bool isReadyForDownload();
    KURL getCompleteDownloadUrl();
    QString getUnpackLocation();



protected:

    DownloadFormatMap m_downloadFormats;
    QString m_userName;
    QString m_password;
    QString m_downloadMessage;

    //the following members are for storing the user selections regarding a download
    QString m_unpackUrl;
    QString m_selectedDownloadFormat;

};

#endif
