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

#include "ContextObserver.h"
#include "context/DataEngine.h"
#include "core/meta/Meta.h"
#include "NetworkAccessManagerProxy.h"
#include "PhotosInfo.h"

using namespace Context;

 /**
   *   This class provide photos from flickr
   *
   */
class PhotosEngine : public DataEngine, public ContextObserver, Meta::Observer
{
    Q_OBJECT
public:
    PhotosEngine( QObject* parent, const QList<QVariant>& args );
    virtual ~PhotosEngine();

    QStringList sources() const;
    // reimplemented from Context::Observer
    virtual void message( const ContextState& state );
    // reimplemented from Meta::Observer
    using Observer::metadataChanged;
    void metadataChanged( Meta::TrackPtr track );

protected:
    //reimplement from Plasma::DataEngine
    bool sourceRequestEvent( const QString& name );

private slots:

 /**
   *   This slots will handle Flickr result for this query :
   *   API key is : 9c5a288116c34c17ecee37877397fe31
   *   Secret is : cc25e5a9532ddc97
   *   http://api.flickr.com/services/rest/?method=flickr.photos.search&api_key=9c5a288116c34c17ecee37877397fe31&text=My+Bloody+Valentine
   *   see here for details: http://www.flickr.com/services/api/
   */
    void resultFlickr( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );

  /**
   *   An image fetcher, will store the QPixmap in the corresponding videoInfo
   */
    void resultImageFetcher( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );

  /**
   *   This method will send the info to the applet and order them if every job are finished
   */
    void resultFinalize();


private:
  /**
   *   Engine was updated, so we check if the songs is different, and if it is, we delete every and start
   *   all the query/ fetching stuff
   */
    void update();

    // TODO implement a reload
    void reloadPhotos();

    int m_nbFlickr;
    int m_nbPhotos;

    QSet<KUrl> m_flickrUrls;
    QSet<KUrl> m_imageUrls;

    QStringList m_sources;

    Meta::TrackPtr m_currentTrack;
    // Cache the artist of the current track so we can check against metadata
    // updates. We only want to update the photos if the artist change

    QString    m_artist;
    QString    m_keywords;

    // stores what features are enabled
    bool m_requested;
    bool m_reload;

    //!  List containing all the info
    QList < PhotosInfo *>m_photos;      // Item with all the information
    QList < PhotosInfo *>m_photosInit;  // Item not finished to download

};

Q_DECLARE_METATYPE ( QList < PhotosInfo * > )
K_EXPORT_AMAROK_DATAENGINE( photos, PhotosEngine )
#endif

