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

// TODO Split this interface up for amaroK 2.0


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
   virtual void seekRelative(int s) = 0;                    ///< Seek to a position relative to the current track position.
   virtual void enableRandomMode(bool enable) = 0;          ///< Switch Random Mode on or off.

   /* These two methods are used by mediacontrol. DO NOT TOUCH! */
   virtual void addMedia(const KURL &) = 0;                 ///< Add audio media specified by the url.
   virtual void addMediaList(const KURL::List &) = 0;       ///< Add some audio media specified by the url.

   virtual void scanCollection() = 0;                       ///< Scan the collection.

   virtual void clearPlaylist() = 0;                        ///< Clears the playlist.
   virtual void shufflePlaylist() = 0;                      ///< Shuffles the playlist.
   virtual void saveCurrentPlaylist() = 0;                  ///< Saves the current playlist to current.xml

   /* These two methods return raw time in seconds, this is useful for apps like mediacontrol, which calculate
      track lengths and positions in seconds. DO NOT TOUCH! */
   virtual int  trackTotalTime() = 0;                       ///< Return track length in seconds.
   virtual int  trackCurrentTime() = 0;                     ///< Return current play position in seconds.

   virtual bool isPlaying() = 0;                            ///< Return true if something is playing now.
   virtual int  status() = 0;                               ///< Return playback status: 0 - stopped, 1 - paused, 2 - playing. < 0 - error
   virtual bool repeatTrackStatus() = 0;                    ///< Return repeat track status.
   virtual bool repeatPlaylistStatus() = 0;                 ///< Return repeat playlist status.
   virtual bool randomModeStatus() = 0;                     ///< Return random mode status.

   /* Compatibility method (used by mediacontrol as well). DO NOT TOUCH! */
   virtual QString nowPlaying() = 0;                        ///< The title of now playing media.

   /* New player API */
   virtual QString artist() = 0;                            ///< Return the artist of the currently playing track.
   virtual QString title() = 0;                             ///< Return the title of the currently playing track.
   virtual QString track() = 0;                             ///< Return the track number.
   virtual QString album() = 0;                             ///< Return the album of the currently playing track.
   virtual QString totalTime() = 0;                         ///< Return the total length of the currently playing track ([h:]mm:ss format).
   virtual QString currentTime() = 0;                       ///< Return the position of the currently playing track ([h:]mm:ss format).
   virtual QString genre() = 0;                             ///< Return the genre of the currently playing track.
   virtual QString year() = 0;                              ///< Return the year of the currently playing track.
   virtual QString comment() = 0;                           ///< Return the comment of the currently playing track.
   virtual QString bitrate() = 0;                           ///< Return the bitrate of the currently playing track (XX kbps).
   virtual int sampleRate() = 0;                            ///< Return the sample rate of the currently playing track.
   virtual QString encodedURL() = 0;                        ///< Return the encoded URL of the currently playing track.
   virtual QString coverImage() = 0;                        ///< Return the encoded URL of the current track's cover image

   virtual void setVolume(int volume) = 0;                  ///< Set volume in range 0-100%.
   virtual int  getVolume() = 0;                            ///< Return volume in range 0-100%.
   virtual void volumeUp() = 0;                             ///< Increase volume by a reasonable step.
   virtual void volumeDown() = 0;                           ///< Decrease volume by a reasonable step.
   virtual void mute() = 0;                                 ///< Toggle mute.
   virtual void setEqualizerEnabled( bool active ) = 0;     ///< Toggle equalizer.
   virtual void configEqualizer() = 0;                      ///< Toggle equalizer config dialog.
   virtual void enableOSD(bool enable) = 0;                 ///< Switch OSD display on or off.
   virtual void showOSD() = 0;                              ///< Show the OSD display on the screen.

   virtual void togglePlaylist() = 0;                       ///< Toggle the Playlist-window
   virtual int  score() = 0;                                ///< Return the score of the currently playing track.
   virtual void playMedia(const KURL &) = 0;                ///< Add audio media specified by the url.
   virtual void shortStatusMessage(const QString&) = 0;     ///< Shows a temporary message on the statusbar
};

#endif
