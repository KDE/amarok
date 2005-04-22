#ifndef _HXSPLAY_H_INCLUDED_
#define _HXSPLAY_H_INCLUDED_

#include <qthread.h>
#include <helix-sp.h>

class HXSplay : public HelixSimplePlayer
{
public:
   enum pthr_states { STOP, PAUSE, PLAY };

   HXSplay();
   virtual ~HXSplay();

   int getError() const { return m_Err; }
   void init(const char *corelibpath, 
             const char *pluginslibpath, 
             const char *codecspath, 
             int numPlayers = 1);
   int  addPlayer();                                       // add another player
   void play(int playerIndex = 0);                         // play the current url, waiting for it to finish
   int  setURL(const char *file, int playerIndex = 0);     // set the current url
   void stop(int playerIndex = 0);                         // stop the player(s)
   void pause(int playerIndex = 0);                        // pause the player(s)
   void resume(int playerIndex = 0);                       // resume the player(s)

   pthr_states state(int playerIndex = 0);                 // returns the state of the player

protected:
   virtual void play_finished(int playerIndex) {}

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
   void      *handle;
   int        m_Err;
   int        m_numPlayers;

   void startPlayer(int playerIndex = ALL_PLAYERS);                  // start playing the current url
   void stopPlayer(int playerIndex = ALL_PLAYERS);                   // start playing the current url
   void pausePlayer(int playerIndex = ALL_PLAYERS);                  // pause the player(s)
   void resumePlayer(int playerIndex = ALL_PLAYERS);                 // pause the player(s)

};



#endif
