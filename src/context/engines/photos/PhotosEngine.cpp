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

#define DEBUG_PREFIX "PhotosEngine"

#include "PhotosEngine.h"

#include "EngineController.h"
#include "context/ContextView.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QXmlStreamReader>
#include <QPixmap>

using namespace Context;

PhotosEngine::PhotosEngine( QObject* parent, const QList<QVariant>& /*args*/ )
        : DataEngine( parent )
        , m_nbPhotos( 10 )
{
    m_sources << "flickr" ;
}

PhotosEngine::~PhotosEngine()
{
}

void
PhotosEngine::init()
{
    DEBUG_BLOCK
    EngineController *controller = The::engineController();
    connect( controller, SIGNAL(trackMetadataChanged(Meta::TrackPtr)), SLOT(trackChanged(Meta::TrackPtr)) );
    connect( controller, SIGNAL(trackChanged(Meta::TrackPtr)), SLOT(trackChanged(Meta::TrackPtr)) );
    connect( controller, SIGNAL(stopped(qint64,qint64)), SLOT(stopped()) );
}

void
PhotosEngine::stopped()
{
    DEBUG_BLOCK
    removeAllData( "photos" );
    setData( "photos", "message", "stopped" );
    m_artist.clear();
    m_currentTrack.clear();
}

void
PhotosEngine::trackChanged( Meta::TrackPtr track )
{
    if( !track )
        return;

    update();
}

QStringList
PhotosEngine::sources() const
{
    return m_sources;
}

int
PhotosEngine::fetchSize() const
{
    return m_nbPhotos;
}

void
PhotosEngine::setFetchSize( int size )
{
    m_nbPhotos = size;
}

QStringList
PhotosEngine::keywords() const
{
    return m_keywords;
}

void
PhotosEngine::setKeywords( const QStringList &keywords )
{
    m_keywords = keywords;
}

bool
PhotosEngine::sourceRequestEvent( const QString& name )
{
    DEBUG_BLOCK
    bool force( false );
    QStringList tokens = name.split( QLatin1Char(':'), QString::SkipEmptyParts );
    if( tokens.contains( QLatin1String("forceUpdate") ) )
        force = true;
    update( force );
    return true;
}

void
PhotosEngine::metadataChanged( Meta::TrackPtr track )
{
    const bool hasChanged = !track->artist() || track->artist()->name() != m_artist;
    if ( hasChanged )
        update();
}

void
PhotosEngine::update( bool force )
{
    QString tmpYoutStr;
    // prevent
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( !currentTrack || !currentTrack->artist() )
    {
        debug() << "invalid current track";
        setData( "photos", Plasma::DataEngine::Data() );
        return;
    }
    else if( !force && currentTrack->artist()->name() == m_artist )
    {
        debug() << "artist name unchanged";
        setData( "photos", Plasma::DataEngine::Data() );
        return;
    }
    else
    {
        unsubscribeFrom( m_currentTrack );
        m_currentTrack = currentTrack;
        subscribeTo( currentTrack );

        if ( !currentTrack )
            return;

        // Save artist
        m_artist = currentTrack->artist()->name();

        removeAllData( "photos" );

        // Show the information
        if( !m_artist.isEmpty() )
        {
            setData( "photos", "message", "Fetching");
            setData( "photos", "artist", m_artist );
        }
        else
        {
            removeAllData( "photos" );
            return;
        }

        QStringList tags = m_keywords;
        tags << m_artist;
        tags.removeDuplicates();

        // Query flickr, order by relevance, 10 max
        // Flickr :http://api.flickr.com/services/rest/?method=flickr.photos.search&api_key=9c5a288116c34c17ecee37877397fe31&text=ARTIST&per_page=20
        QUrl flickrUrl;
        flickrUrl.setScheme( "http" );
        flickrUrl.setHost( "api.flickr.com" );
        flickrUrl.setPath( "/services/rest/" );
        flickrUrl.addQueryItem( "method", "flickr.photos.search" );
        flickrUrl.addQueryItem( "api_key", Amarok::flickrApiKey() );
        flickrUrl.addQueryItem( "per_page", QString::number( m_nbPhotos ) );
        flickrUrl.addQueryItem( "sort", "date-posted-desc" );
        flickrUrl.addQueryItem( "media", "photos" );
        flickrUrl.addQueryItem( "content_type", QString::number(1) );
        flickrUrl.addQueryItem( "text", tags.join(" ") );
        debug() << "Flickr url:" << flickrUrl;

        m_flickrUrls << flickrUrl;
        The::networkAccessManager()->getData( flickrUrl, this,
             SLOT(resultFlickr(QUrl,QByteArray,NetworkAccessManagerProxy::Error)) );

    }
}

void
PhotosEngine::resultFlickr( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( !m_flickrUrls.contains( url ) )
        return;

    DEBUG_BLOCK
    m_flickrUrls.remove( url );
    if( e.code != QNetworkReply::NoError )
    {
        setData( "photos", "message", i18n( "Unable to retrieve from Flickr.com: %1", e.description ) );
        debug() << "Unable to retrieve Flickr information:" << e.description;
        return;
    }

    if( data.isNull() )
    {
        debug() << "Got bad xml!";
        return;
    }

    removeAllData( "photos" );
    QXmlStreamReader xml( data );
    PhotosInfo::List photosInfo = photosListFromXml( xml );
    debug() << "got" << photosInfo.size() << "photo info";
    setData( "photos", "artist", m_artist );
    setData( "photos", "data", qVariantFromValue( photosInfo ) );
}

PhotosInfo::List
PhotosEngine::photosListFromXml( QXmlStreamReader &xml )
{
    PhotosInfo::List photoList;
    xml.readNextStartElement(); // rsp
    if( xml.attributes().value(QLatin1String("stat")) != QLatin1String("ok") )
        return photoList;

    xml.readNextStartElement(); // photos
    while( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String("photo") )
        {
            const QXmlStreamAttributes &attr = xml.attributes();
            QStringRef id     = attr.value( QLatin1String("id") );
            QStringRef farm   = attr.value( QLatin1String("farm") );
            QStringRef owner  = attr.value( QLatin1String("owner") );
            QStringRef secret = attr.value( QLatin1String("secret") );
            QStringRef server = attr.value( QLatin1String("server") );
            QStringRef title  = attr.value( QLatin1String("title") );

            QUrl photoUrl;
            photoUrl.setScheme( "http" );
            photoUrl.setHost( QString("farm%1.static.flickr.com").arg( farm.toString() ) );
            photoUrl.setPath( QString("/%1/%2_%3.jpg").arg( server.toString(), id.toString(), secret.toString() ) );

            QUrl pageUrl;
            pageUrl.setScheme( "http" );
            pageUrl.setHost( QLatin1String("www.flickr.com") );
            pageUrl.setPath( QString("/photos/%1/%2").arg( owner.toString(), id.toString() ) );

            PhotosInfoPtr info( new PhotosInfo );
            info->title = title.toString();
            info->urlpage = pageUrl;
            info->urlphoto = photoUrl;
            photoList.append( info );
        }
        xml.skipCurrentElement();
    }
    return photoList;
}

