/***************************************************************************
          masengine.h  -  Media Application Server (MAS) audio interface
                         -------------------
begin                : Jul 04 2004
copyright            : (C) 2004 by Roland Gigler
email                : rolandg@web.de
what                 : interface to the Media Application Server (MAS)
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_MASENGINE_H
#define AMAROK_MASENGINE_H

#include "enginebase.h"

//#include <vector>

#ifdef __cplusplus
extern "C" {
#endif
  #include <mas/mas.h>
  #include <mas/mas_core.h>
#ifdef __cplusplus
}
#endif


class QTimer;
class KURL;

class MasEngine : public Engine::Base
{
    Q_OBJECT

    public:
        MasEngine();
        ~MasEngine();

        bool init();

        bool initMixer( bool hardware );
        bool canDecode( const KURL& ) const;
        uint position() const;
        uint length() const;
        Engine::State state() const {return m_state;}
/*
        const Engine::Scope& scope();
        bool decoderConfigurable();
        void configureDecoder();
 */
        bool supportsXFade() const     { return false; }

    public slots:
        bool load( const KURL&, bool stream );
        bool play( unsigned int offset = 0);
        void stop();
        void pause();

        void seek( unsigned int ms );
        void setVolumeSW( unsigned int percent );
    private slots:
        void playingTimeout();

    private:
        //void startXfade();
        //void timerEvent( QTimerEvent* );
        bool masinit();
        /////////////////////////////////////////////////////////////////////////////////////
        // ATTRIBUTES
        /////////////////////////////////////////////////////////////////////////////////////
        static const int MAS_TIMER  = 250;   //ms
        //static const int TIMEOUT    = 4000;  //ms FIXME make option?
        bool m_inited;
        uint m_lastKnownPosition;

        Engine::State m_state;
        //long m_scopeId;
        //int  m_scopeSize;
        //bool m_xfadeFadeout;
        //float m_xfadeValue;
        //QString m_xfadeCurrent;
        QTimer* m_pPlayingTimer;

        KURL m_url;

        mas_device_t m_mp1a_source_device;
        mas_device_t m_visual;
        mas_device_t m_sbuf;
        mas_device_t m_codec;
        mas_device_t m_mix_device;
        mas_port_t m_mix_sink;
        mas_device_t m_sink_mc;
        mas_device_t m_source_mc;
        int32 m_sink_clkid;
        //int32 m_source_clkid;
        //double m_measured_sample_freq;
};

#endif     // AMAROK_MASENGINE_H

