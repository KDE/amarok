#ifndef _HXSPLAY_H_INCLUDED_
#define _HXSPLAY_H_INCLUDED_

#include <qthread.h>
#include <qmutex.h>
#include <helix-sp.h>

class HXSplay : public HelixSimplePlayer
{
public:
   enum pthr_states { STOP, PAUSE, PLAY };

   HXSplay(int &);
   virtual ~HXSplay();

   int getError() const { return m_Err; }
   void init(const char *corelibpath, 
             const char *pluginslibpath, 
             const char *codecspath);
   void play();                         // play the current url, waiting for it to finish
   int  setURL(const char *file);       // set the current url
   void stop();                         // stop the player
   void pause();                        // pause the player
   void resume();                       // resume the player
   void seek(unsigned long ms);         // seek player
   unsigned long where() const;         // return where we are in the timeline of the current player
   unsigned long duration() const;      // get the duration of the current player
   pthr_states state() const;           // returns the state of the player

protected:
   virtual void play_finished(int) {}

   // play thread
   class PlayerPkg : public QThread
   {
   public:
      PlayerPkg(HXSplay *player, int playerIndex);
      HXSplay              *m_player;
      int                   m_playerIndex;
      pthread_t             m_playthreadid;
      HXSplay::pthr_states  m_pt_state;
      QMutex                m_ptm;
   private:
      virtual void run();
   } *m_playerPkg[MAX_PLAYERS];


private:
   HXSplay();
   void      *handle;
   int        m_Err;
   int        m_numPlayers;
   int        m_current;
   int       &m_xfadeLength;

   int  addPlayer();                                       // add another player
   void stop(int playerIndex);                             // stop the player(s)

   void startPlayer(int playerIndex = ALL_PLAYERS);        // start playing the current url
   void stopPlayer(int playerIndex = ALL_PLAYERS);         // start playing the current url
   void pausePlayer(int playerIndex = ALL_PLAYERS);        // pause the player(s)
   void resumePlayer(int playerIndex = ALL_PLAYERS);       // pause the player(s)

};



#endif
