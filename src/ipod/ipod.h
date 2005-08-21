/***************************************************************************
 *   Copyright (C) 2004 by Michael Schulze                                 *
 *   mike.s@genion.de                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef IPOD_H
#define IPOD_H

#include <qstring.h>

#include "itunesdb.h"
#include "trackmetadata.h"

#define LOGFILEPREFIX "/kio_ipod-"
#define LOG_DEFAULT true

/**
This class represents all functionalities to access and modify information about the iPod/itunesDB

@author Michael Schulze
*/
class IPod {
public:
    enum LogActionType {
        ACT_ADD_PLAYLIST= 0,
        ACT_REM_PLAYLIST,
        ACT_RENAME_PLAYLIST,
        ACT_REM_ALBUM,
        ACT_RENAME_ALBUM,
        ACT_ADD_TO_PLAYLIST,
        ACT_REM_FROM_PLAYLIST,
        ACT_ADD_TRACK,
        ACT_MOV_TRACK,
        ACT_REM_TRACK,
        ACT_DELETE_ARTIST,
        NUM_ACTIONS
    };

    enum IPodError {
        Err_None = 0,
        Err_ParseError,
        Err_NotOpen,
        Err_AlreadyExists,
        Err_DoesNotExist,
        Err_NotEmpty,
        Err_Internal
    };

    IPod();
    virtual ~IPod();

    /**
     * Tries to open an ipod. That means reading all important information from the device.
     * @return true if successful, otherwise false
     */
    bool open(const QString& ipod_base);

    /**
     * Returns true if open() was called successfully for this instance.
     * @return true if open() was called successfully for this instance.
     */
    bool isOpen();

    /**
     * Returns true if the ipod represented by this instance is still connected to the system
     * @return true if the ipod represented by this instance is still connected to the system false otherwise
     */
    bool isStillConnected();

    /**
     * Clears all data we got from the IPod whether or not it was changed.
     * Resets the state of this instance.
     */
    void close();

    /**
     * Makes sure that data is consistent between instances opened on the same IPod and the IPod is still available
     */
    bool ensureConsistency();

    /**
     * removes an empty artist
     */
    IPodError deleteArtist(const QString& artistname, bool log = true);

    /**
     * Renames an album with the given title and artist to newtitle, newartist
     * @return AlreadyExists if an album with the given newtitle already exists or DoesNotExist if an album with the given title doesn't exist
     */
    IPodError renameAlbum(const QString& artistname, const QString& title, const QString& newartistname, const QString& newtitle, bool log = LOG_DEFAULT);

    /**
     * removes an album and all it's contents from the database
     */
    IPodError deleteAlbum(const QString& artistname, const QString& title, bool log = LOG_DEFAULT);

    /**
     * Creates a new playlist with the given title
     * @return Err_AlreadyExists if a playlist with the given title already exists
     */
    IPodError createPlaylist(const QString& playlisttitle, bool log = LOG_DEFAULT);

    /**
     * Renames the playlist with the given title to newtitle.
     * @return AlreadyExists if a playlist with the given newtitle already exists or DoesNotExist if a playlist with the given title doesn't exist
     */
    IPodError renamePlaylist(const QString& title, const QString& newtitle, bool log = LOG_DEFAULT);

    /**
     * deletes the playlist with the given name from the database and frees the memory resources
     * ATTENTION: pointers to the playlist with the given name will become invalid
     * @return DoesNotExist if a playlisz with the given title does not exist
     */
    IPodError deletePlaylist(const QString& title, bool log = LOG_DEFAULT);

    /**
     * Removes the Track with the given trackid from the database.
     * This also removes all playlist references to this track
     * @param trackid ID of the track to be deleted
     * @return the trackid of the Track or 0 if no such Track exists
     */
    IPodError deleteTrack(Q_UINT32 trackid, bool log = LOG_DEFAULT);

    /**
     * moves the given track from one album to another.
     * This method may be not accessible in the future when/if track::setAlbum() track::setArtist() work correctly
     */
    IPodError moveTrack(TrackMetadata& track, const QString& newartist, const QString& newalbum, bool log = LOG_DEFAULT);

    /**
     * Fills the given QStringList with the names of all playlists in the database
     * @return a pointer to the given StringList
     */
    QStringList * getPlaylistTitles(QStringList &buffer);

    /**
     * Returns a playlist by the given playlist title.
     * @param playlisttitle title of the playlist in question
     * @return a pointer to the playlist found or NULL if no such playlist exists
     */
    TrackList * getPlaylistByTitle(const QString& title) const;

    /**
     * Fills the given QStringList with the names of all artists in the database
     * @return a pointer to the StringList
     */
    QStringList * getArtists( QStringList &buffer) const;

    /**
     * Returns an Artist by the given name
     * @param artist the artists name
     * @return an Artist by the given name or NULL if the artist could not be found by the given name
     */
    Artist * getArtistByName( const QString &artist) const;

    /**
     * Returns a new TrackMetadata instance for adding a new track to the database.
     * The new instance gets initialized with an unused trackid and trackpath.
     * Subsequent calls will get TrackMetadata with the same trackid until the returned track is as added
     * via addTrack() thus a call to addTrack() should be done afterwards.
     */
    TrackMetadata createNewTrackMetadata();

    /**
     * adds a new Track to the database after itunesDBlistener is done (finished)
     */
    void addTrack(const TrackMetadata& track, bool log = LOG_DEFAULT);

    /**
     * Adds the track with the given trackid to the playlist with the given title.
     */
    IPodError addTrackToPlaylist(const TrackMetadata& track, const QString& playlisttitle, bool log = LOG_DEFAULT);

    /**
     * Removes the track at the given position from the given playlist
     */
    IPodError removeFromPlaylist(Q_UINT32 position, const QString& playlisttitle, bool log = LOG_DEFAULT);

    /**
     * returns the Track corresponding to the given ID
     * @param id ID of the track
     * @return the Track corresponding to the given ID
     */
    TrackMetadata * getTrackByID( const Q_UINT32 id) const;

    /**
     * Returns an album (list of trackIDs) by the given Artist/Album
     * @param artistname name of the artist of the album in question
     * @param albumname name of the album
     * @return a pointer to the album or NULL if no album could by found
     */
    TrackList * getAlbum(const QString &artistname, const QString &albumname) const;

    /**
     * Returns the name of the iPod
     */
    QString getName();

    uint getNumPlaylists() { return itunesdb.getNumPlaylists(); }

    uint getNumTracks() { return itunesdb.getNumTracks(); }

    QString getITunesDbFilename() { return itunesdb.getFilename(); }

    /**
     * Sets the name of the iPod. When the iPod is not opened (isOpen() == false) nothing is changed
     */
    void setName(const QString& name);

    const QString& getItunesDBError() const;

    /**
     * translates the pathinfo stored on the ipod to a real path info and adds the
     * ipodBase path so we can access the real file
     */
    QString getRealPath( QString pathinfo) const { return ipodBase + pathinfo.replace( ":", "/"); }

    void writeItunesDB();
    void writeItunesDB(const QString& filename);

    bool isChanged() { return pendingchanges; }

    /**
     * Lock control functions to prevent concurrent access to the iPod from different slaves
     */
    void lock(bool write_lock);
    bool isLocked();
    void unlock();

protected:
    ITunesDB itunesdb;
    QString ipodBase;
    QFile logfile;

    QString getLogfileName() { return ipodBase + LOGFILEPREFIX + QString::number(itunesdb.lastModified()); }

    bool appendLogEntry(IPod::LogActionType type, const QStringList& values);
    void replayLog();
    void flushLog();

private:
    bool locked;
    bool pendingchanges;
    uint logfileentrypos;
};

#endif
