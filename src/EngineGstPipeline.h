/****************************************************************************************
 * Copyright (C) 2025 Tuomas Nurmi <tuomas@norsumanageri.org>                           *
 * Mostly based on phonon-gstreamer code by:                                            *
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).                    *
 * Copyright (C) 2011 Torrie Fischer <tdfischer@kde.org>                                *
 * Copyright (C) 2011 Harald Sitter <sitter@kde.org>                                    *
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

#include "amarok_export.h"

#include <QMultiMap>
#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QUrl>
#include <QWaitCondition>

#include <gst/gst.h>

class QTimer;
typedef QMultiMap<QString, QString> TagMap;

class AMAROK_EXPORT EngineGstPipeline : public QObject {
    Q_OBJECT
public:
    EngineGstPipeline();
    ~EngineGstPipeline();

    QStringList availableMimeTypes();

    void setSource(const QUrl &source, bool reset = false);
    void setNextSource(const QUrl &source);
    qint64 totalDuration() const;
    qint64 position() const;
    qint64 remainingTime() const;
    QUrl currentSource() const;
    TagMap metaData() const;
    bool isMuted();
    qreal volume();

    // GStreamer callbacks from phonon-gstreamer
    static gboolean cb_eos(GstBus *bus, GstMessage *msg, gpointer data);
    static gboolean cb_warning(GstBus *bus, GstMessage *msg, gpointer data);
    static gboolean cb_duration(GstBus *bus, GstMessage *msg, gpointer data);
    static gboolean cb_buffering(GstBus *bus, GstMessage *msg, gpointer data);
    static gboolean cb_state(GstBus *bus, GstMessage *msg, gpointer data);
    static gboolean cb_error(GstBus *bus, GstMessage *msg, gpointer data);
    static gboolean cb_tag(GstBus *bus, GstMessage *msg, gpointer data);
    static gboolean cb_streamStart(GstBus *bus, GstMessage *msg, gpointer data);

    static void cb_aboutToFinish(GstElement *appSrc, gpointer data);
    static void cb_endOfPads(GstElement *playbin, gpointer data);

    static void cb_audioTagsChanged(GstElement *playbin, gint stream, gpointer data);
    static void cb_setupSource(GstElement *playbin, GParamSpec *spec, gpointer data);
    static void cb_volumeChanged(GstElement *playbin, GParamSpec *spec, gpointer data);
    GstState state() const;
    GstStateChangeReturn setState(GstState state);
    void requestState(GstState state);

    bool seekToMSec(qint64 time);
    bool isSeekable() const;
    bool isPlaybackQueueEmpty();
    int playbackQueueLength();

    bool isReplayGainReady();
public Q_SLOTS:
    void emitTick();
    void handleStateChange(GstState oldState, GstState newState);
    void handleStreamChange();
    void handleEndOfStream();
    void handleAboutToFinish();
    void enqueuePlayback( const QUrl &source );
    void clearPlaybackQueue();
    void stop();
    void play();
    void pause();
    void setMuted(bool status);
    void setVolume(qreal newVolume);
    void setGain(qreal newGain);
Q_SIGNALS:
    void tick(qint64 time);
    void eos();
    void warning(const QString &message);
    void durationChanged(qint64 totalDuration);
    void trackCountChanged(int tracks);
    void buffering(int);
    void stateChanged(GstState oldState, GstState newState);
    void internalStateChanged(GstState oldState, GstState newState);
    void audioTagChanged(int stream);
    void currentSourceChanged( const QUrl &url );
    // Only emitted when metadata changes in the middle of a stream.
    void metaDataChanged();
    void seekableChanged(bool isSeekable);
    void aboutToFinish();
    void finished();
    void streamChanged();
    void volumeChanged(qreal newVolume);
    void mutedChanged(bool mute);

private:
    GstPipeline *m_pipeline;
    GstElement *m_gstVolume;
    GstElement *m_replayGainElement;
    //This simply pauses the gst signal handler 'till we get something
    QMutex m_aboutToFinishLock;
    QWaitCondition m_aboutToFinishWait;
    /*** Tracks wherever theactively handling an aboutToFinish CB right now. */
    bool m_handlingAboutToFinish;

    GstState m_currentState;
    QQueue<QUrl> m_playbackQueue;
    QUrl m_currentSource;
    QTimer *m_tickTimer;
    bool m_waitingForNextSource;
    bool m_waitingForPreviousSource;
    bool m_seeking;
    bool m_resetting;
    bool m_skipGapless;
    bool m_skippingEOS;
    bool m_doingEOS;
    int m_bufferPercent;
    qint64 m_posAtReset;
    QMutex m_tagLock;
    QMultiMap<QString, QString> m_metaData;
};
