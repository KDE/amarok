// Copyright (c) Max Howell 2004
// Licensed under GPL v2
// "lack of pussy makes you brave!"

#ifndef NMM_ENGINE_H
#define NMM_ENGINE_H

#include <config.h>
#ifdef HAVE_NMM

#include "enginebase.h"
#include <nmm/NMMTypes.hpp>

namespace NMM { class MP3ReadNode; }

class NmmEngine : public EngineBase
{
    Q_OBJECT

    public:

        //this is the receiver for progress events
        NMM::Result setProgress( u_int64_t&, u_int64_t& );
        NMM::Result endTrack();

                                                 NmmEngine();
                                                 ~NmmEngine();

        bool                                     initMixer( bool hardware );
        bool                                     canDecode( const KURL &url, mode_t mode, mode_t permissions );
        long                                     length() const;
        long                                     position() const;
        EngineBase::EngineState                  state() const;
        bool                                     isStream() const;

        std::vector<float>*                      scope();

        QStringList                              availableEffects() const;
        std::vector<long>                        activeEffects() const;
        QString                                  effectNameForId( long id ) const;
        bool                                     effectConfigurable( long id ) const;
        long                                     createEffect( const QString& name );
        void                                     removeEffect( long id );
        void                                     configureEffect( long id );

        bool                                     decoderConfigurable();

    public slots:
        const QObject*                           play( const KURL& );
        void                                     play();
        void                                     stop();
        void                                     pause();

        void                                     seek( long ms );
        void                                     setVolume( int percent );
        void                                     configureDecoder();

    private:
        void                                     startXfade();
        void                                     timerEvent( QTimerEvent* );

        void                                     loadEffects();
        void                                     saveEffects();

        double m_progress;
        bool   m_firstTime; //FIXME I HATE BOOLS LIKE THESE!
        EngineBase::EngineState m_state;
};

#endif
#endif
