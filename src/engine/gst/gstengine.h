/***************************************************************************
                      gstengine.h  -  GStreamer audio interface
                         -------------------
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

#include "enginebase.h"

#include <vector>

#include <qobject.h>
#include <qstringlist.h>

#include <gst/gst.h>

using std::vector;

class KURL;

class GstEngine : public EngineBase
{
        Q_OBJECT

    public:
                                                 GstEngine();
                                                 ~GstEngine();

        void                                     init( bool& restart, int scopeSize, bool restoreEffects ); 
                                                                                                  
        bool                                     initMixer( bool hardware );
        bool                                     canDecode( const KURL &url, mode_t mode, mode_t permissions );
        int                                      streamingMode() { return 0; }
        QStringList                              getOutputsList() { return getPluginList( "Sink/Audio" ); }

        long                                     length() const { return 0; }
        long                                     position() const;
        EngineBase::EngineState                  state() const;
        bool                                     isStream() const;
        std::vector<float>*                      scope();

    public slots:
        const QObject*                           play( const KURL& );
        void                                     play();
        void                                     stop();
        void                                     pause();
        void                                     seek( long ms );
        void                                     setVolume( int percent );
        void                                     newStreamData( char* data, int size );

    private:
        static void                              eos_cb( GstElement*, GstElement* );
        static void                              handoff_cb( GstElement*, GstBuffer*, gpointer );
        static void                              handoff_fakesrc_cb( GstElement*, GstBuffer*, GstPad, gpointer );
        static void                              typefindFound_cb( GstElement*, GstCaps*, GstElement* );

        /** Get a list of available plugins from a specified Class */
        QStringList                              getPluginList( const QCString& classname );
        
        void                                     cleanPipeline();
        void                                     interpolate( const vector<float>& inVec, vector<float>& outVec );
        /////////////////////////////////////////////////////////////////////////////////////
        // ATTRIBUTES
        /////////////////////////////////////////////////////////////////////////////////////
        static GstEngine*                        self;
        
        GstElement*                              m_pThread;
        GstElement*                              m_pAudiosink;
        GstElement*                              m_pSpider;
        GstElement*                              m_pFilesrc;
        GstElement*                              m_pIdentity;
        GstElement*                              m_pVolume;

        vector<float>                            m_scopeBuf;
        uint                                     m_scopeBufIndex;
        uint                                     m_scopeSize;
       
        vector<char>                             m_streamBuf;
        uint                                     m_streamBufIn;
        uint                                     m_streamBufOut;
        bool                                     m_playFlag;
        
        bool                                     m_typefindResult;
        bool                                     m_pipelineFilled;
};


#endif /*AMAROK_GSTENGINE_H*/

