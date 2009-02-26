/***************************************************************************
* copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>          *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LASTFMSCROBBLERADAPTER_H
#define LASTFMSCROBBLERADAPTER_H

#include "EngineObserver.h"
#include "meta/Meta.h"

#include <lastfm/Scrobbler.h>
#include <lastfm/types/Track.h>

#include <QVariant>

class ScrobblerAdapter : public QObject, public EngineObserver
{
    Q_OBJECT

public:
    ScrobblerAdapter( QObject *parent, const QString &clientId );
    virtual ~ScrobblerAdapter();

    virtual void enginePlaybackEnded( int finalPosition, int trackLength, PlaybackEndedReason reason );
    virtual void engineTrackPositionChanged( long position , bool userSeek );
    virtual void engineNewTrackPlaying();
    virtual void engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged ); // for stream scrobbling

    void skip();
    void love();
    void ban();

public slots:
    void loveTrack( Meta::TrackPtr );

private slots:
    void statusChanged( int statusCode, QVariant data );

private:
    void resetVariables();
    void checkScrobble();

    Scrobbler *m_scrobbler;
    MutableTrack m_current;
    long m_lastPosition;
    uint m_totalPlayed;
    QString m_clientId;
};

#endif // LASTFMSCROBBLERADAPTER_H
