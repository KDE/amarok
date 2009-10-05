/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef LASTFMSCROBBLERADAPTER_H
#define LASTFMSCROBBLERADAPTER_H

#include "EngineObserver.h"
#include "meta/Meta.h"

#include <lastfm/Audioscrobbler>
#include <lastfm/Track>

#include <QVariant>


class ScrobblerAdapter : public QObject, public EngineObserver
{
    Q_OBJECT

public:
    ScrobblerAdapter( QObject *parent, const QString &clientId );
    virtual ~ScrobblerAdapter();

    virtual void enginePlaybackEnded( qint64 finalPosition, qint64 trackLength, PlaybackEndedReason reason );
    virtual void engineTrackPositionChanged( long position , bool userSeek );
    virtual void engineNewTrackPlaying();
    virtual void engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged ); // for stream scrobbling

    void skip();
    void love();
    void ban();

public slots:
    void loveTrack( Meta::TrackPtr );
    void banTrack();

private:
    void resetVariables();
    void checkScrobble();

    lastfm::Audioscrobbler *m_scrobbler;
    lastfm::MutableTrack m_current;
    qint64 m_lastPosition;
    qint64 m_totalPlayed;
    QString m_clientId;

    qint64 m_lastSaved;
};

#endif // LASTFMSCROBBLERADAPTER_H
