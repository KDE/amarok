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

#include "enginebase.h"

#include <vector>
#include <qobject.h>
#include <arts/soundserver.h>

namespace KDE
{
    class PlayObject;
};

class KArtsDispatcher;
class QStringList;
class KURL;

class ArtsEngine : public EngineBase
{
        Q_OBJECT

    public:
                                                 ArtsEngine( bool& restart );
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

    private slots:
        void                                     connectPlayObject();

    private:
        void                                     enableScope();
        void                                     disableScope();
        /////////////////////////////////////////////////////////////////////////////////////
        // ATTRIBUTES
        /////////////////////////////////////////////////////////////////////////////////////
        KArtsDispatcher*                         m_pArtsDispatcher;
        KDE::PlayObject*                         m_pPlayObject;
        Arts::SoundServerV2                      m_server;
        Arts::StereoFFTScope                     m_scope;
        Arts::StereoEffectStack                  m_globalEffectStack;
        Arts::StereoEffectStack                  m_effectStack;
        Arts::StereoVolumeControl                m_volumeControl;
        Arts::Synth_AMAN_PLAY                    m_amanPlay;
//         Amarok::Synth_STEREO_XFADE               m_XFade;
        
       long                                      m_scopeId;
};

#endif
