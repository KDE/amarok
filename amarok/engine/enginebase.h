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

class QString;
class QStringList;

class EngineBase : public QObject
{
    Q_OBJECT
    
    signals:
        void                         endOfTrack();
        void                         metaData( QString title, QString url, QString kbps );
            
    public:
        enum                         EngineState { Empty, Idle, Playing, Paused };

        virtual                      ~EngineBase();

        virtual bool                 initMixer( bool software )                        = 0;
        virtual bool                 canDecode( const KURL &url )                      = 0;

        //@return time position in ms
        virtual long                 position() const                                  = 0;
        virtual EngineState          state() const                                     = 0;
        
        //@return true if media is loaded, system is ready to play
                bool                 loaded()       { return state() != Empty; }  
        
        //@return volume in range 0 to 99
        inline  int                  volume() const { return m_volume; }
        virtual bool                 isStream() const                                  = 0;

        //@return pointer to result of FFT calculation. must be deleted after use.
        virtual std::vector<float>*  scope()                                           = 0;

        virtual QStringList          availableEffects() const                          = 0; 
        virtual bool                 effectConfigurable( const QString& name ) const   = 0;        
                
        virtual void                 open( KURL )                                      = 0;

        virtual void                 play()                                            = 0;
        virtual void                 stop()                                            = 0;
        virtual void                 pause()                                           = 0;

        virtual void                 seek( long ms )                                   = 0;
        //@param percent set volume in range 0 to 99
        virtual void                 setVolume( int percent )                          = 0;
        QStringList                  listEngines();
        //
        //@param system name of multimedia framework
        //@param restart signals sound deamon must be restarted due to plugin installation. applies only to arts
        //@param scopeSize size of vector the scope delivers
        static EngineBase*           createEngine( QString system, bool& restart, int scopeSize );

    protected:
        bool                         initMixerHW();
        void                         setVolumeHW( int percent );
        /////////////////////////////////////////////////////////////////////////////////////
        // ATTRIBUTES
        /////////////////////////////////////////////////////////////////////////////////////
        int                          m_mixerHW;
        int                          m_volume;
};

#endif
