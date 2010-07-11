/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 * Copyright (c) 2010 Daniel Faust <hessijames@gmail.com>                               *
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

#ifndef AMAROK_LABELS_ENGINE
#define AMAROK_LABELS_ENGINE

#include "LabelsInfo.h"

#include "ContextObserver.h"
#include "context/DataEngine.h"
#include "core/meta/Meta.h"

class KJob;

using namespace Context;

 /**
   *   This class provide labels from flickr
   *
   */
class LabelsEngine : public DataEngine, public ContextObserver, Meta::Observer
{
    Q_OBJECT
public:
    LabelsEngine( QObject *parent, const QList<QVariant> &args );
    virtual ~LabelsEngine();

    QStringList sources() const;
    // reimplemented from Context::Observer
    virtual void message( const ContextState &state );
    // reimplemented from Meta::Observer
    using Observer::metadataChanged;
    void metadataChanged( Meta::TrackPtr track );

protected:
    //reimplement from Plasma::DataEngine
    bool sourceRequestEvent( const QString &name );

private slots:

 /**
   *   This slots will handle last.fm result for this query:
   *   API key is : 402d3ca8e9bc9d3cf9b85e1202944ca5
   *   http://ws.audioscrobbler.com/2.0/?method=track.gettoptags&artist=radiohead&track=paranoid+android&api_key=b25b959554ed76058ac220b7b2e0a026
   *   see here for details: http://www.lastfm.com/api/
   */
    void resultLastFm( KJob * );

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
    void reloadLabels(); // NOTE what is that function doing? reload already implemented.

    KJob                    *m_jobLastFm;

    int                     m_numLastFm;         // number of labels we got from last.fm

    QStringList             m_sources;

    Meta::TrackPtr          m_currentTrack;
    
    // Cache the artist and title of the current track so we can check against metadata
    // updates. We only want to update the labels if the artist change
    QString                 m_artist;
    QString                 m_title;

    int                     m_try;

    // stores what features are enabled
    bool                    m_requested;
    bool                    m_reload;

    //!  List containing all the info
    QList < LabelsInfo * >  m_labels;           // Item with all the information

};

Q_DECLARE_METATYPE ( QList < LabelsInfo * > )
K_EXPORT_AMAROK_DATAENGINE( labels, LabelsEngine )
#endif

