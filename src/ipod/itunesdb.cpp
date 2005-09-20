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

#include "itunesdb.h"
#include "itunesdb/itunesdbparser.h"
#include "itunesdb/itunesdbwriter.h"

#include <sys/stat.h>
#include <sys/file.h>

#include <qfile.h>
#include <qfileinfo.h>

#include <kdebug.h>

ITunesDB::ITunesDB(bool resolve_slashes)
    : artistmap(101, false), playlistmap(17, false), playlistiterator(playlistmap)
{
    maxtrackid = 0;
    timestamp = QDateTime();
    resolveslashes= resolve_slashes;
    artistmap.setAutoDelete(true);
    playlistmap.setAutoDelete(true);
}

bool ITunesDB::open(const QString& ipod_base) {
    // TODO remove trailing slash from ipod_base if there is one
    itunesdbfile.setName(ipod_base + "/iPod_Control/iTunes/iTunesDB");
    itunessdfile.setName(ipod_base + "/iPod_Control/iTunes/iTunesSD");
    if(itunesdbfile.exists()) {
        timestamp = QFileInfo(itunesdbfile).lastModified();
        ItunesDBParser parser(*this);
        parser.parse(itunesdbfile);
    } else {
        return false;
    }

    return true;
}

bool ITunesDB::isOpen() {
    return timestamp.isValid();
}

bool ITunesDB::writeDatabase(const QString& filename) {
    QFile outfile(filename);
    if(filename.isEmpty())
        outfile.setName(itunesdbfile.name());

    ItunesDBWriter writer(this);
    writer.write(outfile);

    QFile outfilesd( itunessdfile.name() );
    writer.writeSD( outfilesd );

    return true;
}

bool ITunesDB::dbFileChanged() {
    return !itunesdbfile.exists() || QFileInfo(itunesdbfile.name()).lastModified() != timestamp;
}

ITunesDB::~ITunesDB()
{
    clear();
}

/******************************************************
 *
 *       ItunesDBDataSource Methods
 *
 * for documentation of the following methods
 * see itunesdb/itunesdbdatasource.h
 *****************************************************/

void ITunesDB::writeInit() {
    error= QString::null;

    // remove deleted tracklist items
    for( PlaylistMapIterator iterator(playlistmap); iterator.current(); ++iterator) {
        iterator.current()->removeAll(LISTITEM_DELETED);
    }
}

void ITunesDB::writeFinished() {
    changed= false;    // container is in sync with the database now
}

Q_UINT32 ITunesDB::getNumPlaylists()
{
    return playlistmap.count();
}


/*!
    \fn ITunesDB::getNumTracks()
 */
Q_UINT32 ITunesDB::getNumTracks()
{
    return trackmap.count();
}


IPodPlaylist * ITunesDB::getMainplaylist() {
    return &mainlist;
}


IPodPlaylist * ITunesDB::firstPlaylist()
{
    playlistiterator= PlaylistMapIterator( playlistmap);
    return playlistiterator.current();
}


IPodPlaylist * ITunesDB::nextPlaylist()
{
    return playlistiterator.current() ? ++playlistiterator : NULL;
}


Track * ITunesDB::firstTrack()
{
    trackiterator= trackmap.begin();
    if (trackiterator == trackmap.end())
        return NULL;

    TrackMetadata * track = *trackiterator;
    TrackList * album = getAlbum(track->getArtist(), track->getAlbum());
    if (album != NULL)
        track->setNumTracksInAlbum(album->getNumTracks());
    return track;
}


Track* ITunesDB::nextTrack()
{
    if( trackiterator == trackmap.end() || ++trackiterator == trackmap.end())
        return NULL;

    TrackMetadata * track = *trackiterator;
    TrackList * album = getAlbum(track->getArtist(), track->getAlbum());
    if (album != NULL)
        track->setNumTracksInAlbum(album->getNumTracks());
    return track;
}


/*************************************************
 *
 *       ItunesDBListener Methods
 *
 * for documentation of the following methods
 * see itunesdb/itunesdblistener.h
 *************************************************/


void ITunesDB::parseStarted() {
    error= QString::null;
    changed= true;
}


void ITunesDB::parseFinished()
{
    changed= false;

    if (mainlist.getTitle().isEmpty()) {
        mainlist.setTitle("kpod");
    }

    if( maxtrackid == 0) {
        maxtrackid = 2000;
    }

    // set all album hnge flags to false (quick hack: this should be handled elsewhere)
    // iterate over artists
    for (ArtistMapIterator artistiter(artistmap); artistiter.current(); ++artistiter) {
        for (ArtistIterator albums(*(artistiter.current())); albums.current(); ++albums) {
            albums.current()->setChangeFlag(false);
        }
    }
    // remove unknown playlist entries and reset album change flags
    /* we don't need this anymore since we don't use the mainlist stored on the ipod anymore
       instead we generate our own
    TrackList::Iterator mainlist_iter= mainlist.getTrackIDs();
    while( mainlist_iter.hasNext()) {
        TrackMetadata * track= getTrackByID( mainlist_iter.next());
        if( track == NULL) {    // track couldn't be found
            kdDebug() << "ITunesDB::parseFinished() removing unknown trackid" << endl;
            mainlist.removeTrackAt( mainlist_iter);
            changed= true;
        } else {
            TrackList * album = getAlbum(track->getArtist(), track->getAlbum());
            if(album != NULL)
                album->setChangeFlag(false);
            else
                kdDebug() << "ITunesDB::parseFinished() album " << track->getArtist() << " : " << track->getAlbum() << " not found" << endl;
                ;
        }
    }
    */

    for( IPodPlaylist * playlist= firstPlaylist(); playlist != NULL; playlist= nextPlaylist()) {
        IPodPlaylist::Iterator track_iter= playlist->getTrackIDs();
        while( track_iter.hasNext()) {
            Track * track= getTrackByID( track_iter.next());
            if( track == NULL) {    // track couldn't be found
                playlist->removeTrackAt( track_iter);
                changed= true;
            }
        }
    }
}


/*!
    \fn ITunesDB::handleTrack( Track * track)
 */
void ITunesDB::handleTrack(const Track& track)
{
    if(track.getID() == 0) {
        // not initialized - don't care about this one
        return;
    }
    TrackMetadata * trackmetadata = new TrackMetadata( track);

    if (maxtrackid < track.getID())
        maxtrackid = track.getID();

    insertTrackToDataBase( *trackmetadata);
    mainlist.addPlaylistItem(track);

    changed = true;
}


void ITunesDB::handlePlaylist(const IPodPlaylist& playlist) {
    // TODO find out another way to find out if this is the mainlist (maybe a handleMainlist() or have some state thingy
    if (mainlist.getTitle().isEmpty()) {
        mainlist.setTitle(playlist.getTitle());
        return;    // that's all we wanna know for now
    }

    TrackList * pTracklist = new TrackList( playlist);

    // consistency checks
    if( playlistmap.find( pTracklist->getTitle()) == NULL) {   // dont overwrite existing playlists
        TrackList::Iterator trackid_iter = pTracklist->getTrackIDs();
        while (trackid_iter.hasNext()) {
            Q_UINT32 trackid = trackid_iter.next();
            TrackMetadata * track = getTrackByID(trackid);
            if (track != NULL && (track->getTrackNumber() > pTracklist->getMaxTrackNumber()))
                pTracklist->setMaxTrackNumber(track->getTrackNumber());
        }
        playlistmap.insert( pTracklist->getTitle(), pTracklist);
    } else
        delete pTracklist;
    changed= true;
}


void ITunesDB::handleError(const QString &message)
{
    error= message;
}


void ITunesDB::setNumPlaylists( Q_UINT32 numplaylists)
{
    // oh really? !nteresting!

    Q_UNUSED( numplaylists );
}


void ITunesDB::setNumTracks(Q_UINT32 tracknum)
{
    // oh really? !nteresting!

    Q_UNUSED( tracknum );
}


/*************************************************
 *
 *       Service Methods
 *
 *************************************************/

/**
 * adds a new track to the collection
 * @parameter track the Track to add
 */
void ITunesDB::addTrack(const TrackMetadata& track) {
    handleTrack(track);
}

/**
 * returns the Track corresponding to the given ID
 * @param id ID of the track
 * @return the Track corresponding to the given ID
 */
TrackMetadata* ITunesDB::getTrackByID( const Q_UINT32 id) const
{
    TrackMap::const_iterator track= trackmap.find( id);
    if( track == trackmap.end())
        return NULL;
    else
        return *track;
}


/*!
    \fn ITunesDB::getArtists( QStringList &buffer)
 */
QStringList* ITunesDB::getArtists( QStringList &buffer) const
{
    for( ArtistMapIterator artist( artistmap); artist.current(); ++artist) {
        buffer.append( artist.currentKey());
    }
    return &buffer;
}

Artist * ITunesDB::getArtistByName(const QString& artistname) const {
    return artistmap.find(artistname);
}


/*!
    \fn ITunesDB::getAlbumsByArtist( QString &artist, QStringList &buffer)
 */
Artist * ITunesDB::getArtistByName(const QString& artistname, bool create)
{
    Artist * artist = artistmap.find(artistname);
    if (artist == NULL && create) {
        // artist not in the map yet: create default entry
        artist = new Artist(17, false);
        artist->setAutoDelete(true);
        artistmap.insert(artistname, artist);
    }
    return artist;
}


/*!
    \fn ITunesDB::getPlaylistByTitle( const QString& playlisttitle)
 */
TrackList * ITunesDB::getPlaylistByTitle( const QString& playlisttitle) const
{
    return playlistmap.find( playlisttitle);
}


/*!
    \fn ITunesDB::getAlbum(QString &artist, QString &album)
 */
TrackList * ITunesDB::getAlbum(const QString &artistname, const QString &albumname) const
{
    Artist * artist = artistmap.find( artistname);
    TrackList * album;

    // check if artist exists
    if (artist == NULL) {
        // artist not in the map
        return NULL;
    }

    // find the album
    if( ( album = artist->find( albumname)) == NULL) {
        // album not in the map
        return NULL;
    }

    return album;
}


/*!
    \fn ITunesDB::clear()
 */
void ITunesDB::clear()
{
    // if( trackmap.empty())
    //     return;

    // delete all tracks
    TrackMap::iterator track_it= trackmap.begin();
    for( ; track_it!= trackmap.end(); ++track_it) {
        delete *track_it;
    }
    trackmap.clear();

    // delete all albums
    artistmap.clear();

    // clear playlists
    playlistmap.clear();

    itunesdbfile.setName(QString());
    timestamp = QDateTime();
    maxtrackid = 0;
    mainlist = TrackList();
}


bool ITunesDB::removeArtist(const QString& artistname) {
    Artist * artist = artistmap.find(artistname);
    if (!artist || !artist->isEmpty())
        return false;

    return artistmap.remove(artistname);
}


/*!
    \fn ITunesDB::removePlaylist( const QString& title)
 */
bool ITunesDB::removePlaylist( const QString& title, bool delete_instance) {
    bool existed = false;

    if( delete_instance) {
        existed = playlistmap.remove( title);
    } else {
        existed = playlistmap.take( title) != NULL;
    }

    if( existed)
        changed= true;

    return existed;
}


/*!
    \fn ITunesDB::removeTrack(Q_UINT32 trackid, bool delete_instance = true)
 */
Q_UINT32 ITunesDB::removeTrack(Q_UINT32 trackid, bool delete_instance)
{
    TrackMetadata * track = getTrackByID(trackid);
    if(track == NULL)
        return 0;

    // remove track from track table
    trackmap.remove(trackid);

    // remove track from album
    TrackList * album = getAlbum(track->getArtist(), track->getAlbum());
    if(album != NULL)
        album->removeAll(trackid);

    // remove track from playlists
    for( PlaylistMapIterator iterator(playlistmap); iterator.current(); ++iterator) {
        iterator.current()->removeAll(trackid);
    }

    // remove track from main playlists
    mainlist.removeAll(trackid);

    if(delete_instance)
        delete track;

    return trackid;
}

TrackList * ITunesDB::renameAlbum(TrackList& album, const QString& newartistname, const QString& newtitle) {
    QString artistname;

    // update track info
    TrackList::Iterator trackiter = album.getTrackIDs();
    while (trackiter.hasNext()) {
        TrackMetadata * track = getTrackByID(trackiter.next());
        if(track == NULL) {
            continue;
        }
        if (artistname.isEmpty())
            artistname = track->getArtist();

        track->setArtist(newartistname);
        track->setAlbum(newtitle);
    }

    Artist * artist = getArtistByName(artistname);
    if (artist != NULL)
        artist->take(album.getTitle());
    else
        kdDebug() << "ITunesDB::renameAlbum() Artist " << artistname << " not found" << endl;

    artist = getArtistByName(newartistname, true);
    if (artist == NULL)
        return NULL;

    album.setTitle(newtitle);
    artist->insert(newtitle, &album);

    return getAlbum(newartistname, newtitle);
}

bool ITunesDB::moveTrack(TrackMetadata& track, const QString& newartist, const QString& newalbum) {
    TrackList * album = getAlbum(track.getArtist(), track.getAlbum());
    if (album == NULL)
        return false;

    album->removeAll(track.getID());
    trackmap.remove(track.getID());
    track.setArtist(newartist);
    track.setAlbum(newalbum);

    insertTrackToDataBase(track);

    return true;
}


/*!
    \fn ITunesDB::isChanged()
 */
bool ITunesDB::isChanged() {
    return changed;
}


/***************************************************************************
 *
 *        private/protected methods
 *
 ***************************************************************************/


void ITunesDB::lock(bool write_lock) {
    if(!itunesdbfile.isOpen())
        itunesdbfile.open(IO_ReadOnly);
    if(write_lock)
        flock(itunesdbfile.handle(), LOCK_EX);
    else
        flock(itunesdbfile.handle(), LOCK_SH);
}

void ITunesDB::unlock() {
    flock(itunesdbfile.handle(), LOCK_UN);
    itunesdbfile.close();
}


/*!
    \fn ITunesDB::insertTrackToDataBase()
 */
void ITunesDB::insertTrackToDataBase(TrackMetadata& track)
{
    TrackList * album;
    QString artiststr= track.getArtist();
    QString albumstr= track.getAlbum();

    trackmap.insert(track.getID(), &track);

    if (resolveslashes) {
        albumstr= albumstr.replace( "/", "%2f");
        artiststr= artiststr.replace( "/", "%2f");
    }

    // find the artist
    Artist * artist = getArtistByName(artiststr, true);
    if (artist == NULL) {
        // shouldn't happen
        return;
    }

    // find the album
    if((album = artist->find( albumstr)) == NULL) {
        // album not in the map yet: create default entry
        album = new TrackList();
        album->setTitle(albumstr);
        artist->insert(albumstr, album);
    }

    int trackpos = album->addPlaylistItem(track);

    // if tracknum is not set yet - set it to the position in the album
    if(track.getTrackNumber() == 0) {
        track.setTrackNumber(trackpos);
    }

    // kdDebug() << "ITunesDB::insertTrackToDataBase() done" << endl;
}

