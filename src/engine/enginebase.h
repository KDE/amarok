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
// You must handle your own media, do not rely on amaroK to call stop() before play() etc.



namespace Engine
{
    typedef std::vector<int16_t> Scope;

    class Effects;

    enum State { Empty, Idle, Playing, Paused };
    enum StreamingMode { Socket, Signal, NoStreaming };


    class Base : public QObject, public amaroK::Plugin
    {
    Q_OBJECT

    public:
        virtual ~Base();

        //return false and the engine will not be used
        virtual bool init() = 0;

        //can you decode this media? make this function fast!
        virtual bool canDecode( const KURL &url ) = 0;

        //are you streaming the currently playing media?
        //TODO instead make a method that can say, no, most of this stuff is not available.
        // <markey> The engine does not determine this. EngineController does.
        inline bool isStream() { return m_isStream; }

        //prepare the engine to play url, note that play(KURL) calls load(KURL) then play()
        //the very minimum thing you must do here is call the base implementation
        //ensure you return false if you fail to prepare the engine for url
        virtual bool load( const KURL &url, bool stream = false );

        //convenience function for amaroK to use
        bool play( const KURL &u, bool stream = false ) { return load( u, stream ) && play(); }

        //return success as a bool, emit stateChanged
        //do not unpause in this function, _always_ restart the media at m_url
        //start the playback at offset milliseconds
        //you should play m_url
        virtual bool play( uint offset = 0 ) = 0;

        //you must emit stateChanged()
        // <markey> stop() also unloads the current track.
        virtual void stop() = 0;

        //this must toggle the pause state
        //emit stateChanged
        virtual void pause() = 0;

        //Some important points
        // If you are Playing, Paused or Idle (ie loaded), you will handle play/pause/stop gracefully
        // You will not be expected to handle these functions if Empty
        virtual State state() const = 0;

        virtual uint position() const = 0;
        virtual void seek( uint ms ) = 0;

        inline bool   isMixerHW()      const { return m_mixer != -1; }
        inline bool   loaded()         const { return state() != Empty; }
        inline uint   volume()         const { return m_volume; }
        inline bool   hasEffects()     const { return m_effects; }
        inline bool   hasXFade()       const { return m_hasXFade; }
        StreamingMode streamingMode()  const { return m_streamingMode; }
        Effects&      effects()        const { return *m_effects; } //WARNING! calling when there are none will crash amaroK!

        virtual const Scope &scope() { return m_scope; };

        bool setHardwareMixer( bool );
        void setVolume( uint pc ) { m_volume = pc; if( isMixerHW() ) setVolumeHW( pc ); else setVolumeSW( pc ); }

    signals:
        void trackEnded();
        void statusText( const QString& );
        void stateChanged( Engine::State );

    protected:
        Base( StreamingMode = NoStreaming, bool hasConfigure = false, bool hasXFade = false, Effects* = 0 );

        virtual void setVolumeSW( uint percent ) = 0;
        void setVolumeHW( uint percent );

        void setEffects( Effects *e ) { m_effects = e; }
        void setStreamingMode( StreamingMode m ) { m_streamingMode = m; }

        Base( const Base& ); //disable
        const Base &operator=( const Base& ); //disable

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
