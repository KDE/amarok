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
#include <climits>
#include <cmath>
#include <stdarg.h>

#include <config.h>

#include <iostream>

#include "debug.h"

#include <klocale.h>
#include <kmessagebox.h>

#include <qapplication.h>
#include <qdir.h>
#include <qstringlist.h>

#include "helix-engine.h"
#include "helix-configdialog.h"
#include "config/helixconfig.h"

AMAROK_EXPORT_PLUGIN( HelixEngine )

#define DEBUG_PREFIX "helix-engine"

using namespace std;

extern "C"
{
    #include <unistd.h>
}

#define HELIX_ENGINE_TIMER 10     // 10 ms timer
#define SCOPE_MAX_BEHIND   200    // 200 postmix buffers



#ifndef LLONG_MAX
#define LLONG_MAX 9223372036854775807LL
#endif


///returns the configuration we will use
static inline QCString configPath() { return QFile::encodeName( QDir::homeDirPath() + "/.helix/config" ); }


HelixEngine::HelixEngine()
   : EngineBase(), HelixSimplePlayer(),
     m_state(Engine::Empty),
     m_coredir(HELIX_LIBS "/common"),
     m_pluginsdir(HELIX_LIBS "/plugins"),
     m_codecsdir(HELIX_LIBS "/codecs"),
     m_inited(false),
     m_item(0),
#ifdef DEBUG_PURPOSES_ONLY
     m_fps(0.0),m_fcount(0),m_ftime(0.0),m_scopebufwaste(0), m_scopebufnone(0), m_scopebuftotal(0),
#endif
     m_lasttime(0), m_lastpos(0), m_scopeindex(0)
{
   addPluginProperty( "StreamingMode", "NoStreaming" ); // this means we'll handle streaming (not using KIO)
   addPluginProperty( "HasConfigure", "true" );
   addPluginProperty( "HasEqualizer", "true" );
   //addPluginProperty( "HasCrossfade", "true" );

   memset(&m_md, 0, sizeof(m_md));

}

HelixEngine::~HelixEngine()
{
   m_mimes.clear();
}

int HelixEngine::print2stdout(const char *fmt, ...)
{
    va_list args;
    char buf[1024];

    va_start(args, fmt);

    int ret = vsprintf(buf, fmt, args);
    debug() << buf;

    va_end(args);

    return ret;
}


int HelixEngine::print2stderr(const char *fmt, ...)
{
    va_list args;
    char buf[1024];

    va_start(args, fmt);

    int ret = vsprintf(buf, fmt, args);
    debug() << buf;

    va_end(args);

    return ret;
}


void HelixEngine::onContacting(const char *host)
{
   emit statusText( i18n("Contacting: ").arg( QString(host) ) );
}

void HelixEngine::onBuffering(int pcnt)
{
   if (pcnt != 100) // let's not report that...
      emit statusText( i18n( "Buffering %1%" ).arg( pcnt ) );
}


amaroK::PluginConfig*
HelixEngine::configure() const
{
   debug() << "Starting HelixConfigDialog\n";
   return new HelixConfigDialog( (HelixEngine *)this );
}

int HelixEngine::fallbackToOSS()
{
   KMessageBox::information( 0, i18n("The helix library you have configured does not support ALSA, the helix-engine has fallen back to OSS") );
   debug() << "Falling back to OSS\n";
   return (HelixConfigDialog::setSoundSystem( (int) OSS ));
}

bool
HelixEngine::init()
{
   debug() << "Initializing HelixEngine\n";
   struct stat s;
   bool exists = false;
   stop();
   m_state = Engine::Empty;

   m_numPlayers = 2;
   m_current = 1;

   m_coredir = HelixConfig::coreDirectory();
   if (m_coredir.isEmpty())
      m_coredir = HELIX_LIBS "/common";

   m_pluginsdir = HelixConfig::pluginDirectory();
   if (m_pluginsdir.isEmpty())
      m_pluginsdir = HELIX_LIBS "/plugins";

   m_codecsdir = HelixConfig::codecsDirectory();
   if (m_codecsdir.isEmpty())
      m_codecsdir = HELIX_LIBS "/codecs";

   if (HelixConfig::outputplugin() == "oss")
      setOutputSink( OSS );
   else
   {
      setOutputSink( ALSA );
      if (HelixConfig::deviceenabled())
         setDevice( HelixConfig::device().utf8() );
      else
         setDevice("default");
   }


   if (!stat(m_coredir.utf8(), &s) && !stat(m_pluginsdir.utf8(), &s) && !stat(m_codecsdir.utf8(), &s))
   {
      long vol=0;
      bool eqenabled=false;
      int savedpreamp=0;
      QValueList<int> savedequalizerGains;

      if (m_inited)
      {
         vol = HelixSimplePlayer::getVolume(m_current);
         eqenabled = HelixSimplePlayer::isEQenabled();
         for (unsigned int i=0; i < m_equalizerGains.size(); i++)
            savedequalizerGains.append(m_equalizerGains[i]);
         savedpreamp = m_preamp;
         HelixSimplePlayer::tearDown();
      }

      HelixSimplePlayer::init(m_coredir.utf8(), m_pluginsdir.utf8(), m_codecsdir.utf8(), 2);
      if (HelixSimplePlayer::initDirectSS())
      {
         fallbackToOSS();

         HelixSimplePlayer::initDirectSS();
      }

      if (m_inited)
      {
         HelixSimplePlayer::setVolume(vol);
         setEqualizerParameters(savedpreamp, savedequalizerGains);
         setEqualizerEnabled(eqenabled);
      }

      m_inited = exists = true;
   }


   if (!exists || HelixSimplePlayer::getError())
   {
      KMessageBox::error( 0, i18n("amaroK could not initialize the helix-engine. Please check the paths in \"amaroK Settings\" -> \"Engine\"") );
      // we need to return true here so that the user has an oppportunity to change the directory
      //return false;
      return true;
   }

   // create a list of mime types and ext for use in canDecode()
   m_mimes.resize( getMimeListLen() );
   int i = 0;
   const MimeList *ml = getMimeList();
   MimeEntry *entry;
   while (ml)
   {
      QString mt = ml->mimetypes;
      QString me = ml->mimeexts;

      entry = new MimeEntry;
      entry->type = QStringList::split('|', mt);
      entry->ext = QStringList::split('|', me);
      m_mimes[i] = *entry;
      i++;
      ml = ml->fwd;
   }

   debug() << "Succussful init\n";

   return true;
}


bool
HelixEngine::load( const KURL &url, bool isStream )
{
   debug() << "In load " << url.url() << endl;

   if (!m_inited)
      return false;

   if (!canDecode(url))
   {
      const QString path = url.path();
      const QString ext  = path.mid( path.findRev( '.' ) + 1 ).lower();
      emit statusText( i18n("No plugin found for the %1 format").arg(ext) );
      return false;
   }

   debug() << "xfadeLength is " << m_xfadeLength << endl;
   if( m_xfadeLength > 0 && m_state == Engine::Playing )
   {
   }
   else
      cleanup();

   m_isStream = isStream;
   int nextPlayer;

   nextPlayer = m_current ? 0 : 1;

   Engine::Base::load( url, isStream || url.protocol() == "http" );
   m_state = Engine::Idle;
   emit stateChanged( Engine::Idle );
   m_url = url;

   if (url.isLocalFile())
      HelixSimplePlayer::setURL( QFile::encodeName( url.url() ), nextPlayer, !m_isStream );
   else
   {
      m_isStream = true;
      HelixSimplePlayer::setURL( QFile::encodeName( url.url() ), nextPlayer, !m_isStream );
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

   startTimer(HELIX_ENGINE_TIMER);
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
HelixEngine::cleanup()
{
   if (!m_inited)
      return;

   m_url = KURL();
   HelixSimplePlayer::stop(); // stop all players
   resetScope();
   killTimers();
   m_lasttime = 0;
   m_lastpos = 0;
   m_scopeindex = 0;
   m_isStream = false;
   memset(&m_md, 0, sizeof(m_md));
}

void 
HelixEngine::stop()
{
   if (!m_inited)
      return;

   debug() << "In stop\n";
   cleanup();
   cleanUpStream(m_current);
   m_state = Engine::Empty;
   emit stateChanged( m_state );   
}


void HelixEngine::play_finished(int /*playerIndex*/)
{
   debug() << "Ok, finished playing the track, so now I'm idle\n";
   m_state = Engine::Idle;
   cleanUpStream(m_current);
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

   //debug() << "In length\n";
   return HelixSimplePlayer::duration(m_current);
}

void
HelixEngine::seek( uint ms )
{
   if (!m_inited)
      return;

   debug() << "In seek\n";
   resetScope();
   HelixSimplePlayer::seek(ms, m_current);
}

void
HelixEngine::setVolumeSW( uint vol )
{
   if (!m_inited)
      return;

   debug() << "In setVolumeSW\n";
   HelixSimplePlayer::setVolume(vol); // set the volume in all players!
}


bool
HelixEngine::canDecode( const KURL &url ) const
{
   if (!m_inited)
      return false;

   debug() << "In canDecode " << url.prettyURL() << endl;

   if (url.protocol() == "http" || url.protocol() == "rtsp")
      return true;

   const QString path = url.path();
   const QString ext  = path.mid( path.findRev( '.' ) + 1 ).lower();

   if (ext != "txt")
      for (int i=0; i<(int)m_mimes.size(); i++)
      {
         if (m_mimes[i].type.grep("audio").count() ||
             m_mimes[i].type.grep("video").count() ||
             m_mimes[i].type.grep("application").count())
            if (m_mimes[i].ext.grep(ext).count())
            {
               return true;
            }
      }

   return false;
}

void
HelixEngine::timerEvent( QTimerEvent * )
{
   HelixSimplePlayer::dispatch(); // dispatch the players
   if (m_xfadeLength <= 0 && m_state == Engine::Playing && HelixSimplePlayer::done(m_current))
      play_finished(m_current);

   m_lasttime += HELIX_ENGINE_TIMER;

   prune();

#ifdef DEBUG_PURPOSES_ONLY
   // calculate the frame rate of the scope
   m_ftime += HELIX_ENGINE_TIMER;
   if (m_ftime > 10000)
   {
      m_ftime /= 1000;
      m_fps = (double) m_fcount / m_ftime;
      m_ftime = 0;
      m_fcount = 0;
   }
#endif

   HelixSimplePlayer::metaData *md = getMetaData(m_current);
   if (m_isStream &&
       (strcmp(m_md.title, md->title) || strcmp(m_md.artist, md->artist)))
   {
      memcpy(&m_md, md, sizeof(m_md));

      //debug() << "Title: " << md->title << " Artist: " << md->artist << " Bitrate: " << md->bitrate << endl;

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


unsigned long HelixEngine::prune()
{
   //
   // this bit is to help us keep more accurate time than helix provides
   /////////////////////////////////////////////////////////////////////
   unsigned long w;
   unsigned long hpos = position();

   //debug() << "hpos=" << hpos << " lastpos=" << m_lastpos << " lasttime=" << m_lasttime << endl;
   //if (m_item)
   //   debug() << "      time=" << m_item->time << " etime=" << m_item->etime << endl;

   if (hpos == m_lastpos)
   {
      if (m_item && hpos >= m_item->time && hpos <= m_item->etime && (m_lasttime < m_item->time || m_lasttime > m_item->etime) )
      {
         w = hpos;
         m_lasttime = hpos;
      }
      else
         w = m_lasttime;
   }
   else
   {
      w = hpos;
      m_lasttime = hpos;
   }
   m_lastpos = hpos;

   if ( getScopeCount() > SCOPE_MAX_BEHIND ) // protect against naughty streams
   {
      resetScope();
      return w;
   }

   if (!w || !m_item)
   {
#ifdef DEBUG_PURPOSES_ONLY
      m_scopebufnone++; // for tuning the scope... (scope is tuned for 44.1kHz sample rate)
#endif
      return w;
   }

   while (m_item && w > m_item->etime)
   {
      // need to prune some buffers
      delete m_item;
      m_item = getScopeBuf();

      m_sb++;
   }

   return w;
}

const Engine::Scope &HelixEngine::scope()
{
   int i;
   unsigned long t;

   m_sb = 0;

#ifdef DEBUG_PURPOSES_ONLY
   m_fcount++;
#endif

   if (!m_inited)
      return m_scope;

   if (!m_item && !peekScopeTime(t))
   {
      m_item = getScopeBuf();
      if (m_item)
         m_sb++;
   }

   unsigned long w = prune();
   if (!w)
      return m_scope;

#ifdef DEBUG_PURPOSES_ONLY
   m_scopebuftotal += m_sb;
   if (m_sb > 1)
      m_scopebufwaste += (m_sb-1);
#endif

   if (!m_item)
   {
#ifdef DEBUG_PURPOSES_ONLY
      m_scopebufnone++;
#endif
      return m_scope;
   }      

   if ( w < m_item->time ) // wait for the player to catchup
      return m_scope;

   int j,k=0;
   short int *pint;
   unsigned char b[4];

   // convert to mono
   int a;
   //i=0;
   // calculate the starting offset into the buffer
   int off =  (m_item->spb * (w - m_item->time) / (m_item->etime - m_item->time)) * m_item->nchan * m_item->bps;
   k = off;
   while (m_item && m_scopeindex < 512)
   {
      while (k < (int) m_item->len)
      {
         a = 0;
         for (j=0; j<m_item->nchan; j++)
         {
            switch (m_item->bps)
            {
               case 1:
                  b[1] = 0;
                  b[0] = m_item->buf[k];
                  break;
               case 2:
                  b[1] = m_item->buf[k+1];
                  b[0] = m_item->buf[k];
                  break;
            }

            pint = (short *) &b[0];

            a += (int) *pint;
            k += m_item->bps;
         }
         a /= m_item->nchan;

         //m_scope[i] = a;
         m_currentScope[m_scopeindex] = a;
         m_scopeindex++;
         if (m_scopeindex >= 512)
         {
            m_scopeindex = 512;
            break;
         }
      }
      if (m_scopeindex < 512 && !peekScopeTime(t)) // as long as we know there's another buffer...otherwise we need to wait for another
      {
         delete m_item;
         m_item = getScopeBuf();
         k = 0;

#ifdef DEBUG_PURPOSES_ONLY
         if (m_item)
            m_scopebuftotal++;
#endif
         if (!m_item)
         {
#ifdef DEBUG_PURPOSES_ONLY
            m_scopebufnone++;
#endif
            return m_scope; // wait until there are some more buffers available
         }
      }
      else
      {
         if (k >= (int) m_item->len)
         {
            delete m_item;
            m_item = 0;
         }
         break;
      }
   }

   // ok, we must have a full buffer here, give it to the scope
   for (i=0; i<512; i++)
      m_scope[i] = m_currentScope[i];
   m_scopeindex = 0;

   return m_scope;
}

void
HelixEngine::resetScope()
{
   // make sure the scope is clear of old buffers
   clearScopeQ();
   m_scopeindex = 0;
   delete m_item;
   m_item = 0;
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
