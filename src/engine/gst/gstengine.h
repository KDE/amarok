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
#include <qstringlist.h>
#include <kio/jobclasses.h>

#include <gst/gst.h>
#include <adapter.h>

using std::vector;

class QTimerEvent;
class KURL;

class GstEngine : public Engine::Base
{
        friend class GstConfigDialog;

        Q_OBJECT

    signals:
        void sigGstError( GError*, gchar* );
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
        void handleGstError( GError*, gchar* );
        void endOfStreamReached();
        void kioFinished();
        void newKioData( KIO::Job*, const QByteArray& array );
        void errorNoOutput() const;
        
    private:
        static GstEngine* instance() { return s_instance; }

        QStringList getOutputsList() { return getPluginList( "Sink/Audio" ); }

        /** Called at end of track */
        static void eos_cb( GstElement*, GstElement* );
        /** Duplicates audio data for application side processing */
        static void handoff_cb( GstElement*, GstBuffer*, gpointer );
        
        static void error_cb( GstElement*, GstElement*, GError*, gchar*, gpointer );
        static void kio_resume_cb();
        static void shutdown_cb();

        /** Get a list of available plugins from a specified Class */
        QStringList getPluginList( const QCString& classname ) const; 

        /**
         * Creates a GStreamer element and puts it into pipeline. 
         * @param factoryName Name of the element class to create.
         * @param bin Container into which the element is put.
         * @param name Identifier for the element.
         * @return Pointer to the created element, or NULL for failure.
         */
        GstElement* createElement( const QCString& factoryName, GstElement* bin = 0, const QCString& name = 0 ) const;
        
        /** Stops playback, deletes the current pipeline and frees ressources */
        void destroyPipeline();

        /////////////////////////////////////////////////////////////////////////////////////
        // ATTRIBUTES
        /////////////////////////////////////////////////////////////////////////////////////
        static const int TIMER_INTERVAL = 100; //msec
        static GError* error_msg;
        static GstEngine* s_instance;

        GstElement* m_gst_thread;
        GstElement* m_gst_src;
        GstElement* m_gst_audiosink;
        GstElement* m_gst_spider;
        GstElement* m_gst_identity;
        GstElement* m_gst_volume;
        GstElement* m_gst_volumeFade;
        GstElement* m_gst_audioconvert;
        GstElement* m_gst_audioscale;

        GstAdapter* m_gst_adapter;

        char* m_streamBuf;
        int m_streamBufIndex;
        bool m_streamBufStop;
        KIO::TransferJob* m_transferJob;
        QMutex m_mutexScope;
        
        float m_fadeValue;

        bool m_pipelineFilled;
        bool m_shutdown;
};


#endif /*AMAROK_GSTENGINE_H*/

