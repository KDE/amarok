/*
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef AMAROK_LASTFMMETA_P_H
#define AMAROK_LASTFMMETA_P_H

#include "debug.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "lastfm.h"
#include "meta.h"

#include <QImage>
#include <QList>
#include <QStringList>

#include <kio/job.h>
#include <kio/jobclasses.h>


using namespace LastFm;

class Track::Private : public QObject
{
    Q_OBJECT

    public:
        Track *t;
        KUrl proxyUrl;
        QString lastFmUri;

        QList<Meta::TrackObserver*> observers;
        WebService *service;

        QImage albumArt;
        QString artist;
        QString album;
        QString track;
        int length;

        //not sure what these are for but they exist in the LastFmBundle
        QString albumUrl;
        QString artistUrl;
        QString trackUrl;

    public:
        void notifyObservers()
        {
            foreach( Meta::TrackObserver *observer, observers )
                observer->metadataChanged( t );
        }

    public slots:
        void metaDataFinished( int /* id */, bool error )
        {
            DEBUG_BLOCK

            AmarokHttp* http = (AmarokHttp*) sender();
            http->deleteLater();
            if( error ) return;

            const QString result( http->readAll() );
            debug() << result << endl;

            int errCode = service->parameter( "error", result ).toInt();
            if ( errCode > 0 ) {
                debug() << "Metadata failed with error code: " << errCode << endl;
                service->showError( errCode );
                return;
            }
            artist = service->parameter( "artist", result );
            album = service->parameter( "album", result );
            track = service->parameter( "track", result );
            length = service->parameter( "trackduration", result ).toInt();
            artistUrl = service->parameter( "artist_url", result );
            albumUrl = service->parameter( "album_url", result );
            trackUrl = service->parameter( "track_url", result );

            QString imageUrl = service->parameter( "albumcover_medium", result );
            if( imageUrl == "http://static.last.fm/coverart/" ||
                imageUrl == "http://static.last.fm/depth/catalogue/no_album_large.gif" )
            {
                //no image available, get default image.
                //TODO: implement retrieval of default image in Meta::Track
                notifyObservers();
                return;
            }
            KIO::Job* job = KIO::storedGet( KUrl( imageUrl ), true, false );
            connect( job, SIGNAL( result( KIO::Job* ) ), this, SLOT( fetchImageFinished( KIO::Job* ) ) );
        }

        void fetchImageFinished( KIO::Job* job )
        {
            if( job->error() == 0 ) {
                //do we still need to save the image to disk??
                const QString path = Amarok::saveLocation() + "lastfm_image.png";
                const int size = AmarokConfig::coverPreviewSize();

                QImage img = QImage::fromData( static_cast<KIO::StoredTransferJob*>( job )->data() );
                img.scaled( size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation ).save( path, "PNG" );

                albumArt = img;
            }
            else
            {
                //TODO use default image
            }
            notifyObservers();
        }
};

#endif
