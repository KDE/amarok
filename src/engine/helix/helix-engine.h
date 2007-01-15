/***************************************************************************
 *   Copyright (C) 2005-2006 Paul Cifarelli <paul@cifarelli.net>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _HELIX_ENGINE_H_
#define _HELIX_ENGINE_H_

#include "enginebase.h"
#include <qobject.h>
#include <sys/types.h>
#include <hxplayercontrol.h>

class QStringList;
struct timeval;

class HelixEngine : public Engine::Base, public PlayerControl
{
   Q_OBJECT

public:
   HelixEngine();
   ~HelixEngine();

   virtual bool init();
   virtual bool canDecode( const KURL& ) const;
   virtual uint position() const;
   virtual uint length() const;
   virtual Engine::State state() const;

   virtual void play_finished(int playerIndex);
   virtual const Engine::Scope &scope();

   virtual Amarok::PluginConfig *configure() const;

   virtual void onContacting(const char *host);
   virtual void onBuffering(int pcnt);

   virtual int fallbackToOSS();

public slots:
   virtual bool load( const KURL &url, bool stream );
   virtual bool play( uint = 0 );
   virtual void stop();
   virtual void pause();
   virtual void unpause();
   virtual void seek( uint );

   virtual void setEqualizerEnabled( bool );
   virtual void setEqualizerParameters( int preamp, const QValueList<int>& );


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

   int scope(int playerIndex);
   int prune();
   int prune(int playerIndex);
   bool m_scopeplayerlast;
   float m_sfps;
   struct timeval m_scopetm;
   unsigned long m_scopedelta;
   int   m_sframes;
   int   m_lframes;
   struct HelixScope
   {
      DelayQueue *m_item;
      unsigned long  m_lasttime;
      unsigned long  m_lastpos;
      unsigned short m_currentScope[SCOPESIZE];
      int            m_scopeindex;
      unsigned long  m_w; // more accurate position estimate for the player
   } hscope[2];

   typedef struct MimeEntry
   {
      QStringList type;
      QStringList ext;
   };

   std::vector<MimeEntry> m_mimes;

   struct FadeTrack
   {
      unsigned long m_startfadetime;
      bool m_fadeactive;
      bool m_stopfade;
   } m_pfade[2];

   void cleanup();
   void timerEvent( QTimerEvent * );
   void resetScope(int playerIndex);

   int print2stdout(const char *fmt, ...)
#ifdef __GNUC__
      __attribute__ ((format (printf, 2, 3)))
#endif
      ;
   int print2stderr(const char *fmt, ...)
#ifdef __GNUC__
      __attribute__ ((format (printf, 2, 3)))
#endif
      ;
   void notifyUser(unsigned long code, const char *moreinfo, const char *moreinfourl);
   void interruptUser(unsigned long code, const char *moreinfo, const char *moreinfourl);

   friend class HelixConfigDialogBase;
};


#endif
