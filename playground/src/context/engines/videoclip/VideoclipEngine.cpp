/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 * copyright            : (C) 2008 Mark Kretschmann <kretschmann@kde.org>  *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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


using namespace Context;

VideoclipEngine::VideoclipEngine( QObject* parent, const QList<QVariant>& /*args*/ )
        : DataEngine( parent )
        , ContextObserver( ContextView::self() )
        , m_jobYoutube( 0 )
        , m_jobDailymotion( 0 )
        , m_jobVimeo( 0 )
        , m_nbJobs( 0 )
        , m_nbVidsPerService( 7 )
        , m_requested( true )
{
    m_sources << "youtube" << "dailymotion" << "vimeo" ;
    update();
}

VideoclipEngine::~VideoclipEngine()
{
    DEBUG_BLOCK
}

QStringList VideoclipEngine::sources() const
{
    return m_sources;
}

bool VideoclipEngine::sourceRequestEvent( const QString& name )
{
    Q_UNUSED( name )
    m_requested = true; // someone is asking for data, so we turn ourselves on :)
    removeAllData( name );
    setData( name, QVariant() );
    update();
    return true;
}

void VideoclipEngine::message( const ContextState& state )
{
    if ( state == Current && m_requested )
        update();
}

void VideoclipEngine::metadataChanged( Meta::TrackPtr track )
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
        unsubscribeFrom( m_currentTrack );
        m_currentTrack = currentTrack;
        subscribeTo( currentTrack );

        // Save artist and title
        m_title = currentTrack->name();
        m_artist = currentTrack->artist()->name();
        m_length = currentTrack->length();

        // Clean stuff
        removeAllData( "videoclip" );
        foreach ( VideoInfo *info, m_video )
            delete info;
        m_video.clear();
        m_nbJobs = 0;
        
        // Show the information
        setData( "videoclip", "message", i18n( "Fetching content.." ) );

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

void VideoclipEngine::resultYoutube( KJob* job )
{
    DEBUG_BLOCK
    if ( !m_jobYoutube ) //track changed while we were fetching
        return;

    if ( job->error() != KJob::NoError && job == m_jobYoutube ) // It's the correct job but it errored out
    {
        setData( "videoclip", "message", i18n( "Unable to retrieve Youtube information: %1", job->errorString() ) );
        debug() << "Unable to retrieve Youtube information: " << job->errorString();
        m_jobYoutube = 0; // clear job
        return;
    }
    // Get the result
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QDomDocument xmlDoc;
    xmlDoc.setContent( storedJob->data() );
    QDomNodeList xmlNodeList = xmlDoc.elementsByTagName( "entry" );

    QTime tim, time( 0, 0 );
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
   
        // Push the VideoInfo in the main list
        m_video << item;   
   
        // Send a job to get the downloadable link
        KJob *jobu = KIO::storedGet( KUrl( item->url ), KIO::NoReload, KIO::HideProgressInfo );
        connect( jobu, SIGNAL( result( KJob* ) ), SLOT( resultYoutubeGetLink( KJob* ) ) );
        
        // Send a job to get every pixmap
        KJob* job = KIO::storedGet( KUrl( item->coverurl ), KIO::Reload, KIO::HideProgressInfo );
        connect( job, SIGNAL( result( KJob* ) ), SLOT( resultImageFetcher( KJob* ) ) );
        m_nbJobs+=2;
    }
    // Check how many clip we've find and send message if all the job are finished but no clip were find
    debug() << "Youtube fetch : " << xmlNodeList.length() << " songs ";
    
    resultFinalize();
    m_jobYoutube = 0;
}

void VideoclipEngine::resultYoutubeGetLink( KJob* job )
{
//   DEBUG_BLOCK
    if ( job->error() != KJob::NoError )
    {
    //    debug() << "VideoclipEngine | Unable to retrieve Youtube direct videolink: " ;
        job=0;
        return;
    }
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString url( storedJob->url().toMimeDataString() );
    QString url2=url;
    QString vidlink="bad_link";
    QString page = storedJob->data();
    QString regex( "&t=" );
    
    url.replace( "watch?v", "get_video?video_id" );
    if ( page.indexOf( regex ) != -1 )
    {
        page = page.mid( page.indexOf( regex ) + regex.size() );
        vidlink = url + QString( "&t=" ) + page.mid( 0, page.indexOf( "&" ) );
        m_nbJobs-=1;        
    }
    
    foreach (VideoInfo *item, m_video )
        if ( item->url == url2 )
            item->videolink = vidlink;

    resultFinalize();
    job = 0;
}

void VideoclipEngine::resultDailymotion( KJob* job )
{
	DEBUG_BLOCK
	
    if ( !m_jobDailymotion ) 
        return; //track changed while we were fetching
    
    if ( job->error() != KJob::NoError && job == m_jobDailymotion ) // It's the correct job but it errored out
    {
        setData( "videoclip", "message", i18n( "Unable to retrieve Dailymotion information: %1", job->errorString() ) );
        debug() << "Unable to retrieve Dailymotion information: " << job->errorString();
        m_jobDailymotion = 0; // clear job
        return;
    }
    // Get the result
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QDomDocument xmlDoc;
    xmlDoc.setContent( storedJob->data() );
    QDomNodeList xmlNodeList = xmlDoc.elementsByTagName( "item" );
    
    int nb = m_nbVidsPerService < (int)xmlNodeList.length() ? m_nbVidsPerService : (int)xmlNodeList.length();
    QTime tim, time( 0, 0 );
    for ( int i = 0; i < nb; i++ )
    {
        QDomNode xmlNode = xmlNodeList.at( i );
        VideoInfo *item = new VideoInfo;
        
        // Get all the information
        item->title = xmlNode.firstChildElement( "title" ).text();
        item->url = xmlNode.firstChildElement( "link" ).text().split( "?" )[ 0 ];
        item->coverurl = xmlNode.firstChildElement( "media:thumbnail" ).attribute( "url" ).split( "?" )[ 0 ].replace( "/320x240/", "/160x120/" );
        item->length = xmlNode.firstChildElement( "media:group" ).firstChildElement( "media:content" ).attribute( "duration" ).toInt();
        item->duration  = time.addSecs( item->length ).toString( "mm:ss" );
        item->views = xmlNode.firstChildElement( "dm:views" ).text();
        item->desc = xmlNode.firstChildElement( "itunes:summary" ).text();
        item->rating = xmlNode.firstChildElement( "dm:videorating" ).text().toFloat();
        item->source = QString( "dailymotion" );
        
        // remove one line makes it easier
        xmlNode.firstChildElement( "media:group" ).removeChild( xmlNode.firstChildElement( "media:group" ).firstChildElement( "media:content" ) );
        item->videolink = QString (xmlNode.firstChildElement( "media:group" ).firstChildElement( "media:content" ).attribute( "url" ) ).replace( "80x60" , "320x240" );

        // Push the VideoInfo in the main list
        m_video << item;

        // Send a job to get the pixmap
        KJob* job = KIO::storedGet( KUrl( item->coverurl ), KIO::Reload, KIO::HideProgressInfo );
        connect( job, SIGNAL( result( KJob* ) ), SLOT( resultImageFetcher( KJob* ) ) );
        m_nbJobs+=1;
    }
    
    // Check how many clip we've find and send message if all the job are finished but no clip were find
    debug() << "Dailymotion fetch : " << nb << " songs ";

    resultFinalize();
    m_jobDailymotion = 0;
}

void VideoclipEngine::resultVimeo( KJob* job )
{
    DEBUG_BLOCK
	
	if ( !m_jobVimeo ) 
        return; //track changed while we were fetching


    if ( job->error() != KJob::NoError && job == m_jobVimeo ) // It's the correct job but it errored out
    {
        setData( "videoclip", "message", i18n( "Unable to retrieve Vimeo information: %1", job->errorString() ) );
        debug() << "Unable to retrieve Vimeo information: " << job->errorString();
        m_jobVimeo = 0; // clear job
        return;
    }
    // Get the result
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString page = storedJob->data();
    
    QString regex( "<div class=\"title\"><a href=\"/" );
    int count = 0;
    while ( ( page.indexOf( regex ) != -1) && ( count < m_nbVidsPerService ) )
    {
        count++;
        page = page.mid( page.indexOf( regex ) + regex.size() );
        QString id = QString( page.mid( 0, page.indexOf( "\"" ) ) ) ;

        // send a job to get info
        KUrl vimeoURL( QString( "http://vimeo.com/api/clip/" ) + id + QString( ".xml" ) );
        KJob *jobVimeo = KIO::storedGet( vimeoURL, KIO::Reload, KIO::HideProgressInfo );
        connect( jobVimeo, SIGNAL( result( KJob* ) ), SLOT( resultVimeoBis( KJob* ) ) );
        m_nbJobs+=1;
    }
    debug() << "Vimeo fetch : " << count << " songs ";
    resultFinalize();
    m_jobVimeo = 0;
}

void VideoclipEngine::resultVimeoBis( KJob *job )
{   
 //   DEBUG_BLOCK
    if ( job->error() != KJob::NoError )
    {
        setData( "videoclip", "message", i18n( "Unable to retrieve one Vimeo song information: %1", job->errorString() ) );
        job = 0; // clear job
        return;
    }
    // Get the result
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
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
    
    // Push the VideoInfo in the main list
    m_video << item;
    m_nbJobs-=1;
   // send a job to get the full link
    KJob *jobVimeoBis = KIO::storedGet( QString( "http://www.vimeo.com/moogaloop/load/clip:" ) + xmlNode.firstChildElement( "clip_id" ).text(), 
        KIO::Reload, KIO::HideProgressInfo );
    connect( jobVimeoBis, SIGNAL( result( KJob* ) ), SLOT( resultVimeoGetLink( KJob* ) ) );
    
    // Send a job to get every pixmap
    KJob* jab = KIO::storedGet( KUrl( item->coverurl ), KIO::Reload, KIO::HideProgressInfo );
    connect( jab, SIGNAL( result( KJob* ) ), SLOT( resultImageFetcher( KJob* ) ) );
    m_nbJobs+=2;
    job = 0;
}

void VideoclipEngine::resultVimeoGetLink( KJob *job )
{
//   DEBUG_BLOCK
    if ( job->error() != KJob::NoError )
    {
        setData( "videoclip", "message", i18n( "Unable to retrieve one Vimeo song information: %1", job->errorString() ) );
        job = 0; // clear job
        return;
    }
    // Get the result
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QDomDocument xmlDoc;
    xmlDoc.setContent( storedJob->data() );
    
    QDomNode xmlNode = xmlDoc.elementsByTagName( "xml" ).at( 0 );
    QString id( xmlNode.firstChildElement( "video" ).firstChildElement( "nodeId" ).text() );
    QString key( xmlNode.firstChildElement( "request_signature" ).text() );
    QString expire( xmlNode.firstChildElement( "request_signature_expires" ).text() );
    QString vidlink( ( "http://vimeo.com/moogaloop/play/clip:" ) + id + QString( "/" ) + key + QString( "/" ) + expire + QString( "/?q=hd" ) );
    
    QString urlclean( xmlNode.firstChildElement( "video" ).firstChildElement( "url_clean" ).text() );
      
    foreach (VideoInfo *item, m_video )
        if ( item->url == urlclean )
        {
            item->videolink = vidlink;
            m_nbJobs-=1; 
        }
    job = 0;
}

void VideoclipEngine::resultImageFetcher( KJob *job )
{
    if ( job->error() != KJob::NoError )
    {
        setData( "videoclip", "message", i18n( "Unable to retrieve an image information") );
        m_nbJobs-=1;
        job = 0; // clear job
        return;
    }
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString url( storedJob->url().toMimeDataString() );

    QPixmap pix;
    if ( pix.loadFromData( storedJob->data() ) ) { ; }
    
    foreach ( VideoInfo *item, m_video )
        if (item->coverurl == url )
        {
            item->cover =  pix ;
            m_nbJobs-=1;
        }
    
    resultFinalize();
    job = 0;
}

void VideoclipEngine::resultFinalize()
{
	//DEBUG_BLOCK
    if ( m_video.empty())
    {
 //       debug() << "No Video clip found";
        setData( "videoclip", "message", i18n( "No video clip found..." ) );
        return;
    }
    // Check that all the jobs are finished
    if ( m_nbJobs == 0 )
    { 
  //      debug() << "Fetched : " << m_video.size() << " entries";
       
        // Ordering need to be done here   
        foreach ( VideoInfo *item, m_video )
        {
            item->relevancy=0;
            // title contain artist AND title
            if ( item->title.contains( m_artist, Qt::CaseInsensitive )) 
                item->relevancy+=10;
                
            if ( item->title.contains( m_title, Qt::CaseInsensitive ) )
                item->relevancy+=10;
                
            if (item->relevancy == 20 )
                item->relevancy+=30;
            
            if ( item->desc.contains( m_artist, Qt::CaseInsensitive ) && item->desc.contains( m_title, Qt::CaseInsensitive ) )
                item->relevancy+=20;
               
            if ( item->title.contains( "Official video" , Qt::CaseInsensitive) && !item->title.contains( "non" , Qt::CaseInsensitive) )
                item->relevancy+=40;
            
            if ( item->desc.contains( "Official video" , Qt::CaseInsensitive) && !item->desc.contains( "non" , Qt::CaseInsensitive) )
                item->relevancy+=40;            
                
            if ( item->title.contains( "Promo video" , Qt::CaseInsensitive) )
                item->relevancy+=30;
                
            if ( item->desc.contains( "Promo video" , Qt::CaseInsensitive) )
                item->relevancy+=30;
            
            if ( m_length  != 0 )
                item->relevancy+= (int)( ( (float)( 1 - abs( m_length - item->length ) / (float)m_length ) ) * 30. ) ;

        }
        
        // sort against relevancy
        QList < QPair <int, QString > > sorting;
        foreach ( VideoInfo *item, m_video )
            sorting << QPair < int, QString> (item->relevancy, item->url) ;
        qSort(sorting.begin(), sorting.end(), qGreater<QPair < int, QString> >());

        
        // remove previous message
        removeData( "videoclip", "message" );
        
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
                    var.setValue<VideoInfo>(*item);
                    setData( "videoclip", QString().setNum(pos) , var );
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

