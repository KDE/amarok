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

#ifndef LASTFMAUDIOCONTROLLER_H
#define LASTFMAUDIOCONTROLLER_H

#include "core/RadioPlaylist.h"
#include "TrackInfo.h"

#include <QStringList>

class AudioController : public QObject
{
    Q_OBJECT

public:
    AudioController( QObject *parent = 0 );
    virtual ~AudioController();

    void setVolume( int vol );
    void play();
    void play( RadioPlaylist& playlist );
    void play( const QUrl& trackUrl );
    void play( const TrackInfo& track );
    void stop();
    void loadNext();

    const QString& currentTrackUrl() const { return m_currentTrackUrl; }

    QStringList soundSystems();
    QStringList devices();

private:
    QString m_currentTrackUrl;
};

#endif // LASTFMAUDIOCONTROLLER_H
