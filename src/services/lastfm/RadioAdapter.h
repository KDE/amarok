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

#ifndef LASTFMRADIOADAPTER_H
#define LASTFMRADIOADAPTER_H

#include "meta/LastFmMeta.h"

#include <lastfm/radio/Tuner.h>

#include <QQueue>

class RadioAdapter : public QObject
{
    Q_OBJECT

public:
    RadioAdapter( QObject *parent );
    virtual ~RadioAdapter();

    LastFm::Track* currentTrack() const { return m_currentTrack; }

    void play( LastFm::Track *track );
    void next();
    void stop();
    void skip() { next(); } // for compatibility reasons

signals:
    void haveTrack( bool track );

private slots:
    void error( Ws::Error error );
    void slotStationName( const QString& name );
    void slotNewTracks( const QList< Track >& tracks );

private:
    Tuner *m_tuner;
    LastFm::Track* m_currentTrack;
    QQueue< Track > m_upcomingTracks;

};

#endif // LASTFMRADIOADAPTER_H
