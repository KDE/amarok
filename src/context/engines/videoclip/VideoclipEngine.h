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

#ifndef AMAROK_VIDEOCLIP_ENGINE
#define AMAROK_VIDEOCLIP_ENGINE

#include "ContextObserver.h"
#include "context/DataEngine.h"
#include "meta/Meta.h"
#include "VideoclipInfo.h"

#include <KIO/Job>

#include <QVariant>

#include <iostream>
#include <sstream>

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

 /**
   *   This slots will handle Youtube result for this query :
   *   http://gdata.youtube.com/feeds/videos?q=ARTIST TITLE&orderby=relevance&max-results=7
   *   see here for details: http://code.google.com/intl/fr/apis/youtube/2.0/reference.html
   */
    void resultYoutube( KJob* );

 /**
   *   This method will fetch the required key for the video downloading.For this we retrieve
   *   the page "http://www.youtube.com/watch?v=2dCaCy-7oUE" and then parse the source code
   *   looking for a key like this t=vjVQa1PpcFMG_rV3ADlYhv_c239yN2qIB1RMVIHg5jc%3D
   *   which is the key that protect the data. The final url will look like this ;
   *   http://www.youtube.com/get_video?video_id=2dCaCy-7oUE&t=vjVQa1PpcFMG_rV3ADlYhv_c239yN2qIB1RMVIHg5jc%3D"
   *
   *   \warning : Not legal, and this will probably change like every 6 month with every new youtube api :/
   */
    void resultYoutubeGetLink( KJob* );

  /**
   *   This slots will handle Dailymotion result for this query :
   *   http://www.dailymotion.com/rss/rated/search/ARTIST TITLE
   *   see here for details: No dailymotion API for now :/
   *   But we get every info from one single page which is great.
   */
    void resultDailymotion( KJob* );

    //TODO Find a simplier way to get vimeo info, maybe going through the API
  /**
   *   Vimeo is not fun, we need 3 jobs for one complete Video item :/
   *   http://vimeo.com/videos/search:ARTIST TITLE
   */
    void resultVimeo( KJob* );

  /**
   *   Query result for one video item like this :
   *   http://vimeo.com/api/clip/1774707.xml
   */
    void resultVimeoBis( KJob* );

  /**
   *   We also need to query http://vimeo.com/api/clip/1774707.xml
   *   to get the link, the 2 importants parameters are 'request_signature' and 'request_signature_expires'
   *   final ddl looks like this
   *   http://vimeo.com/moogaloop/play/clip:ID/REQUEST_SIGNATURE/REQUEST_SIGNATURE_EXPIRES/?q=hd
   */
    void resultVimeoGetLink( KJob* );

  /**
   *   An image fetcher, will store the QPixmap in the corresponding videoInfo
   */
    void resultImageFetcher( KJob * );

  /**
   *   This method will send the info to the applet and order them if every jobs are finished
   */
    void resultFinalize();


private:
  /**
   *   Engine was updated, so we check if the songs is different, and if it is, we delete every and start
   *   all the query/ fetching stuff
   */
    void update();

    // TODO implement a reload
    void reloadVid();

  /**
   *   We don't want some jobs if it's useless
   *   So if the video title doesn't contain ARTIST or TITLE
   *   and the description doesn't contain ARTIST either, we remove this item
   *   \return true if the video Info is OK, else false
   */
    bool isVideoInfoValid( VideoInfo * );

    KJob* m_jobYoutube;
    KJob* m_jobDailymotion;
    KJob* m_jobVimeo;

    int m_nbYoutube;
    int m_nbDailymotion;
    int m_nbVimeo;

    QList < QString > m_listJob;
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

    bool       m_youtubeHQ; // boolean, store youtube HQ state, default is no

    // stores what features are enabled
    bool m_requested;

    //!  List containing all the info
    QList < VideoInfo *> m_video;

};

Q_DECLARE_METATYPE ( VideoInfo * )
K_EXPORT_AMAROK_DATAENGINE( videoclip, VideoclipEngine )

#endif

