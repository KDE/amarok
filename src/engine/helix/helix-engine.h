/***************************************************************************
 *   Copyright (C) 2005 Paul Cifarelli <paulc2@optonline.net>              *
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
#include <qthread.h>
#include <qobject.h>
#include <sys/types.h>
#include <helix-sp.h>

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

protected:
   virtual void setVolumeSW( uint );

private:
   Engine::State m_state;
   KURL          m_url;

   QCString      m_coredir;
   QCString      m_pluginsdir;
   QCString      m_codecsdir;

   int           m_numPlayers;
   int           m_current;  // the current player
   int           m_xfadeLength;
   
   void timerEvent( QTimerEvent * );

   friend class HelixConfigDialog;
};


#endif
