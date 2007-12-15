/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef RADIOPLAYLIST_H
#define RADIOPLAYLIST_H

#include "XspfResolver.h"
#include "RadioEnums.h"
#include "TrackInfo.h"
#include "WebService/fwd.h"
#include "StationUrl.h"

#include <QQueue>

/*************************************************************************/ /**
    Responsible for keeping the radio supplied with tracks.
    
    This was initially designed as only fetching track descriptions as XSPF
    in chunks from a web service and re-query for new chunks when needed.
    To do this, a client calls setSession with a radio session string.

    Later, it turned out previewing and playlist support wouldn't be handled
    through the XSPF web service, instead we get the XSPF returned directly
    from the ChangeStationRequest. For these cases, I added the setXspf
    method and the Type enum which essentially makes this class operate in two
    different modes. The mode is determined simply by which of the setSession
    or setXspf functions was called last. This is a bit ugly but it was the
    most pragmatic solution at the time and I couldn't think of a better
    design that solved the issues nicely. Maybe in the future we might want
    to implement some form ox XspfProvider class which can handle the details
    of how the XSPF is retrieved.
    
    Returns tracks in a sequence to client code via the nextTrack function.
******************************************************************************/
class RadioPlaylist : public QObject
{
    Q_OBJECT

public:
    
    enum Type
    {
        Type_Station,
        Type_Playlist
    };

    RadioPlaylist();
    virtual ~RadioPlaylist();

    /*********************************************************************/ /**
        Tell it to relate to a radio session and fetch XSPF dynamically as we
        go. Used for proper radio stations. Will trigger a refetch of XSPF
        playlist.
    **************************************************************************/
    void
    setSession( const QString& session );

    /*********************************************************************/ /**
        Needed for radio web services.
    **************************************************************************/
    void
    setBasePath( const QString& path ) { m_basePath = path; }

    /*********************************************************************/ /**
        Tell it to use the XSPF playlist we pass in and not use the XSPF web
        service. Used for previews and playlists.
    **************************************************************************/
    void
    setXspf( const QByteArray& xspf );

    /*********************************************************************/ /**
        Returns true if nextTrack has more tracks to return.
    **************************************************************************/
    Type
    type() const;

    /*********************************************************************/ /**
        Returns the number of tracks currently left in the playlist.
    **************************************************************************/
    int
    size() const { return m_trackQueue.size(); }

    /*********************************************************************/ /**
        Returns true if nextTrack has more tracks to return.
    **************************************************************************/
    bool
    hasMore() const;
    
    /*********************************************************************/ /**
        Returns true if the station has no more tracks to supply. It can be
        the case that hasMore returns false while this method returns true
        if we're in the process of refetching.
    **************************************************************************/
    bool
    isOutOfContent() const;

    /*********************************************************************/ /**
        Get next track.
    **************************************************************************/
    TrackInfo
    nextTrack();

    /*********************************************************************/ /**
        Get current track.
    **************************************************************************/
    TrackInfo
    currentTrack();

    /*********************************************************************/ /**
        Clear playlist and abort any on-going web request.
    **************************************************************************/
    void
    clear();

    /*********************************************************************/ /**
        Clear all cached tracks and retrieve new chunk. Useful if the user just
        switched discovery mode on for example.
    **************************************************************************/
    void
    discardRemaining();

    /*********************************************************************/ /**
        Abort any on-going web request.
    **************************************************************************/
    void
    abort();

signals:

    /*********************************************************************/ /**
        Emitted when internal track queue has been filled and we're ready
        to roll.
    **************************************************************************/
    void
    playlistLoaded( const QString& stationName, int skipsLeft = -1 );

    void
    error( RadioError error, const QString& message );

private:

    /*********************************************************************/ /**
        Ask web service for a playlist chunk.
    **************************************************************************/
    void
    requestPlaylistChunk();

    /*********************************************************************/ /**
        Parse XSPF stream and resolve it into TrackInfo objects.
    **************************************************************************/
    void
    parseXspf( QByteArray& xspf );

    QQueue<TrackInfo> m_trackQueue;
    TrackInfo m_currentTrack;
    
    // TODO: potentially hold a pointer to a base class resolver in the future
    // and let the playlist be configurable with different types of resolvers.
    // Overkill for now.
    XspfResolver m_resolver;

    // The playlist uses either/or of these, if it's a radio station, it uses
    // the session to get XSPF from a web service, if it's a pre-made playlist
    // it gets the XSPF up front and stores it in m_xspf;
    QString m_session;
    QByteArray m_xspf;

    QString m_basePath;
    
    // Need to keep a member pointer so that we can abort
    GetXspfPlaylistRequest* m_currentRequest;

    bool m_allXspfRetrieved; /// have we reached the end of the station as we know it?
    bool m_requestingPlaylist; /// are we in the process of asking for more?
    
private slots:
    
    void
    xspfPlaylistRequestReturn( Request* request );

};

#endif // RADIOPLAYLIST_H
