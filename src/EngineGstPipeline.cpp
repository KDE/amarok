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

#include "EngineGstPipeline.h"
#include "amarokconfig.h"
#include "core/support/Debug.h"
#include "Version.h"

#include <QTimer>

#include <gst/audio/streamvolume.h>
#include <gst/audio/audio-format.h>

EngineGstPipeline::EngineGstPipeline()
    : m_currentSinkElement(nullptr)
    , m_replayGainElement(nullptr)
    , m_equalizerElement(nullptr)
    , m_analyzerDataSize(512)
    , m_channels(1)
    , m_handlingAboutToFinish(false)
    , m_tickTimer(new QTimer(this))
    , m_waitingForNextSource(true)
    , m_waitingForPreviousSource(false)
    , m_seeking(false)
    , m_resetting(false)
    , m_skipGapless(false)
    , m_skippingEOS(false)
    , m_doingEOS(false)
    , m_bufferPercent(0)
    , m_posAtReset(0)
{

    DEBUG_BLOCK
    qRegisterMetaType<GstState>("GstState");
    m_pipeline = GST_PIPELINE(gst_element_factory_make("playbin3", nullptr));
    gst_object_ref_sink (m_pipeline);
    g_signal_connect(m_pipeline, "audio-tags-changed", G_CALLBACK(cb_audioTagsChanged), this);
    g_signal_connect(m_pipeline, "source-setup", G_CALLBACK(cb_setupSource), this);
    g_signal_connect(m_pipeline, "about-to-finish", G_CALLBACK(cb_aboutToFinish), this);

    GstBus *bus = gst_pipeline_get_bus(m_pipeline);
    gst_bus_set_sync_handler(bus, gst_bus_sync_signal_handler, nullptr, nullptr);
    g_signal_connect(bus, "sync-message::eos", G_CALLBACK(cb_eos), this);
    g_signal_connect(bus, "sync-message::warning", G_CALLBACK(cb_warning), this);
    g_signal_connect(bus, "sync-message::duration-changed", G_CALLBACK(cb_duration), this);
    g_signal_connect(bus, "sync-message::buffering", G_CALLBACK(cb_buffering), this);
    g_signal_connect(bus, "sync-message::state-changed", G_CALLBACK(cb_state), this);
    g_signal_connect(bus, "sync-message::error", G_CALLBACK(cb_error), this);
    g_signal_connect(bus, "sync-message::stream-start", G_CALLBACK(cb_streamStart), this);
    g_signal_connect(bus, "sync-message::tag", G_CALLBACK(cb_tag), this);
    gst_object_unref(bus);

    g_object_set(m_pipeline, "video-sink", gst_element_factory_make( "fakesink", "discard_video" ), nullptr); // Discard any video

    m_replayGainElement = gst_element_factory_make("volume", "replaygain");
    m_equalizerElement = gst_element_factory_make("equalizer-10bands", "equalizer");
    GstElement *teeElement = gst_element_factory_make("tee", "tee");

    if( !m_replayGainElement )
        debug() << "failed to create replay gain volume element";
    if( !m_replayGainElement )
        debug() << "failed to create equalizer element";
    if( !teeElement )
        debug() << "failed to create tee element";

    if( m_equalizerElement || m_replayGainElement || teeElement )
    {
        GstElement *bin = gst_bin_new("bin-with-extra");

        GstElement *queue = gst_element_factory_make("queue", "output-queue");
        GstElement *audioSink = gst_element_factory_make("autoaudiosink", "audio-output");
        g_signal_connect(audioSink, "child-added", G_CALLBACK(cb_sinkElementAdded), this);
        g_signal_connect(audioSink, "child-removed", G_CALLBACK(cb_sinkElementRemoved), this);

        gst_bin_add_many(GST_BIN(bin), queue, audioSink, nullptr);
        if( !gst_element_link(queue, audioSink) )
            debug() << "failed to create custom playback bin (queue + sink)";

        //analyzer code somewhat based on phonon-gstreamer AudioDataOutput ctor
        GstElement* analyzersink = gst_element_factory_make("fakesink", "analyzer-fakesink");
        GstElement* analyzerqueue = gst_element_factory_make("queue", "analyzer-queue");
        GstElement* analyzerconvert = gst_element_factory_make("audioconvert", "analyzer-convert");
        gst_bin_add_many(GST_BIN(bin), analyzersink, analyzerconvert, analyzerqueue, nullptr);
        if( !analyzersink || !analyzerqueue || !analyzerconvert )
            debug() << "failed to create custom playback bin (analyzer queue)";

        g_signal_connect(analyzersink, "handoff", G_CALLBACK(analyzerProcessBuffer), this);
        g_object_set(G_OBJECT(analyzersink), "signal-handoffs", true, nullptr);

        //G_BYTE_ORDER is the host machine's endianness
        GstCaps *caps = gst_caps_new_simple("audio/x-raw",
                                            "format", G_TYPE_STRING, GST_AUDIO_NE (S16),
                                            nullptr);

        gst_element_sync_state_with_parent( analyzerqueue );
        gst_element_sync_state_with_parent( analyzerconvert );
        gst_element_sync_state_with_parent( analyzersink );

        gst_element_link(analyzerqueue, analyzerconvert);
        gst_element_link_filtered(analyzerconvert, analyzersink, caps);
        gst_caps_unref(caps);
        g_object_set(G_OBJECT(analyzersink), "sync", true, nullptr);


        GstElement *target = queue;
        if( m_replayGainElement )
            gst_bin_add(GST_BIN(bin), m_replayGainElement);
        if( m_equalizerElement )
            gst_bin_add(GST_BIN(bin), m_equalizerElement);
        if( teeElement )
        {
            gst_bin_add(GST_BIN(bin), teeElement);
            target = teeElement;
        }

        if( m_replayGainElement && m_equalizerElement )
        {
            if( !gst_element_link( m_replayGainElement, m_equalizerElement) ||
                !gst_element_link( m_equalizerElement, target) )
                    debug() << "failed to create custom playback bin";
        }
        else if( m_replayGainElement )
        {
            if( !gst_element_link( m_replayGainElement, target) )
                debug() << "failed to create custom playback bin (replaygain)";
        }
        else if( m_equalizerElement )
        {
            if( !gst_element_link( m_equalizerElement, target) )
                debug() << "failed to create custom playback bin (equalizer)";
        }
        if( teeElement )
        {
            if( !gst_element_link( teeElement, queue) )
                debug() << "failed to create custom playback bin (tee)";
            if( !gst_element_link( teeElement, analyzerqueue) )
                debug() << "failed to link analyzer queue";
        }

        GstPad *pad, *ghostPad;
        if( m_replayGainElement )
            pad = gst_element_get_static_pad(m_replayGainElement, "sink");
        else
            pad = gst_element_get_static_pad(m_equalizerElement, "sink");
        if (!pad) {
            debug() << "no sink pad";
        }
        ghostPad = gst_ghost_pad_new("sink", pad);
        gst_pad_set_active(ghostPad, TRUE);
        gst_element_add_pad(bin, ghostPad);
        gst_object_unref(pad);

        g_object_set( GST_BIN(m_pipeline), "audio-sink", bin, nullptr);
    }
    else
    {
        debug() << "failed to create custom playback bin";
        g_signal_connect( m_pipeline, "notify::volume", G_CALLBACK (cb_volumeChanged), this);
        g_signal_connect( m_pipeline, "notify::mute", G_CALLBACK (cb_muteChanged), this);
    }

    if(m_replayGainElement)
        gst_object_ref(m_replayGainElement);
    if(m_equalizerElement)
        gst_object_ref(m_equalizerElement);

    m_tickTimer->setInterval( 100 );
    connect(m_tickTimer, &QTimer::timeout, this, &EngineGstPipeline::emitTick);
    connect(this, &EngineGstPipeline::internalStateChanged, this, &EngineGstPipeline::handleStateChange);
}

EngineGstPipeline::~EngineGstPipeline()
{
    if(m_replayGainElement)
        gst_object_unref(m_replayGainElement);
    if(m_equalizerElement)
        gst_object_unref(m_equalizerElement);
    g_signal_handlers_disconnect_by_data(m_pipeline, this);
    gst_element_set_state(GST_ELEMENT(m_pipeline), GST_STATE_NULL);
    gst_object_unref(m_pipeline);
    m_pipeline = nullptr;
}

bool
EngineGstPipeline::isReplayGainReady()
{
    return m_replayGainElement;
}

TagMap
EngineGstPipeline::metaData() const
{
    return m_metaData;
}

bool
EngineGstPipeline::isMuted()
{
    gboolean mute;
    if( !m_currentSinkElement )
        g_object_get(GST_BIN(m_pipeline), "mute", &mute, nullptr);
    else
        g_object_get(G_OBJECT(m_currentSinkElement), "mute", &mute, nullptr);
    return mute;
}

qreal
EngineGstPipeline::volume()
{
    gdouble vol;
    if( !m_currentSinkElement )
        g_object_get(GST_BIN(m_pipeline), "volume", &vol, nullptr);
    else
        g_object_get(G_OBJECT(m_currentSinkElement), "volume", &vol, nullptr);
    return gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_LINEAR, GST_STREAM_VOLUME_FORMAT_CUBIC, vol);
}

GstElement*
EngineGstPipeline::eqElement()
{
    return m_equalizerElement;
}

void
EngineGstPipeline::setMuted(bool status)
{
    if( m_currentSinkElement )
        g_object_set( G_OBJECT(m_currentSinkElement), "mute", status, nullptr);
    else
        g_object_set( GST_BIN(m_pipeline), "mute", status, nullptr);
}

void
EngineGstPipeline::setVolume(qreal newVolume)
{
    if( m_currentSinkElement )
        g_object_set( G_OBJECT(m_currentSinkElement), "volume", gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_CUBIC, GST_STREAM_VOLUME_FORMAT_LINEAR, newVolume), nullptr);
    else
        g_object_set( GST_BIN(m_pipeline), "volume", gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_CUBIC, GST_STREAM_VOLUME_FORMAT_LINEAR, newVolume), nullptr);
}

static const qreal log10over20 = 0.1151292546497022842; // ln(10) / 20

void
EngineGstPipeline::setGain(qreal newGain)
{
    if( m_replayGainElement )
        gst_stream_volume_set_volume(GST_STREAM_VOLUME(m_replayGainElement), GST_STREAM_VOLUME_FORMAT_LINEAR, qExp( newGain * log10over20 ) );
}

void
EngineGstPipeline::handleStateChange(GstState oldState, GstState newState)
{
    DEBUG_BLOCK;

    m_currentState = newState;
    debug() << "Moving from" << gst_element_state_get_name( oldState ) << "to" << gst_element_state_get_name( newState );
    if (GST_STATE_TRANSITION(oldState, newState) == GST_STATE_CHANGE_NULL_TO_READY) {
        // TODO loadingComplete();
    }
    if (GST_STATE_TRANSITION(oldState, newState) == GST_STATE_CHANGE_READY_TO_PAUSED) { //TODO && m_pendingTitle != 0) {
        // TODO _iface_setCurrentTitle(m_pendingTitle);
    }
    if (newState == GST_STATE_PLAYING) {
        m_tickTimer->start();
    } else {
        m_tickTimer->stop();
    }

    if (newState == GST_STATE_READY) {
        Q_EMIT tick(0);
    }

    if (!m_doingEOS) {
         Q_EMIT stateChanged(oldState, newState);
    }
}

void
EngineGstPipeline::emitTick()
{
   /* if (m_resumeState) {
        return;
    }*/

    qint64 currentTime = position();

    Q_EMIT tick(currentTime);

    /*if (m_state == GST_STATE_PLAYING {
        if (currentTime >= totalTime() - m_prefinishMark) {
            if (m_prefinishMarkReachedNotEmitted) {
                m_prefinishMarkReachedNotEmitted = false;
                emit prefinishMarkReached(totalTime() - currentTime);
            }
        }
    }*/ //TODO
}

void
EngineGstPipeline::setNextSource(const QUrl &source)
{
    DEBUG_BLOCK

    if (m_handlingAboutToFinish) {
        debug() << "Got next source. Waiting for end of current.";

        // If next source is valid and is not empty (an empty source is sent by Phonon if
        // there are no more sources) skip EOS for the current source in order to seamlessly
        // pass to the next source.
        // if (source.type() == Phonon::MediaSource::Invalid ||
        //     source.type() == Phonon::MediaSource::Empty
        if( source.isEmpty() ) {
             m_skippingEOS = false;
        } else {
             m_skippingEOS = true;
        }

        m_waitingForNextSource = true;
        m_waitingForPreviousSource = false;
        m_skipGapless = false;
        setSource( source );
        m_aboutToFinishWait.wakeAll();
    } else {
        qDebug() << "Ignoring source as no aboutToFinish handling is in progress.";
    }
}


void
EngineGstPipeline::setSource(const QUrl &source, bool reset)
{
    //m_isStream = false;
    m_seeking = false;
    //m_isHttpUrl = false;
    m_metaData.clear();

    debug() << "New source:" << source;
    QByteArray gstUri = source.toEncoded();
    if( source.scheme() == QStringLiteral("audiocd") )
    {
        QStringList pathItems = source.path().split( QLatin1Char('/'), Qt::KeepEmptyParts );
        int trackNumber = pathItems.at( 2 ).toInt( );  // path has already been verified to be sane by EngineController
        if( trackNumber < 1 )
        {
            trackNumber = 1;
            debug() << "Something strange with CD track number, playing track 1";
        }
        gstUri = "cdda://" + QByteArray::number(trackNumber);
    }

    m_currentSource = source;

    GstState oldState = state();

    if (reset && oldState > GST_STATE_READY) {
        debug() << "Resetting pipeline for reverse seek";
        m_resetting = true;
        m_posAtReset = position();
        gst_element_set_state(GST_ELEMENT(m_pipeline), GST_STATE_READY);
    }

    debug() << "uri" << gstUri;
    g_object_set(m_pipeline, "uri", gstUri.constData(), nullptr);

    if (reset && oldState > GST_STATE_READY) {
        gst_element_set_state(GST_ELEMENT(m_pipeline), oldState);
    }

    m_skipGapless = false;
    m_aboutToFinishWait.wakeAll();
}

QUrl
EngineGstPipeline::currentSource() const
{
    return m_currentSource;
}

qint64
EngineGstPipeline::totalDuration() const
{
    gint64 duration = 0;
    if (gst_element_query_duration(GST_ELEMENT(m_pipeline), GST_FORMAT_TIME, &duration)) {
        return duration/GST_MSECOND;
    }
    return -1;
}

qint64
EngineGstPipeline::position() const
{
    if (m_resetting) {
        return m_posAtReset;
    }

    gint64 pos = 0;
    gst_element_query_position(GST_ELEMENT(m_pipeline), GST_FORMAT_TIME, &pos);
    return (pos / GST_MSECOND);
}

GstState
EngineGstPipeline::state() const
{
    GstState state;
    gst_element_get_state(GST_ELEMENT(m_pipeline), &state, nullptr, 1000);
    return state;
}

void
EngineGstPipeline::cb_sinkElementAdded(GstChildProxy *self, GObject *object, gchar *name, gpointer data)
{
    Q_UNUSED(self)
    Q_UNUSED(name)
    if( !g_object_class_find_property(G_OBJECT_GET_CLASS(object), "volume") )
        return;
    EngineGstPipeline *that = static_cast<EngineGstPipeline*>(data);
    that->m_currentSinkElement = GST_ELEMENT(object);
    g_object_set(G_OBJECT(that->m_currentSinkElement), "mute", AmarokConfig::muteState(), NULL);
    g_object_set(G_OBJECT(that->m_currentSinkElement), "volume", gst_stream_volume_convert_volume(GST_STREAM_VOLUME_FORMAT_CUBIC, GST_STREAM_VOLUME_FORMAT_LINEAR, AmarokConfig::masterVolume()/100.0), NULL);
    g_signal_connect(that->m_currentSinkElement, "notify::volume", G_CALLBACK (cb_volumeChanged), that);
    g_signal_connect(that->m_currentSinkElement, "notify::mute", G_CALLBACK (cb_muteChanged), that);
}


void
EngineGstPipeline::cb_sinkElementRemoved(GstChildProxy *self, GObject *object, gchar *name, gpointer data)
{
    Q_UNUSED(self)
    Q_UNUSED(name)
    EngineGstPipeline *that = static_cast<EngineGstPipeline*>(data);
    if(GST_ELEMENT(object) == that->m_currentSinkElement)
        that->m_currentSinkElement = nullptr;
}

gboolean
EngineGstPipeline::cb_eos(GstBus *bus, GstMessage *gstMessage, gpointer data)
{
    Q_UNUSED(bus)
    Q_UNUSED(gstMessage)
    EngineGstPipeline *that = static_cast<EngineGstPipeline*>(data);
    that->handleEndOfStream();
    return true;
}

gboolean
EngineGstPipeline::cb_warning(GstBus *bus, GstMessage *gstMessage, gpointer data)
{
    Q_UNUSED(bus)
    gchar *debug;
    GError *err;
    EngineGstPipeline *that = static_cast<EngineGstPipeline*>(data);
    gst_message_parse_warning(gstMessage, &err, &debug);
    QString msgString =
        QStringLiteral("Warning: %1\nMessage:%2").arg(QLatin1String(debug)).arg(QLatin1String(err->message));
    Q_EMIT that->warning(msgString);
    g_free (debug);
    g_error_free (err);
    return true;
}

gboolean
EngineGstPipeline::cb_error(GstBus *bus, GstMessage *gstMessage, gpointer data)
{
    Q_UNUSED(bus)
    Q_UNUSED(data)

    GError *err;
    //TODO: Log the error
    //TODO handle specifically missing codecs/plugins
    gst_message_parse_error (gstMessage, &err, nullptr);
    debug()<<err->message;
    g_error_free(err);

    return true;
}


gboolean
EngineGstPipeline::cb_duration(GstBus *bus, GstMessage *gstMessage, gpointer data)
{
    DEBUG_BLOCK;
    Q_UNUSED(bus)
    Q_UNUSED(gstMessage)
    EngineGstPipeline *that = static_cast<EngineGstPipeline*>(data);
    if (that->m_resetting) {
        return true;
    }

    Q_EMIT that->durationChanged(that->totalDuration());
    return true;
}

void
EngineGstPipeline::cb_setupSource(GstElement *playbin, GstElement *source, gpointer data)
{
    Q_UNUSED(playbin);
    DEBUG_BLOCK;

    EngineGstPipeline *that = static_cast<EngineGstPipeline*>(data);
    Q_ASSERT(that->m_pipeline);

    if ( ( that->currentSource().scheme().startsWith(QLatin1String("http") ) ||
           that->currentSource().scheme().startsWith(QLatin1String("rt") ) ) // rtp, rtsp
        // Check whether this property exists.
        // Setting it on a source other than souphttpsrc (which supports it) may break playback.
        && g_object_class_find_property(G_OBJECT_GET_CLASS(source), "user-agent") )
    {
        QString userAgent = ( QStringLiteral( "Amarok/" ) + QStringLiteral(AMAROK_VERSION) );
        g_object_set(source, "user-agent", userAgent.toUtf8().constData(), nullptr);
    }
}

void
EngineGstPipeline::cb_volumeChanged(GstElement *playbin, GParamSpec *spec, gpointer data)
{
    Q_UNUSED(playbin)

    EngineGstPipeline *that = static_cast<EngineGstPipeline *>(data);
    Q_EMIT that->volumeChanged( that->volume() );
    Q_UNUSED(spec)
    Q_UNUSED(data)
}

void
EngineGstPipeline::cb_muteChanged(GstElement *playbin, GParamSpec *spec, gpointer data)
{
    Q_UNUSED(playbin)

    EngineGstPipeline *that = static_cast<EngineGstPipeline *>(data);
    Q_EMIT that->mutedChanged( that->isMuted() );
    Q_UNUSED(spec)
    Q_UNUSED(data)
}

void
EngineGstPipeline::cb_audioTagsChanged(GstElement *playbin, gint stream, gpointer data)
{
    Q_UNUSED(playbin)
    EngineGstPipeline *that = static_cast<EngineGstPipeline *>(data);
    Q_EMIT that->audioTagChanged(stream);
}

gboolean
EngineGstPipeline::cb_buffering(GstBus *bus, GstMessage *gstMessage, gpointer data)
{
    DEBUG_BLOCK;
    Q_UNUSED(bus)
    EngineGstPipeline *that = static_cast<EngineGstPipeline*>(data);
    gint percent = 0;
    gst_message_parse_buffering(gstMessage, &percent);

    // we should not trigger paused state or gstreamer will starts buffering again
    if (percent == 0) {
        return true;
    }

    debug() << Q_FUNC_INFO << "Buffering :" << percent;

    // Instead of playing when the pipeline is still streaming, we pause
    // and let gst finish streaming.
    if ( percent < 100 && gstMessage->type == GST_MESSAGE_BUFFERING) {
        QMetaObject::invokeMethod(that, "setState", Qt::QueuedConnection, Q_ARG(GstState, GST_STATE_PAUSED));

    } else {
        QMetaObject::invokeMethod(that, "setState", Qt::QueuedConnection, Q_ARG(GstState, GST_STATE_PLAYING));
    }

    if (that->m_bufferPercent != percent) {
        Q_EMIT that->buffering(percent);
        that->m_bufferPercent = percent;
    }

    return true;
}

gboolean
EngineGstPipeline::cb_streamStart(GstBus *bus, GstMessage *msg, gpointer data)
{
    Q_UNUSED(bus)
    Q_UNUSED(msg)
    EngineGstPipeline *that = static_cast<EngineGstPipeline*>(data);
    gchar *uri;
    g_object_get(that->m_pipeline, "uri", &uri, nullptr);
    debug() << "Stream changed to" << uri;
    g_free(uri);
    if (!that->m_resetting) {
        Q_EMIT that->handleStreamChange();
    }
    return true;
}

/*
 * Used to iterate through the gst_tag_list and extract values
 */
void foreach_tag_function(const GstTagList *list, const gchar *tag, gpointer user_data)
{
    TagMap *newData = static_cast<TagMap *>(user_data);
    QString value;
    GType type = gst_tag_get_type(tag);
    switch (type) {
    case G_TYPE_STRING: {
            char *str = nullptr;
            gst_tag_list_get_string(list, tag, &str);
            value = QString::fromUtf8(str);
            g_free(str);
        }
        break;

    case G_TYPE_BOOLEAN: {
            int bval;
            gst_tag_list_get_boolean(list, tag, &bval);
            value = QString::number(bval);
        }
        break;

    case G_TYPE_INT: {
            int ival;
            gst_tag_list_get_int(list, tag, &ival);
            value = QString::number(ival);
        }
        break;

    case G_TYPE_UINT: {
            unsigned int uival;
            gst_tag_list_get_uint(list, tag, &uival);
            value = QString::number(uival);
        }
        break;

    case G_TYPE_FLOAT: {
            float fval;
            gst_tag_list_get_float(list, tag, &fval);
            value = QString::number(fval);
        }
        break;

    case G_TYPE_DOUBLE: {
            double dval;
            gst_tag_list_get_double(list, tag, &dval);
            value = QString::number(dval);
        }
        break;

    default:
        //debug() << "Unsupported tag type:" << g_type_name(type);
        break;
    }

    QString key = QString(QLatin1String(tag)).toUpper();
    QString currVal = newData->value(key);
    if (!value.isEmpty() && !(newData->contains(key) && currVal == value)) {
        newData->insert(key, value);
    }
}

gboolean
EngineGstPipeline::cb_tag(GstBus *bus, GstMessage *msg, gpointer data)
{
    DEBUG_BLOCK
    Q_UNUSED(bus)
    EngineGstPipeline *that = static_cast<EngineGstPipeline*>(data);
    QMutexLocker lock(&that->m_tagLock);

    bool isStream = that->currentSource().scheme().startsWith(QLatin1String("http") ) ||
                        that->currentSource().scheme().startsWith(QLatin1String("rt") ); // rtp, rtsp
    GstTagList* tag_list = nullptr;
    gst_message_parse_tag(msg, &tag_list);
    if (tag_list) {
        TagMap newTags;
        gst_tag_list_foreach(tag_list, &foreach_tag_function, &newTags);
        gst_tag_list_unref(tag_list);

        // Determine if we should no fake the album/artist tags.
        // This is a little confusing as we want to fake it on initial
        // connection where title, album and artist are all missing.
        // There are however times when we get just other information,
        // e.g. codec, and so we want to only do clever stuff if we
        // have a commonly available tag (ORGANIZATION) or we have a
        // change in title

       bool fake_it =
           (isStream
            && ((!newTags.contains(QStringLiteral("TITLE"))
                 && newTags.contains(QStringLiteral("ORGANIZATION")))
                || (newTags.contains(QStringLiteral("TITLE"))
                    && that->m_metaData.value(QStringLiteral("TITLE")) != newTags.value(QStringLiteral("TITLE"))))
            && !newTags.contains(QStringLiteral("ALBUM"))
            && !newTags.contains(QStringLiteral("ARTIST")));

        TagMap oldMap = that->m_metaData; // Keep a copy of the old one for reference

        // Now we've checked the new data, append any new meta tags to the existing tag list
        // We cannot use TagMap::iterator as this is a multimap and when streaming data
        // could in theory be lost.
        QList<QString> keys = newTags.keys();
        for (QList<QString>::iterator i = keys.begin(); i != keys.end(); ++i) {
            QString key = *i;
            if (isStream) {
                // If we're streaming, we need to remove data in m_metaData
                // in order to stop it filling up indefinitely (as it's a multimap)
                that->m_metaData.remove(key);
            }
            QList<QString> values = newTags.values(key);
            for (QList<QString>::iterator j = values.begin(); j != values.end(); ++j) {
                QString value = *j;
                QString currVal = that->m_metaData.value(key);
                if (!that->m_metaData.contains(key) || currVal != value) {
                    that->m_metaData.insert(key, value);
                }
            }
        }

        if (that->m_metaData.contains(QStringLiteral("TRACK-COUNT"))) {
            that->m_metaData.insert(QStringLiteral("TRACKNUMBER"), newTags.value(QStringLiteral("TRACK-COUNT")));
            Q_EMIT that->trackCountChanged(newTags.value(QStringLiteral("TRACK-COUNT")).toInt());
        }

        debug() << that << "Meta tags found";
        if (oldMap != that->m_metaData) {
            // This is a bit of a hack to ensure that stream metadata is
            // returned. We get as much as we can from the Shoutcast server's
            // StreamTitle= header. If further info is decoded from the stream
            // itself later, then it will overwrite this info.
            if (fake_it) {
                that->m_metaData.remove(QStringLiteral("ALBUM"));
                that->m_metaData.remove(QStringLiteral("ARTIST"));

                // Detect whether we want to "fill in the blanks"
                QString str;
                if (that->m_metaData.contains(QStringLiteral("TITLE")))
                {
                    str = that->m_metaData.value(QStringLiteral("TITLE"));
                    int splitpoint;
                    // Check to see if our title matches "%s - %s"
                    // Where neither %s are empty...
                    if ((splitpoint = str.indexOf(QStringLiteral(" - "))) > 0
                        && str.size() > (splitpoint+3)) {
                        that->m_metaData.insert(QStringLiteral("ARTIST"), str.left(splitpoint));
                        that->m_metaData.replace(QStringLiteral("TITLE"), str.mid(splitpoint+3));
                    }
                } else {
                    str = that->m_metaData.value(QStringLiteral("GENRE"));
                    if (!str.isEmpty()) {
                        that->m_metaData.insert(QStringLiteral("TITLE"), str);
                    } else {
                        that->m_metaData.insert(QStringLiteral("TITLE"), QStringLiteral("Streaming Data"));
                    }
                }
                if (!that->m_metaData.contains(QStringLiteral("ARTIST"))) {
                    str = that->m_metaData.value(QStringLiteral("LOCATION"));
                    if (!str.isEmpty()) {
                        that->m_metaData.insert(QStringLiteral("ARTIST"), str);
                    } else {
                        that->m_metaData.insert(QStringLiteral("ARTIST"), QStringLiteral("Streaming Data"));
                    }
                }
                str = that->m_metaData.value(QStringLiteral("ORGANIZATION"));
                if (!str.isEmpty()) {
                    that->m_metaData.insert(QStringLiteral("ALBUM"), str);
                } else {
                    that->m_metaData.insert(QStringLiteral("ALBUM"), QStringLiteral("Streaming Data"));
                }
            }

            Q_EMIT that->metaDataChanged();
        }
    }
    return true;
}

void
EngineGstPipeline::cb_aboutToFinish(GstElement *appSrc, gpointer data)
{
    Q_UNUSED(appSrc);
    EngineGstPipeline *that = static_cast<EngineGstPipeline*>(data);
    Q_EMIT that->handleAboutToFinish();
}

gboolean
EngineGstPipeline::cb_state(GstBus *bus, GstMessage *gstMessage, gpointer data)
{
    Q_UNUSED(bus)
    GstState oldState;
    GstState newState;
    GstState pendingState;
    EngineGstPipeline *that = static_cast<EngineGstPipeline*>(data);
    gst_message_parse_state_changed(gstMessage, &oldState, &newState, &pendingState);

    if (oldState == newState) {
        return true;
    }

    if (gstMessage->src != GST_OBJECT(that->m_pipeline)) {
        return true;
    }

    // Apparently gstreamer sometimes enters the same state twice.
    // FIXME: Sometimes we enter the same state twice. currently not disallowed by the state machine
    if (that->m_seeking) {
        if (GST_STATE_TRANSITION(oldState, newState) == GST_STATE_CHANGE_PAUSED_TO_PLAYING) {
            that->m_seeking = false;
        }
        return true;
    }
    debug() << "State change";

    //FIXME: This is a hack until proper state engine is implemented in the pipeline
    // Wait to update stuff until we're at the final requested state
    if (pendingState == GST_STATE_VOID_PENDING && newState > GST_STATE_READY && that->m_resetting) {
        that->m_resetting = false;
        that->seekToMSec(that->m_posAtReset);
//        return;
    }

    if (pendingState == GST_STATE_VOID_PENDING) {
        Q_EMIT that->durationChanged(that->totalDuration());
        Q_EMIT that->seekableChanged(that->isSeekable());
    }

    Q_EMIT that->internalStateChanged(oldState, newState);
    if( oldState < GST_STATE_PAUSED && newState > GST_STATE_READY ) // apply any changes that were done when playback was stopped
    {
        if( int( ( that->volume() + 0.005 ) * 100) != AmarokConfig::masterVolume()  )
            that->setVolume( AmarokConfig::masterVolume() / 100.0 );
        if( that->isMuted() != AmarokConfig::muteState() )
            that->setMuted( AmarokConfig::muteState() );
    }
    return true;
}

GstStateChangeReturn
EngineGstPipeline::setState(GstState state)
{
    DEBUG_BLOCK
    if(m_currentState == state)
    {
        debug() << "not reapplying gst state";
        return GST_STATE_CHANGE_SUCCESS;
    }
    debug() << "Transitioning to state" << gst_element_state_get_name(state);

    return gst_element_set_state(GST_ELEMENT(m_pipeline), state);
}

void
EngineGstPipeline::requestState(GstState state)
{
    DEBUG_BLOCK
    debug() << "requested state" << gst_element_state_get_name( state );
    if(m_currentState == state)
    {
        debug() << "not reapplying gst state";
        return;
    }
    // Only abort handling here iff the handler is active.
    if (m_aboutToFinishLock.tryLock()) {
        // Note that this is not condition to unlocking, so the nesting is
        // necessary.
        if (m_handlingAboutToFinish) {
            qDebug() << "Aborting aboutToFinish handling.";
            m_skipGapless = true;
            m_aboutToFinishWait.wakeAll();
        }
        m_aboutToFinishLock.unlock();
    }
    gst_element_set_state(GST_ELEMENT(m_pipeline), state);
}

void
EngineGstPipeline::handleStreamChange()
{
    DEBUG_BLOCK;
    debug() << m_waitingForPreviousSource;
    if (m_waitingForPreviousSource) {
        m_waitingForPreviousSource = false;
    } else {
        m_waitingForNextSource = false;
        Q_EMIT metaDataChanged();
        Q_EMIT currentSourceChanged(currentSource());
        Q_EMIT streamChanged();
    }
}

void
EngineGstPipeline::handleEndOfStream()
{
    DEBUG_BLOCK;
    if (!m_skippingEOS) {
        debug() << "not skipping EOS";
        m_doingEOS = true;
        { // When working on EOS we do not want signals emitted to avoid bogus UI updates.
            Q_EMIT stateChanged(m_currentState, GST_STATE_READY);
            m_aboutToFinishWait.wakeAll();
            Q_EMIT finished();
            QMetaObject::invokeMethod(this, "setState", Qt::QueuedConnection, Q_ARG(GstState, GST_STATE_READY));

        }
        m_doingEOS = false;
    } else {
        debug() << "skipping EOS";
        GstState state = m_currentState;
        setState(GST_STATE_READY);
        setState(state);
        m_skippingEOS = false;
    }
}

bool
EngineGstPipeline::seekToMSec(qint64 time)
{
    if (m_waitingForNextSource) {
        debug() << "Seeking back within old source";
        m_waitingForNextSource = false;
        m_waitingForPreviousSource = true;
        setSource(m_currentSource, true);
    }

    m_posAtReset = time;
    if (m_resetting) {
        return true;
    }
    if (state() == GST_STATE_PLAYING) {
        m_seeking = true;
    }
    return gst_element_seek(GST_ELEMENT(m_pipeline), 1.0, GST_FORMAT_TIME,
                     GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET,
                     time * GST_MSECOND, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
}

void
EngineGstPipeline::stop()
{
    requestState( GST_STATE_READY );
}

void
EngineGstPipeline::play()
{
    requestState( GST_STATE_PLAYING );
}

void
EngineGstPipeline::pause()
{
    requestState( GST_STATE_PAUSED );
}

bool
EngineGstPipeline::isSeekable() const
{
    gboolean seekable = 0;
    GstQuery *query;
    gboolean result;
    gint64 start, stop;
    query = gst_query_new_seeking(GST_FORMAT_TIME);
    result = gst_element_query(GST_ELEMENT(m_pipeline), query);
    if (result) {
        GstFormat format;
        gst_query_parse_seeking(query, &format, &seekable, &start, &stop);
    } else {
        //TODO: Log failure
    }
    gst_query_unref(query);
    return seekable;
}

void
EngineGstPipeline::enqueuePlayback( const QUrl &source )
{
    m_playbackQueue.enqueue( source );
}

void
EngineGstPipeline::clearPlaybackQueue()
{
    m_playbackQueue.clear();
}

bool
EngineGstPipeline::isPlaybackQueueEmpty()
{
    return m_playbackQueue.isEmpty();
}

int
EngineGstPipeline::playbackQueueLength()
{
    return m_playbackQueue.length();
}

qint64
EngineGstPipeline::remainingTime() const
{
    return totalDuration() - position();
}

void
EngineGstPipeline::handleAboutToFinish()
{
    DEBUG_BLOCK
    debug() << "About to finish";

    if(m_playbackQueue.isEmpty() || !m_waitingForNextSource)
    {
        debug() << "need more queue";
        Q_EMIT aboutToFinish();
        if(m_playbackQueue.isEmpty())
            return;
        debug() << "playback queue ok";
    }
    m_aboutToFinishLock.lock();
    m_handlingAboutToFinish = true;
    setNextSource(m_playbackQueue.dequeue());

    // As our signal gets emitted queued we need to wait here until either a
    // new source or a timeout is reached.
    // If we got a new source in time -> hooray + gapless
    // If we did not get a new source in time -> boooh + stop()
    if (!m_skipGapless) {
        // Dynamic lock timeout is our friend.
        // Instead of simply waiting for a fixed amount of ms for the next source, we wait for the
        // most sensible amount of time. This is whatever amount of time is remaining to play
        // minus a 0.5 seconds safety delta (time values not precise etc.).
        // A source for which we have no time or for which the remaining time is < 0.5 seconds is
        // immediately unlocked again. Otherwise the consumer has as much time as gst gave us to
        // set a new source.
        // This in particular prevents pointless excessive locking on sources which have a totalTime
        // < whatever fixed lock value we have (so that we'd lock longer than what we are playing).
        // An issue apparent with notification-like sounds, that are rather short and do not need
        // gapless transitioning. As outlined in https://bugs.kde.org/show_bug.cgi?id=307530
        unsigned long timeout = 0;
        debug() << "total time" << totalDuration();
        debug() << "current time" << position();
        debug() << "remaining time" << remainingTime();
        if (totalDuration() <= 0 || (remainingTime() - 500 <= 0)) {
            timeout = 0;
        } else {
            timeout = remainingTime() - 500;
        }

        debug() << "waiting for" << timeout;
        if (m_aboutToFinishWait.wait(&m_aboutToFinishLock, timeout)) {
            debug() << "Finally got a source";
            if (m_skipGapless) { // Was explicitly set by stateChange interrupt
                debug() << "...oh, no, just got aborted, skipping EOS";
                m_skippingEOS = false;
            }
        } else {
            debug() << "aboutToFinishWait timed out!";
            m_skippingEOS = false;
        }
    } else {
        debug() << "Skipping gapless audio";
        m_skippingEOS = false;
    }
    m_handlingAboutToFinish = false;
    m_aboutToFinishLock.unlock();
}

QStringList
EngineGstPipeline::availableMimeTypes()
{
    QStringList availableMimeTypes;

    GstElementFactory *mpegFactory;
    // Add mp3 as a separate mime type as people are likely to look for it.
    if ((mpegFactory = gst_element_factory_find("ffmpeg")) ||
        (mpegFactory = gst_element_factory_find("mad")) ||
        (mpegFactory = gst_element_factory_find("flump3dec"))) {
          availableMimeTypes << QLatin1String("audio/x-mp3");
          availableMimeTypes << QLatin1String("audio/x-ape");// ape is available from ffmpeg
          gst_object_unref(mpegFactory);
    }

    // Iterate over all audio and video decoders and extract mime types from sink caps
    GList* factoryList;
    factoryList = gst_registry_get_feature_list(gst_registry_get(), GST_TYPE_ELEMENT_FACTORY);
    for (GList* iter = g_list_first(factoryList) ; iter != nullptr ; iter = g_list_next(iter)) {
        GstPluginFeature *feature = GST_PLUGIN_FEATURE(iter->data);
        QString klass = QLatin1String( gst_element_factory_get_klass(GST_ELEMENT_FACTORY(feature)) );

        if (klass == QLatin1String("Codec/Decoder") ||
            klass == QLatin1String("Codec/Decoder/Audio") ||
            klass == QLatin1String("Codec/Decoder/Video") ||
            klass == QLatin1String("Codec/Demuxer") ||
            klass == QLatin1String("Codec/Demuxer/Audio") ||
            klass == QLatin1String("Codec/Demuxer/Video") ||
            klass == QLatin1String("Codec/Parser") ||
            klass == QLatin1String("Codec/Parser/Audio") ||
            klass == QLatin1String("Codec/Parser/Video")) {

            const GList *static_templates;
            GstElementFactory *factory = GST_ELEMENT_FACTORY(feature);
            static_templates = gst_element_factory_get_static_pad_templates(factory);

            for (; static_templates != nullptr ; static_templates = static_templates->next) {
                GstStaticPadTemplate *padTemplate = (GstStaticPadTemplate *) static_templates->data;
                if (padTemplate && padTemplate->direction == GST_PAD_SINK) {
                    GstCaps *caps = gst_static_pad_template_get_caps(padTemplate);

                    if (caps) {
                        for (unsigned int struct_idx = 0; struct_idx < gst_caps_get_size(caps); struct_idx++) {
                            const GstStructure* capsStruct = gst_caps_get_structure(caps, struct_idx);
                            QString mime = QString::fromUtf8(gst_structure_get_name(capsStruct));
                            if (!availableMimeTypes.contains(mime)) {
                                availableMimeTypes.append(mime);
                            }
                        }
                        gst_caps_unref(caps);
                    }
                }
            }
        }
    }
    gst_plugin_feature_list_free(factoryList);

    if (availableMimeTypes.contains(QLatin1String("audio/x-vorbis")) && availableMimeTypes.contains(QLatin1String("application/x-ogm-audio"))) {
        if (!availableMimeTypes.contains(QLatin1String("audio/x-vorbis+ogg"))) {
            availableMimeTypes.append(QLatin1String("audio/x-vorbis+ogg"));
        }
        if (!availableMimeTypes.contains(QLatin1String("application/ogg"))) { /* *.ogg */
            availableMimeTypes.append(QLatin1String("application/ogg"));
        }
        if (!availableMimeTypes.contains(QLatin1String("audio/ogg"))) { /* *.oga */
            availableMimeTypes.append(QLatin1String("audio/ogg"));
        }
    }
    availableMimeTypes.sort();
    return availableMimeTypes;
}

//from phonon-gstreamer AudioDataOutput
inline void EngineGstPipeline::analyzerConvertAndEmit(bool isEndOfMedia)
{
    QMap<int, QVector<qint16> > map;

    for (int i = 0 ; i < m_channels ; ++i) {
        map.insert(i, m_analyzerChannelBuffers[i]);
        Q_ASSERT(i == 0 || m_analyzerChannelBuffers[i - 1].size() == m_analyzerChannelBuffers[i].size());
    }

    if (isEndOfMedia) {
        Q_EMIT analyzerEndOfMedia(m_analyzerChannelBuffers[0].size());
    }
    Q_EMIT analyzerDataReady(map);


    for (int j = 0 ; j < m_channels ; ++j) {
        // QVector::resize doesn't reallocate the buffer
        m_analyzerChannelBuffers[j].resize(0);
    }
}

void EngineGstPipeline::analyzerFlushPendingData()
{
    if (m_analyzerPendingData.size() == 0) {
        return;
    }

    // Since m_analyzerPendingData is a concatenation of buffers it must share its
    // attribute of being a multiple of channelCount
    Q_ASSERT((m_analyzerPendingData.size() % m_channels) == 0);
    for (int i = 0; i < m_analyzerPendingData.size(); i += m_channels) {
        for (int j = 0; j < m_channels; ++j) {
            m_analyzerChannelBuffers[j].append(m_analyzerPendingData[i + j]);
        }
    }

    m_analyzerPendingData.resize(0);
}

void EngineGstPipeline::analyzerProcessBuffer(GstElement*, GstBuffer* buffer, GstPad* pad, gpointer gThat)
{
    // TODO emit endOfMedia
    EngineGstPipeline *that = static_cast<EngineGstPipeline *>(gThat);

    // Copied locally to avoid multithead problems
    qint32 dataSize = that->m_analyzerDataSize;
    if (dataSize == 0) {
        return;
    }

    int channelsCount = 0;

    // determine the number of channels
    GstCaps *caps = gst_pad_get_current_caps(GST_PAD(pad));
    GstStructure *structure = gst_caps_get_structure(caps, 0);
    gst_structure_get_int(structure, "channels", &channelsCount);
    gst_caps_unref(caps);

    // Channels count have changed, so emit the pending data that have the old
    // channels count, before we fill the buffer with new pending data
    if (that->m_analyzerPendingData.size() > 0 && channelsCount != that->m_channels) {
        const bool isEndOfMedia = (that->m_analyzerPendingData.size() / that->m_channels) == dataSize;
        that->analyzerFlushPendingData();
        that->analyzerConvertAndEmit(isEndOfMedia);
    }

    // Now update the channels count
    that->m_channels = channelsCount;

    // Let's get the buffers
    gint16 *gstBufferData;
    guint gstBufferSize;
    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_READ);
    gstBufferData = reinterpret_cast<gint16 *>(info.data);
    gstBufferSize = info.size / sizeof(gint16);
    gst_buffer_unmap(buffer,&info);

    if (gstBufferSize == 0) {
        qWarning() << Q_FUNC_INFO << ": received a buffer of 0 size ... doing nothing";
        return;
    }

    if ((gstBufferSize % that->m_channels) != 0) {
        qWarning() << Q_FUNC_INFO << ": corrupted data";
        return;
    }

    if (that->m_analyzerPendingData.capacity() != dataSize) {
        that->m_analyzerPendingData.reserve(dataSize);
    }

    // I set the number of channels
    if (that->m_analyzerChannelBuffers.size() != that->m_channels) {
        that->m_analyzerChannelBuffers.resize(that->m_channels);
    }

    // check how many emits I will perform
    int nBlockToSend = (that->m_analyzerPendingData.size() + gstBufferSize) / (dataSize * that->m_channels);

    if (nBlockToSend == 0) { // add data to pending buffer
        const int prevPendingSize = that->m_analyzerPendingData.size();
        for (quint32 i = 0; i < gstBufferSize ; ++i) {
            that->m_analyzerPendingData.append(gstBufferData[i]);
        }
        Q_ASSERT(int(prevPendingSize + gstBufferSize) == that->m_analyzerPendingData.size());
        return;
    }

    // SENDING DATA

    // 1) I write pending data to buffers
    that->analyzerFlushPendingData();

    // 2) I fill with fresh data and send
    for (int i = 0 ; i < that->m_channels ; ++i) {
        if (that->m_analyzerChannelBuffers[i].capacity() != dataSize) {
            that->m_analyzerChannelBuffers[i].reserve(dataSize);
        }
    }

    quint32 bufferPosition = 0;
    for (int i = 0 ; i < nBlockToSend ; ++i) {
        for ( ; (that->m_analyzerChannelBuffers[0].size() < dataSize) && (bufferPosition < gstBufferSize) ; bufferPosition += that->m_channels ) {
            for (int j = 0 ; j < that->m_channels ; ++j) {
                that->m_analyzerChannelBuffers[j].append(gstBufferData[bufferPosition+j]);
            }
        }

        that->analyzerConvertAndEmit(false);
    }

    // 3) I store the rest of data
    while (bufferPosition < gstBufferSize) {
        that->m_analyzerPendingData.append(gstBufferData[bufferPosition]);
        ++bufferPosition;
    }
}
