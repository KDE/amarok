/***************************************************************************
 *   Copyright (C) 2005 Paul Cifarelli                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <qthread.h>
#include <sys/param.h>

#include "helix-engine.h"
#include "hxsplay.h"
#include "helix-config.h"

AMAROK_EXPORT_PLUGIN( HelixEngine )

#define DEBUG_PREFIX "helix-engine"
#define indent helix_indent

#include <climits>
#include <cmath>
//#include <iostream>

#include "debug.h"

#include <klocale.h>
#include <kmessagebox.h>

#include <qapplication.h>
#include <qdir.h>

using namespace std;

extern "C"
{
    #include <unistd.h>
}

#ifndef LLONG_MAX
#define LLONG_MAX 9223372036854775807LL
#endif


///returns the configuration we will use
static inline QCString configPath() { return QFile::encodeName( QDir::homeDirPath() + "/.helix/config" ); }


HelixEngine::HelixEngine()
   : EngineBase(), HXSplay(),
     m_coredir("/usr/local/RealPlayer/common"),
     m_pluginsdir("/usr/local/RealPlayer/plugins"),
     m_codecsdir("/usr/local/RealPlayer/codecs"),
     m_state(Engine::Empty)
{
    addPluginProperty( "StreamingMode", "Socket" );
    addPluginProperty( "HasConfigure", "true" );
    //addPluginProperty( "HasEqualizer", "false" );
    //addPluginProperty( "HasCrossfade", "true" );
}

HelixEngine::~HelixEngine()
{
}

amaroK::PluginConfig*
HelixEngine::configure() const
{
    return new HelixConfigDialog( (HelixEngine *)this );
}


bool
HelixEngine::init()
{
   //debug() << "Initializing HelixEngine\n";

   // TODO: more intelligent path determination
   HXSplay::init(m_coredir, m_pluginsdir, m_codecsdir);
   if (HXSplay::getError())
   {
      KMessageBox::error( 0, i18n("amaroK could not initialize helix-engine.") );
      return false;
   }

   debug() << "Succussful init\n";
   return true;
}


bool
HelixEngine::load( const KURL &url, bool isStream )
{
   debug() << "In load " << url.url() << endl;

   stop();

   Engine::Base::load( url, isStream || url.protocol() == "http" );
   m_state = Engine::Idle;
   emit stateChanged( Engine::Idle );
   m_url = url;

   // most unfortunate...KURL represents a file with no leading slash, Helix uses a leading slash 'file:///home/abc.mp3' for example
   if (url.isLocalFile())
   {
      QString tmp;
      tmp ="file://" + url.directory() + "/" + url.filename();
      //char tmp[MAXPATHLEN];
      //strcpy(tmp, "file://");
      //strcat(tmp, (const char *)url.directory());
      //strcat(tmp, "/");
      //strcat(tmp, (const char *)url.filename());
      debug() << tmp << endl;
      HXSplay::setURL( QFile::encodeName( tmp ) );      
   }
   else
      HXSplay::setURL( QFile::encodeName( url.prettyURL() ) );

   return true;
}

bool
HelixEngine::play( uint offset )
{
   debug() << "In play" << endl;

   HXSplay::play();
   if (offset)
      HXSplay::seek( offset );
   
   if (!HXSplay::getError())
   {
      m_state = Engine::Playing;
      emit stateChanged( Engine::Playing );

      return true;
   }
   
   HXSplay::stop();
   m_state = Engine::Empty;
   emit stateChanged( Engine::Empty );
   
   return false;
}

void
HelixEngine::stop()
{
   debug() << "In stop\n";
   m_url = KURL();
   HXSplay::stop();
   m_state = Engine::Empty;
   emit stateChanged( Engine::Empty );
}

void HelixEngine::play_finished(int playerIndex)
{
   debug() << "Ok, finished playing the track, so now I'm idle\n";
   m_state = Engine::Idle;
   startTimer( 250 ); // should be resonable until we build the crossfader
}

void
HelixEngine::pause()
{
   debug() << "In pause\n";
   
   if( HXSplay::state() == HXSplay::PLAY )
   {
      HXSplay::pause();
      m_state = Engine::Paused;
      emit stateChanged( Engine::Paused );
   } 
   else if ( HXSplay::state() == HXSplay::PAUSE )
   {
      HXSplay::resume();
      m_state = Engine::Playing;
      emit stateChanged( Engine::Playing );
   }
}

Engine::State
HelixEngine::state() const
{
   //debug() << "In state, state is " << m_state << endl;

   HXSplay::pthr_states state = HXSplay::state();
   switch( state )
   {
      case HXSplay::PLAY:  
         return Engine::Playing;
      case HXSplay::PAUSE: 
         return Engine::Paused;
      case HXSplay::STOP:
         return m_url.isEmpty() ? Engine::Empty : Engine::Idle;
      default:
         // stop(); shouldnt ever get here, but const nature of this function prevents us from doing this
         return Engine::Empty;
   }
}

uint
HelixEngine::position() const
{
   return HXSplay::where(0);
}

uint
HelixEngine::length() const
{
   debug() << "In length\n";
   return HXSplay::duration(0);
}

void
HelixEngine::seek( uint ms )
{
   debug() << "In seek\n";
   HXSplay::seek(ms);
}

void
HelixEngine::setVolumeSW( uint vol )
{
   debug() << "In setVolumeSW\n";
   HXSplay::setVolume(vol);
}


bool
HelixEngine::canDecode( const KURL &url ) const
{
   debug() << "In canDecode\n";   
    //TODO check if the url really is supported by Helix
   return true;
}

void 
HelixEngine::timerEvent( QTimerEvent * )
{
   if (state() == Engine::Idle)
   {
      killTimers();
      debug() << "emitting trackEnded\n";
      emit trackEnded();
   }
}


//amaroK::PluginConfig*
//HelixEngine::configure() const
//{
//    return new XineConfigDialog( m_xine );
//}

namespace Debug
{
    #undef helix_indent
    QCString helix_indent;
}

#include "helix-engine.moc"
