//Copyright: (C) 2003 Mark Kretschmann
//           (C) 2004 Max Howell, <max.howell@methylblue.com>
//License:   See COPYING

#ifndef AMAROK_ENGINEBASE_H
#define AMAROK_ENGINEBASE_H

#include "plugin/plugin.h" //baseclass
#include <qobject.h>       //baseclass

#include <kurl.h>
#include <sys/types.h>
#include <vector>


// DEVELOPMENT NOTES
// * You must handle your own media, do not rely on amaroK to call stop() before play() etc.
// * Generally at this time, emitting stateChanged( Engine::Idle ) is not necessary
// * You must return Idle from state() when the track has finished playback but you are still loaded
//   or track transitions will not occur
// * Basically, reimplement everything virtual and ensure you emit stateChanged() correctly,
//   try not to block in any function that is called by amaroK, try to keep the user informed
//   with statusText()s


namespace Engine
{
    typedef std::vector<int16_t> Scope;

    class Effects;

    enum State { Empty, Idle, Playing, Paused };
    enum StreamingMode { Socket, Signal, NoStreaming };


    class Base : public QObject, public amaroK::Plugin
    {
    Q_OBJECT

    signals:
        /** Emitted when end of current track is reached. */
        void trackEnded();

        /** Transmits status message. */
        void statusText( const QString& );

        /** Signals a change in the engine's state. */
        void stateChanged( Engine::State );

        /** Shows amaroK config dialog at specified page */
        void showConfigDialog( int );

    public:
        virtual ~Base();

        /**
         * Initializes the engine. Must be called after the engine was loaded.
         * @return True if initialization was successful.
         */
        virtual bool init() = 0;

        /**
         * Determines if the engine is able to play a given URL.
         * @param url The URL of the file/stream.
         * @return True if we can play the URL.
         */
        virtual bool canDecode( const KURL &url ) const = 0;

        /**
         * Determines if current track is a stream.
         * @return True if track is a stream.
         */
        inline bool isStream() { return m_isStream; }

        /**
         * Load new track for playing.
         * @param url URL to be played.
         * @param stream True if URL is a stream.
         * @return True for success.
         */
        virtual bool load( const KURL &url, bool stream = false );

        /**
         * Load new track and start Playback. Convenience function for amaroK to use.
         * @param url URL to be played.
         * @param stream True if URL is a stream.
         * @return True for success.
         */
        bool play( const KURL &u, bool stream = false ) { return load( u, stream ) && play(); }

        /**
         * Start playback.
         * @param offset Start playing at @p msec position.
         * @return True for success.
         */
        virtual bool play( uint offset = 0 ) = 0;

        /** Stops playback */
        virtual void stop() = 0;

        /** Pauses playback */
        virtual void pause() = 0;

        /** Get current engine status. */
        virtual State state() const = 0;

        /** Get Time position (msec). */
        virtual uint position() const = 0;

        /**
         * Jump to new time position.
         * @param ms New position.
         */
        virtual void seek( uint ms ) = 0;

        /** Returns whether we are using the hardware volume mixer */
        inline bool isMixerHW() const { return m_mixer != -1; }

        /**
         * Determines whether media is currently loaded.
         * @return True if media is loaded, system is ready to play.
         */
        inline bool loaded() const { return state() != Empty; }

        inline uint volume() const { return m_volume; }
        inline bool hasEffects() const { return m_effects; }

        /**
         * Determines whether the engine supports crossfading.
         * @return True if crossfading is supported.
         */
        inline bool hasXFade() const { return m_hasXFade; }

        /**
         * Determines how streaming is handled with this engine.
         * @return The supported streaming mode.
         */
        StreamingMode streamingMode() const { return m_streamingMode; }

        Effects& effects() const { return *m_effects; } //WARNING! calling when there are none will crash amaroK!

        /**
         * Fetch the current audio sample buffer.
         * @return Audio sample buffer.
         */
        virtual const Scope &scope() { return m_scope; };

        bool setHardwareMixer( bool );

        /**
         * Set new volume value.
         * @param value Volume in range 0 to 100.
         */
        void setVolume( uint value );

        /** Set new crossfade length (msec) */
        void setXfadeLength( int value ) { m_xfadeLength = value; }

    protected:
        Base( StreamingMode = NoStreaming, bool hasConfigure = false, bool hasXFade = false, Effects* = 0 );

        /** shows the amaroK configuration dialog at the engine page */
        void showEngineConfigDialog() { emit showConfigDialog( 4 ); }

        virtual void setVolumeSW( uint percent ) = 0;
        void setVolumeHW( uint percent );

        void setEffects( Effects *e ) { m_effects = e; }
        void setStreamingMode( StreamingMode m ) { m_streamingMode = m; }

        Base( const Base& ); //disable copy constructor
        const Base &operator=( const Base& ); //disable copy constructor

        int           m_xfadeLength;

    private:
        StreamingMode m_streamingMode;
        bool          m_hasXFade;
        Effects      *m_effects;
        int           m_mixer;

    protected:
        uint  m_volume;
        KURL  m_url;
        Scope m_scope;
        bool  m_isStream;
    };


    class Effects
    {
    public:
        virtual QStringList availableEffects() const = 0;
        virtual std::vector<long> activeEffects() const = 0;
        virtual QString effectNameForId( long ) const = 0;
        virtual bool effectConfigurable( long ) const = 0;
        virtual long createEffect( const QString& ) = 0;
        virtual void removeEffect( long ) = 0;
        virtual void configureEffect( long ) = 0;
    };
}

typedef Engine::Base EngineBase;

#endif
