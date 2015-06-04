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

#include "ContextObserver.h"
#include "context/DataEngine.h"
#include "core/meta/forward_declarations.h"
#include "network/NetworkAccessManagerProxy.h"

#include <QMap>
#include <QTimer>
#include <QWeakPointer>

using namespace Context;

 /**
   *   This class provide labels from last.fm
   *
   */
class LabelsEngine : public DataEngine, public ContextObserver
{
    Q_OBJECT
public:
    LabelsEngine( QObject *parent, const QList<QVariant> &args );
    virtual ~LabelsEngine();

    QStringList sources() const;

protected:
    // reimplemented from Plasma::DataEngine
    bool sourceRequestEvent( const QString &name );

private Q_SLOTS:

    void update( bool reload = false );

    /**
     *   This slots will handle last.fm result for this query:
     *   API key is : 402d3ca8e9bc9d3cf9b85e1202944ca5
     *   http://ws.audioscrobbler.com/2.0/?method=track.gettoptags&artist=radiohead&track=paranoid+android&api_key=b25b959554ed76058ac220b7b2e0a026
     *   see here for details: http://www.lastfm.com/api/
     */
    void resultLastFm( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );

    void resultReady( const Meta::LabelList &labels );
    void dataQueryDone();

    void timeout();

private:
  /**
   *   Engine was updated, so we check if the songs is different, and if it is, we delete every and start
   *   all the query/ fetching stuff
   */
    void fetchLastFm();
    void updateLocal();

    QTimer m_timeoutTimer;

    /// The URL for the network request
    QUrl m_lastFmUrl;

    QStringList                 m_sources;

    // Cache the artist and title of the current track so we can check against metadata
    // updates. We only want to update the labels if the artist change
    QString                     m_artist;
    QString                     m_title;
    // Send the album name to the applet, used to filter labels that match the album
    QString                     m_album;

    int                         m_try;

    QStringList                 m_allLabels;           // all labels known to amarok
    QStringList                 m_userLabels;          // user labels
    QMap < QString, QVariant >  m_webLabels;           // downloaded labels

};

AMAROK_EXPORT_DATAENGINE( labels, LabelsEngine )
#endif

