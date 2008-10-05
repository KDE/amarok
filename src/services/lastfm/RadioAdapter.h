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

#ifndef LASTFMRADIOADAPTER_H
#define LASTFMRADIOADAPTER_H

#include "core/Radio.h"
#include "meta/LastFmMeta.h"

class RadioAdapter : public QObject
{
    Q_OBJECT

public:
    RadioAdapter( QObject *parent, const QString &username, const QString &password );
    virtual ~RadioAdapter();

    LastFm::TrackPtr currentTrack() const { return m_currentTrack; }

    void play( const LastFm::TrackPtr &track );
    void next();
    void stop();

signals:
    void haveTrack( bool track );

private slots:
    void error( RadioError errorCode, const QString& message );

private:
    Radio *m_radio;
    LastFm::TrackPtr m_currentTrack;

    friend Radio &The::radio();
};

#endif // LASTFMRADIOADAPTER_H
