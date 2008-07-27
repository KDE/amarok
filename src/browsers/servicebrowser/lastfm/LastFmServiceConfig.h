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

#ifndef LASTFMSERVICECONFIG_H
#define LASTFMSERVICECONFIG_H

#include <QString>

class LastFmServiceConfig
{
public:
    static const char *configSectionName() { return "Service_LastFm"; }

    LastFmServiceConfig();

    void load();
    void save();
    void reset();

    const QString &username() { return m_username; }
    void setUsername( const QString &username ) { m_username = username; }

    const QString &password() { return m_password; }
    void setPassword( const QString &password ) { m_password = password; }

    bool scrobble() { return m_scrobble; }
    void setScrobble( bool scrobble ) { m_scrobble = scrobble; }

    bool fetchSimilar() { return m_fetchSimilar; }
    void setFetchSimilar( bool fetchSimilar ) { m_fetchSimilar = fetchSimilar; }

private:
    QString m_username;
    QString m_password;
    bool m_scrobble;
    bool m_fetchSimilar;
};

#endif // LASTFMSERVICECONFIG_H
