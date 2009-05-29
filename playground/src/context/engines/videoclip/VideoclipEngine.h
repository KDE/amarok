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

#ifndef AMAROK_VIDEOCLIP_ENGINE
#define AMAROK_VIDEOCLIP_ENGINE

#include "ContextObserver.h"
#include "context/DataEngine.h"
#include "meta/Meta.h"

#include <KIO/Job>

#include <QVariant>

//! Struct VideoInfo, contain all the info
struct VideoInfo {
    QString url;        // Url for the browser (http://www.youtube.com/watch?v=153d9tc3Oao )
    QString title;      
    QString coverurl;   
    QString duration;   // formatted as a QString(mm:ss)
    QString desc;       // full description
    QPixmap cover;      // Image data
    QString views;      
    float rating;       // rating should be beetween 0 to 5
    QString videolink;  // direct video link to the downloadable file
    QString source;     // "youtub" or "dailymotion" or "vimeo" or whatever
    int relevancy;      // used to filter and order the files
    int length;
};

using namespace Context;

 /**
   *   This class provide video-clip youtube, dailymotion and vimeo data 
   *   for the Video-clip context applet
   */
class VideoclipEngine : public DataEngine, public ContextObserver, Meta::Observer
{
    Q_OBJECT        
public:
    VideoclipEngine( QObject* parent, const QList<QVariant>& args );
    virtual ~VideoclipEngine();

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
    void resultYoutube( KJob* );
    void resultYoutubeGetLink( KJob* );
    void resultDailymotion( KJob* );
    void resultVimeo( KJob* );
    void resultVimeoBis( KJob* );
    void resultVimeoGetLink( KJob* );

    void resultImageFetcher( KJob * );
    void resultFinalize();
    
private:
    void update();
    void reloadVid();

    KJob* m_jobYoutube;
    KJob* m_jobDailymotion;
    KJob* m_jobVimeo;
    
    int m_nbJobs;
    int m_nbVidsPerService;
    
    QStringList m_sources;

    Meta::TrackPtr m_currentTrack;
    // Cache the title/artist of the current track so we can check against
    // metadata updates. We only want to update the video clip if either the
    // title or the artist change (but not other attributes like rating, score,
    // composer etc).
    QString    m_title;
    QString    m_artist;
    int        m_length;
    // stores what features are enabled
    bool m_requested;
    
    // List containing all the info
    QList < VideoInfo *> m_video;

};

Q_DECLARE_METATYPE ( VideoInfo );
K_EXPORT_AMAROK_DATAENGINE( videoclip, VideoclipEngine )

#endif

