/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#ifndef SERVICEALBUMCOVERDOWNLOADER_H
#define SERVICEALBUMCOVERDOWNLOADER_H


#include "servicemetabase.h"

#include "amarok_export.h"
#include <kio/jobclasses.h>
#include <kio/job.h>
#include <KTempDir>


#include <QObject>

namespace Meta
{

//forward declaration
class ServiceAlbumCoverDownloader;

class AMAROK_EXPORT ServiceAlbumWithCover : public ServiceAlbum
{
private:

    mutable QImage m_cover;
    mutable bool m_hasFetchedCover;
    mutable bool m_isFetchingCover;
    QString m_coverDownloadPath;
    mutable ServiceAlbumCoverDownloader * m_coverDownloader;

public:
    
    ServiceAlbumWithCover( QString name );
    ServiceAlbumWithCover( QStringList resultRow );
    
    virtual ~ServiceAlbumWithCover();
    
    virtual QString downloadPrefix() const = 0;

    virtual void setCoverUrl( const QString &coverUrl ) = 0;
    virtual QString coverUrl() const = 0;

    void setImage( const QImage & image ) const;
    void imageDownloadCanceled() const;

    virtual QPixmap image( int size = 1, bool withShadow = false); //overridden from Meta::Album
};


/**
A helper class for fetching covers from online services

	@author 
*/
class ServiceAlbumCoverDownloader : public QObject
{
    Q_OBJECT

    public:

        ServiceAlbumCoverDownloader();
        ~ServiceAlbumCoverDownloader();

        void downloadCover( const Meta::ServiceAlbumWithCover * album );

    private slots:

        void coverDownloadComplete( KJob * downloadJob );
        void coverDownloadCanceled( KJob * downloadJob );
    private:
        const ServiceAlbumWithCover * m_album;
        QString m_coverDownloadPath;
        KIO::FileCopyJob * m_albumDownloadJob;
        KTempDir * m_tempDir;
};

}

#endif
