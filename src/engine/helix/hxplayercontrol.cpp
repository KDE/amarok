/***************************************************************************
 *   Copyright (C) 2006 Paul Cifarelli <paul@cifarelli.net>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/***************************************************************************
 basically this implements a rudamentary rpc mechanism so the we can have
 each player in a separate process.  this is solely done to implement a 
 reliable crossfade (or more precicely, put the crossfade in the hands of 
 the alsa guys
 ***************************************************************************/

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sys/mman.h>

using namespace std;

#include "hxplayercontrol.h"

class HSPPlayerControlled : public HelixSimplePlayer
{
public:
   HSPPlayerControlled(PlayerControl *pcntl, int index) : m_pcntl(pcntl), m_index(index) {}
   virtual ~HSPPlayerControlled() {}

   virtual void notifyUser(unsigned long code, const char *moreinfo, const char *moreinfourl);
   virtual void interruptUser(unsigned long code, const char *moreinfo, const char *moreinfourl);
   virtual void onContacting(const char *host);
   virtual void onBuffering(int percentage);

private:
   PlayerControl *m_pcntl;
   int            m_index;
};


void HSPPlayerControlled::notifyUser(unsigned long code, const char *moreinfo, const char *moreinfourl)
{
   m_pcntl->sendnotifyuser(code, moreinfo, moreinfourl);
}

void HSPPlayerControlled::interruptUser(unsigned long code, const char *moreinfo, const char *moreinfourl)
{
   m_pcntl->sendinterruptuser(code, moreinfo, moreinfourl);
}

void HSPPlayerControlled::onContacting(const char *host)
{
   m_pcntl->sendcontacting(host);
}

void HSPPlayerControlled::onBuffering(int percentage)
{
   m_pcntl->sendbuffering(percentage);
}


PlayerControl::PlayerControl() : m_eq_enabled(false), m_preamp(0), m_err(0), iamparent(0), m_index(0), nNumPlayers(0),
                                 m_inited(false), m_api( HelixSimplePlayer::OSS ), m_device(0), mimehead(0), mimelistlen(0),
                                 m_numPlugins(0), m_pluginInfo(0)
{
   memset(m_children, 0, sizeof(m_children));
}

PlayerControl::~PlayerControl()
{
   tearDown();

   print2stderr("In PlayerControl::~PlayerControl(), m_index=%d\n", m_index);

   delete m_device;

   if (pmapped)
      munmap(pmapped, sizeof(stateStuff) * 2);
}

// init functions
void PlayerControl::init(const char *corelibpath, const char *pluginslibpath, const char *codecspath, int numPlayers)
{
   int err;
   iamparent = 0;
   nNumPlayers = numPlayers;
   m_err = 0;

   print2stderr("In PlayerControl::init(), m_api=%d, m_device=%s\n", m_api, m_device ? m_device : "DEVICE NOT SET");

   if (numPlayers > 2) // it's impossible
   {
      m_err = -1;
      return;
   }

   memset(&m_children, 0, numPlayers * sizeof(struct playerChildren));

   m_inited = false;

   // create a shared memory region for state like stuff
   if ( MAP_FAILED == (pmapped = (stateStuff *) mmap( (void *) statestuff, 
                                                      sizeof(stateStuff) * 2, 
                                                      PROT_READ | PROT_WRITE,
#ifdef __linux__
                                                      MAP_SHARED | MAP_ANONYMOUS,
#else
                                                      MAP_SHARED | MAP_ANON,
#endif
                                                      -1, 0)) )
      pmapped = 0;
      
   // we do this the old fashioned way, so that we don't have to include an executable with our plugin...
   for (int i = 0; i < numPlayers; i++)
   {
      if (pmapped)
      {
         m_children[i].current_time = &(pmapped[i].current_time);
         m_children[i].duration = &(pmapped[i].duration);
         m_children[i].md = &(pmapped[i].md);

         m_children[i].m_current = &(pmapped[i].m_current);
         m_children[i].m_consumed = &(pmapped[i].m_consumed);
         //m_children[i].q = pmapped[i].q;
         //for (int j=0; j<NUM_SCOPEBUFS; j++)
         //{
         //   m_children[i].q[j].allocd = false;
         //   m_children[i].q[j].buf = pmapped[i].b[j];
         //}
      }
      err = pipe(m_children[i].m_pipeA);
      err |= pipe(m_children[i].m_pipeB);
      if ( !err && (iamparent = fork()) )
      {
         // parent
         print2stderr("%%%%%% parent initializes player %d\n", i);

         // parent's m_pid remains 0
         m_children[i].m_pid = iamparent;
         close(m_children[i].m_pipeA[1]); // parent uses A for reading
         close(m_children[i].m_pipeB[0]); // and B for writing
      }
      else if (!err)
      {
         // child

         cerr << "%%%%%% child initializes as player " << i << endl;;

         m_index = i; // child's index is saved
         close(m_children[i].m_pipeA[0]); // child uses A for writing
         close(m_children[i].m_pipeB[1]); // and B for reading
         break;
      }
   }


   if (!iamparent) // children stay here, parents return
   {
      int rfd = m_children[m_index].m_pipeB[0];
      int wfd = m_children[m_index].m_pipeA[1];
      int n;
      struct timeval timeout;
      HSPPlayerControlled *player = 0;
      bool playing = false;

      player = new HSPPlayerControlled(this, m_index);

      timeout.tv_sec = 0;
      timeout.tv_usec = 10000;
      fd_set rdset, wrset;
      FD_ZERO(&rdset);
      FD_ZERO(&wrset);
      FD_SET(rfd, &rdset);
      FD_SET(wfd, &wrset); // really should check to see if we can write, but not gonna

      while ( 1 )
      {
         FD_SET(rfd, &rdset);

         n = select(rfd + 1, &rdset, 0, 0, &timeout);
         if ( FD_ISSET(rfd, &rdset) )
         {
            msgid m;
            unsigned char buf[65536];
            int sz = 0;

            if (getmessage(rfd, m, buf, sz) && player)
            {
               switch (m)
               {
                  case INIT:
                     cerr << "INIT\n";
                     if (!sz)
                     {
                        player->setOutputSink(m_api);
                        if (m_device)
                           player->setDevice(m_device);
                        player->init(corelibpath, pluginslibpath, codecspath, 1);
                        m_inited = true;
                        if (!m_index) // only player 0 need send the mime and plugin stuff
                        {
                           sendplugins(wfd, player);
                           sendmimetypes(wfd, player);
                        }
                        cerr << "%%%%%% player " << m_index << " child sends ready\n";
                        sendready(wfd);
                     }
                     else
                        cerr << "CHILD " << m_index << " sz not right in INIT, sz=" << sz << endl;
                     break;
                  case SETURL:
                  {
                     bool islocal = (bool) buf[0];
                     int len = strlen((const char *)&buf[1]); // not that this would prevent a crash...
                     if (m_inited)
                     {
                        cerr << "CHILD " << m_index << " setURL for " << (const char *)&buf[1] << 
                           ",islocal=" << islocal << ",len=" << len << endl;
                        if (sz == len + 2) 
                           player->setURL((const char *)&buf[1], 0, islocal); // remember, we sent the null...
                        else
                           cerr << "CHILD " << m_index << " sz not right in SETURL, sz=" << sz << endl;
                     }
                  }
                  break;
                  case START:
                  {
                     bool fadein = (bool) buf[0];
                     unsigned long fadetime;

                     if (m_inited)
                     {
                        if (pmapped)
                           *m_children[m_index].current_time = 0;

                        if (sz == sizeof(unsigned long) + 1)
                        {
                           cerr << "CHILD " << m_index << " gets START\n";
                           memcpy((void *)&fadetime, (void *)&buf[1], sizeof(unsigned long));
                           playing = true;
                           player->start(0, fadein, fadetime);
                        }
                        else
                           cerr << "CHILD " << m_index << " sz not right in START, sz=" << sz << endl;
                     }
                  }
                  break;
                  case STOP:
                     if (m_inited)
                        if (!sz)
                        {
                           player->stop();
                           playing = false;
                           cerr << "CHILD " << m_index << " gets STOP\n";
                        }
                        else
                           cerr << "CHILD " << m_index << " sz not right in STOP, sz=" << sz << endl;
                     break;
                  case PAUSE:
                     if (m_inited)
                        if (!sz)
                           player->pause();
                        else
                           cerr << "CHILD " << m_index << " sz not right in PAUSE, sz=" << sz << endl;
                     break;
                  case RESUME:
                     if (m_inited)
                        if (!sz)
                           player->resume();
                        else
                           cerr << "CHILD " << m_index << " sz not right in RESUME, sz=" << sz << endl;
                     break;
                  case SEEK:
                     if (m_inited)
                        if (sz == sizeof(unsigned long))
                        {
                           unsigned long pos;
                           memcpy( (void *) &pos, (void *) buf, sizeof(unsigned long) );
                           if (pos < player->duration(0))
                              player->seek(pos, 0);
                        }
                        else
                           cerr << "CHILD " << m_index << " sz not right in SEEK, sz=" << sz << endl;
                     break;
                  case SETVOLUME:
                  {
                     if (m_inited)
                        if (sz == sizeof(unsigned long))
                        {
                           memcpy( (void *) &m_volume, (void *) buf, sizeof(unsigned long));
                           cerr << "CHILD: received setvolume request " << m_volume <<endl;;
                           player->setVolume(m_volume);
                        }
                        else
                           cerr << "CHILD " << m_index << " sz not right in SETVOLUME, sz=" << sz << endl;
                  }
                  break;
                  case OUTPUTSINK:
                     if (m_inited)
                        if (sz == 1)
                        {
                           m_api = (HelixSimplePlayer::AUDIOAPI) buf[0];
                           cerr << "CHILD: received OUTPUTSINK: " << m_api <<endl;;
                        }
                        else
                           cerr << "CHILD " << m_index << " sz not right in OUTPUTSINK, sz=" << sz << endl;
                     break;
                  case DEVICE:
                  {
                     char* dev = strdup((const char*) buf);
                     if (m_inited)
                        if ((unsigned)sz == strlen(dev) + 1)
                        {
                           cerr << "CHILD " << m_index << " gets device " << dev << endl;
                           setDevice( dev );
                        }
                        else
                           cerr << "CHILD " << m_index << " sz not right in DEVICE, sz=" << sz << endl;
                     free(dev);
                  }
                  break;
                  case SETFADE:
                  {
                     if (m_inited)
                        if (sz == sizeof(unsigned long) + 1)
                        {
                           bool fadeout;
                           unsigned long fadelength;
                           fadeout = (bool) buf[0];
                           memcpy((void *) &fadelength, (void *) &buf[1], sizeof(unsigned long));
                           player->setFadeout(fadeout, fadelength, 0);
                        }
                        else
                           cerr << "CHILD " << m_index << " sz not right in SETFADE, sz=" << sz << endl;
                  }
                  break;
                  case ENABLEEQ:
                  {
                     if (m_inited)
                        if (sz == 1)
                        {
                           m_eq_enabled = (bool) buf[0];
                           player->enableEQ(m_eq_enabled);
                           cerr << "CHILD " << m_index << " enables EQ\n";
                        }
                        else
                           cerr << "CHILD " << m_index << " sz not right in ENABLEEQ, sz=" << sz << endl;
                  }
                  break;
                  case UPDATEEQGAINS:
                  {
                     int i, n, k;

                     cerr << "UPDATEGAINS\n";
                     if (m_inited)
                     {
                        if (sz >= 2)
                        {
                           memcpy( (void *) &m_preamp, (void *) buf, sizeof(m_preamp) );
                           memcpy( (void *) &n, (void *) &buf[ sizeof(m_preamp) ], sizeof(int) );
                           player->m_preamp = m_preamp;
                        }
                        else
                           cerr << "CHILD " << m_index << " sz not right in UPDATEEQGAINS, sz=" << sz << endl;

                        if ((unsigned)sz == sizeof(m_preamp) + sizeof(int) + n * sizeof(int))
                        {
                           if (n > 0)
                           {
                              m_equalizerGains.resize(n);
                              cerr << "CHILD " << m_index << " receives " << n << " equalizer gains\n";
                              for (i=0; i<n; i++)
                              {
                                 memcpy( (void *) &k, (void *) &buf[ sizeof(m_preamp) + (i+1)*sizeof(int) ], sizeof(int) );
                                 m_equalizerGains[i] = k;
                              }
                              player->m_equalizerGains = m_equalizerGains;
                              if (!m_eq_enabled)
                                 player->enableEQ(true);
                              player->updateEQgains();
                              player->enableEQ(m_eq_enabled);
                           }
                        }
                        else
                           cerr << "CHILD " << m_index << " sz not right in UPDATEEQGAINS, sz=" << sz << endl;
                     }
                  }
                  break;
                  case SCOPECLEAR:
                     if (m_inited)
                     {
                        player->clearScopeQ(0);
                        if (pmapped)
                           *m_children[m_index].m_consumed = *m_children[m_index].m_current = 0;
                     }
                     break;
                  case TEARDOWN:
                     if (!sz)
                     {
                        cerr << "CHILD: " << m_index << " received shutdown request\n";
                        player->stop(0);
                        delete player;
                        raise(15);
                        exit(0);
                     }
                     else
                        cerr << "CHILD " << m_index << " sz not right in TEARDOWN, sz=" << sz << endl;
                     break;

                  default: // send an error to the parent
                        cerr << "CHILD " << m_index << " received unhandled message, sz=" << sz << endl;
                     break;
               }
            }
            else
            {
               cerr << "CHILD " << m_index << " gets EOD\n";
               break;
            }
         }

         if (m_inited)
         {
            player->dispatch();

            if (playing && player->done(0))
            {
               player->stop(0);
               player->clearScopeQ(0);
               senddone(wfd);
               playing = false;
               if (pmapped)
               {
                  *m_children[m_index].current_time = 0;
                  *m_children[m_index].duration = 0;
               }
            }
            
            if (pmapped)
            {
               *m_children[m_index].current_time = player->where(0);
               *m_children[m_index].duration = player->duration(0);

               HelixSimplePlayer::metaData *md = player->getMetaData(0);
               if (md)
                  memcpy((void *) m_children[m_index].md, (void *) md, sizeof(HelixSimplePlayer::metaData));
               
               struct DelayQueue *item;
               //int j;
               while ((item = player->getScopeBuf(0)))
               {
                  //j = (*m_children[m_index].m_current + 1) % NUM_SCOPEBUFS;
                  
                  //cerr << "player:" << m_index << " j=" << j << " time=" << item->time << " etime=" << item->etime << " len=" << item->len << endl;
                  //m_children[m_index].q[j].len = item->len;
                  //m_children[m_index].q[j].time = item->time;
                  //m_children[m_index].q[j].etime = item->etime;
                  //m_children[m_index].q[j].nchan = item->nchan;
                  //m_children[m_index].q[j].bps = item->bps;
                  //m_children[m_index].q[j].tps = item->tps;
                  //m_children[m_index].q[j].spb = item->spb;
                  //memcpy((void *)m_children[m_index].q[j].buf, (void *) item->buf, item->len );
                  //*m_children[m_index].m_current = j;
                  //cerr << "player:" << m_index << " time=" << item->time << " etime=" << item->etime << endl;
                  sendscopebuf(wfd, item);
                  delete item;
               }
            }
         }
         
         timeout.tv_sec = 0;
         timeout.tv_usec = 10000;
      }
      cerr << "CHILD " << m_index << " will exit!\n";
   }
   else
   {
      int i;
      bool done = false, dead = false;

      sendsetoutputsink();
      sendsetdevice();
      sendinit();

      // wait for ready from children
      while (!done && !dead)
      {
         dispatch();
         done = true;
         for (i=0; i<numPlayers; i++)
         {
            done &= m_children[i].isready;
            dead |= m_children[i].isdead;
         }
      }

      if (dead)
      {
         m_err = -1;
         return;
      }
   }
   m_inited = true;
}


void PlayerControl::setOutputSink( HelixSimplePlayer::AUDIOAPI out )
{
   print2stderr("%%%% In PlayerControl::setOutputSink:%d\n", out);
   m_api = out;
}

void PlayerControl::setDevice( const char *dev )
{
   delete [] m_device;

   int len = strlen(dev);
   m_device = new char [len + 1];
   strcpy(m_device, dev); 
   print2stderr("%%%% In PlayerControl::setDevice:%s\n", dev);
}

int PlayerControl::initDirectSS()
{
   return 0;
}

void PlayerControl::tearDown()
{
   int tmp;
   if (iamparent)
   {
      for (int i = 0; i < nNumPlayers; i++)
      {
         if (m_inited)
         {
            sendteardown(m_children[i].m_pipeB[1]);
            close(m_children[i].m_pipeB[1]);
            close(m_children[i].m_pipeA[0]);
            cerr << "About to waitpid for pid " << m_children[i].m_pid << endl;
            kill(m_children[i].m_pid, SIGTERM);
            waitpid(m_children[i].m_pid, &tmp, 0);
         }
      }
   }
}

void PlayerControl::start(int playerIndex, bool fadein, unsigned long fadetime)
{
   m_children[playerIndex].isplaying = true;
   if (pmapped)
      *m_children[playerIndex].m_consumed = *m_children[playerIndex].m_current = 0;
   sendstart(m_children[playerIndex].m_pipeB[1], fadein, fadetime);
}

int PlayerControl::setURL(const char *url, int playerIndex, bool islocal)
{
   m_children[playerIndex].islocal = islocal;
   if (sendsetURL(m_children[playerIndex].m_pipeB[1], url, islocal))
      return 0;

   return -1;
}

bool PlayerControl::done(int playerIndex)
{
   return (!m_children[playerIndex].isplaying);
}

void PlayerControl::stop(int playerIndex)
{
   if (playerIndex == HelixSimplePlayer::ALL_PLAYERS)
   {
      for (int i=0; i<nNumPlayers; i++)
         stop(i);
   }
   else
   {
      m_children[playerIndex].isplaying = false;
      sendstop(m_children[playerIndex].m_pipeB[1]);
   }
}

void PlayerControl::pause(int playerIndex)
{
   sendpause(m_children[playerIndex].m_pipeB[1]);
}

void PlayerControl::resume(int playerIndex)
{
   sendresume(m_children[playerIndex].m_pipeB[1]);
}

void PlayerControl::seek(unsigned long pos, int playerIndex)
{
   sendmessage(m_children[playerIndex].m_pipeB[1], SEEK, (unsigned char *) &pos, sizeof(unsigned long));
}

unsigned long PlayerControl::where(int playerIndex) const
{
   if (pmapped)
      return *m_children[playerIndex].current_time;
   else
      return 0;
}

unsigned long PlayerControl::duration(int playerIndex) const
{
   if (pmapped)
      return *m_children[playerIndex].duration;
   else
      return 0;
}

unsigned long PlayerControl::getVolume()
{
   return m_volume;
}

void PlayerControl::setVolume(unsigned long vol)
{
   m_volume = vol;
   for (int i = 0; i < nNumPlayers; i++)
      sendsetvolume(m_children[i].m_pipeB[1], vol);
}

void PlayerControl::dispatch()
{
   int i;
   struct timeval timeout;
   int n = -1, ntot;
   int rfd;
   int wfd;

   timeout.tv_sec = 0;
   timeout.tv_usec = 0;
   fd_set rdset, wrset;

   FD_ZERO(&rdset);
   FD_ZERO(&wrset);

   for (i=0; i<nNumPlayers; i++)
   {
      rfd = m_children[i].m_pipeA[0];
      wfd = m_children[i].m_pipeB[1];
      FD_SET(rfd, &rdset);
      FD_SET(wfd, &wrset); // really should check to see if we can write, but not gonna
      if (rfd > n)
         n = rfd;
   }
   
   if (n < 0)
      return;
   
   ntot = select(n + 1, &rdset, 0, 0, &timeout);
   for (i=0; ntot && i < nNumPlayers; i++)
   {
      rfd = m_children[i].m_pipeA[0];
      wfd = m_children[i].m_pipeB[1];
      if ( FD_ISSET(rfd, &rdset) )
      {
         msgid m;
         unsigned char buf[65536];
         int sz = 0;
         
         if (getmessage(rfd, m, buf, sz))
         {
            switch (m)
            {
               case READY:
                  print2stderr("CHILD %d is READY\n", i);
                  m_children[i].isready = true;;
                  break;
                  
               case DONE:
                  print2stderr("CHILD %d is DONE\n", i);
                  if (!sz)
                  {
                     m_children[i].isplaying = false;
                     clearScopeQ(i);
                     play_finished(i);
                  }
                  else
                     print2stderr("PARENT: sz does not agree in DONE\n");
                  break;
                  
               case MIMETYPES:
               {
                  int len, slen;
                  MimeList *entry;
                  char *tmp;
                  char tmpbuf[65536];
                  
                  mimehead = 0;
                  memcpy( (void *) &mimelistlen, (void *) buf, sizeof(mimelistlen) );
                  len = sizeof(mimelistlen);
                  
                  print2stderr("%%%%%%% Received %d mimetypes\n", mimelistlen);
                  for (int j = 0; j < mimelistlen; j++)
                  {
                     tmp = (char *) &buf[len];
                     slen = strlen(tmp);
                     strcpy(tmpbuf, tmp);
                     tmp += slen + 1;
                     len += slen + 1;
                     entry = new MimeList(tmpbuf, tmp);
                     slen = strlen(tmp);
                     len += slen + 1;
                     
                     entry->fwd = mimehead;
                     mimehead = entry;
                  }

                  if (sz != len) // sanity check
                     cerr << "PARENT: sz not = len in MIMETYPES " << sz << " " << len << endl;                  
               }
               break;
               
               case PLUGINS:
               {
                  int len, slen;
                  int nplugins;
                  char *tmp;
                  
                  memcpy( (void *) &nplugins, (void *) buf, sizeof(nplugins) );
                  len = sizeof(nplugins);
                  m_pluginInfo = new pluginInfo* [nplugins];
                  m_numPlugins = nplugins;
                  
                  print2stderr("%%%%%%% Received %d plugins\n", nplugins);
                  for (int j = 0; j < nplugins; j++)
                  {
                     m_pluginInfo[j] = new pluginInfo;
                     
                     tmp = (char *) &buf[len];
                     slen = strlen(tmp);
                     m_pluginInfo[j]->description = new char[ slen + 1 ];
                     strcpy(m_pluginInfo[j]->description, tmp);
                     len += slen + 1;
                     
                     tmp = (char *) &buf[len];
                     slen = strlen(tmp);
                     m_pluginInfo[j]->copyright = new char[ slen + 1 ];
                     strcpy(m_pluginInfo[j]->copyright, tmp);
                     len += slen + 1;
                     
                     tmp = (char *) &buf[len];
                     slen = strlen(tmp);
                     m_pluginInfo[j]->moreinfourl = new char[ slen + 1 ];
                     strcpy(m_pluginInfo[j]->moreinfourl, tmp);
                     len += slen + 1;
                  }

                  if (sz != len) // sanity check
                     cerr << "PARENT: sz not = len in PLUGINS " << sz << " " << len << endl;
               }
               break;
               
               case CONTACTING:
               {
                  int len = strlen((const char *)buf);
                  if (sz == len + 1)
                     onContacting((const char *)buf);
                  else
                     cerr << "PARENT: sz not right in CONTACTING sz=" << sz << endl;
               }
               break;
               
               case BUFFERING:
               {
                  unsigned long percent;
                  if (sz == sizeof(unsigned long))
                  {
                     memcpy((void *)&percent, (void *) buf, sizeof(unsigned long));
                     onBuffering(percent);
                  }
                  else
                     cerr << "PARENT: sz not right in BUFFERING sz=" << sz << endl;                     
               }
               break;
               
               case NOTIFYUSER:
               {
                  int len1, len2;
                  unsigned long code;
                  const char *moreinfo, *moreinfourl;
                  memcpy((void *) &code, (void *) buf, sizeof(unsigned long));
                  moreinfo = (const char *) &buf[sizeof(unsigned long)];
                  len1 = strlen(moreinfo);
                  moreinfourl = (const char *) &moreinfo[ len1 + 1];
                  len2 = strlen(moreinfourl);
                  // sanity check
                  if ((unsigned) sz != sizeof(unsigned long) + len1 + len2 + 2)
                     cerr << "PARENT: sz not right in NOTIFYUSER sz=" << sz << endl;
                  
                  notifyUser(code, moreinfo, moreinfourl);
               }
               break;
               
               case INTERRUPTUSER:
               {
                  int len1, len2;
                  unsigned long code;
                  const char *moreinfo, *moreinfourl;
                  memcpy((void *) &code, (void *) buf, sizeof(unsigned long));
                  moreinfo = (const char *) &buf[sizeof(unsigned long)];
                  len1 = strlen(moreinfo);
                  moreinfourl = (const char *) &moreinfo[ len1 + 1];
                  len2 = strlen(moreinfourl);
                  // sanity check
                  if ((unsigned) sz != sizeof(unsigned long) + len1 + len2 + 2)
                     cerr << "PARENT: sz not right in INTERRUPTUSER sz=" << sz << endl;
                  
                  interruptUser(code, moreinfo, moreinfourl);
               }
               break;
               
               case SCOPEBUF:
               {
                  DelayQueue *item;
                  int bufsz, len = 0;
                  memcpy( (void *) &bufsz, (void *) buf, sizeof(int) ); len += sizeof(int);
                  if ((int) (bufsz + 2 * sizeof(unsigned long) + 4 * sizeof(int) + sizeof(double)) == sz)
                  {
                     item = new DelayQueue(bufsz);
                     memcpy( (void *) &item->time, (void *) &buf[len], sizeof(unsigned long) ); 
                     len += sizeof(unsigned long);
                     memcpy( (void *) &item->etime, (void *) &buf[len], sizeof(unsigned long) ); 
                     len += sizeof(unsigned long);
                     memcpy( (void *) &item->nchan, (void *) &buf[len], sizeof(int) ); len += sizeof(int);
                     memcpy( (void *) &item->bps, (void *) &buf[len], sizeof(int) ); len += sizeof(int);
                     memcpy( (void *) &item->tps, (void *) &buf[len], sizeof(double) ); len += sizeof(double);
                     memcpy( (void *) &item->spb, (void *) &buf[len], sizeof(int) ); len += sizeof(int);
                     memcpy( (void *) item->buf, (void *) &buf[len], item->len ); len += item->len;      

                     addScopeBuf(item, i);
                  }
                  else
                     cerr << "PARENT: sz not right in SCOPEBUF sz=" << sz << endl;
               }
               break;
               default:
                  print2stderr("PARENT recvd unhandled message %d\n", m);
                  break;
            }
         }
         else
         {
            m_children[i].isdead = true;
            return; // should never happen
         }
      }
   }
   
   for (i=0; pmapped && i<nNumPlayers; i++)
   {
      while (*m_children[i].m_consumed != *m_children[i].m_current)
      {
         addScopeBuf(&m_children[i].q[*m_children[i].m_consumed], i);

         //cerr << "j=" << *m_children[i].m_consumed << 
         //   " time=" << m_children[i].q[*m_children[i].m_consumed].time << 
         //   " etime=" << m_children[i].q[*m_children[i].m_consumed].etime << 
         //   " len=" << m_children[i].q[*m_children[i].m_consumed].len << endl;

         *m_children[i].m_consumed = (*m_children[i].m_consumed + 1) % NUM_SCOPEBUFS;
      }
   }
}

void PlayerControl::cleanUpStream(int playerIndex)
{
   stop(playerIndex);
}

bool PlayerControl::isPlaying(int playerIndex) const
{
   return m_children[playerIndex].isplaying;
}

bool PlayerControl::isLocal(int playerIndex) const
{
   return m_children[playerIndex].islocal;
}


void PlayerControl::setFadeout(bool fadeout, unsigned long fadelength, int playerIndex)
{
   sendsetfade(m_children[playerIndex].m_pipeB[1], fadeout, fadelength);
}

void PlayerControl::addScopeBuf(struct DelayQueue *item, int playerIndex) 
{
   if (playerIndex >=0 && playerIndex < nNumPlayers)
   {
      if (m_children[playerIndex].scopebuftail)
      {
         item->fwd = 0;
         m_children[playerIndex].scopebuftail->fwd = item;
         m_children[playerIndex].scopebuftail = item;
         m_children[playerIndex].scopecount++;
      }
      else
      {
         item->fwd = 0;
         m_children[playerIndex].scopebufhead = item;
         m_children[playerIndex].scopebuftail = item;
         m_children[playerIndex].scopecount = 1;
      }
   }
}

DelayQueue *PlayerControl::getScopeBuf(int playerIndex)
{
   if (playerIndex >=0 && playerIndex < nNumPlayers)
   {
      struct DelayQueue *item = m_children[playerIndex].scopebufhead;
   
      if (item)
      {
         m_children[playerIndex].scopebufhead = item->fwd;
         m_children[playerIndex].scopecount--;
         if (!m_children[playerIndex].scopebufhead)
            m_children[playerIndex].scopebuftail = 0;
      }
      return item;
   }
   else
      return 0;
}

int PlayerControl::getScopeCount(int playerIndex)
{
   return (playerIndex >= 0 && playerIndex < nNumPlayers ? m_children[playerIndex].scopecount : 0);
}

int PlayerControl::peekScopeTime(unsigned long &t, int playerIndex)
{
   if (playerIndex >=0 && playerIndex < nNumPlayers)
   {
      if (m_children[playerIndex].scopebufhead)
         t = m_children[playerIndex].scopebufhead->time;
      else
         return -1;
      return 0;
   }
   return -1;
}

void PlayerControl::clearScopeQ(int playerIndex)
{
   if (playerIndex < 0)
   {
      for (int i=0; i<nNumPlayers; i++)
         clearScopeQ(i);
   }
   else
   {
      sendscopeclear(m_children[playerIndex].m_pipeB[1]);
      struct DelayQueue *item;
      while ((item = getScopeBuf(playerIndex)))
         if (item->allocd)
            delete item;
   }
}

void PlayerControl::enableEQ(bool enabled)
{
   int i;
   unsigned char c = (char) enabled;

   for (i=0; i<nNumPlayers; i++)
      sendmessage(m_children[i].m_pipeB[1], ENABLEEQ, (unsigned char *) &c, 1);

   m_eq_enabled = enabled;
}

bool PlayerControl::isEQenabled()
{
   return m_eq_enabled;
}

void PlayerControl::updateEQgains()
{
   sendupdateeqgains();
}

int PlayerControl::numPlugins() const
{
   return m_numPlugins;
}

int PlayerControl::getPluginInfo(int index, const char *&description, const char *&copyright, const char *&moreinfourl) const
{
   if (m_pluginInfo && index < m_numPlugins)
   {
      description = m_pluginInfo[index]->description;
      copyright   = m_pluginInfo[index]->copyright;
      moreinfourl = m_pluginInfo[index]->moreinfourl;
      
      return 0;
   }
   return -1;
}

const MimeList *PlayerControl::getMimeList() const
{
   return mimehead;
}

int PlayerControl::getMimeListLen() const
{
   return mimelistlen;
}

HelixSimplePlayer::metaData *PlayerControl::getMetaData(int playerIndex )
{
   return m_children[playerIndex].md;
}

bool PlayerControl::sendsetoutputsink()
{
   int i;
   char c = (char) m_api;
   bool ok = false;

   for (i=0; i<nNumPlayers; i++)
      ok |= sendmessage(m_children[i].m_pipeB[1], OUTPUTSINK, (unsigned char *) &c, 1); 

   return ok;
}

bool PlayerControl::sendsetdevice()
{
   if (!m_device)
      return false;

   int i, len = strlen( m_device );
   bool ok = false;

   for (i=0; i<nNumPlayers; i++)
      ok |= sendmessage(m_children[i].m_pipeB[1], DEVICE, (unsigned char *) m_device, len + 1);

   return ok;
}

bool PlayerControl::sendinit()
{
   int i;
   bool ok = false;

   for (i=0; i<nNumPlayers; i++)
      ok |= sendrequest(m_children[i].m_pipeB[1], INIT);

   return ok;
}

bool PlayerControl::sendupdateeqgains()
{
   unsigned char buf[ 65535 ];
   int bandGain;
   uint i;
   bool ok = false;

   memcpy((void *) buf, (void *) &m_preamp, sizeof(m_preamp));
   i = m_equalizerGains.size();
   memcpy( (void *) &buf[ sizeof(m_preamp) ], (void *) &i, sizeof(int) );
   for ( i = 0; i < m_equalizerGains.size(); i++ )
   {
      bandGain = m_equalizerGains[i];
      memcpy((void *)&buf[ sizeof(m_preamp) + (i+1) * sizeof(int) ], (void *) &bandGain, sizeof(int));
   }
   for ( i = 0; i < (uint) nNumPlayers; i++ )
      ok |= sendmessage(m_children[i].m_pipeB[1], UPDATEEQGAINS, buf, sizeof(m_preamp) + (m_equalizerGains.size()+1) * sizeof(int));

   return ok;
}

// children send this!
bool PlayerControl::sendnotifyuser(unsigned long code, const char *moreinfo, const char *moreinfourl)
{
   int len1 = strlen(moreinfo), len2 = strlen(moreinfourl), len;
   unsigned char buf[65536];

   memcpy( (void *) buf, (void *) &code, sizeof(unsigned long) );
   len = sizeof(unsigned long);
   memcpy( (void *) &buf[ len ], (void *) moreinfo, len1 + 1);
   len += len1 + 1;
   memcpy( (void *) &buf[ len ], (void *) moreinfourl, len2 + 1);
   len += len2 + 1;

   return (sendmessage(m_children[m_index].m_pipeA[1], NOTIFYUSER, buf, len));
}

// children send this!
bool PlayerControl::sendinterruptuser(unsigned long code, const char *moreinfo, const char *moreinfourl)
{
   int len1 = strlen(moreinfo), len2 = strlen(moreinfourl), len;
   unsigned char buf[65536];

   memcpy( (void *) buf, (void *) &code, sizeof(unsigned long) );
   len = sizeof(unsigned long);
   memcpy( (void *) &buf[ len ], (void *) moreinfo, len1 + 1);
   len += len1 + 1;
   memcpy( (void *) &buf[ len ], (void *) moreinfourl, len2 + 1);
   len += len2 + 1;

   return (sendmessage(m_children[m_index].m_pipeA[1], INTERRUPTUSER, buf, len));
}

// children send this!
bool PlayerControl::sendcontacting(const char *host)
{
   int len = strlen(host);

   return (sendmessage(m_children[m_index].m_pipeA[1], CONTACTING, (unsigned char *) host, len + 1));
}

// children send this!
bool PlayerControl::sendbuffering(int percentage)
{
   return (sendmessage(m_children[m_index].m_pipeA[1], BUFFERING, (unsigned char *) &percentage, sizeof(unsigned long)));
}


///////////// statics //////////////

bool PlayerControl::getmessage(int fd, msgid &m, unsigned char *buf, int &sz)
{
   int nbytes = 0, bytes = 0;
   unsigned char mm;

   bytes = read(fd, (void *) &mm, 1);
   if (bytes <= 0)
      return false;

   m = (msgid) mm;

   nbytes = bytes;

   nbytes = 0;
   char *tmp = (char *) &sz;
   while ( bytes > 0 && 4 != nbytes )
   {
      bytes = read(fd, (void *) &tmp[ nbytes ], 4 - nbytes);
      nbytes += bytes;
   }

   if (sz)
   {
      nbytes = 0;
      while ( bytes > 0 && sz != nbytes )
      {
         bytes = read(fd, (void *) &buf[ nbytes ], sz - nbytes);
         nbytes += bytes;
      }
   }

   return (nbytes > 0);
}

bool PlayerControl::sendmessage(int fd, msgid m, unsigned char *buf, int sz)
{
   unsigned char hdr[5];
   hdr[0] = (unsigned char) m;
   int ret = 0;

   memcpy(&hdr[1], (void *) &sz, 4);
   ret = write(fd, (void *) hdr, 5);
   if (sz)
      ret += write(fd, (void *) buf, sz);

   return (ret == (sz + 5));
}

bool PlayerControl::sendsetURL(int fd, const char *url, bool islocal)
{
   int len = strlen(url);
   unsigned char* buf = new unsigned char[ len + 2 ];

   buf[0] = (unsigned char) islocal;
   memcpy((void *) &buf[1], (void *) url, len + 1);  // go ahead and send the null, what the hell
   bool r = sendmessage(fd, SETURL, (unsigned char *)buf, len + 2);
   delete [] buf;
   return r;
}

bool PlayerControl::sendstart(int fd, bool fadin, unsigned long fadetime)
{
   unsigned char buf[32];

   buf[0] = (unsigned char) fadin;
   memcpy( (void *) &buf[1], (void *) &fadetime, sizeof(unsigned long) );

   return sendmessage(fd, START, buf, sizeof(unsigned long) + 1);
}

bool PlayerControl::sendsetvolume(int fd, unsigned long volume)
{
   return sendmessage(fd, SETVOLUME, (unsigned char *) &volume, sizeof(unsigned long));
}

bool PlayerControl::sendvolume(int fd, unsigned long volume)
{
   return sendmessage(fd, VOLUME, (unsigned char *) &volume, sizeof(unsigned long));
}

bool PlayerControl::sendsetfade(int fd, bool fadeout, unsigned long fadelength)
{
   unsigned char buf[ sizeof(bool) + sizeof(unsigned long) ];

   buf[0] = (char )fadeout;
   memcpy((void *) &buf[1], (void *) &fadelength, sizeof(unsigned long));
   return sendmessage(fd, SETFADE, buf, 1 + sizeof(unsigned long));
}

bool PlayerControl::sendplugins(int fd, HelixSimplePlayer *player)
{
   unsigned char buf[65536];
   int sz, slen;
   int nplugins = player->numPlugins();
   const char *description, *copyright, *moreinfourl;

   memcpy( (void *) buf, (void *) &nplugins, sizeof(nplugins) );
   sz = sizeof(nplugins);

   for (int i = 0; i < nplugins; i++)
   {
      player->getPluginInfo(i, description, copyright, moreinfourl);

      slen = strlen(description);
      memcpy( (void *) &buf[sz], (void *) description, slen + 1 ); // expecting the null      
      sz += slen + 1;
      slen = strlen(copyright);
      memcpy( (void *) &buf[sz], (void *) copyright, slen + 1 ); // expecting the null      
      sz += slen + 1;
      slen = strlen(moreinfourl);
      memcpy( (void *) &buf[sz], (void *) moreinfourl, slen + 1 ); // expecting the null      
      sz += slen + 1;
   }

   cerr << "CHILD: nplugins " << nplugins << " sz " << sz << endl;
   return sendmessage(fd, PLUGINS, buf, sz);
}

bool PlayerControl::sendmimetypes(int fd, HelixSimplePlayer *player)
{
   unsigned char buf[65536];
   int sz, slen;
   int mimelistlen = player->getMimeListLen();
   const MimeList *mimelisthead = player->getMimeList();

   memcpy( (void *) buf, (void *) &mimelistlen, sizeof(mimelistlen) );
   sz = sizeof(mimelistlen);

   while (mimelisthead)
   {
      slen = strlen(mimelisthead->mimetypes);
      memcpy( (void *) &buf[sz], (void *) mimelisthead->mimetypes, slen + 1 ); // expecting the null      
      sz += slen + 1;
      slen = strlen(mimelisthead->mimeexts);
      memcpy( (void *) &buf[sz], (void *) mimelisthead->mimeexts, slen + 1 ); // expecting the null      
      sz += slen + 1;

      mimelisthead = mimelisthead->fwd;
   }

   return sendmessage(fd, MIMETYPES, buf, sz);
}

bool PlayerControl::sendscopebuf(int fd, DelayQueue *item)
{
   unsigned char buf[65536];
   int len = 0;

   if (len + 2 * sizeof(unsigned long) + 4 * sizeof(int) + sizeof(double) < 65536)
   {
      memcpy( (void *) buf, (void *) &item->len, sizeof(int) ); len += sizeof(int);
      memcpy( (void *) &buf[len], (void *) &item->time, sizeof(unsigned long) ); len += sizeof(unsigned long);
      memcpy( (void *) &buf[len], (void *) &item->etime, sizeof(unsigned long) ); len += sizeof(unsigned long);
      memcpy( (void *) &buf[len], (void *) &item->nchan, sizeof(int) ); len += sizeof(int);
      memcpy( (void *) &buf[len], (void *) &item->bps, sizeof(int) ); len += sizeof(int);
      memcpy( (void *) &buf[len], (void *) &item->tps, sizeof(double) ); len += sizeof(double);
      memcpy( (void *) &buf[len], (void *) &item->spb, sizeof(int) ); len += sizeof(int);
      memcpy( (void *) &buf[len], (void *) item->buf, item->len ); len += item->len;      

      return sendmessage(fd, SCOPEBUF, buf, len);
   }
   return false;
}
