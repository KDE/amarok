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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "helix-engine.h"
#include "helix-configdialog.h"
#include "config/helixconfig.h"

AMAROK_EXPORT_PLUGIN( HelixEngine )

#define DEBUG_PREFIX "helix-engine"
#define SCOPE_DELAY_TOLERANCE 200 // milliseconds

#include <climits>
#include <cmath>
#include <iostream>

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

#define HELIX_ENGINE_TIMER 20


#ifndef LLONG_MAX
#define LLONG_MAX 9223372036854775807LL
#endif


///returns the configuration we will use
static inline QCString configPath() { return QFile::encodeName( QDir::homeDirPath() + "/.helix/config" ); }


HelixEngine::HelixEngine()
   : EngineBase(), HelixSimplePlayer(),
     m_state(Engine::Empty),
     m_coredir("/usr/local/RealPlayer/common"),
     m_pluginsdir("/usr/local/RealPlayer/plugins"),
     m_codecsdir("/usr/local/RealPlayer/codecs"),
     m_inited(false),
     m_xfadeLength(0)
#ifdef DEBUG_PURPOSES_ONLY
     ,m_fps(0.0),m_fcount(0),m_ftime(0.0),m_scopebufwaste(0), m_scopebufnone(0), m_scopebuftotal(0)
#endif
{
   addPluginProperty( "StreamingMode", "NoStreaming" ); // this is counter intuitive :-)
   addPluginProperty( "HasConfigure", "true" );
   addPluginProperty( "HasEqualizer", "true" );
   //addPluginProperty( "HasCrossfade", "true" );

   memset(&m_md, 0, sizeof(m_md));

}

HelixEngine::~HelixEngine()
{
}

void HelixEngine::onContacting(const char *host)
{
   emit statusText( i18n("Contacting: ").arg( QString(host) ) );
}

void HelixEngine::onBuffering(const int pcnt)
{
   QString message( i18n("Buffering...").sprintf("%d%%",pcnt) );
   emit statusText( message );
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
   struct stat s;
   bool exists = false;

   m_numPlayers = 2;
   m_current = 1;

   m_coredir = HelixConfig::coreDirectory();
   if (!m_coredir.length())
      m_coredir = "/usr/local/RealPlayer/common";

   m_pluginsdir = HelixConfig::pluginDirectory();
   if (!m_pluginsdir.length())
      m_pluginsdir = "/usr/local/RealPlayer/plugins";
   
   m_codecsdir = HelixConfig::codecsDirectory();
   if (!m_codecsdir.length())
      m_codecsdir = "/usr/local/RealPlayer/codecs";

   if (!stat(m_coredir.utf8(), &s) && !stat(m_pluginsdir.utf8(), &s) && !stat(m_codecsdir.utf8(), &s))
   {
      if (m_inited)
         HelixSimplePlayer::tearDown();

      HelixSimplePlayer::init(m_coredir.utf8(), m_pluginsdir.utf8(), m_codecsdir.utf8(), 2);
      m_inited = exists = true;
   }

   if (!exists || HelixSimplePlayer::getError())
   {
      KMessageBox::error( 0, i18n("amaroK could not initialize the helix-engine. Please check the paths in \"amaroK Settings\" -> \"Engine\"") );
      // we need to return true here so that the user has an oppportunity to change the directory
      //return false;
      return true;
   }

   debug() << "Succussful init\n";
   startTimer( HELIX_ENGINE_TIMER );

   return true;
}


bool
HelixEngine::load( const KURL &url, bool isStream )
{
   debug() << "In load " << url.url() << endl;

   if (!m_inited)
      return false;

   stop();

   m_isStream = isStream;
   int nextPlayer;

   nextPlayer = m_current ? 0 : 1;

   Engine::Base::load( url, isStream || url.protocol() == "http" );
   m_state = Engine::Idle;
   emit stateChanged( Engine::Idle );
   m_url = url;

   // most unfortunate...KURL represents a file with no leading slash, Helix uses a leading slash 'file:///home/abc.mp3' for example
   if (url.isLocalFile())
   {
      QString tmp;
      tmp ="file://" + url.directory() + "/" + url.filename();

      debug() << tmp << endl;
      HelixSimplePlayer::setURL( QFile::encodeName( tmp ), nextPlayer );      
   }
   else
   {
      m_isStream = true;
      HelixSimplePlayer::setURL( QFile::encodeName( url.prettyURL() ), nextPlayer );
   }

   return true;
}

bool
HelixEngine::play( uint offset )
{
   debug() << "In play" << endl;
   int nextPlayer;

   if (!m_inited)
      return false;

   nextPlayer = m_current ? 0 : 1;

   HelixSimplePlayer::start(nextPlayer);
   if (offset)
      HelixSimplePlayer::seek( offset, nextPlayer );

   if (!HelixSimplePlayer::getError())
   {
      if (m_state != Engine::Playing)
      {
         m_state = Engine::Playing;
         emit stateChanged( Engine::Playing );
      }

      m_current = nextPlayer;
      return true;
   }

   HelixSimplePlayer::stop(); // stop all players
   m_state = Engine::Empty;
   emit stateChanged( Engine::Empty );

   return false;
}

void
HelixEngine::stop()
{
   if (!m_inited)
      return;

   debug() << "In stop\n";
   m_url = KURL();
   HelixSimplePlayer::stop(m_current);
   clearScopeQ();
   m_state = Engine::Empty;
   m_isStream = false;
   memset(&m_md, 0, sizeof(m_md));
   emit stateChanged( Engine::Empty );
}

void HelixEngine::play_finished(int /*playerIndex*/)
{
   debug() << "Ok, finished playing the track, so now I'm idle\n";
   m_state = Engine::Idle;
   emit trackEnded();
}

void
HelixEngine::pause()
{
   if (!m_inited)
      return;

   debug() << "In pause\n";
   if( m_state == Engine::Playing )
   {
      HelixSimplePlayer::pause(m_current);
      m_state = Engine::Paused;
      emit stateChanged( Engine::Paused );
   }
   else if ( m_state == Engine::Paused )
   {
      HelixSimplePlayer::resume(m_current);
      m_state = Engine::Playing;
      emit stateChanged( Engine::Playing );
   }
}

Engine::State
HelixEngine::state() const
{
   //debug() << "In state, state is " << m_state << endl;

   if (!m_inited || m_url.isEmpty())
      return (Engine::Empty);

   return m_state;
}

uint
HelixEngine::position() const
{
   if (!m_inited)
      return 0;

   return HelixSimplePlayer::where(m_current);
}

uint
HelixEngine::length() const
{
   if (!m_inited)
      return 0;

   debug() << "In length\n";
   return HelixSimplePlayer::duration(m_current);
}

void
HelixEngine::seek( uint ms )
{
   if (!m_inited)
      return;

   debug() << "In seek\n";
   clearScopeQ();
   HelixSimplePlayer::seek(ms, m_current);
}

void
HelixEngine::setVolumeSW( uint vol )
{
   if (!m_inited)
      return;

   debug() << "In setVolumeSW\n";
   HelixSimplePlayer::setVolume(vol, m_current);
}


bool
HelixEngine::canDecode( const KURL &url ) const
{
   if (!m_inited)
      return false;

   debug() << "In canDecode " << url.prettyURL() << endl;   
   //TODO check if the url really is supported by Helix
   return true;
}

void
HelixEngine::timerEvent( QTimerEvent * )
{
   HelixSimplePlayer::dispatch(); // dispatch the players
   if (m_state == Engine::Playing && HelixSimplePlayer::done(m_current))
      play_finished(m_current);

#ifdef DEBUG_PURPOSES_ONLY
   // calculate the frame rate of the scope
   m_ftime += HELIX_ENGINE_TIMER;
   if (m_ftime > 10000)
   {
      m_ftime /= 1000;
      m_fps = (double) m_fcount / m_ftime;
      cerr << "SCOPE FPS: " << m_fps << endl;
      m_ftime = 0;
      m_fcount = 0;
   }
#endif

   metaData *md = getMetaData(m_current);
   if (m_isStream && 
       (strcmp(m_md.title, md->title) || strcmp(m_md.artist, md->artist) || m_md.bitrate != md->bitrate))
   {
      memcpy(&m_md, md, sizeof(m_md));

      // Ok, helix sends the title of the song in the artist string for streams.
      // this prevents context lookup, so we split it here (the artist and title are separated by a '-'
      // we'll put the 'title' in album instead...
      Engine::SimpleMetaBundle bndl;
      bndl.album = QString::fromUtf8( m_md.title );
      char c,*tmp = strchr(m_md.artist, '-');
      if (tmp)
      {
         tmp--;
         c = *tmp;
         *tmp = '\0';
         bndl.artist = QString::fromUtf8( m_md.artist );
         *tmp = c;
         tmp+=3;
         bndl.title = QString::fromUtf8( tmp );
         bndl.album = QString::fromUtf8( m_md.title );
      }
      else // just copy them as is...
      {
         bndl.title = QString::fromUtf8( m_md.title );
         bndl.artist = QString::fromUtf8( m_md.artist );         
      }
      bndl.bitrate = QString::number( m_md.bitrate / 1000 );
      emit EngineBase::metaData( bndl );
   }
}


const Engine::Scope &HelixEngine::scope()
{
   int i, err;
   struct DelayQueue *item = 0;

#ifdef DEBUG_PURPOSES_ONLY
   m_fcount++;
#endif

   if (!m_inited)
      return m_scope;

   unsigned long w = position();
   unsigned long p;
   err = peekScopeTime(p);

   if (err || !w || w < p) // not enough buffers in the delay queue yet
      return m_scope;

   item = getScopeBuf();
   i = 1;
   if (::abs(w - item->time) > SCOPE_DELAY_TOLERANCE) 
   {
      // need to prune some buffers
      while (!err && p < w)
      {
         if (item)
            delete item;
         
         item = getScopeBuf();
         err = peekScopeTime(p);
         
         i++;
      }
   }

#ifdef DEBUG_PURPOSES_ONLY
   m_scopebuftotal += i;
   if (i > 1)
      m_scopebufwaste += (i-1);
#endif

   if (!item)
   {
#ifdef DEBUG_PURPOSES_ONLY
      m_scopebufnone++; // for tuning the scope... (scope is tuned for 44.1kHz sample rate)
#endif
      return m_scope;
   }

   for (i=0; i < 512; i++)
      m_scope[i] = (short int) item->buf[i];
   
   delete item;

#ifdef DEBUG_PURPOSES_ONLY
   if (!(m_scopebuftotal %100))
      cerr << "total " << m_scopebuftotal << " waste " << m_scopebufwaste << " no bufs " << m_scopebufnone << endl;
#endif

   return m_scope;
}

void
HelixEngine::setEqualizerEnabled( bool enabled ) //SLOT
{
   enableEQ(enabled);
}


// ok, this is lifted from gst... but why mess with what works?
void
HelixEngine::setEqualizerParameters( int preamp, const QValueList<int>& bandGains ) //SLOT
{
   m_preamp = ( preamp + 100 ) / 2;

   m_equalizerGains.resize( bandGains.count() );
   for ( uint i = 0; i < bandGains.count(); i++ )
      m_equalizerGains[i] = ( *bandGains.at( i ) + 100 ) / 2;

   updateEQgains();
}


namespace Debug
{
    #undef helix_indent
    QCString helix_indent;
}

#include "helix-engine.moc"
