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

        long                                     length() const                      { return 0; }
        long                                     position() const;
        EngineBase::EngineState                  state() const;
        bool                                     isStream() const;
        std::vector<float>*                      scope();
        
        QStringList                              availableEffects() const            { return QStringList(); }
        std::vector<long>                        activeEffects() const               { return std::vector<long>(); }
        QString                                  effectNameForId( long id ) const    { return QString(); }
        bool                                     effectConfigurable( long id ) const { return false; }
        long                                     createEffect( const QString& name ) { return 0; }
        void                                     removeEffect( long id ) {}
        void                                     configureEffect( long id ) {}
        
        bool                                     decoderConfigurable()               { return false; }

    public slots:
        const QObject*                           play( const KURL& );
        void                                     play();
        void                                     stop();
        void                                     pause();
        void                                     seek( long ms );
        void                                     setVolume( int percent );
        void                                     configureDecoder() {};                                                   

    private:
        static void                              eos_cb( GstElement *typefind, GstElement *pipeline );
        static void                              handoff_cb( GstElement *identity, GstBuffer *buf, GstElement *pipeline );
        static void                              typefindError_cb( GstElement *typefind, GstElement *pipeline );
        static void                              typefindFound_cb( GstElement *typefind, GstCaps *caps, GstElement *pipeline );

        void                                     interpolate( const vector<float>& inVec, vector<float>& outVec );
        /////////////////////////////////////////////////////////////////////////////////////
        // ATTRIBUTES
        /////////////////////////////////////////////////////////////////////////////////////
        static GstEngine*                        pObject;
        
        GstElement*                              m_pThread;
        GstElement*                              m_pAudiosink;
        GstElement*                              m_pSpider;
        GstElement*                              m_pFilesrc;
        GstElement*                              m_pVolume;

        vector<float>                            m_scopeBuf;
        int                                      m_scopeBufIndex;
        int                                      m_scopeSize;
       
        bool                                     m_typefindResult;
};


#endif /*AMAROK_GSTENGINE_H*/

