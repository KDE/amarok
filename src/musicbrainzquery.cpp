// musicbrainzquery.cpp
//
// Copyright (C)  2003  Zack Rusin <zack@kde.org>
// Copyright (C)  2003 - 2004  Zack Rusin <wheeler@kde.org>
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

#include <config.h>

#if HAVE_MUSICBRAINZ

#include "musicbrainzquery.h"
#include "trackpickerdialog.h"
#include "collectionlist.h"
#include "tag.h"

#include <kmainwindow.h>
#include <kapplication.h>
#include <kstatusbar.h>
#include <kprocess.h>
#include <klocale.h>
#include <kdeversion.h>
#include <kdebug.h>

#include <qtimer.h>
#include <qvaluelist.h>

#include <string>
#include <vector>

MusicBrainzQuery::MusicBrainzQuery(QueryType query, const QStringList &args,
                                   QObject *parent, const char *name)
    : QObject(parent, name),
      MusicBrainz(),
      m_query(query),
      m_arguments(args),
      m_tracks(false)
{

}

void MusicBrainzQuery::start()
{
    if(m_query == File) {
        KProcess *process = new KProcess(this);
        *process << "trm";
        *process << m_arguments.first();

        connect(process, SIGNAL(receivedStdout(KProcess *, char *, int)),
                SLOT(slotTrmData(KProcess *, char *, int)));

        connect(process, SIGNAL(processExited(KProcess *)),
                SLOT(slotTrmGenerationFinished(KProcess *)));

        emit signalStatusMsg( i18n( "Generating TRM signature..." ), 0 );
        bool started = process->start(KProcess::NotifyOnExit, KProcess::AllOutput);
        if(!started) {
            kdDebug(65432) << "trm utility could not be started." << endl;
            emit signalDone();
        }
    }
    else
        QTimer::singleShot(0, this, SLOT(slotQuery()));
}

void MusicBrainzQuery::slotQuery()
{
    std::string queryString;
    std::string resultString;
    std::string extractString;
    AlbumList albums;
    TrackList tracks;
    std::vector<std::string> v;

    queryStrings(queryString, resultString, extractString);

    emit signalStatusMsg( i18n( "Querying MusicBrainz server..." ), 0 );

    // UseUTF8(false);

    SetDepth(4);

    QStringList::ConstIterator it = m_arguments.begin();
    for(; it != m_arguments.end(); ++it)
        v.push_back(std::string((*it).latin1()));

    bool ret = Query(queryString, &v);
    if(ret) {
        int numEntries = DataInt(resultString);

        for(int i = 1; i <= numEntries; ++i) {
            Select(extractString, i);

            if(m_tracks) {
                Track track(extractTrack(i));
                tracks.append(track);
            }
            else {
                Album album(extractAlbum());
                albums.append(album);
            }
        }
    }
    else {
        std::string error;
        GetQueryError(error);
        kdDebug(65432) << "Query failed: " << error.c_str() << endl;
    }

    if (m_tracks)
        emit signalDone(tracks);
    else
        emit signalDone(albums);

    deleteLater(); // schedule deletion
}

QString MusicBrainzQuery::dataExtract(const QString &type, int i)
{
    std::string s = Data(type.latin1(), i);

    if(s.empty())
        return QString::null;

    return QString::fromUtf8(s.c_str());
}

void MusicBrainzQuery::queryStrings(std::string& query, std::string& result, std::string& extraction)
{
    switch(m_query) {
    case CD:
        query      = MBQ_GetCDInfo;
        result     = MBE_GetNumAlbums;
        extraction = MBS_SelectAlbum;
        break;
    case TrackFromTRM:
        query      = MBQ_TrackInfoFromTRMId;
        result     = MBE_GetNumTracks;
        extraction = MBS_SelectTrack;
        m_tracks   = true;
        break;
    case TrackFromID:
        query      = MBQ_QuickTrackInfoFromTrackId;
        result     = MBE_GetNumTracks;
        extraction = MBS_SelectTrack;
        m_tracks   = true;
        break;
    case ArtistByName:
        query      = MBQ_FindArtistByName;
        result     = MBE_GetNumArtists;
        extraction = MBS_SelectArtist ;
        break;
    case AlbumByName:
        query      = MBQ_FindAlbumByName;
        result     = MBE_GetNumAlbums;
        extraction = MBS_SelectAlbum ;
        break;
    case TrackByName:
        query      = MBQ_FindTrackByName;
        result     = MBE_GetNumTracks;
        extraction = MBS_SelectTrack;
        m_tracks   = true;
        break;
    case TRM:
        query      = MBQ_FindDistinctTRMId;
        result     = MBE_GetNumTrmids;
        extraction = MBS_SelectTrack;
        break;
    case ArtistByID:
        query      = MBQ_GetArtistById;
        result     = MBE_GetNumArtists;
        extraction = MBS_SelectArtist ;
        break;
    case AlbumByID:
        query      = MBQ_GetAlbumById;
        result     = MBE_GetNumAlbums;
        extraction = MBS_SelectAlbum;
        break;
    case TrackByID:
        query      = MBQ_GetTrackById;
        result     = MBE_GetNumTracks;
        extraction = MBS_SelectTrack;
        m_tracks   = true;
        break;
    default:
        kdDebug(65432) << "Unrecognized query reported" << endl;
    }
}

MusicBrainzQuery::Album MusicBrainzQuery::extractAlbum()
{
    kdDebug(65432) << "Extracting album" << endl;

    std::string s;

    GetIDFromURL(Data(MBE_AlbumGetAlbumId), s);

    Album album;

    album.id         = s.c_str();
    album.name       = dataExtract(MBE_AlbumGetAlbumName);
    album.numTracks  = DataInt(MBE_AlbumGetNumTracks);
    album.artist     = dataExtract(MBE_AlbumGetArtistName, 1);

    GetIDFromURL(Data(MBE_AlbumGetArtistId, 1), s);

    album.artistId   = s.c_str();
    album.status     = dataExtract(MBE_AlbumGetAlbumStatus);
    album.type       = dataExtract(MBE_AlbumGetAlbumType);
    album.cdIndexId  = dataExtract(MBE_AlbumGetNumCdindexIds);

    TrackList tracks;

    for(int i = 1; i <= album.numTracks; ++i)
        tracks.append(extractTrackFromAlbum(i));

    album.tracksList = tracks;

    return album;
}

MusicBrainzQuery::Track MusicBrainzQuery::extractTrackFromAlbum(int trackNumber)
{
    Track track;

    track.number      = trackNumber;
    track.name        = dataExtract(MBE_AlbumGetTrackName, trackNumber);
    track.duration    = dataExtract(MBE_AlbumGetTrackDuration, trackNumber);
    track.artist      = dataExtract(MBE_AlbumGetArtistName, trackNumber);

    std::string s;

    GetIDFromURL(Data(MBE_AlbumGetTrackId), s);

    track.id = s.empty() ? QString::null : QString(s.c_str());

    GetIDFromURL(Data(MBE_AlbumGetArtistId), s);

    track.artistId = s.empty() ? QString::null : QString(s.c_str());

    return track;
}

MusicBrainzQuery::Track MusicBrainzQuery::extractTrack(int trackNumber)
{
    Track track;
    const std::string source = Data(MBE_TrackGetTrackId);
    std::string target = source;

    track.name     = dataExtract(MBE_TrackGetTrackName, trackNumber);
    track.duration = dataExtract(MBE_TrackGetTrackDuration, trackNumber);
    track.artist   = dataExtract(MBE_TrackGetArtistName, trackNumber);

    GetIDFromURL(source, target);

    track.id = target.empty() ? QString::null : QString(target.c_str());

    Select(MBS_SelectTrackAlbum);

    track.album  = dataExtract(MBE_AlbumGetAlbumName, trackNumber);
    track.number = GetOrdinalFromList(MBE_AlbumGetTrackList, source);

    GetIDFromURL(Data(MBE_AlbumGetArtistId), target);

    track.artistId = target.empty() ? QString::null : QString(target.c_str());

    Select(MBS_Rewind);

    return track;
}

void MusicBrainzQuery::slotTrmData(KProcess *, char *buffer, int bufferLength)
{
    m_trm += QString::fromLatin1(buffer, bufferLength);
}

void MusicBrainzQuery::slotTrmGenerationFinished(KProcess *process)
{
#if KDE_VERSION < KDE_MAKE_VERSION(3,1,90)
    delete process;
#endif
    Q_UNUSED(process);

    m_arguments.clear();
    m_arguments << m_trm;
    m_query = TrackFromTRM;
    kdDebug(65432) << "Generation finished " << m_trm << endl;
    if (m_trm.isEmpty())
        emit signalStatusMsg( i18n( "TRM generation failed" ), 2000 );
    else
        slotQuery();
}

////////////////////////////////////////////////////////////////////////////////
// MusicBrainzFileQuery
////////////////////////////////////////////////////////////////////////////////

MusicBrainzFileQuery::MusicBrainzFileQuery(const FileHandle &file) :
    MusicBrainzQuery(MusicBrainzQuery::File, file.absFilePath()),
    m_file(file)
{
    connect(this, SIGNAL(signalDone(const MusicBrainzQuery::TrackList &)), 
            this, SLOT(slotDone(const MusicBrainzQuery::TrackList &)));

    KMainWindow *w = static_cast<KMainWindow *>(kapp->mainWidget());
    connect(this, SIGNAL(signalStatusMsg(const QString &, int)),
            w->statusBar(), SLOT(message(const QString &, int)));

    start();
}

void MusicBrainzFileQuery::slotDone(const MusicBrainzQuery::TrackList &result)
{
    KMainWindow *w = static_cast<KMainWindow *>(kapp->mainWidget());

    if(result.isEmpty()) {
        w->statusBar()->message(i18n("No matches found."), 2000);
        return;
    }

    TrackPickerDialog *trackPicker =
        new TrackPickerDialog(m_file.absFilePath(), result, w);

    if(trackPicker->exec() != QDialog::Accepted) {
        w->statusBar()->message(i18n("Canceled."), 2000);
        return;
    }

    MusicBrainzQuery::Track track = trackPicker->selectedTrack();

    if(!track.name.isEmpty())
        m_file.tag()->setTitle(track.name);
    if(!track.artist.isEmpty())
        m_file.tag()->setArtist(track.artist);
    if(!track.album.isEmpty())
        m_file.tag()->setAlbum(track.album);
    if(track.number)
        m_file.tag()->setTrack(track.number);

    m_file.tag()->save();
    CollectionList::instance()->slotRefreshItem(m_file.absFilePath());

    w->statusBar()->message(i18n("Done."), 2000);

    deleteLater();
}


#include "musicbrainzquery.moc"

#endif
