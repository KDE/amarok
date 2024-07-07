/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
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

#ifndef AMAROK_PHOTOS_ENGINE
#define AMAROK_PHOTOS_ENGINE

#include "core/meta/Meta.h"
#include "core/meta/Observer.h"
#include "network/NetworkAccessManagerProxy.h"

#include <QObject>
#include <QUrl>
#include <QXmlStreamReader>


 /**
   *   This class provide photos from flickr
   *
   */
 class PhotosEngine : public QObject, public Meta::Observer
{
    Q_OBJECT
    Q_PROPERTY( int fetchSize READ fetchSize WRITE setFetchSize NOTIFY fetchSizeChanged )
    Q_PROPERTY( QStringList keywords READ keywords WRITE setKeywords NOTIFY keywordsChanged )
    Q_PROPERTY( QList<QUrl> photoUrls READ photoUrls NOTIFY photosChanged )
    Q_PROPERTY( QList<QUrl> pageUrls READ pageUrls NOTIFY photosChanged )
    Q_PROPERTY( QList<QString> photoTitles READ photoTitles NOTIFY photosChanged )
    Q_PROPERTY( Status status READ status NOTIFY statusChanged )
    Q_PROPERTY( QString error READ error NOTIFY errorChanged )
    Q_PROPERTY( QString artist READ artist NOTIFY artistChanged )

public:
    enum Status
    {
        Stopped,
        Fetching,
        Completed,
        Error
    };
    Q_ENUM( Status )

    explicit PhotosEngine( QObject* parent = nullptr );
    ~PhotosEngine() override;

    int fetchSize() const;
    void setFetchSize( int size );

    QStringList keywords() const;
    void setKeywords( const QStringList &keywords );

    // reimplemented from Meta::Observer
    using Observer::metadataChanged;
    void metadataChanged( const Meta::TrackPtr &track ) override;

    QList<QUrl> photoUrls() const;
    QList<QUrl> pageUrls() const;
    QList<QString> photoTitles() const;

    Status status() const { return m_status; }
    QString error() const { return m_error; }
    QString artist() const { return m_artist; }

Q_SIGNALS:
    void fetchSizeChanged();
    void keywordsChanged();
    void photosChanged();
    void statusChanged();
    void errorChanged();
    void artistChanged();

private Q_SLOTS:

    /**
     * This slots will handle Flickr result for this query :
     * API key is : 9c5a288116c34c17ecee37877397fe31
     * Secret is : cc25e5a9532ddc97
     * http://api.flickr.com/services/rest/?method=flickr.photos.search&api_key=9c5a288116c34c17ecee37877397fe31&text=My+Bloody+Valentine
     * see here for details: http://www.flickr.com/services/api/
     */
    void resultFlickr(const QUrl &url, const QByteArray &data, const NetworkAccessManagerProxy::Error &e );

    void stopped();
    void trackChanged( const Meta::TrackPtr &track );

private:
    struct PhotoInfo
    {
        QString title;      // Name of the phtos
        QUrl urlphoto;      // url of the photos, for the download
        QUrl urlpage;       // Url for the browser ( http://www.flickr.com/photos/wanderlustg/322285063/ )

        bool operator==( const PhotoInfo &other ) const
        {
            return title == other.title &&
                   urlphoto == other.urlphoto &&
                   urlpage == other.urlpage;
        }
    };

    /**
     * Engine was updated, so we check if the songs is different, and if it is, we delete every and start
     * all the query/ fetching stuff
     */
    void update( bool force = false );

    void setPhotos( const QList<PhotoInfo> &photos );
    void setStatus( Status status );
    void setError( const QString &error );
    void setArtist( const QString &artist );

    QList<PhotoInfo> photosListFromXml( QXmlStreamReader &xml );

    // TODO implement a reload
    void reloadPhotos();

    int m_nbPhotos;

    QSet<QUrl> m_flickrUrls;
    QList<PhotoInfo> m_photos;

    Meta::TrackPtr m_currentTrack;
    // Cache the artist of the current track so we can check against metadata
    // updates. We only want to update the photos if the artist change

    QString m_artist;
    QStringList m_keywords;
    Status m_status;
    QString m_error;
};

#endif
