/***************************************************************************
                     enginebase.h  -  audio engine base class
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

#ifndef AMAROK_ENGINEBASE_H
#define AMAROK_ENGINEBASE_H

#include "plugin/plugin.h"

#include <vector>

#include <qobject.h>    //baseclass
#include <qstringlist.h>

#ifdef __FreeBSD__
 #include <sys/types.h>
#endif

class KURL;

class EngineBase : public QObject, public amaroK::Plugin {
        Q_OBJECT

    signals:
        void endOfTrack();
        void stopped();
        
    public:
        enum EngineState { Empty, Idle, Playing, Paused };
        enum StreamingMode { Socket, Signal, NoStreaming };
        
        EngineBase();
        virtual ~EngineBase();

        virtual void init( bool& restart, int scopeSize, bool restoreEffects ) = 0;

        /**
         * Initialize mixer.
         * @param hardware True for soundcard hardware mixing
         * @return True if using hardware mixing
         */
        virtual bool initMixer( bool hardware ) = 0;

        virtual bool canDecode( const KURL &url, mode_t mode, mode_t permissions ) = 0;
        virtual StreamingMode streamingMode() { return NoStreaming; }       
        /** Get list of available output plugins */
        virtual QStringList getOutputsList() { return QStringList(); }

        /**
         * @return Time position in ms
         */
        virtual long position() const = 0;

        virtual EngineState state() const = 0;

        /**
         * @return True if media is loaded, system is ready to play.
         */
        bool loaded() { return state() != Empty; }

        /**
         * Sets the master volume.
         * @return Volume in range 0 to 100.
         */
        inline int volume() const { return m_volume; }

        /**
         * Sets the master volume.
         * @return True if using hardware mixer.
         */
        bool isMixerHardware() const { return m_mixerHW != -1; }

        virtual bool isStream() const = 0;

        /**
         * Determines whether the engine supports crossfading.
         * @return True if crossfading is supported.
         */
        virtual bool supportsXFade() const { return false; }

        /**
         * Fetches the current audio sample buffer.
         * @return Pointer to result of FFT calculation. Must be deleted after use.
         */
        virtual std::vector<float>* scope() { return new std::vector<float>(); }

        void setRestoreEffects( bool yes ) { m_restoreEffects = yes; }
        virtual QStringList availableEffects() const { return QStringList(); }
        virtual std::vector<long> activeEffects() const { return std::vector<long>(); }
        virtual QString effectNameForId( long ) const { return QString::null; }
        virtual bool effectConfigurable( long ) const { return false; }
        virtual long createEffect( const QString& ) { return -1; }
        virtual void removeEffect( long ) { }
        virtual void configureEffect( long ) { }
        virtual bool decoderConfigurable() const { return false; }

        virtual void setXfadeLength( int ms );
        virtual void setSoundOutput( const QString& output );
        virtual void setSoundDevice( const QString& device ) { m_soundDevice = device; }
        virtual void setDefaultSoundDevice( bool isDefault ) { m_defaultSoundDevice = isDefault; }
        virtual void setThreadPriority( int priority ) { m_threadPriority = priority; }
        
    public slots:
        virtual void play( const KURL&, bool stream = false ) = 0;
        virtual void play() = 0;
        virtual void stop() = 0;
        virtual void pause() = 0;

        /**
         * @param percent Set volume in range 0 to 100.
         */
        virtual void setVolume( int percent ) = 0;
        
        virtual void seek( long ms ) = 0;
        virtual void configureDecoder() {}
        virtual void newStreamData( char* /*data*/, int /*size*/ ) {};
        
    protected:
        void closeMixerHW();
        bool initMixerHW();
        void setVolumeHW( int percent );
        /////////////////////////////////////////////////////////////////////////////////////
        // ATTRIBUTES
        /////////////////////////////////////////////////////////////////////////////////////
        int m_mixerHW;
        int m_volume;
        int m_xfadeLength;
        bool m_restoreEffects;
        QString m_soundOutput;
        QString m_soundDevice;
        bool m_defaultSoundDevice;
        int m_threadPriority;
        
        EngineBase( const EngineBase& ); //disable
        const EngineBase &operator=( const EngineBase& ); //disable
};

#endif
