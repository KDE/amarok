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

#include <vector>           //stack allocated

#include <qobject.h>        //baseclass
#include <qstringlist.h>    //stack allocated

#ifdef __FreeBSD__
 #include <sys/types.h>
#endif

class KURL;

class EngineBase : public QObject, public amaroK::Plugin {
        Q_OBJECT

    signals:
        /** Emitted when end of current track is reached. */
        void endOfTrack();
        
        /** Emitted when current track was stopped explicitly. */
        void stopped();
        
    public:
        enum EngineState { Empty, Idle, Playing, Paused };
        enum StreamingMode { Socket, Signal, NoStreaming };
        
        EngineBase();
        virtual ~EngineBase();
        
        /**
         * Initializes the engine. Must be called after the engine was loaded.
         * @param restart True if artsd must be restarted (aRts-Engine only).
         * @param scopeSize Size of buffer for visualization data (in bytes).
         * @param restoreEffects True if last effect configuration should be restored.
         */
        virtual void init( bool& restart, int scopeSize, bool restoreEffects ) = 0;

        /**
         * Initialize mixer.
         * @param hardware True for soundcard hardware mixing.
         * @return True if using hardware mixing.
         */
        virtual bool initMixer( bool hardware ) = 0;

        /**
         * Determines if the engine is able to play a given URL.
         * @param url The URL of the file/stream.
         * @param mode Determined in PlaylistLoader.
         * @param permissions Determined in PlaylistLoader.
         * @return True if we can play the URL.
         */
        virtual bool canDecode( const KURL &url, mode_t mode, mode_t permissions ) = 0;
        
        /**
         * Determines how streaming is handled with this engine.
         * @return The supported streaming mode.
         */
        virtual StreamingMode streamingMode() { return NoStreaming; }       
        
        /** Get list of available output plugins */
        virtual QStringList getOutputsList() { return QStringList(); }

        /** Get Time position (msec). */
        virtual long position() const = 0;

        /** Get current engine status. */
        virtual EngineState state() const = 0;

        /**
         * Determines whether media is currently loaded.
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

        /**
         * Determines if current track is a stream.
         * @return True if track is a stream.
         */
        bool isStream() const { return m_stream; }

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

        /** Sets whether effects configuration should be remembered */
        void setRestoreEffects( bool yes ) { m_restoreEffects = yes; }
        
        /** Gets the list of currently available effects */
        virtual QStringList availableEffects() const { return QStringList(); }
        
        /** Gets the list of currently active effects */
        virtual std::vector<long> activeEffects() const { return std::vector<long>(); }
        
        /** Gets the ID for a given effect */
        virtual QString effectNameForId( long ) const { return QString::null; }
        
        /** Reports whether effect is configurable */
        virtual bool effectConfigurable( long ) const { return false; }
        
        /** Instantiates new effect */
        virtual long createEffect( const QString& ) { return -1; }
        
        /** Removes effect */
        virtual void removeEffect( long ) { }
        
        /** Configures effect */
        virtual void configureEffect( long ) { }
        
        /** Determine whether decoder for current track is GUI-configurable */
        virtual bool decoderConfigurable() const { return false; }

        /** Sets length of Crossfade transition (in msec) */
        virtual void setXfadeLength( int msec );
        
        /** Sets sound output plugin (GST-Engine only). */
        virtual void setSoundOutput( const QString& output );
        
        /** Sets sound device of output plugin (GST-Engine only). */
        virtual void setSoundDevice( const QString& device ) { m_soundDevice = device; }
        
        /** Sets whether default sound device should be used (GST-Engine only). */
        virtual void setDefaultSoundDevice( bool isDefault ) { m_defaultSoundDevice = isDefault; }
        
        /** Sets the threading priority (GST-Engine only). */
        virtual void setThreadPriority( int priority ) { m_threadPriority = priority; }
        
    public slots:
        /** 
         * Sets new URL for playing.
         * @param url URL to be played.
         * @param stream True if URL is a stream.
         */
        virtual void play( const KURL& url, bool stream = false ) = 0;
        
        /** Starts playback */
        virtual void play() = 0;
        
        /** Stops playback */
        virtual void stop() = 0;
        
        /** Pauses playback */
        virtual void pause() = 0;

        /**
         * @param percent Set volume in range 0 to 100.
         */
        virtual void setVolume( int percent ) = 0;
        
        /**
         * Jump to new time position.
         * @param ms New position.
         */
        virtual void seek( long ms ) = 0;
        
        /** Show configuration GUI for current decoder */
        virtual void configureDecoder() {}
        
        /** Called when new streaming data should be processed. */
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
        bool m_stream;
        
        EngineBase( const EngineBase& ); //disable copy constructor
        const EngineBase &operator=( const EngineBase& ); //disable copy constructor
};

#endif
