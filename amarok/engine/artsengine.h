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

#include "../amarokarts/amarokarts.h"
#include "enginebase.h"

#include <vector>
#include <qobject.h>
#include <arts/soundserver.h>

class QStringList;
class QTimerEvent;

class KArtsDispatcher;
class KURL;

namespace KDE { class PlayObject; };


class ArtsEngine : public EngineBase
{
        Q_OBJECT

    public:
                                                 ArtsEngine( bool& restart, int scopeSize );
                                                 ~ArtsEngine();

        bool                                     initMixer( bool software );
        bool                                     canDecode( const KURL &url );
        long                                     position() const;
        EngineBase::EngineState                  state() const;
        bool                                     isStream() const;

        std::vector<float>*                      scope();
        QStringList                              availableEffects() const;        
        bool                                     effectConfigurable( const QString& name ) const;        
        
    public slots:
        void                                     open( KURL );
        void                                     play();
        void                                     stop();
        void                                     pause();

        void                                     seek( long ms );
        void                                     setVolume( int percent );

    private:
        void                                     enableScope();
        void                                     disableScope();
        void                                     stopCurrent();
        void                                     startXfade();
        void                                     stopXfade();
        void                                     switchXfade();
        void                                     timerEvent( QTimerEvent* );
    
        /////////////////////////////////////////////////////////////////////////////////////
        // ATTRIBUTES
        /////////////////////////////////////////////////////////////////////////////////////
        KArtsDispatcher*                         m_pArtsDispatcher;
        KDE::PlayObject*                         m_pPlayObject;
        KDE::PlayObject*                         m_pPlayObjectXfade;
        Arts::SoundServerV2                      m_server;
        Arts::StereoEffectStack                  m_globalEffectStack;
        Arts::StereoEffectStack                  m_effectStack;
        Arts::StereoVolumeControl                m_volumeControl;
        Arts::Synth_AMAN_PLAY                    m_amanPlay;
        Amarok::RawScope                         m_scope;
        Amarok::Synth_STEREO_XFADE               m_xfade;
               
        long                                     m_scopeId;
        int                                      m_scopeSize;
        long                                     m_volumeId;
        bool                                     m_proxyError;

        bool                                     m_xfadeRunning;
        float                                    m_xfadeValue;
        QString                                  m_xfadeCurrent;
            
    private slots:
        void                                     connectPlayObject();
        void                                     proxyError();
        void                                     receiveStreamMeta( QString title, QString url, QString kbps );
};

#endif
