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
#ifndef TRACKCONTAINER_H
#define TRACKCONTAINER_H

#include <qmap.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qdict.h>
#include <qstringlist.h>

#include "itunesdb/ItunesDBListener.h"
#include "itunesdb/itunesdbdatasource.h"
#include "itunesdb/ipod_playlist.h"
#include "itunesdb/ipod_playlistitem.h"

#include "trackmetadata.h"
#include "tracklist.h"

#define LISTITEM_DELETED PLAYLISTITEM_INVALID

using namespace itunesdb;

/**
@author Michael Schulze
*/

typedef QDict<TrackList> Artist;
typedef QDictIterator<TrackList> ArtistIterator;

class ITunesDB : public ItunesDBListener, public ItunesDBDataSource {

typedef QDict<Artist> ArtistMap;
typedef QDictIterator<Artist> ArtistMapIterator;

typedef QDict<TrackList> PlaylistMap;
typedef QDictIterator<TrackList> PlaylistMapIterator;

typedef QMap<Q_UINT32,TrackMetadata *> TrackMap;

public:
    QString error;

    ITunesDB(bool resolve_slashes= false);
    virtual ~ITunesDB();

    /**
     * Opens the itunesdb file at the given ipod mountpoint
     * @return true if the itunesdb does exist and could be opened. For parse errors check the error member afterwards
     */
    bool open(const QString& ipod_base);

    /**
     * Returns true if open() has already been successfully called.
     * @return true if open has already been successfully called, false otherwise
     */
    bool isOpen();

    bool writeDatabase(const QString& filename = QString());

    /**
     * Returns true if the itunesdb file has been changed thus needs to be reloaded or does not exist anymore
     */
    bool dbFileChanged();

    QString getFilename() { return itunesdbfile.name(); }

    // ItunesDBDataSource Methods - for documentation see itunesdb/itunesdbdatasource.h
    void writeInit();
    void writeFinished();
    Q_UINT32 getNumPlaylists();
    Q_UINT32 getNumTracks();
    IPodPlaylist * getMainplaylist();
    IPodPlaylist * firstPlaylist();
    IPodPlaylist * nextPlaylist();
    Track * firstTrack();
    Track * nextTrack();

    // ItunesDBListener Methods - for documentation see itunesdb/itunesdblistener.h
    void handlePlaylist(const IPodPlaylist& playlist);
    void handleTrack(const Track& track);
    void handleError(const QString &message);
    void setNumPlaylists(Q_UINT32 numplaylists);
    void setNumTracks(Q_UINT32 numtracks);
    void parseStarted();
    void parseFinished();

    // additional methods used by the IOslave

    /**
     * adds a new Track to the database after itunesDBlistener is done (finished)
     */
    void addTrack(const TrackMetadata& track);

    /**
     * returns the Track corresponding to the given ID
     * @param id ID of the track
     * @return the Track corresponding to the given ID
     */
    TrackMetadata * getTrackByID(const Q_UINT32 id) const;

    /**
     * Returns fills the given QStringList with the names of all artists in the database
     * @return a pointer to the StringList
     */
    QStringList * getArtists(QStringList &buffer) const;

    /**
     * Returns an Artist by the given name
     * @param artistname the artists name
     * @return an Artist by the given name or NULL if the artist could not be found by the given name
     */
    Artist * getArtistByName(const QString& artistname) const;

    /**
     * Returns an Artist by the given name
     * @param artistname the artists name
     * @param create if true this function will create a new Artist for the given name of no such artist exists yet
     * @return an Artist by the given name or NULL if the artist could not be found by the given name and create is false
     */
    Artist * getArtistByName(const QString &artistname, bool create);

    /**
     * Returns an album (list of trackIDs) by the given Artist/Album
     * @param artistname name of the artist of the album in question
     * @param albumname name of the album
     * @return a pointer to the album or NULL if no album could by found
     */
    TrackList * getAlbum(const QString &artistname, const QString &albumname) const;

    /**
     * Returns a playlist by the given playlist title.
     * @param playlisttitle title of the playlist in question
     * @return a pointer to the playlist found or NULL if no such playlist exists
     */
    TrackList * getPlaylistByTitle(const QString& playlisttitle) const;

    /**
     * removes an empty artist from the database
     */
    bool removeArtist(const QString& artistname);

    /**
     * Renames an album with the given title and artist to newtitle, newartist
     */
    TrackList * renameAlbum(TrackList& album, const QString& newartistname, const QString& newtitle);

    /**
     * Removes the playlist with the given title
     * @param title the title of the playlist to be removed
     * @param delete_instance if set to true the element gets deleted during removal
     * @return true if successful, otherwise false
     */
    bool removePlaylist( const QString& title, bool delete_instance);

    /**
     * Removes the Track with the given trackid from the database.
     * This also removes all playlist references to this track
     * @param trackid ID of the track to be deleted
     * @param delete_instance if set to true (default) the Track gets deleted during removal
     * @return the trackid of the Track or 0 if no such Track exists
     */
    Q_UINT32 removeTrack(Q_UINT32 trackid, bool delete_instance = true);

    /**
     * Sets a new artist/playlist for the given Trackid
     * @return true if successful, false if the track couldn't be found
     */
    bool moveTrack(TrackMetadata& track, const QString& newartist, const QString& newalbum);

    /**
     * Wipes all data from the container.
     */
    void clear();

    /**
     * returns true if something in the container's control has been changed.
     * a change to a Playlist or a Track for example will not set the change flag yet.
     */
    bool isChanged();

    /**
     * Returns the max track ID found
     */
    Q_UINT32 getMaxTrackID() { return maxtrackid; }

    /**
     * Lock control functions to prevent concurrent access to the Database from different slaves
     */
    void lock(bool write_lock);
    void unlock();

    uint lastModified() { return timestamp.toTime_t(); }

protected:
    TrackMap trackmap;
    ArtistMap artistmap;
    PlaylistMap playlistmap;
    TrackList mainlist;
    bool resolveslashes;    // replace '/' in Strings with %2F ?    TODO doesn't belong here anymore
    bool changed;
    Q_UINT32 maxtrackid;

    void insertTrackToDataBase(TrackMetadata& track);

private:
    TrackMap::iterator trackiterator;
    PlaylistMapIterator playlistiterator;
    QFile itunesdbfile;
    QFile itunessdfile;
    QDateTime timestamp;
};

#endif
