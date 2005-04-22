#include <iostream>
#include <string>
#include <dlfcn.h>
#include <sys/param.h>

#include <qthread.h>
#include <qmutex.h>

#include "hxsplay.h"

using namespace std;

HXSplay::PlayerPkg::PlayerPkg(HXSplay *player, int playerIndex) : 
   m_player(player),
   m_playerIndex(playerIndex),
   m_playthreadid(0),
   m_pt_state(HXSplay::STOP)
{
}


HXSplay::HXSplay() : handle(0),m_Err(0),m_numPlayers(0)
{
}

HXSplay::~HXSplay()
{
   int i;
   for (i=0; i<m_numPlayers; i++)
      stop(i);
}


void HXSplay::init(const char *corelibpath, 
                   const char *pluginslibpath, 
                   const char *codecspath, 
                   int numPlayers)
{
   char tmp[MAXPATHLEN];

   m_Err =0;

   if (!m_Err)
   {
      m_numPlayers = numPlayers;
      HelixSimplePlayer::init(corelibpath, pluginslibpath, codecspath, numPlayers);
      
      int i;
      
      for (i=0; i<m_numPlayers; i++)
         m_playerPkg[i] = new PlayerPkg(this, i);
   }
}

int HXSplay::addPlayer()
{
   if (!m_Err)
   {
      if (m_numPlayers < MAX_PLAYERS)
      {
         m_playerPkg[m_numPlayers] = new PlayerPkg(this, m_numPlayers);
         m_numPlayers++;
         return HelixSimplePlayer::addPlayer();
      }
   }

   return -1;
}

void HXSplay::PlayerPkg::run()
{
   HXSplay *splay = m_player;
   int playerIndex = m_playerIndex;
   HXSplay::pthr_states state = HXSplay::PLAY;

   cerr << "Play thread started for player " << playerIndex << endl;
   splay->startPlayer(playerIndex);
   while (!splay->done(playerIndex))
   {
      usleep(10000);
      m_ptm.lock();  // this mutex is probably overkill, since we only read state
      switch (m_pt_state)
      {
         case HXSplay::STOP:
            if (state != HXSplay::STOP) // pretty much has to be true
               splay->stopPlayer(playerIndex);
            cerr << "Play thread was stopped for player " << playerIndex << endl;
            m_ptm.unlock();
            return;

         case HXSplay::PAUSE:
            if (state == HXSplay::PLAY)
               splay->pausePlayer(playerIndex);
            state = HXSplay::PAUSE;
            break;

         case HXSplay::PLAY:
            if (state == HXSplay::PAUSE)
               splay->resumePlayer(playerIndex);
            state = HXSplay::PLAY;
            break;
      }
      m_ptm.unlock();
      splay->dispatch();
   }
   m_ptm.lock();
   splay->stopPlayer(playerIndex);
   m_pt_state = HXSplay::STOP;
   m_ptm.unlock();

   splay->play_finished(playerIndex);
   cerr << "Play thread finished for player " << playerIndex << endl;
   return;
}


void HXSplay::play(int playerIndex)
{
   PlayerPkg *pkg = m_playerPkg[playerIndex];

   if (playerIndex < m_numPlayers)
   {
      pkg->m_ptm.lock();
      if (!m_Err && pkg->m_pt_state == HXSplay::STOP )
      {
         pkg->m_pt_state = HXSplay::PLAY;
         pkg->start();
      }
      pkg->m_ptm.unlock();
   }
}

HXSplay::pthr_states HXSplay::state(int playerIndex)
{
   return m_playerPkg[playerIndex]->m_pt_state;
}

void HXSplay::stop(int playerIndex)
{
   PlayerPkg *pkg = m_playerPkg[playerIndex];
   HXSplay::pthr_states state;

   pkg->m_ptm.lock();
   state = pkg->m_pt_state;
   pkg->m_pt_state = STOP;
   pkg->m_ptm.unlock();

   if (state != HXSplay::STOP)
      pkg->wait();
}

void HXSplay::pause(int playerIndex)
{
   PlayerPkg *pkg = m_playerPkg[playerIndex];

   pkg->m_ptm.lock();
   if (pkg->m_pt_state == HXSplay::PLAY)
      pkg->m_pt_state = HXSplay::PAUSE;
   pkg->m_ptm.unlock();
}

void HXSplay::resume(int playerIndex)
{
   PlayerPkg *pkg = m_playerPkg[playerIndex];

   pkg->m_ptm.lock();
   if (pkg->m_pt_state == HXSplay::PAUSE)
      pkg->m_pt_state = HXSplay::PLAY;
   pkg->m_ptm.unlock();
}


void HXSplay::startPlayer(int playerIndex)
{
   HelixSimplePlayer::start(playerIndex);
}

int HXSplay::setURL(const char *file, int playerIndex)
{
   return HelixSimplePlayer::setURL(file, playerIndex);
}

void HXSplay::stopPlayer(int playerIndex)
{
   HelixSimplePlayer::stop(playerIndex);
}

void HXSplay::pausePlayer(int playerIndex)
{
   HelixSimplePlayer::pause(playerIndex);
}

void HXSplay::resumePlayer(int playerIndex)
{
   HelixSimplePlayer::resume(playerIndex);
}


#include "hxsplay.moc"
