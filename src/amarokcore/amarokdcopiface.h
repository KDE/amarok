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

///////////////////////////////////////////////////////////////////////
// WARNING! Please ask on #amarok before modifying the DCOP interface!
///////////////////////////////////////////////////////////////////////

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
   virtual void enableRandomMode(bool enable) = 0;          ///< Switch Random Mode on or off.

   /* These two methods are used by mediacontrol. DO NOT TOUCH! */
   virtual void addMedia(const KURL &) = 0;                 ///< Add audio media specified by the url.
   virtual void addMediaList(const KURL::List &) = 0;       ///< Add some audio media specified by the url.

   /* These two methods return raw time in seconds, this is useful for apps like mediacontrol, which calculate
      track lengths and positions in seconds. DO NOT TOUCH! */
   virtual int  trackTotalTime() = 0;                       ///< Return track length in seconds.
   virtual int  trackCurrentTime() = 0;                     ///< Return current play position in seconds.

   virtual bool isPlaying() = 0;                            ///< DEPRECATED Return true if something is playing now.
   virtual int  status() = 0;                               ///< Return playback status: 0 - stopped, 1 - paused, 2 - playing. < 0 - error

   /* Compatibility method (used by mediacontrol as well). DO NOT TOUCH! */
   virtual QString nowPlaying() = 0;                        ///< The title of now playing media.

   /* New player API */
   virtual QString artist() = 0;                            ///< Return the artist of the currently playing track.
   virtual QString title() = 0;                             ///< Return the title of the currently playing track.
   virtual QString album() = 0;                             ///< Return the album of the currently playing track.
   virtual QString totalTime() = 0;                         ///< Return the total length of the currently playing track ([h:]mm:ss format).
   virtual QString currentTime() = 0;                       ///< Return the position of the currently playing track ([h:]mm:ss format).
   virtual QString genre() = 0;                             ///< Return the genre of the currently playing track.
   virtual QString year() = 0;                              ///< Return the year of the currently playing track.
   virtual QString comment() = 0;                           ///< Return the comment of the currently playing track.
   virtual QString bitrate() = 0;                           ///< Return the bitrate of the currently playing track (XX kbps).

   virtual QString encodedURL() = 0;                        ///< Return the encoded URL of the currently playing track.
   virtual QString coverImage() = 0;                        ///< Return the encoded URL of the current track's cover image

   virtual void setVolume(int volume) = 0;                  ///< Set volume in range 0-100%.
   virtual int  getVolume() = 0;                            ///< Return volume in range 0-100%.
   virtual void volumeUp() = 0;                             ///< Increase volume by a reasonable step.
   virtual void volumeDown() = 0;                           ///< Decrease volume by a reasonable step.
   virtual void mute() = 0;                                 ///< Toggle mute.
   virtual void enableOSD(bool enable) = 0;                 ///< Switch OSD display on or off.

   virtual void togglePlaylist() = 0;                       ///< Toggle the Playlist-window
};

#endif
