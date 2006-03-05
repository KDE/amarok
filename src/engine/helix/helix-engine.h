/***************************************************************************
 *   Copyright (C) 2005 Paul Cifarelli <paul@cifarelli.net>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _HELIX_ENGINE_H_
#define _HELIX_ENGINE_H_

#include "engine/enginebase.h"
#include <qobject.h>
#include <sys/types.h>
#include <helix-sp.h>

class QStringList;

class HelixEngine : public Engine::Base, public HelixSimplePlayer
{
   Q_OBJECT

public:
   HelixEngine();
   ~HelixEngine();

   virtual bool init();
   virtual bool canDecode( const KURL& ) const;
   virtual bool load( const KURL &url, bool stream );
   virtual bool play( uint = 0 );
   virtual void stop();
   virtual void pause();
   virtual uint position() const;
   virtual uint length() const;
   virtual void seek( uint );

   virtual Engine::State state() const;

   virtual void play_finished(int playerIndex);
   virtual const Engine::Scope &scope();

   virtual amaroK::PluginConfig *configure() const;

   virtual void setEqualizerEnabled( bool );
   virtual void setEqualizerParameters( int preamp, const QValueList<int>& );

   virtual void onContacting(const char *host);
   virtual void onBuffering(int pcnt);

   virtual int fallbackToOSS();

protected:
   virtual void setVolumeSW( uint );

private:
   Engine::State m_state;
   KURL          m_url;

   QString      m_coredir;
   QString      m_pluginsdir;
   QString      m_codecsdir;
   bool         m_inited;

   int           m_numPlayers;
   int           m_current;  // the current player

   bool          m_isStream;
   HelixSimplePlayer::metaData m_md;

   DelayQueue *m_item;
#ifdef DEBUG_PURPOSES_ONLY
   double m_fps;
   int    m_fcount;
   double m_ftime;
   int m_scopebufwaste;
   int m_scopebufnone;
   int m_scopebuftotal;
#endif
   int m_sb;
   unsigned long  m_lasttime;
   unsigned long  m_lastpos;
   unsigned short m_currentScope[512];
   int            m_scopeindex;

   typedef struct MimeEntry
   {
      QStringList type;
      QStringList ext;
   };

   std::vector<MimeEntry> m_mimes;

   void cleanup();
   void timerEvent( QTimerEvent * );
   void resetScope();

   unsigned long prune();

   int print2stdout(const char *fmt, ...);
   int print2stderr(const char *fmt, ...);

   friend class HelixConfigDialogBase;
};


#endif
