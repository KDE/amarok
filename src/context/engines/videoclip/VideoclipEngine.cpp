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

#include "VideoclipEngine.h"

// Amarok
#include "Amarok.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "Debug.h"
#include "EngineController.h"

// Qt
#include <QDomDocument>

// Standard
#include <sstream>

#define DEBUG_PREFIX "VideoclipEngine"

using namespace Context;

VideoclipEngine::VideoclipEngine( QObject* parent, const QList<QVariant>& /*args*/ )
        : DataEngine( parent )
        , ContextObserver( ContextView::self() )
        , m_jobYoutube( 0 )
        , m_jobDailymotion( 0 )
        , m_jobVimeo( 0 )
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
    else
    {

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
        m_listJob.clear();

        // Show the information
        setData( "videoclip", "message", "Fetching" );

        // Query youtube, order by relevance, 10 max
        // Youtube : http://gdata.youtube.com/feeds/videos?q=ARTIST TITLE&orderby=relevance&max-results=7
        KUrl youtubeUrl( QString( "http://gdata.youtube.com/feeds/videos?q=" ) + m_artist + QString( " " ) + m_title + QString( "&orderby=relevance&max-results=")+ QString().setNum( m_nbVidsPerService ) );
        m_jobYoutube = KIO::storedGet( youtubeUrl, KIO::Reload, KIO::HideProgressInfo );
        connect( m_jobYoutube, SIGNAL( result( KJob* ) ), SLOT( resultYoutube( KJob* ) ) );

        // Query dailymotion, order by rating
        // Dailymotion : http://www.dailymotion.com/rss/rated/search/ARTIST TITLE
        KUrl dailyUrl( QString( "http://www.dailymotion.com/rss/rated/search/" ) + m_artist + QString( " " ) + m_title );
        m_jobDailymotion = KIO::storedGet( dailyUrl, KIO::Reload, KIO::HideProgressInfo );
        connect( m_jobDailymotion, SIGNAL( result( KJob* ) ), SLOT( resultDailymotion( KJob* ) ) );

        // Query vimeo
        // Vimeo : http://vimeo.com/videos/search:ARTIST TITLE
        KUrl vimeoURL( QString( "http://vimeo.com/videos/search:" ) + m_artist + QString( " " ) + m_title );
        m_jobVimeo = KIO::storedGet( vimeoURL, KIO::Reload, KIO::HideProgressInfo );
        connect( m_jobVimeo, SIGNAL( result( KJob* ) ), SLOT( resultVimeo( KJob* ) ) );
    }
}

bool VideoclipEngine::isVideoInfoValid( VideoInfo *item )
{

    item->cover = new QPixmap(); // init the QPixmap pointer, so that we can check if it as been retrieved or not
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

    // time to remove bad choices. If we don't have artist nor thant title in the name of the vid,
    // and no artist in the desc, simply remove this item.
    if ( !bArtistDesc && item->relevancy==-20 )
        return false;
    else
        return true;
}

void VideoclipEngine::resultYoutube( KJob* job )
{

    if ( !m_jobYoutube ) //track changed while we were fetching
        return;

    DEBUG_BLOCK
    if ( job->error() != KJob::NoError && job == m_jobYoutube ) // It's the correct job but it errored out
    {
        setData( "videoclip", "message", i18n( "Unable to retrieve Youtube information: %1", job->errorString() ) );
        debug() << "Unable to retrieve Youtube information: " << job->errorString();
        m_jobYoutube = 0; // clear job
        m_nbYoutube = 0; //say that we didn't fetch any youtube songs (which is true !)
        resultFinalize();
    }
    // Get the result
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QDomDocument xmlDoc;
    xmlDoc.setContent( storedJob->data() );
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
            KJob *jobu = KIO::storedGet( KUrl( item->url ), KIO::Reload, KIO::HideProgressInfo );

            m_listJob << item->url;
            connect( jobu, SIGNAL( result( KJob* ) ), SLOT( resultYoutubeGetLink( KJob* ) ) );

            // Send a job to get every pixmap
            KJob* jobb = KIO::storedGet( KUrl( item->coverurl ), KIO::Reload, KIO::HideProgressInfo );

            m_listJob << item->coverurl;
            connect( jobb, SIGNAL( result( KJob* ) ), SLOT( resultImageFetcher( KJob* ) ) );
        }
        else
        {
            delete item;
            m_nbYoutube--;
        }
    }
    m_nbYoutube += xmlNodeList.length();
    // Check how many clip we've find and send message if all the job are finished but no clip were find
    debug() << "Youtube fetch : " << m_nbYoutube << " songs ";

    m_jobYoutube = 0;
    resultFinalize();
}

void VideoclipEngine::resultYoutubeGetLink( KJob* job )
{
//    DEBUG_BLOCK
    QString jobUrl = static_cast< KIO::StoredTransferJob* >( job )->url().toMimeDataString();

    if ( m_listJob.contains( jobUrl ) )
    {
        if ( job->error() != KJob::NoError )
        {
            debug() << "VideoclipEngine | Unable to retrieve Youtube direct videolink: " ;
            m_listJob.removeOne( jobUrl );
            job = 0;
            resultFinalize();
            return;
        }

        KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
        QString url2=jobUrl;
        jobUrl.replace( "watch?v", "get_video?video_id" );

        QString vidlink = "bad_link" ;
        QString page = storedJob->data();
        bool isHQ18 = false;
        bool isHQ22 = false;


        QString sk;
        QString t;

        // Youtube has change again its api
        QString regex( "var swfArgs =" );
        if ( page.indexOf( regex ) != -1 )
        {
            page = page.mid( page.indexOf( regex ) );
            QString reg( "var isWidescreen = ");
            if ( page.indexOf( reg ) != -1 )
                page = page.mid( 0, page.indexOf( reg ) );
        }

        QString regex1( "\"fmt_map\": " );
        if ( page.indexOf( regex1 ) != -1 )
        {
            page = page.mid( page.indexOf( regex1 ) + regex1.size() + 1 );

            // if the next time we've got true
            if ( page.mid( 0, 3 ).contains( "18" ) || page.mid( 0, 3 ).contains( "35" ))
                isHQ18 = true;

            else if ( page.mid( 0, 3 ).contains( "22" ) )
                isHQ22 = true ;
        }

        QString regex2( "\"sk\": " );
        if ( page.indexOf( regex2 ) != -1 )
        {
            page = page.mid( page.indexOf( regex2 ) + regex2.size() + 1 );
            sk = page.mid( 0, page.indexOf( "\"" ) );
        }

        QString regex3( "\"t\": " );
        if ( page.indexOf( regex3 ) != -1 )
        {
            page = page.mid( page.indexOf( regex3 ) + regex3.size() + 1 );
            t = page.mid( 0, page.indexOf( "\"" ) );
            debug()<<" T " <<t;
        }
        debug() << page ;
        if ( !t.isEmpty() && !sk.isEmpty() )
        {
            vidlink = jobUrl + "&sk=" + sk + "&t=" + t ;
         //   debug() << vidlink;
            // enable youtube HQ if user as request and HQ is available
            if ( m_youtubeHQ && isHQ18 )
                vidlink+="&fmt=18";

            else if ( m_youtubeHQ && isHQ22 )
                vidlink+="&fmt=22";
        }

        foreach (VideoInfo *item, m_video )
        {
            if ( item->url == url2 )
            {
                if ( isHQ18 || isHQ22 )
                    item->isHQ = true;
                item->videolink = vidlink;
            }
        }
        m_listJob.removeOne( url2 );
        resultFinalize();

        job = 0;
    }
}

void VideoclipEngine::resultDailymotion( KJob* job )
{
    if ( !m_jobDailymotion )
        return; //track changed while we were fetching
    DEBUG_BLOCK
    if ( job->error() != KJob::NoError && job == m_jobDailymotion ) // It's the correct job but it errored out
    {
        setData( "videoclip", "message", i18n( "Unable to retrieve Dailymotion information: %1", job->errorString() ) );
        debug() << "Unable to retrieve Dailymotion information: " << job->errorString();
        m_jobDailymotion = 0; // clear job
        m_nbDailymotion = 0; //say that we didn't fetch any youtube songs (which is true !)
        resultFinalize();
        return;
    }
    // Get the result
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QDomDocument xmlDoc;
    xmlDoc.setContent( storedJob->data() );
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
        // remove one line makes it easier
        xmlNode.firstChildElement( "media:group" ).removeChild( xmlNode.firstChildElement( "media:group" ).firstChildElement( "media:content" ) );
        item->videolink = QString (xmlNode.firstChildElement( "media:group" ).firstChildElement( "media:content" ).attribute( "url" ) ).replace( "80x60" , "320x240" );

        // only add if it's valid (no useless jobs)
        if ( isVideoInfoValid(item) )
        {
            // Push the VideoInfo in the main list
            m_video << item;

            // Send a job to get the pixmap
            KJob* jober = KIO::storedGet( KUrl( item->coverurl ), KIO::Reload, KIO::HideProgressInfo );
            m_listJob << item->coverurl;
            connect( jober, SIGNAL( result( KJob* ) ), SLOT( resultImageFetcher( KJob* ) ) );
        }
        else
        {
            delete item;
            m_nbDailymotion--;
        }

    }

    // Check how many clip we've find and send message if all the job are finished but no clip were find
    debug() << "Dailymotion fetch : " << m_nbDailymotion << " songs ";

    m_jobDailymotion = 0;
    resultFinalize();
}

void VideoclipEngine::resultVimeo( KJob* job )
{
	if ( !m_jobVimeo )
        return; //track changed while we were fetching
    DEBUG_BLOCK
    if ( job->error() != KJob::NoError && job == m_jobVimeo ) // It's the correct job but it errored out
    {
        debug() << "Unable to retrieve Vimeo information: " << job->errorString();
        m_jobVimeo = 0; // clear job
        m_nbVimeo = 0; // say that we didn't fetch any vimeo songs (which is true !)
        resultFinalize();
        return;
    }
    // Get the result
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString page = storedJob->data();

    QString regex( "<div class=\"title\"><a href=\"/" );
    m_nbVimeo = 0;
    while ( ( page.indexOf( regex ) != -1) && ( m_nbVimeo < m_nbVidsPerService ) )
    {
        m_nbVimeo++;
        page = page.mid( page.indexOf( regex ) + regex.size() );
        QString id = QString( page.mid( 0, page.indexOf( "\"" ) ) ) ;

        // send a job to get info
        QString vimeoBis = QString( "http://vimeo.com/api/clip/" ) + id + QString( ".xml" );
        KJob *jobVimeo = KIO::storedGet( KUrl( vimeoBis ) , KIO::Reload, KIO::HideProgressInfo );
        m_listJob << vimeoBis;
        connect( jobVimeo, SIGNAL( result( KJob* ) ), SLOT( resultVimeoBis( KJob* ) ) );
    }
    debug() << "Vimeo fetch : " << m_nbVimeo << " songs ";
    m_jobVimeo = 0;
    resultFinalize();
}

void VideoclipEngine::resultVimeoBis( KJob *job )
{
 //   DEBUG_BLOCK
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString jobUrl = storedJob->url().toMimeDataString();

    if ( m_listJob.contains( jobUrl ) )
    {
        if ( job->error() != KJob::NoError )
        {
            debug() << "VideoclipEngine | Unable to retrieve Vimeo Bis: " << job->errorString();
            m_listJob.removeOne( jobUrl );
            job = 0;
            resultFinalize();
            return;
        }

        // Get the result
        QDomDocument xmlDoc;
        xmlDoc.setContent( storedJob->data() );

        QTime tim, time( 0, 0 );
        QDomNode xmlNode = xmlDoc.elementsByTagName( "clip" ).at( 0 );
        VideoInfo *item = new VideoInfo;
        item->title = xmlNode.firstChildElement( "title" ).text();
        item->url = xmlNode.firstChildElement( "url" ).text();
        item->coverurl = xmlNode.firstChildElement( "thumbnail_medium" ).text();
        item->length = xmlNode.firstChildElement( "duration" ).text().toInt();
        item->duration = time.addSecs( item->length ).toString( "mm:ss" );
        item->views = xmlNode.firstChildElement( "stats_number_of_plays" ).text();
        item->desc = xmlNode.firstChildElement( "caption" ).text();
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
            QString getLink = QString( "http://www.vimeo.com/moogaloop/load/clip:" ) + xmlNode.firstChildElement( "clip_id" ).text();
            KJob *jobVimeoBis = KIO::storedGet( getLink, KIO::Reload, KIO::HideProgressInfo );
            m_listJob << getLink ;
            connect( jobVimeoBis, SIGNAL( result( KJob* ) ), SLOT( resultVimeoGetLink( KJob* ) ) );

            // Send a job to get every pixmap
            KJob* jab = KIO::storedGet( KUrl( item->coverurl ), KIO::Reload, KIO::HideProgressInfo );
            m_listJob << item->coverurl;
            connect( jab, SIGNAL( result( KJob* ) ), SLOT( resultImageFetcher( KJob* ) ) );
        }
        else
            delete item;

        m_listJob.removeOne( jobUrl );
        resultFinalize();

        job = 0;
    }
}

void VideoclipEngine::resultVimeoGetLink( KJob *job )
{
//    DEBUG_BLOCK
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString jobUrl = storedJob->url().toMimeDataString();

    if ( m_listJob.contains( jobUrl ) )
    {
        if ( job->error() != KJob::NoError )
        {
            debug() << "VideoclipEngine | Unable to retrieve Vimeo get link: " << job->errorString();
            m_listJob.removeOne( jobUrl );
            job = 0;
            resultFinalize();
            return;
        }

        // Get the result
        QDomDocument xmlDoc;
        xmlDoc.setContent( storedJob->data() );

        QDomNode xmlNode = xmlDoc.elementsByTagName( "xml" ).at( 0 );
        QString id( xmlNode.firstChildElement( "video" ).firstChildElement( "nodeId" ).text() );
        QString key( xmlNode.firstChildElement( "request_signature" ).text() );
        QString expire( xmlNode.firstChildElement( "request_signature_expires" ).text() );
        QString vidlink( ( "http://vimeo.com/moogaloop/play/clip:" ) + id + QString( "/" ) + key + QString( "/" ) + expire + QString( "/?q=hd" ) );

        QString urlclean( xmlNode.firstChildElement( "video" ).firstChildElement( "url_clean" ).text() );

        foreach (VideoInfo *item, m_video )
        {
            if ( item->url == urlclean )
            {
                item->videolink = vidlink;
            }
        }
        m_listJob.removeOne( jobUrl );
        resultFinalize();

        job = 0;
    }
}

void VideoclipEngine::resultImageFetcher( KJob *job )
{
//    DEBUG_BLOCK
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString jobUrl( storedJob->url().toMimeDataString() );
    if ( m_listJob.contains( jobUrl ) )
    {
        if ( job->error() != KJob::NoError )
        {
            debug() << "VideoclipEngine | Unable to retrieve an image: " << job->errorString();
            m_listJob.removeOne( jobUrl );
            resultFinalize();
            job = 0;
            return;
        }

        QPixmap *pix = new QPixmap;
        if ( pix->loadFromData( storedJob->data() ) ) {;}

        foreach ( VideoInfo *item, m_video )
        {
            if (item->coverurl == jobUrl )
            {
                item->cover = pix ;
            }
        }
        m_listJob.removeOne( jobUrl );
        resultFinalize();
        job = 0;
    }
}


void VideoclipEngine::resultFinalize()
{
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
    else if ( m_listJob.empty() && m_nbYoutube!=-1 && m_nbDailymotion!=-1 && m_nbVimeo!=-1 )
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

        foreach ( VideoInfo *item, m_video )
        {
            debug() << " Video Item : " << item->title ;
            debug() << " url : "<< item->url;
            debug() << " videolink : " << item->videolink;
            debug() << " coverurl : " << item->coverurl;
            debug() << " pixmap : " << !item->cover->isNull();
            debug() << " is HQ : " << item->isHQ;
            debug() << " ";
        }


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

