/***************************************************************************
 *   Copyright (C) 2006 Paul Cifarelli <paul@cifarelli.net>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _HXSPLAY_INCLUDED_
#define _HXSPLAY_INCLUDED_

#include "helix-sp.h"

class PlayerControl
{
public:
   PlayerControl();
   virtual ~PlayerControl();

   int getError() const { return m_err; }

   // init functions
   void init(const char *corelibpath, const char *pluginslibpath, const char *codecspath, int numPlayers = 1);
   void setOutputSink( HelixSimplePlayer::AUDIOAPI out );
   void setDevice( const char *dev );
   int  initDirectSS();
   void tearDown();

   // player functions
   int  setURL(const char *url, 
               int playerIndex,
               bool islocal = true);
   bool done(int playerIndex);
   void start(int playerIndex, 
              bool fadein = false, 
              unsigned long fadetime = 0);
   void stop(int playerIndex = HelixSimplePlayer::ALL_PLAYERS);
   void pause(int playerIndex);
   void resume(int playerIndex);
   void seek(unsigned long pos, int playerIndex);
   unsigned long where(int playerIndex) const;
   unsigned long duration(int playerIndex) const;
   unsigned long getVolume();
   void setVolume(unsigned long vol);
   void dispatch();
   void cleanUpStream(int playerIndex);
   bool isPlaying(int playerIndex) const;
   bool isLocal(int playerIndex) const;
   int numPlayers() const { return nNumPlayers; }

   // crossfade
   void setFadeout(bool fadeout, unsigned long fadelength, int playerIndex);

   // scope
   void addScopeBuf(struct DelayQueue *item, int playerIndex);
   DelayQueue *getScopeBuf(int playerIndex);
   int getScopeCount(int playerIndex);
   int peekScopeTime(unsigned long &t, int playerIndex);
   void clearScopeQ(int playerIndex);

   // equalizer
   void enableEQ(bool enabled);
   bool isEQenabled();
   void updateEQgains();

   // config stuff
   int numPlugins() const;
   int getPluginInfo(int index, 
                     const char *&description, 
                     const char *&copyright, 
                     const char *&moreinfourl) const;
   const MimeList *getMimeList() const;
   int getMimeListLen() const;

   virtual void play_finished(int /*playerIndex*/) {}

   // stream meta data
   HelixSimplePlayer::metaData *getMetaData(int playerIndex);

   // need to simulate these
   virtual void notifyUser(unsigned long/*code*/, const char */*moreinfo*/, const char */*moreinfourl*/) {}
   virtual void interruptUser(unsigned long/*code*/, const char */*moreinfo*/, const char */*moreinfourl*/) {}
   virtual int print2stdout(const char */*fmt*/, ...) { return 0; }
   virtual int print2stderr(const char */*fmt*/, ...) { return 0; }
   virtual void onContacting(const char */*host*/) {}
   virtual void onBuffering(int /*percentage*/) {}

   // children send functions
   bool sendnotifyuser(unsigned long code, const char *moreinfo, const char *moreinfourl);
   bool sendinterruptuser(unsigned long code, const char *moreinfo, const char *moreinfourl);
   bool sendcontacting(const char *host);
   bool sendbuffering(int percentage);
   
protected:
   bool                 m_eq_enabled;
   int                  m_preamp;
   vector<int>          m_equalizerGains;

private:
   int   m_err;
   pid_t iamparent;
   int   m_index; 
   int   nNumPlayers;
   bool  m_inited;

   // not yet initd when these are set
   HelixSimplePlayer::AUDIOAPI m_api;
   char                       *m_device;

   static const int NUM_SCOPEBUFS = 50;

   struct playerChildren
   {
      int m_pipeA[2];
      int m_pipeB[2];
      pid_t m_pid;
      bool isplaying;
      bool islocal;
      bool isready;
      bool isdead;
      HelixSimplePlayer::metaData *md;
      int                scopecount;
      struct DelayQueue *scopebufhead;
      struct DelayQueue *scopebuftail;
      unsigned long *current_time;
      unsigned long *duration;
      DelayQueue *q;
      int *m_current;
      int *m_consumed;
   }  m_children[2];

   unsigned long m_volume;

   // supported mime type list
   MimeList            *mimehead;
   int                  mimelistlen;

   int m_numPlugins;
   struct pluginInfo
   {
      char *description; 
      char *copyright; 
      char *moreinfourl; 
   } **m_pluginInfo;

   // for sharing
   struct stateStuff
   {
      unsigned long current_time;
      unsigned long duration;
      HelixSimplePlayer::metaData md;
      //DelayQueue q[NUM_SCOPEBUFS];
      //unsigned char b[NUM_SCOPEBUFS][65535];
      int m_current;
      int m_consumed;
   } statestuff[2], *pmapped;

   // msgs are a 1 byte code follows by a 4 byte sz.  sz does NOT include the 5 bytes of this hdr
   enum msgid
   {
      READY,
      INIT,
      SETURL,
      START,
      STOP,
      PAUSE,
      RESUME,
      SEEK,
      DONE,
      SETVOLUME,
      VOLUME,
      OUTPUTSINK,
      DEVICE,
      SETFADE,
      ENABLEEQ,
      UPDATEEQGAINS,
      SCOPEBUF,
      SCOPECLEAR,
      METADATAREQ,
      METADATA,
      PLUGINS,
      MIMETYPES,
      CONTACTING,
      BUFFERING,
      NOTIFYUSER,
      INTERRUPTUSER,
      TEARDOWN,
      ERRORCODE
   };

   // utility functions   
   bool sendsetoutputsink();
   bool sendsetdevice();
   bool sendinit();
   bool sendupdateeqgains();

   static bool getmessage(int fd, msgid &m, unsigned char *buf, int &sz);
   static bool sendmessage(int fd, msgid m, unsigned char *buf, int sz);
   static bool sendrequest(int fd, msgid m) { return (sendmessage(fd, m, 0, 0)); }
   static bool sendready(int fd) { return (sendrequest(fd, READY)); }
   static bool sendsetURL(int fd, const char *url, bool islocal);
   static bool sendstart(int fd, bool fadin, unsigned long fadetime);
   static bool sendstop(int fd) { return (sendrequest(fd, STOP)); }
   static bool sendpause(int fd) { return (sendrequest(fd, PAUSE)); }
   static bool sendresume(int fd) { return (sendrequest(fd, RESUME)); }
   static bool senddone(int fd) { return (sendrequest(fd, DONE)); }
   static bool sendsetvolume(int fd, unsigned long volume);
   static bool sendvolume(int fd, unsigned long volume);
   static bool sendsetfade(int fd, bool fadeout, unsigned long fadelength);
   static bool sendteardown(int fd) { return (sendrequest(fd, TEARDOWN)); }
   static bool sendscopeclear(int fd) { return (sendrequest(fd, SCOPECLEAR)); }

   static bool sendplugins(int fd, HelixSimplePlayer *player);
   static bool sendmimetypes(int fd, HelixSimplePlayer *player);
   static bool sendscopebuf(int fd, DelayQueue *item);
};


#endif
