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

#include <qstringlist.h>
#include <kio/jobclasses.h>

#include <gst/gst.h>

using std::vector;

class QTimerEvent;
class KURL;

class GstEngine : public Engine::Base
{
        friend class GstConfigDialog;        
        
        Q_OBJECT

    public:
                                                 GstEngine();
                                                 ~GstEngine();

        bool                                     init(); 
                                                                                                  
        bool                                     canDecode( const KURL &url );
        uint                                     position() const;
        Engine::State                            state() const;
        const Engine::Scope&                     scope();

        amaroK::PluginConfig*                    configure() const;
        
    public slots:
        bool                                     load( const KURL&, bool stream );
        bool                                     play( uint offset );
        void                                     stop();
        void                                     pause();
        void                                     seek( uint ms );
        void                                     setVolumeSW( uint percent );
        void                                     newStreamData( char* data, int size );

    protected:
        void                                     timerEvent( QTimerEvent* );
        
    private slots:
        void                                     handleError();
        void                                     stopAtEnd();
        void                                     kioFinished();
        void                                     newKioData( KIO::Job*, const QByteArray& array );
        void                                     errorNoOutput() const;
        
    private:
        static GstEngine*                        instance() { return s_instance; }
        
        QStringList                              getOutputsList() { return getPluginList( "Sink/Audio" ); }
        
        /** Called at end of track */
        static void                              eos_cb( GstElement*, GstElement* );
        /** Duplicates audio data for application side processing */
        static void                              handoff_cb( GstElement*, GstBuffer*, gpointer );
        
        static void                              error_cb( GstElement*, GstElement*, GError*, gchar*, gpointer );
        static void                              kio_resume_cb();

        /** Get a list of available plugins from a specified Class */
        QStringList                              getPluginList( const QCString& classname );
        
        GstElement*                              createElement( const QCString& factoryName, GstElement* bin = 0, const QCString& name = 0 );
        void                                     stopNow();
        void                                     cleanPipeline();
        void                                     interpolate( const Engine::Scope& inVec, Engine::Scope& outVec );
        
        /////////////////////////////////////////////////////////////////////////////////////
        // ATTRIBUTES
        /////////////////////////////////////////////////////////////////////////////////////
        static const int                         TIMER_INTERVAL = 100; //msec
        static GError*                           error_msg;
        static GstEngine*                        s_instance;
        
        GstElement*                              m_gst_thread;
        GstElement*                              m_gst_src;
        GstElement*                              m_gst_audiosink;
        GstElement*                              m_gst_spider;
        GstElement*                              m_gst_identity;
        GstElement*                              m_gst_tee;
        GstElement*                              m_gst_volume;
        GstElement*                              m_gst_volumeFade;
        GstElement*                              m_gst_audioconvert1;
        GstElement*                              m_gst_audioconvert2;
        GstElement*                              m_gst_audioscale;
        GstElement*                              m_gst_bufferconvert;
        
        Engine::Scope                            m_scopeBuf;
        uint                                     m_scopeBufIndex;
       
        char*                                    m_streamBuf;
        int                                      m_streamBufIndex;
        bool                                     m_streamBufStop;
        KIO::TransferJob*                        m_transferJob;
        
        float                                    m_fadeValue;
        
        bool                                     m_pipelineFilled;
};


#endif /*AMAROK_GSTENGINE_H*/

