/***************************************************************************
                          amarokdcopiface.h  -  DCOP Interface
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

#ifndef AMAROK_DCOPIFACE_H
#define AMAROK_DCOPIFACE_H

#include <kurl.h>
#include <dcopobject.h>

class AmarokIface : virtual public DCOPObject
{
   K_DCOP

k_dcop:
   virtual void play() = 0;                                 ///< Equivalent to pressing "Play" button.
   virtual void playPause() = 0;                            ///< Toggle play/pause state (good for mm keyboard users)
   virtual void stop() = 0;                                 ///< Equivalent to pressing "Stop" button.
   virtual void next() = 0;                                 ///< Equivalent to pressing "Next" button.
   virtual void prev() = 0;                                 ///< Equivalent to pressing "Prev" button.
   virtual void pause() = 0;                                ///< Equivalent to pressing "Pause" button.
   virtual void seek(int s) = 0;                            ///< Seek track to seconds position.
   virtual int  trackTotalTime() = 0;                       ///< Return track length in seconds.
   virtual int  trackCurrentTime() = 0;                     ///< Return current play position in seconds.
   virtual void addMedia(const KURL &) = 0;                 ///< Add audio media specified by the url.
   virtual void addMediaList(const KURL::List &) = 0;       ///< Add some audio media specified by the url.
   virtual QString nowPlaying() = 0;                        ///< The title of now playing media.
   virtual bool isPlaying() = 0;                            ///< Return true if something is playing now.
   virtual void setVolume(int volume) = 0;                  ///< Set volume in range 0-100%.
   virtual void volumeUp() = 0;                             ///< Increase volume by a reasonable step.
   virtual void volumeDown() = 0;                           ///< Decrease volume by a reasonable step.
   virtual void enableOSD(bool enable) = 0;                 ///< Switch OSD display on or off
};

#endif
