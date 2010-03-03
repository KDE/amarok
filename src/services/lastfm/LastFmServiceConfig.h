/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef LASTFMSERVICECONFIG_H
#define LASTFMSERVICECONFIG_H

#include <QObject>
#include <QString>

namespace KWallet {
    class Wallet;
}

class KDialog;

class LastFmServiceConfig : public QObject
{
    Q_OBJECT
public:
    static const char *configSectionName() { return "Service_LastFm"; }

    LastFmServiceConfig();
    ~LastFmServiceConfig();

    void load();
    void save();
    void reset();

    const QString &username() { return m_username; }
    void setUsername( const QString &username ) { m_username = username; }

    const QString &password() { return m_password; }
    void setPassword( const QString &password ) { m_password = password; }

    const QString sessionKey() { return m_sessionKey; }
    void setSessionKey( const QString& sessionKey ) { m_sessionKey = sessionKey; }

    bool scrobble() { return m_scrobble; }
    void setScrobble( bool scrobble ) { m_scrobble = scrobble; }

    bool fetchSimilar() { return m_fetchSimilar; }
    void setFetchSimilar( bool fetchSimilar ) { m_fetchSimilar = fetchSimilar; }

private slots:
    void textDialogOK();
    void textDialogCancel();

private:
    QString m_username;
    QString m_password;
    QString m_sessionKey;
    bool m_scrobble;
    bool m_fetchSimilar;

    KDialog* m_askDiag;
    KWallet::Wallet* m_wallet;
};

#endif // LASTFMSERVICECONFIG_H
