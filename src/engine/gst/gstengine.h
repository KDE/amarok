/***************************************************************************
                      gstengine.h - GStreamer audio interface

begin                : Jan 02 2003
copyright            : (C) 2003 by Mark Kretschmann
email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_GSTENGINE_H
#define AMAROK_GSTENGINE_H

#include "gstconfigdialog.h"
#include "enginebase.h"

#include <vector>

#include <qmutex.h>
#include <qptrlist.h>
#include <qstringlist.h>

#include <kio/jobclasses.h>

#include <gst/gst.h>
#include <adapter.h>

using std::vector;

class QTimerEvent;
class KURL;

class InputPipeline;

class GstEngine : public Engine::Base
{
        friend class GstConfigDialog;
        friend class InputPipeline;

        Q_OBJECT

    signals:
        void sigScopeData( GstBuffer* );

    public:
        GstEngine();
        ~GstEngine();

        bool init();

        bool canDecode( const KURL &url ) const;
        uint position() const;
        Engine::State state() const;
        const Engine::Scope& scope();

        amaroK::PluginConfig* configure() const;

    public slots:
        bool load( const KURL&, bool stream );
        bool play( uint offset );
        void stop();
        void pause();
        void seek( uint ms );
        void newStreamData( char* data, int size );

    protected:
        void setVolumeSW( uint percent );
        void timerEvent( QTimerEvent* );

    private slots:
        void handleGstError();
        void endOfStreamReached();
        void kioFinished();
        void newKioData( KIO::Job*, const QByteArray& array );
        void errorNoOutput();
        void configChanged();

    private:
        static GstEngine* instance() { return s_instance; }

        /**
         * Creates a GStreamer element and puts it into pipeline.
         * @param factoryName Name of the element class to create.
         * @param bin Container into which the element is put.
         * @param name Identifier for the element.
         * @return Pointer to the created element, or NULL for failure.
         */
        static GstElement* createElement( const QCString& factoryName, GstElement* bin = 0, const QCString& name = 0 );

        QStringList getOutputsList() { return getPluginList( "Sink/Audio" ); }

        /** Called at end of track */
        static void eos_cb( GstElement*, GstElement* );
        /** Duplicates audio data for application side processing */
        static void handoff_cb( GstElement*, GstBuffer*, gpointer );

        static void candecode_handoff_cb( GstElement*, GstBuffer*, gpointer );
        static void error_cb( GstElement*, GstElement*, GError*, gchar*, gpointer );
        static void kio_resume_cb();
        static void shutdown_cb();

        /** Get a list of available plugins from a specified Class */
        QStringList getPluginList( const QCString& classname ) const;

        /** Construct the output pipeline */
        bool createPipeline();

        /** Stops playback, destroys all input pipelines, destroys output pipeline, and frees ressources */
        void destroyPipeline();

        /** Deletes the current input pipeline and frees ressources */
        void destroyInput( InputPipeline* input );

        /** Beams the streaming buffer status to amaroK */
        void sendBufferStatus();

        /////////////////////////////////////////////////////////////////////////////////////
        // ATTRIBUTES
        /////////////////////////////////////////////////////////////////////////////////////
        static const int TIMER_INTERVAL = 70; //msec
        static GError* error_msg;
        static GstEngine* s_instance;

        // Output pipeline
        GstElement* m_gst_thread;
        GstElement* m_gst_adder;
        GstElement* m_gst_identity;
        GstElement* m_gst_volume;
        GstElement* m_gst_audioscale;
        GstElement* m_gst_audioconvert;
        GstElement* m_gst_audiosink;

        GError* m_gst_error;
        gchar* m_gst_debug;

        QPtrList<InputPipeline> m_inputs;
        InputPipeline* m_currentInput;

        GstAdapter* m_gst_adapter;

        char* m_streamBuf;
        int m_streamBufIndex;
        bool m_streamBufStop;
        KIO::TransferJob* m_transferJob;
        QMutex m_mutexScope;

        float m_fadeValue;

        bool m_shutdown;
        mutable bool m_canDecodeSuccess;

        /** Set when an input pipeline has reached EndOfStream */
        bool m_eos;
};


class InputPipeline
{
    public:
        enum State { NO_FADE, NEAR_DEATH, FADE_IN, FADE_OUT, XFADE_IN, XFADE_OUT };

        InputPipeline();
        ~InputPipeline();

        void prepareToDie();

        State state() { return m_state; }
        void setState( State newState );

        float fade() { return m_fade; }
        void setFade( float newFade) { m_fade = newFade; }

        State m_state;
        float m_fade;

        bool m_error;
        bool m_eos;

        int m_killCounter;

        GstElement* thread;
        GstElement* src;
        GstElement* spider;
        GstElement* volume;
        GstElement* queue;
};


#endif /*AMAROK_GSTENGINE_H*/

