// musicbrainzquery.h
//
// Copyright (C)  2003  Zack Rusin <zack@kde.org>
// Copyright (C)  2003 - 2004 Scott Wheeler <wheeler@kde.org>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.

#ifndef MUSICBRAINZ_H
#define MUSICBRAINZ_H

#include <config.h>

#if HAVE_MUSICBRAINZ

#include <musicbrainz/musicbrainz.h>
#include <qobject.h>
#include <qstringlist.h>

#include "filehandle.h"

class KProcess;

/**
 * This is a class used to issue MusicBrainz queries.
 * It's pseudo-asynchrnous. Pseudo because it depends on
 * asynchrnous libmusicbrainz (someday we'll have to write
 * our own KDE native one :) ). The type of queries are in the
 * Query enum. You have to specify the query with arguments in
 * the constructor. Connect to the query signals and issue the
 * start() call on the job. So for example to find an album by
 * name one would do :
 *
 * QStringList l;
 * l << "h2o";
 * MusicBrainzQuery *query =
 *       new MusicBrainzQuery( MusicBrainzQuery::AlbumByName ,
 *                             l );
 * connect( query, SIGNAL( done(const MusicBrainzQuery::AlbumList&)),
 *         SLOT(slotDone(const MusicBrainzQuery::AlbumList&)) );
 * query->start();
 *
 */

class MusicBrainzQuery : public QObject, public MusicBrainz
{
    Q_OBJECT
public:
    enum QueryType {
        CD,           //! Identifies the CD, doesn't take any arguments
        File,         //! Tries to identify the given file, takes file path
        TrackFromTRM, //! Identifies the song from TRM, takes the TRM
        TrackFromID,  //! Song from track ID, takes the trackId
        ArtistByName, //! Name
        AlbumByName,  //! Name
        TrackByName,  //! Name
        TRM,          //! Artist name and track name.
        ArtistByID,   //! Artist ID
        AlbumByID,    //! Album ID
        TrackByID     //! Track ID
    };

    struct Track {
        Track() : number(0) {}

        int     number;
        QString id;
        QString album;
        QString name;
        QString duration;
        QString artist;
        QString artistId;
    };

    typedef QValueList<Track> TrackList;

    struct Album {
        Album() : numTracks(0) {}

        QString name;
        QString artist;
        QString id;
        QString status;
        QString type;
        QString cdIndexId;
        QString artistId;
        int numTracks;
        TrackList tracksList;
    };

    typedef QValueList<Album> AlbumList;

    MusicBrainzQuery(QueryType query, const QStringList &args,
                     QObject *parent = 0, const char *name = 0);

    void start();

signals:
    void signalStatusMsg(const QString &msg, int timeout);
    void signalDone(const MusicBrainzQuery::AlbumList & = AlbumList());
    void signalDone(const MusicBrainzQuery::TrackList &);

private slots:
    void slotQuery();
    void slotTrmData(KProcess *process, char *buffer, int bufferLength);
    void slotTrmGenerationFinished(KProcess *process);

private:
    QString     dataExtract(const QString &, int i = 0);
    void        queryStrings(std::string &query, std::string &result, std::string &extraction);
    Album       extractAlbum();
    Track       extractTrack(int trackNumber);
    Track       extractTrackFromAlbum(int trackNumber);

    QueryType   m_query;
    QStringList m_arguments;
    QString     m_trm;
    bool        m_tracks; //if only tracks should be extracted
};

/**
 *
 */

class MusicBrainzFileQuery : public MusicBrainzQuery
{
    Q_OBJECT

public:
    MusicBrainzFileQuery(const FileHandle &file);

public slots:
    void slotDone(const MusicBrainzQuery::TrackList &result);

private:
    FileHandle m_file;
};

#endif

#endif
