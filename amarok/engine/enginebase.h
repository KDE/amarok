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

#include <vector>
#include <qobject.h>
#include <kurl.h>

#ifdef __FreeBSD__
    #include <sys/types.h>
#endif

class QString;
class QStringList;

class EngineBase : public QObject
{
    Q_OBJECT

    signals:
        void                         endOfTrack();
            
    public:
        enum                         EngineState { Empty, Idle, Playing, Paused };

        virtual                      ~EngineBase();

        //@param hardware true for soundcard hardware mixing
        //@return true if using hardware mixing
        virtual bool                 initMixer( bool hardware )                        = 0;
        
        virtual bool                 canDecode( const KURL &url,
                                                mode_t mode, mode_t permissions )      = 0;

        //@return time position in ms
        virtual long                 position() const                                  = 0;
        virtual EngineState          state() const                                     = 0;

        //@return true if media is loaded, system is ready to play
                bool                 loaded() { return state() != Empty; }

        //@return volume in range 0 to 99
        inline  int                  volume() const { return m_volume; }

        //@return true if using hardware mixer
                bool                 isMixerHardware() const { return m_mixerHW != -1; }

        virtual bool                 isStream() const                                  = 0;

        //@return pointer to result of FFT calculation. must be deleted after use.
        virtual std::vector<float>*  scope()                                           = 0;

                void                 setRestoreEffects( bool yes )
                                     { m_restoreEffects = yes; }
        virtual QStringList          availableEffects() const                          = 0; 
        virtual std::vector<long>    activeEffects() const                             = 0;
        virtual QString              effectNameForId( long id ) const                  = 0;
        virtual bool                 effectConfigurable( long id ) const               = 0;        
        virtual long                 createEffect( const QString& name )               = 0;
        virtual void                 removeEffect( long id )                           = 0;
        virtual void                 configureEffect( long id )                        = 0;
               
        virtual bool                 open( const KURL& )                               = 0;

        virtual void                 play()                                            = 0;
        virtual void                 stop()                                            = 0;
        virtual void                 pause()                                           = 0;

        virtual void                 seek( long ms )                                   = 0;
        //@param percent set volume in range 0 to 99
        virtual void                 setVolume( int percent )                          = 0;
                void                 setXfadeLength( int ms );

        QStringList                  listEngines();

        //@param system name of multimedia framework
        //@param restart signals sound deamon must be restarted due to plugin installation. applies only to arts
        //@param scopeSize size of vector the scope delivers, exponent to base 2
        static EngineBase*           createEngine( QString system, bool& restart,
                                                   int scopeSize, bool restoreEffects );

    protected:
        void                         closeMixerHW();
        bool                         initMixerHW();
        void                         setVolumeHW( int percent );
        /////////////////////////////////////////////////////////////////////////////////////
        // ATTRIBUTES
        /////////////////////////////////////////////////////////////////////////////////////
        int                          m_mixerHW;
        int                          m_volume;
        int                          m_xfadeLength;
        static bool                  m_restoreEffects;
};

#endif
