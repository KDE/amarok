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
   virtual void addMedia(const KURL &) = 0;                 ///< Add audio media specified by the url.
   virtual void addMediaList(const KURL::List &) = 0;       ///< Add some audio media specified by the url.
   virtual QString PrettyTitle() = 0;			    ///< Return the artist/title of the currently playing track
   virtual QString Artist() = 0;			    ///< Return the artist of the currently playing track
   virtual QString Title() = 0;		                    ///< Return the title of the currently playing track
   virtual QString Album() = 0;		        	    ///< Return the album of the currently playing track
   virtual QString TotalTime() = 0;		            ///< Return the total length of the currently playing track (format mm:ss)
   virtual QString Position() = 0;		            ///< Return the position of the currently playing track (format mm:ss)
   virtual QString Genre() = 0;		                    ///< Return the genre of the currently playing track
   virtual QString Year() = 0;			            ///< Return the year of the currently playing track
   virtual QString Comment() = 0;		            ///< Return the comment of the currently playing track
   virtual QString Bitrate() = 0;		            ///< Return the bitrate of the currently playing track
   virtual bool isPlaying() = 0;                            ///< Return true if something is playing now.
   virtual void setVolume(int volume) = 0;                  ///< Set volume in range 0-100%.
   virtual void volumeUp() = 0;                             ///< Increase volume by a reasonable step.
   virtual void volumeDown() = 0;                           ///< Decrease volume by a reasonable step.
   virtual void enableOSD(bool enable) = 0;                 ///< Switch OSD display on or off
};

#endif
