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

#include "Amarok.h"
#include "Debug.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "EngineController.h"
#include <QDomDocument>
#include <iostream>
#include <sstream>


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
 //   DEBUG_BLOCK
    Q_UNUSED( name )
    m_requested = true; // someone is asking for data, so we turn ourselves on :)
    removeAllData( name );
    setData( name, QVariant());
    update();
    return true;
}

void VideoclipEngine::message( const ContextState& state )
{
    if( state == Current && m_requested )
        update();
}

void VideoclipEngine::metadataChanged( Meta::TrackPtr track )
{
 //   DEBUG_BLOCK
    const bool hasChanged = track->name() != m_title || track->artist()->name() != m_artist;
    if( hasChanged )
        update();    
}

void VideoclipEngine::update()
{
 //   DEBUG_BLOCK
    QString tmpYoutStr;
      // prevent
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( !currentTrack || !currentTrack->artist() )
        return;
    else
    {
        unsubscribeFrom( m_currentTrack );
        m_currentTrack = currentTrack;
        subscribeTo( currentTrack );

        // Save artist and title
        m_title = currentTrack->name();
        m_artist = currentTrack->artist()->name();

        // Clean stuff
        removeAllData( "videoclip" );
        vid_title.clear();
        vid_id.clear();
        vid_cover.clear();
        vid_duration.clear();
        vid_desc.clear();
        vid_views.clear();
        vid_rating.clear();
        vid_coverpix.clear();
        m_nbVimeo=m_nbDailymotion=m_nbYoutube=-1;
        

        // Show the information
        setData( "videoclip", "message", i18n( "Fetching content.." ) );
        
        // Query youtube, order by rating, 10 max
        KUrl youtubeUrl(QString("http://gdata.youtube.com/feeds/videos?q=")+m_artist+QString(" ")+m_title+QString("&orderby=rating&max-results=10"));
        m_jobYoutube = KIO::storedGet( youtubeUrl, KIO::NoReload, KIO::HideProgressInfo );
        connect( m_jobYoutube, SIGNAL( result( KJob* ) ), SLOT( resultYoutube( KJob* ) ) );

        // Query dailymotion, order by rating
        KUrl dailyUrl(QString("http://www.dailymotion.com/rss/rated/search/")+m_artist+QString(" ")+m_title);
        m_jobDailymotion = KIO::storedGet( dailyUrl, KIO::NoReload, KIO::HideProgressInfo );
        connect( m_jobDailymotion, SIGNAL( result( KJob* ) ), SLOT( resultDailymotion( KJob* ) ) );
        
        // Query vimeo
        KUrl vimeoURL(QString("http://vimeo.com/videos/search:")+m_artist+QString(" ")+m_title);
        m_jobVimeo = KIO::storedGet( vimeoURL, KIO::NoReload, KIO::HideProgressInfo );
        connect( m_jobVimeo, SIGNAL( result( KJob* ) ), SLOT( resultVimeo( KJob* ) ) );
        
    }
}

void VideoclipEngine::resultYoutube( KJob* job )
{
    if( !m_jobYoutube ){return;} //track changed while we were fetching
//    DEBUG_BLOCK
    if( job->error() != KJob::NoError && job == m_jobYoutube ) // It's the correct job but it errored out
    {
        setData( "videoclip", "message", i18n( "Unable to retrieve Youtube information: %1", job->errorString() ) );
        debug() << "VideoclipEngine | Unable to retrieve Youtube information: "<<job->errorString();
        m_jobYoutube = 0; // clear job
        return;
    }
    // Get the result
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QDomDocument xmlDoc;
    xmlDoc.setContent(storedJob->data());
    QDomNodeList xmlNodeList=xmlDoc.elementsByTagName("entry");

    QTime tim, time(0,0);
    for (uint i = 0; i < xmlNodeList.length(); i++)
    {
        QDomNode xmlNode= xmlNodeList.at(i);
        vid_title << xmlNode.firstChildElement("title").text();
        vid_id << QString("http://www.youtube.com/watch?v=")+xmlNode.firstChildElement("link").attribute("href").split("=")[1];
        QString cov=xmlNode.firstChildElement("media:group").firstChildElement("media:thumbnail").attribute("url");
        vid_cover << cov;
        tim=time.addSecs(xmlNode.firstChildElement("media:group").firstChildElement("yt:duration").attribute("seconds").toInt());
        vid_views << xmlNode.firstChildElement("yt:statistics").attribute("viewCount");
        vid_duration << tim.toString("mm:ss");
        vid_desc << xmlNode.firstChildElement("media:group").firstChildElement("media:description").text();
        int rat=(int)(xmlNode.firstChildElement("gd:rating").attribute("average").toFloat()*100);
        std::ostringstream stm;
        stm<<(float)rat/100.;
        vid_rating << QString(stm.str().c_str());
        
        // Send a job to get every pixmap 
        KJob* job = KIO::storedGet( KUrl(cov), KIO::NoReload, KIO::HideProgressInfo );
        connect( job, SIGNAL(result( KJob* )), SLOT(resultImageFetcher( KJob* )) );
    }
    // Check how many clip we've find and send message if all the job are finished but no clip were find
    m_nbYoutube=xmlNodeList.length();    
    debug() << "VideoclipEngine | youtube fetch : "<< m_nbYoutube<< " songs ";

    if (m_nbYoutube>0) removeData( "videoclip", "message");
    if (m_nbDailymotion == 0 && m_nbYoutube == 0 && m_nbVimeo == 0 )
    {
        debug() << "VideoclipEngine | No Video clip found";
        setData( "videoclip", "message", i18n( "No video clip found..."));
    }
    m_jobYoutube = 0;
}

void VideoclipEngine::resultDailymotion( KJob* job )
{
    if( !m_jobDailymotion ) return; //track changed while we were fetching
 //   DEBUG_BLOCK
    if( job->error() != KJob::NoError && job == m_jobDailymotion ) // It's the correct job but it errored out
    {
        setData( "videoclip", "message", i18n( "Unable to retrieve Dailymotion information: %1", job->errorString() ) );
        debug() << "VideoclipEngine | Unable to retrieve Dailymotion information: "<<job->errorString();
        m_jobDailymotion = 0; // clear job
        return;
    }
    // Get the result
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QDomDocument xmlDoc;
    xmlDoc.setContent(storedJob->data());
    QDomNodeList xmlNodeList=xmlDoc.elementsByTagName("item");

    QTime tim, time(0,0);
    for (uint i = 0; i < xmlNodeList.length(); i++)
    {
        QDomNode xmlNode= xmlNodeList.at(i);
        vid_title << xmlNode.firstChildElement("title").text();
        vid_id << xmlNode.firstChildElement("link").text().split("?")[0];
        QString cov=xmlNode.firstChildElement("media:thumbnail").attribute("url").split("?")[0].replace("/320x240/", "/160x120/");
        vid_cover << cov;
        tim=time.addSecs(xmlNode.firstChildElement("media:group").firstChildElement("media:content").attribute("duration").toInt());
        vid_views << xmlNode.firstChildElement("dm:views").text();
        vid_duration << tim.toString("mm:ss");
        vid_desc << xmlNode.firstChildElement("itunes:summary").text();
        vid_rating << xmlNode.firstChildElement("dm:videorating").text();;

        // Send a job to get every pixmap
        KJob* job = KIO::storedGet( KUrl(cov), KIO::NoReload, KIO::HideProgressInfo );
        connect( job, SIGNAL(result( KJob* )), SLOT(resultImageFetcher( KJob* )) );
    }  
    // Check how many clip we've find and send message if all the job are finished but no clip were find
    m_nbDailymotion=xmlNodeList.length();
    debug() << "VideoclipEngine | dailymotion fetch : "<< m_nbDailymotion<< " songs ";
    if (m_nbDailymotion>0) removeData( "videoclip", "message");
    if (m_nbDailymotion == 0 && m_nbYoutube == 0 && m_nbVimeo == 0 )
    {
        debug() << "VideoclipEngine | No Video clip found";
        setData( "videoclip", "message", i18n( "No video clip found..."));
    }
    m_jobDailymotion = 0;
}


void VideoclipEngine::resultVimeo( KJob* job )
{
    if( !m_jobVimeo ) return; //track changed while we were fetching
        
 //   DEBUG_BLOCK
    if( job->error() != KJob::NoError && job == m_jobVimeo ) // It's the correct job but it errored out
    {
        setData( "videoclip", "message", i18n( "Unable to retrieve Vimeo information: %1", job->errorString() ) );
        debug() << "VideoclipEngine | Unable to retrieve Vimeo information: "<<job->errorString();
        m_jobVimeo = 0; // clear job
        return;
    }
        // Get the result
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString page=storedJob->data();

    QString regex("<div class=\"title\"><a href=\"/");
    int count=0;
    while (page.indexOf(regex) != -1)
    {
        count++;
        page=page.mid(page.indexOf(regex)+regex.size());
        
        KUrl vimeoURL(QString("http://vimeo.com/api/clip/")+page.mid(0, page.indexOf("\""))+QString(".xml"));
        KJob *jobVimeo = KIO::storedGet( vimeoURL, KIO::NoReload, KIO::HideProgressInfo );
        connect( jobVimeo, SIGNAL( result( KJob* ) ), SLOT( resultVimeoBis( KJob* ) ) );
    }
    // Check how many clip we've find and send message if all the job are finished but no clip were find
    m_nbVimeo=count;
    debug() << "VideoclipEngine | vimeo fetch : "<< m_nbVimeo<< " songs ";
    if (m_nbVimeo>0) removeData( "videoclip", "message");
    if (m_nbDailymotion == 0 && m_nbYoutube == 0 && m_nbVimeo == 0 )
    {
        debug() << "VideoclipEngine | No Video clip found";
        setData( "videoclip", "message", i18n( "No video clip found..."));
    }
    m_jobVimeo = 0;
}

void VideoclipEngine::resultVimeoBis( KJob *job)
{
//    DEBUG_BLOCK
    if( job->error() != KJob::NoError)
    {
        setData( "videoclip", "message", i18n( "Unable to retrieve one Vimeo song information: %1", job->errorString() ) );
        job = 0; // clear job
        return;
    }

    // Get the result
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QDomDocument xmlDoc;
    xmlDoc.setContent(storedJob->data());
    QDomNodeList xmlNodeList=xmlDoc.elementsByTagName("clip");

    QTime tim, time(0,0);
    QDomNode xmlNode= xmlNodeList.at(0);
    vid_title << xmlNode.firstChildElement("title").text();
    vid_id << xmlNode.firstChildElement("url").text();
    QString cov=xmlNode.firstChildElement("thumbnail_medium").text();
    vid_cover << cov;
    tim=time.addSecs(xmlNode.firstChildElement("duration").text().toInt());
    vid_views << xmlNode.firstChildElement("stats_number_of_plays").text();
    vid_duration << tim.toString("mm:ss");
    vid_desc << xmlNode.firstChildElement("caption").text();
    vid_rating << 0;

    // Send a job to get every pixmap
    KJob* jab = KIO::storedGet( KUrl(cov), KIO::NoReload, KIO::HideProgressInfo );
    connect( jab, SIGNAL(result( KJob* )), SLOT(resultImageFetcher( KJob* )) );

//     debug() << "VideoclipEngine | vimeo title : "<<vid_title;
//     debug() << "VideoclipEngine | vimeo id : "<<vid_id;
//     debug() << "VideoclipEngine | vimeo cover : "<<vid_cover;
//     debug() << "VideoclipEngine | vimeo duration : "<<vid_duration;
//     debug() << "VideoclipEngine | vimeo views : "<<vid_views;
//     debug() << "VideoclipEngine | vimeo description : "<<vid_desc;
//     debug() << "VideoclipEngine | vimeo rating : "<<vid_rating;
}

void VideoclipEngine::resultImageFetcher( KJob *job)
{
//    DEBUG_BLOCK
    if(job->error() != KJob::NoError)
    {
        return;
    }
    
    KIO::StoredTransferJob* jobi =static_cast<KIO::StoredTransferJob*>( job );
    QString url(jobi->url().toMimeDataString());
    QPixmap pix;
    
    if (!pix.loadFromData( jobi->data() ) || pix.width() <= 1 ){;}
    else {vid_coverpix[url]=QVariant(pix); }

    // If all the image are downloaded, we send the information
    if ((vid_coverpix.size()==vid_cover.size()) && !vid_coverpix.empty())
    {
        debug() << "VideoclipEngine | Fetched : " << vid_views.size()<< " entries";

        DEBUG_BLOCK

        //We can here try

        // here we can do something fancy
        setData( "videoclip", "title", vid_title );
        setData( "videoclip", "id", vid_id );
        setData( "videoclip", "cover", vid_cover );
        setData( "videoclip", "duration", vid_duration );
        setData( "videoclip", "description", vid_desc );
        setData( "videoclip", "views", vid_views );
        setData( "videoclip", "rating", vid_rating );
        setData( "videoclip", "coverpix", QVariant(vid_coverpix));
    }
}



/*
void
VideoclipEngine::reloadVideoClip()
{
    DEBUG_BLOCK
    
    debug() << "wiki url: " << m_wikiCurrentUrl;
    removeSource( "youtube" );
    setData( "youtube", "message", i18n( "Fetching content.." ) );
    m_wikiJob = KIO::storedGet( m_wikiCurrentUrl, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_wikiJob, SIGNAL( result( KJob* ) ), SLOT( wikiResult( KJob* ) ) );
}
*/
#include "VideoclipEngine.moc"

