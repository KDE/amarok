/***************************************************************************
                      artsengine.h  -  aRts audio interface
                         -------------------
begin                : Dec 31 2003
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

#ifndef AMAROK_ARTSENGINE_H
#define AMAROK_ARTSENGINE_H

#include "amarokarts.h"
#include "enginebase.h"

#include <vector>

#include <qguardedptr.h>
#include <qmap.h>
#include <qstringlist.h>
#include <qwidget.h>

#include <arts/artsgui.h>
#include <arts/soundserver.h>

class QStringList;
class QTimer;
class QTimerEvent;

class KArtsDispatcher;
class KArtsWidget;
class KURL;


namespace KDE { class PlayObject; }


class ArtsEngine : public Engine::Base
{
    Q_OBJECT

    public:
                                                 ArtsEngine();
                                                 ~ArtsEngine();

        bool                                     init();

        bool                                     canDecode( const KURL &url ) const;
        uint                                     position() const;
        uint                                     length() const;
        Engine::State                            state() const;
        const Engine::Scope&                     scope();

        bool                                     decoderConfigurable();

    public slots:
        bool                                     load( const KURL&, bool stream );
        bool                                     play( uint offset = 0 );
        void                                     stop();
        void                                     pause();

        void                                     seek( uint ms );
        void                                     configureDecoder();

    protected:
        void                                     setVolumeSW( uint percent );

    private slots:
        void                                     connectPlayObject();
        void                                     connectTimeout();

    private:
        void                                     startXfade();
        void                                     timerEvent( QTimerEvent* );

        /////////////////////////////////////////////////////////////////////////////////////
        // ATTRIBUTES
        /////////////////////////////////////////////////////////////////////////////////////
        static const int                         ARTS_TIMER = 100;   //ms
        static const int                         TIMEOUT    = 4000;  //ms FIXME make option?
        static const uint                        SCOPE_SIZE = 512;

        KArtsDispatcher*                         m_artsDispatcher;
        KDE::PlayObject*                         m_playObject;
        KDE::PlayObject*                         m_playObjectXfade;
        Arts::SoundServerV2                      m_server;
        Arts::StereoEffectStack                  m_globalEffectStack;
        Arts::StereoEffectStack                  m_effectStack;
        Arts::StereoVolumeControl                m_volumeControl;
        Arts::Synth_AMAN_PLAY                    m_amanPlay;
        Amarok::RawScope                         m_scope;
        Amarok::Synth_STEREO_XFADE               m_xfade;

        long                                     m_scopeId;
        long                                     m_volumeId;
//         QMap<long, EffectContainer>              m_effectMap;

        bool                                     m_xfadeFadeout;
        float                                    m_xfadeValue;
        QString                                  m_xfadeCurrent;
//         QGuardedPtr<ArtsConfigWidget>            m_pDecoderConfigWidget;
        QTimer*                                  m_connectTimer;
};


#endif

