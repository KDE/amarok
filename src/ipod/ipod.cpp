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
#include "ipod.h"

#include <kdebug.h>

IPod::IPod()
    : itunesdb(true)
{
}


IPod::~IPod()
{
    itunesdb.clear();
}

bool IPod::open(const QString& ipod_base) {
    ipodBase = ipod_base;
    logfileentrypos = 0;
    locked = false;
    pendingchanges = false;
    return itunesdb.open(ipod_base);
}

bool IPod::isOpen() {
    return itunesdb.isOpen();
}

bool IPod::isStillConnected() {
    return !itunesdb.dbFileChanged();
}

void IPod::close() {
    flushLog();
    itunesdb.clear();
}


bool IPod::ensureConsistency() {
    kdDebug() << "IPod::ensureConsistency()" << endl;
    if(!isStillConnected()) {
        flushLog();
        return false;
    }

    replayLog();

    return true;
}


TrackMetadata IPod::createNewTrackMetadata() {
    Q_UINT32 trackid = itunesdb.getMaxTrackID();
    while (itunesdb.getTrackByID( ++trackid) != NULL);    // look for the next free number

    TrackMetadata track( trackid);
    // calculate the directory
    Q_UINT32 dir = trackid % 20;

    // form the trackpath
    QString trackpath;
    trackpath.sprintf( ":iPod_Control:Music:F%02d:%s", dir, (QString("kpod") + QString::number(trackid)).latin1());
    track.setPath(trackpath);
 
    return track;
}

IPod::IPodError IPod::deleteArtist(const QString& artistname, bool log) {
    if (!itunesdb.removeArtist(artistname)) {
        return Err_NotEmpty;    // dirty since false is also emmitted if the artist doesn't exist
    }

    if (log) {
        QStringList actions;
        actions << artistname;
        appendLogEntry(ACT_DELETE_ARTIST, actions);
    }

    return Err_None;
}

/*!
    \fn IPod::renameAlbum(const QString& name, const QString& newname)
 */ 
IPod::IPodError IPod::renameAlbum(const QString& artistname, const QString& title, const QString& newartistname, const QString& newtitle, bool log)
{
    kdDebug() << "IPod::renameAlbum() " << title << endl;

    if (!itunesdb.isOpen()) {
        return Err_NotOpen;
    }
    if (itunesdb.getAlbum(newartistname, newtitle)) {    // does it already exist?
        return Err_AlreadyExists;
    }

    TrackList * album = itunesdb.getAlbum(artistname, title);
    if (album == NULL) {
        return Err_DoesNotExist;
    }

    if(!itunesdb.renameAlbum(*album, newartistname, newtitle)) {
        kdDebug() << "IPod::renameAlbum() issued an internal error" << endl;
        return Err_Internal;
    }

    if (log) {
        QStringList actions;
        actions << artistname << title << newartistname << newtitle;
        appendLogEntry(ACT_RENAME_ALBUM, actions);
    }

    pendingchanges = true;

    kdDebug() << "IPod::renameAlbum() finished" << endl;
    return Err_None;
}


IPod::IPodError IPod::deleteAlbum(const QString& artistname, const QString& title, bool log) {
    TrackList * album = getAlbum(artistname, title);
    if (album == NULL)
        return Err_DoesNotExist;

    TrackList::Iterator trackiter = album->getTrackIDs();
    while (trackiter.hasNext()) {
        TrackMetadata * track = getTrackByID(trackiter.next());
        album->removeTrackAt(trackiter);

        if (track == NULL) {
            continue;
        }
        QString filename = getRealPath(track->getPath());
        if (QFile::exists(filename))
            QFile::remove(filename);
        itunesdb.removeTrack(track->getID());
    }

    Artist * artist = getArtistByName(artistname);
    if (artist != NULL)
        artist->remove(album->getTitle());

    if (log) {
        QStringList actions;
        actions << artistname << title;
        appendLogEntry(ACT_REM_ALBUM, actions);
    }

    return Err_None;
}

TrackList * IPod::getAlbum(const QString &artistname, const QString &albumname) const {
    return itunesdb.getAlbum(artistname, albumname);
}


/*!
    \fn IPod::renamePlaylist(const QString& name, const QString& newname)
 */
IPod::IPodError IPod::renamePlaylist(const QString& title, const QString& newtitle, bool log)
{
    if( !itunesdb.isOpen()) {
        return Err_NotOpen;
    }
    if( itunesdb.getPlaylistByTitle(newtitle)) {    // does it already exist?
        return Err_AlreadyExists;
    }

    TrackList * playlist = itunesdb.getPlaylistByTitle(title);
    if( playlist == NULL) {
        return Err_DoesNotExist;
    }
    itunesdb.removePlaylist(title, false);
    playlist->setTitle(newtitle);
    itunesdb.handlePlaylist(*playlist);
    delete playlist;

    if (log) {
        QStringList actions;
        actions << title << newtitle;
        appendLogEntry( ACT_RENAME_PLAYLIST, actions);
    }
    pendingchanges = true;

    return Err_None;
}


/*!
    \fn IPod::deletePlaylist(const QString& name)
 */
IPod::IPodError IPod::deletePlaylist(const QString& title, bool log)
{
    if(!itunesdb.removePlaylist(title, true))
        return Err_DoesNotExist;

    if (log) appendLogEntry(ACT_REM_PLAYLIST, QStringList(title));
    pendingchanges = true;
    return Err_None;
}


QStringList * IPod::getPlaylistTitles(QStringList& buffer) {
    for(IPodPlaylist * playlist= itunesdb.firstPlaylist(); playlist != NULL; playlist= itunesdb.nextPlaylist()) {
        buffer.append(playlist->getTitle());
    }
    return &buffer;
}


/*!
    \fn IPod::getPlaylistByTitle(const QString& title)
 */
TrackList * IPod::getPlaylistByTitle(const QString& title) const
{
    return itunesdb.getPlaylistByTitle(title);
}


QStringList * IPod::getArtists( QStringList &buffer) const 
{
    return itunesdb.getArtists(buffer);
}

QString IPod::getName() {
    IPodPlaylist * mainlist = itunesdb.getMainplaylist();
    if(mainlist != NULL)
        return mainlist->getTitle();
    else
        return QString();
}

void IPod::setName(const QString& name) {
    IPodPlaylist * mainlist = itunesdb.getMainplaylist();
    if(mainlist != NULL) {
        mainlist->setTitle(name);
        pendingchanges = true;
    }
}

const QString& IPod::getItunesDBError() const {
    return itunesdb.error;
}


IPod::IPodError IPod::createPlaylist(const QString& playlisttitle, bool log) {
    if(itunesdb.getPlaylistByTitle(playlisttitle) != NULL) {
        return Err_AlreadyExists;
    }

    IPodPlaylist playlist;
    playlist.setTitle(playlisttitle);
    itunesdb.handlePlaylist(playlist);

    if (log) appendLogEntry( ACT_ADD_PLAYLIST, QStringList(playlist.getTitle()));
    pendingchanges = true;

    return Err_None;
}


void IPod::writeItunesDB() {
    itunesdb.writeDatabase();
    flushLog();
}


void IPod::writeItunesDB(const QString& filename) {
    itunesdb.writeDatabase(filename);
    flushLog();
}

TrackMetadata * IPod::getTrackByID(const Q_UINT32 id) const {
    return itunesdb.getTrackByID(id);
}


Artist * IPod::getArtistByName(const QString& artist) const {
    return itunesdb.getArtistByName(artist);
}


IPod::IPodError IPod::addTrackToPlaylist(const TrackMetadata& track, const QString& playlisttitle, bool log) {
    TrackList * playlist = itunesdb.getPlaylistByTitle(playlisttitle);
    if(playlist == NULL) {
        return Err_DoesNotExist;
    }

    playlist->addPlaylistItem(track);
    pendingchanges = true;

    if (log) {
        QStringList actions;
        actions << playlist->getTitle() << QString::number(track.getID());
        appendLogEntry(ACT_ADD_TO_PLAYLIST, actions);
    }

    return Err_None;
}

IPod::IPodError IPod::removeFromPlaylist(Q_UINT32 position, const QString& playlisttitle, bool log) {
    TrackList * playlist = itunesdb.getPlaylistByTitle(playlisttitle);
    if(playlist == NULL) {
        return Err_DoesNotExist;
    }

    playlist->setTrackIDAt(position, LISTITEM_DELETED);
    pendingchanges = true;

    if (log) {
        QStringList actions;
        actions << playlist->getTitle() << QString::number(position);
        appendLogEntry(ACT_REM_FROM_PLAYLIST, actions);
    }

    return Err_None;
}


void IPod::addTrack(const TrackMetadata& track, bool log) {
    itunesdb.addTrack(track);

    if (log) {
        QStringList actions;
        actions = track.toLogEntry(actions);
        appendLogEntry( ACT_ADD_TRACK, actions);
    }

    pendingchanges = true;
}

IPod::IPodError IPod::moveTrack(TrackMetadata& track, const QString& newartist, const QString& newalbum, bool log) {
    if (!itunesdb.moveTrack(track, newartist, newalbum)) {
        return Err_DoesNotExist;
    }

    if (log) {
        QStringList actions;
        actions << QString::number(track.getID()) << newartist << newalbum;
        appendLogEntry(ACT_MOV_TRACK, actions);
    }
    pendingchanges = true;

    return Err_None;
}

IPod::IPodError IPod::deleteTrack(Q_UINT32 trackid, bool log) {
    if (!itunesdb.removeTrack(trackid, true)) {
        return Err_DoesNotExist;
    }

    if (log) {
        QStringList actions;
        actions << QString::number(trackid);
        appendLogEntry(ACT_REM_TRACK, actions);
    }

    pendingchanges = true;
    return Err_None;
}


void IPod::lock(bool write_lock) {
    if(write_lock) {
        itunesdb.lock(write_lock);
        locked = true;
    }
}

bool IPod::isLocked() {
    return locked;
}


void IPod::unlock() {
    if(locked) {
        itunesdb.unlock();
        locked = false;
    }
}


/*!
    \fn IPod::appendLogEntry( int type, QStringList& values)
 */
bool IPod::appendLogEntry(IPod::LogActionType type, const QStringList& values)
{
    bool _unlock_ = false;
    QFile logfile(getLogfileName());
    if (!logfile.open(IO_ReadWrite | IO_Append)) {
        return false;
    }

    if (!isLocked()) {
        lock(true);
        _unlock_ = true;
    }

    QByteArray logentry;
    QDataStream stream(logentry, IO_WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    for (QStringList::const_iterator value_it = values.constBegin(); value_it != values.constEnd(); ++value_it) {
        stream << *value_it;
    }

    QDataStream logstream(&logfile);
    logstream.setByteOrder(QDataStream::LittleEndian);
    logstream << type;
    logstream << logentry;

    logstream.unsetDevice();

    logfileentrypos++;

    logfile.flush();
    logfile.close();

    if(_unlock_)
        unlock();

    return true;
}


/*!
    \fn kio_ipodslaveProtocol::replayLog()
 */
void IPod::replayLog()
{
    kdDebug() << "IPod::replayLog()" << endl;
    bool _unlock_ = false;
    if (!isLocked()) {
        lock(false);
        _unlock_ = true;
    }

    // kdDebug() << "IPod::replayLog() locked!" << endl;

    QFile logfile(getLogfileName());
    if (!logfile.open(IO_ReadOnly)) {
        if(_unlock_)
            unlock();
        return;
    }

    // kdDebug() << "IPod::replayLog() logfile opened!" << endl;

    QDataStream logstream(&logfile);
    logstream.setByteOrder(QDataStream::LittleEndian);

    // ignore the changes we already know about
    for (uint i= 0; i< logfileentrypos; i++) {
        Q_UINT32 type;
        QByteArray buffer;
        if( logstream.atEnd()) {    // ick
            logfileentrypos= i;
            break;
        }
        logstream >> type;
        logstream >> buffer;
    }

    while (!logstream.atEnd()) {    // read new log entries
        QByteArray entrydata;
        Q_UINT32 type;
        QStringList values;

        logstream >> type;
        logstream >> entrydata;

        if (type >= NUM_ACTIONS) {
            continue;
        }

        logfileentrypos++;

        if( entrydata.isEmpty())
            continue;

        // parse entry elements
        QDataStream entrystream(entrydata, IO_ReadOnly);
        entrystream.setByteOrder( QDataStream::LittleEndian);
        while(!entrystream.atEnd()) {
            QString value;
            entrystream >> value;
            values.push_back(value);
        }

        switch( type) {    // handle logfile entry
        case ACT_ADD_PLAYLIST:
            // add playlist
            if (values.size()> 0) {
                createPlaylist(values[0], false);
            }
            break;
        case ACT_REM_PLAYLIST:
            if (values.size()> 0) {
                deletePlaylist( values[ 0], false);
            }
            break;
        case ACT_RENAME_PLAYLIST:
            if (values.size()> 1) {
                renamePlaylist(values[0], values[1], false);
            }
            break;
        case ACT_REM_ALBUM:
            if (values.size()> 1) {
                deleteAlbum(values[0], values[1], false);
            }
            break;
        case ACT_RENAME_ALBUM:
            if (values.size()> 3) {
                renameAlbum(values[0], values[1], values[2], values[3], false);
            }
            break;
        case ACT_ADD_TO_PLAYLIST: {
            if (values.size()> 1) {
                bool conversion_ok= true;
                Q_UINT32 trackid= values[ 1].toUInt(&conversion_ok);
                if(conversion_ok) {
                    TrackMetadata * track = getTrackByID( trackid);
                    if(track != NULL)
                        addTrackToPlaylist(*track, values[0], false);
                }
            }
            }
            break;
        case ACT_REM_FROM_PLAYLIST:
            if (values.size()> 1) {
                bool conversion_ok = true;
                int tracknum = values[ 1].toUInt(&conversion_ok);
                if(conversion_ok)
                    removeFromPlaylist(tracknum, values[0], false);
            }
            break;
        case ACT_ADD_TRACK:
            if (values.size() > 0) {
                TrackMetadata track;
                if(track.readFromLogEntry( values)) {
                    addTrack(track, false);
                }
            }    // ignore otherwise
            break;
        case ACT_MOV_TRACK:
            if (values.size() > 2) {
                bool conversion_ok = true;
                int trackid = values[0].toUInt(&conversion_ok);
                if (!conversion_ok)
                    break;
                TrackMetadata * track = getTrackByID(trackid);
                if (track == NULL)
                    break;
                moveTrack(*track, values[1], values[2], false);
            }
            break;
        case ACT_REM_TRACK: {
            if (values.size() > 0) {
                bool conversion_ok = true;
                int trackid = values[ 0].toUInt(&conversion_ok);
                if(conversion_ok)
                    deleteTrack(trackid, false);
            }    // ignore otherwise
            }
            break;
        case ACT_DELETE_ARTIST: {
            if (values.size() > 0) {
                deleteArtist(values[0], false);
            }    // ignore otherwise
            }
            break;
        default:
            break;
        }
    }

    if(_unlock_)
        unlock();
}


/*!
    \fn kio_ipodslaveProtocol::flushLog
 */
void IPod::flushLog()
{
    if( QFile::exists(getLogfileName())) {
        QFile::remove(getLogfileName());
    }
    logfileentrypos = 0;
}
