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

#define DEBUG_PREFIX "VideoclipEngine"

#include "VideoclipEngine.h"

// Amarok
#include "core/support/Amarok.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "core/support/Debug.h"
#include "EngineController.h"

// Qt
#include <QDomDocument>

using namespace Context;

VideoclipEngine::VideoclipEngine( QObject* parent, const QList<QVariant>& /*args*/ )
        : DataEngine( parent )
        , ContextObserver( ContextView::self() )
        , m_nbYoutube( -1 )
        , m_nbDailymotion( -1 )
        , m_nbVimeo( -1 )
        , m_nbVidsPerService( 7 )
        , m_youtubeHQ( 0 )
        , m_requested( true )
{
    m_sources << "youtube" << "dailymotion" << "vimeo" ;
    update();
}

VideoclipEngine::~VideoclipEngine()
{
    DEBUG_BLOCK
    foreach ( VideoInfo *info, m_video )
        delete info;
    m_video.clear();
}

QStringList
VideoclipEngine::sources() const
{
    return m_sources;
}

bool
VideoclipEngine::sourceRequestEvent( const QString& name )
{
    Q_UNUSED( name )
    m_requested = true; // someone is asking for data, so we turn ourselves on :)

    QStringList tokens = name.split( ':' );

    // user has enable/disable youtubeHQ settings
    if ( tokens.contains( "youtubeHQ" ) && tokens.size() > 1 )
    {
        if ( ( tokens.at( 1 ) == QString( "youtubeHQ" ) ) && ( tokens.size() > 2 ) )
        {
            m_youtubeHQ = tokens.at( 2 ).toInt();
            return false;
        }
    }

    removeAllData( name );
    setData( name, QVariant() );
    update();
    return true;
}

void
VideoclipEngine::message( const ContextState& state )
{
    if ( state == Current && m_requested )
        update();
}

void
VideoclipEngine::metadataChanged( Meta::TrackPtr track )
{
    if (!track)
        return;

    const bool hasChanged = track->name() != m_title || track->artist()->name() != m_artist;
    if ( hasChanged )
        update();
}

void VideoclipEngine::update()
{
    DEBUG_BLOCK
    QString tmpYoutStr;
    // prevent
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if ( !currentTrack || !currentTrack->artist() )
        return;

    if ( !currentTrack )
        return;

    // SORT of hack. Do not search if the current playing is a video from youtube etc !!!
    if ( currentTrack->prettyUrl().contains( "http://www.youtube.com/" ) || currentTrack->prettyUrl().contains( "http://www.dailymotion.com/" ) )
        return;

    unsubscribeFrom( m_currentTrack );
    m_currentTrack = currentTrack;
    subscribeTo( currentTrack );

    // Save artist and title
    m_title = currentTrack->name();
    m_artist = currentTrack->artist()->name();
    m_length = currentTrack->length() / 1000;

    // Clean stuff
    foreach ( VideoInfo *info, m_video )
        delete info;

    m_nbYoutube=m_nbDailymotion=m_nbVimeo=-1;

    removeAllData( "videoclip" );
    m_video.clear();

    // Show the information
    setData( "videoclip", "message", "Fetching" );

    // Query youtube, order by relevance, 10 max
    // Youtube : http://gdata.youtube.com/feeds/videos?q=ARTIST TITLE&orderby=relevance&max-results=7
    KUrl youtubeUrl( QString("http://gdata.youtube.com/feeds/videos?q=%1 %2").arg(m_artist).arg(m_title) + QString( "&orderby=relevance&max-results=")+ QString().setNum( m_nbVidsPerService ) );
    m_youtubeUrl = youtubeUrl;
    The::networkAccessManager()->getData( youtubeUrl, this,
            SLOT(resultYoutube(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );

    // Query dailymotion, order by rating
    // Dailymotion : http://www.dailymotion.com/rss/rated/search/ARTIST TITLE
    KUrl dailyUrl( QString("http://www.dailymotion.com/rss/rated/search/%1 %2").arg(m_artist).arg(m_title) );
    m_dailyUrl = dailyUrl;
    The::networkAccessManager()->getData( dailyUrl, this,
            SLOT(resultDailymotion(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );

    // Query vimeo
    // Vimeo : http://vimeo.com/videos/search:ARTIST TITLE
    KUrl vimeoUrl( QString("http://vimeo.com/videos/search:%1 %2").arg(m_artist).arg(m_title) );
    m_vimeoUrl = vimeoUrl;
    debug() << "Vimeo query url:" << vimeoUrl;
    The::networkAccessManager()->getData( vimeoUrl, this,
            SLOT(resultVimeo(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

bool VideoclipEngine::isVideoInfoValid( VideoInfo *item )
{
    item->relevancy=0;
    // title contain artist AND title
    if ( item->title.contains( m_artist, Qt::CaseInsensitive ))
        item->relevancy+=10;
    else
        item->relevancy-=10;

    if ( item->title.contains( m_title, Qt::CaseInsensitive ) )
        item->relevancy+=10;
    else
        item->relevancy-=10;

    // if it contains both of them : good we add 30 more points
    if (item->relevancy == 20 )
        item->relevancy+=30;

    bool bArtistDesc = item->desc.contains( m_artist, Qt::CaseInsensitive );
    bool bTitleDesc = item->desc.contains( m_title, Qt::CaseInsensitive );

    // if we have both of them in the description, good !
    if ( bArtistDesc && bTitleDesc )
        item->relevancy+=20;

    // init to false;
    item->isHQ = false;

    // time to remove bad choices. If we don't have artist nor than title in the name of the vid,
    // and no artist in the desc, simply remove this item.
    if ( !bArtistDesc && item->relevancy==-20 )
        return false;
    else
        return true;
}

void VideoclipEngine::resultYoutube( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( m_youtubeUrl != url )
        return;

    m_youtubeUrl.clear();
    if( e.code != QNetworkReply::NoError )
    {
        setData( "videoclip", "message", i18n( "Unable to retrieve Youtube information: %1", e.description ) );
        debug() << "Unable to retrieve Youtube information:" << e.description;
        m_nbYoutube = 0; //say that we didn't fetch any youtube songs (which is true !)
        resultFinalize();
        return;
    }
    // Get the result
    QDomDocument xmlDoc;
    xmlDoc.setContent( data );
    QDomNodeList xmlNodeList = xmlDoc.elementsByTagName( "entry" );

    QTime tim, time( 0, 0 );
    m_nbYoutube = 0;
    for ( uint i = 0; i < xmlNodeList.length() ; i++ )
    {
        QDomNode xmlNode = xmlNodeList.at( i );
        VideoInfo *item = new VideoInfo;

        // Get all the information
        item->title = xmlNode.firstChildElement( "title" ).text();
        item->url = QString( "http://www.youtube.com/watch?v=" ) + xmlNode.firstChildElement( "link" ).attribute( "href" ).split( "=" )[1];
        item->coverurl = xmlNode.firstChildElement( "media:group" ).firstChildElement( "media:thumbnail" ).attribute( "url" );
        item->length = xmlNode.firstChildElement( "media:group" ).firstChildElement( "yt:duration" ).attribute( "seconds" ).toInt();
        item->duration = time.addSecs( item->length ).toString( "mm:ss" );
        item->desc = xmlNode.firstChildElement( "media:group" ).firstChildElement( "media:description" ).text();
        item->rating = ( float ) ( ( int )( xmlNode.firstChildElement( "gd:rating" ).attribute( "average" ).toFloat() * 100 ) / 100 );
        item->source = QString( "youtube" );
        item->views = xmlNode.firstChildElement( "yt:statistics" ).attribute( "viewCount" );

        // only add if it's valid (no useless jobs)
        if ( isVideoInfoValid(item) )
        {
            // Insert the item in the list
            m_video << item;

            // Send a job to get the downloadable link
            const KUrl videoUrl( item->url );
            m_videoUrls[videoUrl] = Youtube;
            The::networkAccessManager()->getData( videoUrl, this,
                 SLOT(resultYoutubeGetLink(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );

            const KUrl coverUrl( item->coverurl );
            m_imageUrls << coverUrl;
            The::networkAccessManager()->getData( coverUrl, this,
                 SLOT(resultImageFetcher(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
        }
        else
        {
            delete item;
            m_nbYoutube--;
        }
    }
    m_nbYoutube += xmlNodeList.length();
    // Check how many clip we've find and send message if all the job are finished but no clip were find
    //debug() << "Youtube fetch : " << m_nbYoutube << " songs ";
    resultFinalize();
}

void VideoclipEngine::resultYoutubeGetLink( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
//    DEBUG_BLOCK
    if( !m_videoUrls.contains( url ) || (m_videoUrls.value( url ) != Youtube) )
        return;

    m_videoUrls.remove( url );
    if( e.code != QNetworkReply::NoError )
    {
        debug() << "Unable to retrieve Youtube direct videolink:" << e.description;
        resultFinalize();
        return;
    }

    QString jobUrl = url.url();
    jobUrl.replace( "watch?v", "get_video?video_id" );

    QString vidlink = "bad_link" ;
    QString page( data );
    bool isHQ18 = false;
    bool isHQ22 = false;

    QString t;

    // Youtube has changed again its api
    // It changed once more (10 april 2010)
    QString regex( "\"flashvars\\\"" );
    if ( page.indexOf( regex ) != -1 )
    {
        page = page.mid( page.indexOf( regex ) );
        QString reg( "flashvars=");
        if ( page.indexOf( reg ) != -1 )
            page = page.mid( 0, page.indexOf( reg ) );
    }

    QString regex1( "&fmt_map=" );
    if ( page.indexOf( regex1 ) != -1 )
    {
        QString fmtPage = page.mid( page.indexOf( regex1 ) + regex1.size() );

        // if the next time we've got true
        if ( fmtPage.mid( 0, 3 ).contains( "18" ) || fmtPage.mid( 0, 3 ).contains( "35" ))
            isHQ18 = true;

        else if ( fmtPage.mid( 0, 3 ).contains( "22" ) )
            isHQ22 = true ;
    }

    QString regex2( "&t=" );
    if ( page.indexOf( regex2 ) != -1 )
    {
        QString tPage = page.mid( page.indexOf( regex2 ) + regex2.size() );
        t = tPage.mid( 0, tPage.indexOf( "&" ) );
        //         debug()<<" T " <<t;
    }

    //        debug() << page ;
    if ( !t.isEmpty() )
    {
        vidlink = jobUrl + "&t=" + t + "=" ;

        // enable youtube HQ if user as request and HQ is available
        if ( m_youtubeHQ && isHQ18 )
            vidlink+="&fmt=18";

        else if ( m_youtubeHQ && isHQ22 )
            vidlink+="&fmt=22";
    }

    foreach (VideoInfo *item, m_video )
    {
        if ( item->url == url.url() )
        {
            if ( isHQ18 || isHQ22 )
                item->isHQ = true;
            item->videolink = vidlink;
        }
    }
    resultFinalize();
}

void VideoclipEngine::resultDailymotion( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( m_dailyUrl != url )
        return;

    m_dailyUrl.clear();
    if( e.code != QNetworkReply::NoError )
    {
        setData( "videoclip", "message", i18n("Unable to retrieve Dailymotion information: %1", e.description) );
        debug() << "Unable to retrieve Dailymotion information:" << e.description;
        m_nbDailymotion = 0; //say that we didn't fetch any youtube songs (which is true !)
        resultFinalize();
        return;
    }

    // Get the result
    QDomDocument xmlDoc;
    xmlDoc.setContent( data );
    QDomNodeList xmlNodeList = xmlDoc.elementsByTagName( "item" );

    int tmp = m_nbVidsPerService < (int)xmlNodeList.length() ? m_nbVidsPerService : (int)xmlNodeList.length();
    m_nbDailymotion = tmp;
    QTime tim, time( 0, 0 );
    for ( int i = 0; i < tmp; i++ )
    {
        QDomNode xmlNode = xmlNodeList.at( i );
        VideoInfo *item = new VideoInfo;

        // Get all the information
        item->title = xmlNode.firstChildElement( "title" ).text();
        item->url = xmlNode.firstChildElement( "link" ).text().split( '?' )[ 0 ];
        item->coverurl = xmlNode.firstChildElement( "media:thumbnail" ).attribute( "url" ).split( '?' )[ 0 ].replace( "/320x240/", "/160x120/" );
        item->length = xmlNode.firstChildElement( "media:group" ).firstChildElement( "media:content" ).attribute( "duration" ).toInt();
        item->duration  = time.addSecs( item->length ).toString( "mm:ss" );
        item->views = xmlNode.firstChildElement( "dm:views" ).text();
        item->desc = xmlNode.firstChildElement( "itunes:summary" ).text();
        item->rating = xmlNode.firstChildElement( "dm:videorating" ).text().toFloat();
        item->source = QString( "dailymotion" );

        // only add if it's valid (no useless jobs)
        if ( isVideoInfoValid(item) )
        {
            // Push the VideoInfo in the main list
            m_video << item;

            // Send a job to get the downloadable link
            const KUrl videoUrl( item->url );
            m_videoUrls[videoUrl] = Dailymotion;
            The::networkAccessManager()->getData( videoUrl, this,
                 SLOT(resultDailymotionGetLink(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );

            // Send a job to get the pixmap
            const KUrl coverUrl( item->coverurl );
            m_imageUrls << coverUrl;
            The::networkAccessManager()->getData( coverUrl, this,
                 SLOT(resultImageFetcher(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
        }
        else
        {
            delete item;
            m_nbDailymotion--;
        }
    }

    // Check how many clip we've find and send message if all the job are finished but no clip were find
    //debug() << "Dailymotion fetch : " << m_nbDailymotion << " songs ";
    resultFinalize();
}

void VideoclipEngine::resultDailymotionGetLink( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( !m_videoUrls.contains( url ) || (m_videoUrls.value( url ) != Dailymotion) )
        return;

    m_videoUrls.remove( url );
    if( e.code != QNetworkReply::NoError )
    {
        debug() << "Unable to retrieve DailyMotion direct videolink:" << e.description;
        resultFinalize();
        return;
    }
    // DEBUG_BLOCK
    QString page( data );
    QStringList vidlink;
    QString vidFLV;
    QString vidH264;
    bool isHQ = false ;

    // Focus on the line which contain .addVariable("video", "
    QString regex( ".addVariable(\"video\", \"" );
    if ( page.indexOf( regex ) != -1 )
    {
        page = page.mid( page.indexOf( regex ) + regex.size() );
        regex = "\");";
        if ( page.indexOf( regex ) != -1 )
            page = page.mid( 0, page.indexOf( regex ) );

        // replace strange caracter by correct ones, and split into different url
        page.replace( "%3A", ":");
        page.replace( "%2F", "/");
        page.replace( "%3F", "?");
        page.replace( "%3D", "=");
        page.replace( "%40", "@");

        regex = "%7C%7C";

        // now vidlink contain all the url (320x240xflv, 80x60flv, and HQ)
        vidlink = page.split( regex );
        //   debug() << vidlink ;


        foreach( const QString &urlstring, vidlink )
        {
            if ( urlstring.contains( "@@h264" ) )
            {
                isHQ = true ;
                vidH264 = urlstring ;
            }
            if ( urlstring.contains( "FLV-320x240" ) )
                vidFLV = urlstring ;
        }

        foreach( VideoInfo *item, m_video )
        {
            if ( item->url == url.url() )
            {
                if ( isHQ )
                    item->isHQ = true;

                if ( isHQ && m_youtubeHQ )
                    item->videolink = vidH264;

                else if ( vidFLV.size() != 0 )
                    item->videolink = vidFLV;
                else
                    item->videolink = "bad_link" ;
            }
        }
    }
    resultFinalize();
}

void VideoclipEngine::resultVimeo( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( m_vimeoUrl != url )
        return;

    m_vimeoUrl.clear();
    if( e.code != QNetworkReply::NoError )
    {
        setData( "videoclip", "message", i18n("Unable to retrieve Vimeo information: %1", e.description) );
        debug() << "Unable to retrieve Vimeo information:" << e.description;
        m_nbVimeo = 0; // say that we didn't fetch any vimeo songs (which is true !)
        resultFinalize();
        return;
    }
    // DEBUG_BLOCK
    // Get the result
    QString page( data );
    if( page.isNull() )
    {
        debug() << "Vimeo info is null";
        return;
    }

    QRegExp regex( "<a href=\"/(\\d+)\".*</a>" );
    m_nbVimeo = 0;
    int pos = 0;
    while ( (pos = regex.indexIn(page, pos)) != -1 && ( m_nbVimeo < m_nbVidsPerService ) )
    {
        m_nbVimeo++;
        // page = page.mid( page.indexOf( regex ) + regex.size() );
        // QString id = QString( page.mid( 0, page.indexOf( "\"" ) ) ) ;
        QString id = regex.cap( 1 );

        // send a job to get info
        QString vimeoBis = QString( "http://vimeo.com/api/v2/video/" ) + id + QString( ".xml" );
        debug() << "Vimeo info url" << vimeoBis;
        m_vimeoBisUrl = KUrl( vimeoBis );
        The::networkAccessManager()->getData( m_vimeoBisUrl, this,
             SLOT(resultVimeoBis(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );

        pos += regex.matchedLength();
    }
    //debug() << "Vimeo fetch : " << m_nbVimeo << " songs ";
    resultFinalize();
}

void VideoclipEngine::resultVimeoBis( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( m_vimeoBisUrl != url )
        return;

    m_vimeoBisUrl.clear();
    if( e.code != QNetworkReply::NoError )
    {
        setData( "videoclip", "message", i18n("Unable to retrieve Vimeo Bis information: %1", e.description) );
        debug() << "Unable to retrieve Vimeo Bis information:" << e.description;
        resultFinalize();
        return;
    }
    //   DEBUG_BLOCK

    // Get the result
    QDomDocument xmlDoc;
    xmlDoc.setContent( data );

    QTime tim, time( 0, 0 );
    QDomNode xmlNode = xmlDoc.elementsByTagName( "video" ).at( 0 );
    VideoInfo *item = new VideoInfo;
    item->title = xmlNode.firstChildElement( "title" ).text();
    item->url = xmlNode.firstChildElement( "url" ).text();
    item->coverurl = xmlNode.firstChildElement( "thumbnail_medium" ).text();
    item->length = xmlNode.firstChildElement( "duration" ).text().toInt();
    item->duration = time.addSecs( item->length ).toString( "mm:ss" );
    item->views = xmlNode.firstChildElement( "stats_number_of_plays" ).text();
    item->desc = xmlNode.firstChildElement( "description" ).text();
    item->source = QString( "vimeo" );
    item->rating = 0;

    // only add if it's valid (no useless jobs)
    if ( isVideoInfoValid(item) )
    {
        // All vimeo are streamed in HQ
        item->isHQ = true ;
        // Push the VideoInfo in the main list
        m_video << item;
        // send a job to get the full link
        QString getLink = QString( "http://www.vimeo.com/moogaloop/load/clip:" ) + xmlNode.firstChildElement( "id" ).text();
        const KUrl videoUrl( getLink );
        // m_videoUrls << videoUrl;
        m_videoUrls[videoUrl] = Vimeo;
        The::networkAccessManager()->getData( videoUrl, this,
             SLOT(resultVimeoGetLink(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );

        // Send a job to get every pixmap
        const KUrl imageUrl( item->coverurl );
        m_imageUrls << imageUrl;
        The::networkAccessManager()->getData( imageUrl, this,
             SLOT(resultImageFetcher(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
    }
    else
        delete item;

    resultFinalize();
}

void VideoclipEngine::resultVimeoGetLink( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( !m_videoUrls.contains( url ) || (m_videoUrls.value( url ) != Vimeo) )
        return;

    m_videoUrls.remove( url );
    if( e.code != QNetworkReply::NoError )
    {
        debug() << "Unable to retrieve Vimeo direct videolink:" << e.description;
        resultFinalize();
        return;
    }
    //    DEBUG_BLOCK
    // Get the result
    QDomDocument xmlDoc;
    xmlDoc.setContent( data );

    QDomNode xmlNode = xmlDoc.elementsByTagName( "xml" ).at( 0 );
    QString id( xmlNode.firstChildElement( "video" ).firstChildElement( "nodeId" ).text() );
    QString key( xmlNode.firstChildElement( "request_signature" ).text() );
    QString expire( xmlNode.firstChildElement( "request_signature_expires" ).text() );
    QString vidlink( ( "http://vimeo.com/moogaloop/play/clip:" ) + id + QString( "/" ) + key + QString( "/" ) + expire + QString( "/?q=hd" ) );

    QString urlclean( xmlNode.firstChildElement( "video" ).firstChildElement( "url_clean" ).text() );

    debug() << "Vimeo video play url" << urlclean;

    foreach (VideoInfo *item, m_video )
    {
        if ( item->url == urlclean )
        {
            item->videolink = vidlink;
        }
    }
    resultFinalize();
}

void VideoclipEngine::resultImageFetcher( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( !m_imageUrls.contains( url ) )
        return;

    m_imageUrls.remove( url );
    if( e.code != QNetworkReply::NoError )
    {
        debug() << "Unable to retrieve an image:" << e.description;
        resultFinalize();
        return;
    }
    //    DEBUG_BLOCK
    QPixmap pix;
    if( !pix.loadFromData( data ) )
    {
        debug() << "Error loading image data";
    }
    else
    {
        foreach( VideoInfo *item, m_video )
        {
            if( item->coverurl == url.url() )
                item->cover = pix;
        }
    }
    resultFinalize();
}

void VideoclipEngine::resultFinalize()
{
    // debug() << "resultFinalize:" << m_infoUrls.size() << m_videoUrls.size() << m_imageUrls.size()
            // << m_nbYoutube << m_nbDailymotion << m_nbVimeo;
//    DEBUG_BLOCK
    // if 3 websites have been called, but no video :
    if ( m_nbYoutube==0 && m_nbDailymotion==0 && m_nbVimeo==0 )
    {
        DEBUG_BLOCK
        debug() << "No Video clip found";
        setData( "videoclip", "message", i18n( "No video clip found..." ) );
        return;
    }
    // else if nb job finished and they have been called
    else if ( m_infoUrls.empty() && m_videoUrls.empty() && m_imageUrls.empty() &&
              m_nbYoutube!=-1 && m_nbDailymotion!=-1 && m_nbVimeo!=-1 )
    {
        DEBUG_BLOCK
        // add some more point with stupid criteria
        foreach ( VideoInfo *item, m_video )
        {
            if ( item->title.contains( "Official video" , Qt::CaseInsensitive) && !item->title.contains( "non" , Qt::CaseInsensitive) )
                item->relevancy+=40;

            if ( item->desc.contains( "Official video" , Qt::CaseInsensitive) && !item->desc.contains( "non" , Qt::CaseInsensitive) )
                item->relevancy+=40;

            if ( item->title.contains( "Promo video" , Qt::CaseInsensitive) )
                item->relevancy+=30;

            if ( item->desc.contains( "Promo video" , Qt::CaseInsensitive) )
                item->relevancy+=30;

            // Danger, MATHS inside
            if ( m_length  != 0 )
                item->relevancy+= (int)( ( (float)( 1 - abs( m_length - item->length ) / (float)m_length ) ) * 30. ) ;

            item->artist=m_artist;
        }

        debug() << "VideoClipEngine total Fetched : " << m_video.size() << " entries";

        // sort against relevancy
        QList < QPair <int, QString > > sorting;
        foreach ( VideoInfo *item, m_video )
            sorting << QPair < int, QString> (item->relevancy, item->url) ;
        qSort(sorting.begin(), sorting.end(), qGreater<QPair < int, QString> >());

        // remove previous message
        removeData( "videoclip", "message" );

        /*
        foreach ( VideoInfo *item, m_video )
        {
            debug() << " Video Item : " << item->title ;
            debug() << " url : "<< item->url;
            debug() << " videolink : " << item->videolink;
            debug() << " coverurl : " << item->coverurl;
            debug() << " pixmap : " << !item->cover.isNull();
            debug() << " is HQ : " << item->isHQ;
            debug() << " ";
        }
        */


        // if the song hasn't change while fetchin, we sen the info
        if ( m_currentTrack != The::engineController()->currentTrack() )
            return;

        // then send them
        QList < QPair <int, QString > >::iterator i;
        int pos=0;
        for (i = sorting.begin(); i != sorting.end(); ++i)
        {
            foreach ( VideoInfo *item, m_video)
            {
                if ( (*i).second == item->url )
                {
                    QVariant var;
                    var.setValue<VideoInfo *>( item );
                    setData( "videoclip", QString( "item:" ) + QString().setNum( pos ) , var );
                    pos++;
                }
            }
        }
    }
}

//     debug() << "VideoclipEngine | vimeo title : "<<vid_title;
//     debug() << "VideoclipEngine | vimeo id : "<<vid_id;
//     debug() << "VideoclipEngine | vimeo cover : "<<vid_cover;
//     debug() << "VideoclipEngine | vimeo duration : "<<vid_duration;
//     debug() << "VideoclipEngine | vimeo views : "<<vid_views;
//     debug() << "VideoclipEngine | vimeo description : "<<vid_desc;
//     debug() << "VideoclipEngine | vimeo rating : "<<vid_rating;

#include "VideoclipEngine.moc"

