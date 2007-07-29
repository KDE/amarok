//
// C++ Interface: ServiceAlbumCoverDownloader
//
// Description: 
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef SERVICEALBUMCOVERDOWNLOADER_H
#define SERVICEALBUMCOVERDOWNLOADER_H


#include "servicemetabase.h"

#include <kio/jobclasses.h>
#include <kio/job.h>
#include <KTempDir>


#include <QObject>

namespace Meta
{

//forward declaration
class ServiceAlbumCoverDownloader;

class ServiceAlbumWithCover : public ServiceAlbum
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

    virtual QPixmap image( int size, bool withShadow ) const; //overridden from Meta::Album
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

};

#endif
