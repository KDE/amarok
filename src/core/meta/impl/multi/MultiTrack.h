/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef METAMULTITRACK_H
#define METAMULTITRACK_H

#include "core/meta/Meta.h"
#include "core/playlists/Playlist.h"
#include "core/capabilities/MultiSourceCapability.h"

namespace Meta {

/**
A track that wraps a playlist. This is useful, for instance, for adding radio streams with multiple fallback streams to the playlist as a single item

    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/

class MultiTrack : public QObject, public Track, public Meta::Observer
{
    Q_OBJECT
public:
    MultiTrack( PlaylistPtr playlist );
    ~MultiTrack();

    KUrl first();
    KUrl next();

    int current();
    QStringList sources();
    void setSource( int source );

    virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
    virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );


    //forward lots of stuff:

    //TODO: sanity checks on m_currentTrack
    virtual QString name() const { return m_currentTrack->name(); }
    virtual QString prettyName() const { return m_currentTrack->prettyName(); }
    virtual KUrl playableUrl() const { return m_currentTrack->playableUrl(); }
    virtual QString prettyUrl() const { return m_currentTrack->prettyUrl(); }
    virtual QString uidUrl() const { return m_currentTrack->uidUrl(); }

    virtual AlbumPtr album() const { return m_currentTrack->album(); }
    virtual ArtistPtr artist() const { return m_currentTrack->artist(); }
    virtual ComposerPtr composer() const { return m_currentTrack->composer(); }
    virtual GenrePtr genre() const { return m_currentTrack->genre(); }
    virtual YearPtr year() const { return m_currentTrack->year(); }

    virtual qreal bpm() const { return m_currentTrack->bpm(); }
    virtual QString comment() const { return m_currentTrack->comment(); }
    virtual double score() const { return m_currentTrack->score(); }
    virtual void setScore( double newScore ) { m_currentTrack->setScore( newScore ); }
    virtual int rating() const { return m_currentTrack->rating(); }
    virtual void setRating( int newRating ) { m_currentTrack->setRating( newRating ); }
    virtual qint64 length() const { return m_currentTrack->length(); }
    virtual int filesize() const { return m_currentTrack->filesize(); }
    virtual int sampleRate() const { return m_currentTrack->sampleRate(); }
    virtual int bitrate() const { return m_currentTrack->bitrate(); }
    virtual int trackNumber() const { return m_currentTrack->trackNumber(); }
    virtual int discNumber() const { return m_currentTrack->discNumber(); }
    virtual uint lastPlayed() const { return m_currentTrack->lastPlayed(); }
    virtual int playCount() const { return m_currentTrack->playCount(); }

    virtual bool isPlayable() const { return m_currentTrack->isPlayable(); }
    virtual QString type() const { return m_currentTrack->type(); }

    using Observer::metadataChanged;
    virtual void metadataChanged( Meta::TrackPtr track );


signals:
    void urlChanged( const KUrl &url );

private:
    PlaylistPtr m_playlist;
    TrackPtr m_currentTrack;
    int m_index;

};

}

#endif
