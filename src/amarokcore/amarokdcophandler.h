/***************************************************************************
                          amarokdcophandler.h  -  DCOP Implementation
                             -------------------
    begin                : Sat Oct 11 2003
    copyright            : (C) 2003 by Stanislav Karchebny
    email                : berkus@users.sf.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_DCOP_HANDLER_H
#define AMAROK_DCOP_HANDLER_H

#include <qobject.h>
#include "amarokdcopiface.h"

namespace amaroK
{

class DcopHandler : public QObject, virtual public AmarokIface
{
      Q_OBJECT

   public:
      DcopHandler();

   public /* DCOP */ slots:
      virtual void play();
      virtual void playPause();
      virtual void stop();
      virtual void next();
      virtual void prev();
      virtual void pause();
      virtual void seek(int s);
      virtual void enableRandomMode(bool enable);
      virtual int  trackTotalTime();
      virtual int  trackCurrentTime();
      virtual void addMedia(const KURL &);
      virtual void addMediaList(const KURL::List &);
      virtual void scanCollection();
      virtual void clearPlaylist();
      virtual QString nowPlaying();
      virtual QString artist();
      virtual QString title();
      virtual QString track();
      virtual QString album();
      virtual QString totalTime();
      virtual QString currentTime();
      virtual QString genre();
      virtual QString year();
      virtual QString comment();
      virtual QString bitrate();
      virtual int sampleRate();
      virtual QString encodedURL();
      virtual QString coverImage();
      virtual bool isPlaying();
      virtual int  status();
      virtual void setVolume(int);
      virtual int  getVolume();
      virtual void volumeUp();
      virtual void volumeDown();
      virtual void mute();
      virtual void setEqualizerEnabled( bool active );
      virtual void configEqualizer();
      virtual void enableOSD(bool enable);
      virtual void showOSD();
      virtual void togglePlaylist();
      virtual int score ();
      virtual void playMedia(const KURL &);
   private:
};

}

#endif
