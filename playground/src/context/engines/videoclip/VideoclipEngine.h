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
#include "meta/Meta.h"

#include <context/DataEngine.h>

#include <KIO/Job>
#include <QLocale>
#include <QString>
#include <QHash>

/**
    This class provide video clip from youtube and dailymotion data for use in Context applets.

NOTE: The QVariant data is structured like this:
           * the key name is the artist
           * the data is a QSTring containing the link of the video
*/

using namespace Context;

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
    void resultDailymotion( KJob* );
    void resultVimeo( KJob* );
    void resultVimeoBis( KJob* );
    
    void resultImageFetcher( KJob * );
    
private:
    void update();
    void reloadVid();

    KJob* m_jobYoutube;
    KJob* m_jobDailymotion;
    KJob* m_jobVimeo;
    
    QStringList m_sources;

    Meta::TrackPtr m_currentTrack;
    // Cache the title/artist of the current track so we can check against
    // metadata updates. We only want to update the video clip if either the
    // title or the artist change (but not other attributes like rating, score,
    // composer etc).
    QString    m_title;
    QString    m_artist;

    // stores what features are enabled
    bool m_requested;


    QStringList vid_title, vid_id, vid_cover, vid_duration, vid_desc, vid_views, vid_rating;
    QHash<QString, QVariant> vid_coverpix;
};

K_EXPORT_AMAROK_DATAENGINE( videoclip, VideoclipEngine )

#endif

