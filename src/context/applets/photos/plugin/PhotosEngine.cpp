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

#define DEBUG_PREFIX "Photos"

#include "PhotosEngine.h"

#include "EngineController.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QXmlStreamReader>
#include <QUrlQuery>


PhotosEngine::PhotosEngine( QObject* parent )
    : QObject( parent )
    , m_nbPhotos( 10 )
    , m_status( Stopped )
{
    DEBUG_BLOCK

    EngineController *controller = The::engineController();
    connect( controller, &EngineController::trackMetadataChanged, this, &PhotosEngine::trackChanged );
    connect( controller, &EngineController::trackChanged, this, &PhotosEngine::trackChanged );
    connect( controller, &EngineController::stopped, this, &PhotosEngine::stopped );
}

PhotosEngine::~PhotosEngine()
{
}

void
PhotosEngine::stopped()
{
    DEBUG_BLOCK

    setPhotos( QList<PhotoInfo>() );
    setStatus( Stopped );
    setArtist( QString() );
    m_currentTrack.clear();
}

void
PhotosEngine::trackChanged( const Meta::TrackPtr &track )
{
    if( !track )
        return;

    update();
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
    if( m_keywords == keywords )
        return;

    m_keywords = keywords;
    Q_EMIT keywordsChanged();
}

void
PhotosEngine::metadataChanged(const Meta::TrackPtr &track )
{
    const bool hasChanged = !track->artist() || track->artist()->name() != m_artist;
    if ( hasChanged )
        update();
}

void
PhotosEngine::update( bool force )
{
    // prevent
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( !currentTrack || !currentTrack->artist() )
    {
        debug() << "invalid current track";
        setPhotos( QList<PhotoInfo>() );
        return;
    }
    else if( !force && currentTrack->artist()->name() == m_artist )
    {
        debug() << "artist name unchanged";
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
        setArtist( currentTrack->artist()->name() );
        setPhotos( QList<PhotoInfo>() );

        // Show the information
        if( !m_artist.isEmpty() )
        {
            setStatus( Fetching );
        }
        else
        {
            setPhotos( QList<PhotoInfo>() );
            return;
        }

        QStringList tags = m_keywords;
        tags << m_artist;
        tags << QStringLiteral("music");
        tags.removeDuplicates();

        // Query flickr, order by relevance, 10 max
        // Flickr :http://api.flickr.com/services/rest/?method=flickr.photos.search&api_key=9c5a288116c34c17ecee37877397fe31&text=ARTIST&per_page=20
        QUrl flickrUrl;
        QUrlQuery query;
        flickrUrl.setScheme( QStringLiteral("https") );
        flickrUrl.setHost( QStringLiteral("api.flickr.com") );
        flickrUrl.setPath( QStringLiteral("/services/rest/") );
        query.addQueryItem( QStringLiteral("method"), QStringLiteral("flickr.photos.search") );
        query.addQueryItem( QStringLiteral("api_key"), QLatin1String(Amarok::flickrApiKey()) );
        query.addQueryItem( QStringLiteral("per_page"), QString::number( m_nbPhotos ) );
        query.addQueryItem( QStringLiteral("sort"), QStringLiteral("relevance") );
        query.addQueryItem( QStringLiteral("media"), QStringLiteral("photos") );
        query.addQueryItem( QStringLiteral("content_type"), QString::number(1) );
        query.addQueryItem( QStringLiteral("text"), tags.join(QStringLiteral(" ")) );
        flickrUrl.setQuery( query );
        debug() << "Flickr url:" << flickrUrl;

        m_flickrUrls << flickrUrl;
        The::networkAccessManager()->getData( flickrUrl, this, &PhotosEngine::resultFlickr );

    }
}

void
PhotosEngine::resultFlickr(const QUrl &url, const QByteArray &data, const NetworkAccessManagerProxy::Error &e )
{
    if( !m_flickrUrls.contains( url ) )
        return;

    DEBUG_BLOCK

    m_flickrUrls.remove( url );
    if( e.code != QNetworkReply::NoError )
    {
        setError( e.description );
        debug() << "Unable to retrieve Flickr information:" << e.description;
        return;
    }

    if( data.isNull() )
    {
        debug() << "Got bad xml!";
        return;
    }

    setPhotos( QList<PhotoInfo>() );
    QXmlStreamReader xml( data );
    QList<PhotoInfo> photosInfo = photosListFromXml( xml );
    debug() << "got" << photosInfo.size() << "photo info";
    setPhotos( photosInfo );
    setStatus( Completed );
}

QList<PhotosEngine::PhotoInfo>
PhotosEngine::photosListFromXml( QXmlStreamReader &xml )
{
    QList<PhotoInfo> photoList;
    xml.readNextStartElement(); // rsp
    if( xml.attributes().value(QLatin1String("stat")) != QLatin1String("ok") )
        return photoList;

    xml.readNextStartElement(); // photos
    while( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String("photo") )
        {
            const QXmlStreamAttributes &attr = xml.attributes();
            QStringView id     = attr.value( QLatin1String("id") );
            QStringView owner  = attr.value( QLatin1String("owner") );
            QStringView secret = attr.value( QLatin1String("secret") );
            QStringView server = attr.value( QLatin1String("server") );
            QStringView title  = attr.value( QLatin1String("title") );

            QUrl photoUrl;
            photoUrl.setScheme( QStringLiteral("https") );
            photoUrl.setHost( QStringLiteral("live.staticflickr.com") );
            photoUrl.setPath( QStringLiteral("/%1/%2_%3.jpg").arg( server.toString(), id.toString(), secret.toString() ) );

            QUrl pageUrl;
            pageUrl.setScheme( QStringLiteral("https") );
            pageUrl.setHost( QLatin1String("www.flickr.com") );
            pageUrl.setPath( QStringLiteral("/photos/%1/%2").arg( owner.toString(), id.toString() ) );

            PhotoInfo info;
            info.title = title.toString();
            info.urlpage = pageUrl;
            info.urlphoto = photoUrl;
            photoList.append( info );
        }
        xml.skipCurrentElement();
    }
    return photoList;
}

void
PhotosEngine::setPhotos( const QList<PhotoInfo> &photos )
{
    if( m_photos == photos )
        return;

    m_photos = photos;
    Q_EMIT photosChanged();
}

void
PhotosEngine::setStatus( Status status )
{
    if( m_status == status )
        return;

    m_status = status;
    Q_EMIT statusChanged();
}

void
PhotosEngine::setError( const QString &error )
{
    if( m_error == error )
        return;

    m_error = error;
    Q_EMIT errorChanged();
}

void
PhotosEngine::setArtist( const QString &artist )
{
    if( m_artist == artist )
        return;

    m_artist = artist;
    Q_EMIT artistChanged();
}

QList<QUrl>
PhotosEngine::photoUrls() const
{
    QList<QUrl> list;
    for( const auto &photo : m_photos )
    {
        list << photo.urlphoto;
    }
    return list;
}

QList<QUrl>
PhotosEngine::pageUrls() const
{
    QList<QUrl> list;
    for( const auto &photo : m_photos )
    {
        list << photo.urlpage;
    }
    return list;
}

QList<QString>
PhotosEngine::photoTitles() const
{
    QList<QString> list;
    for( const auto &photo : m_photos )
    {
        list << photo.title;
    }
    return list;
}
