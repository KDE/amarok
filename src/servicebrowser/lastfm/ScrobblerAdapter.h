/***************************************************************************
 * copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
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

#include "engineobserver.h"
#include "core/Scrobbler-12.h"

#include <QVariant>

class ScrobblerAdapter : public QObject, public EngineObserver
{
    Q_OBJECT

public:
    ScrobblerAdapter( QObject *parent, const QString &username, const QString &password );
    virtual ~ScrobblerAdapter();

    virtual void engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged );
    virtual void engineTrackEnded( int finalPosition, int trackLength, const QString &reason );
    virtual void engineTrackPositionChanged( long position , bool userSeek );

private slots:
    void statusChanged( int statusCode, QVariant data );

private:
    ScrobblerManager *m_manager;
    TrackInfo m_current;
    long m_lastPosition;
    long m_totalPlayed;
    QString m_username;
};

#endif // LASTFMSCROBBLERADAPTER_H
