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

    ~LastFmServiceConfig();

    void load();
    void save();
    void reset();

    QString username() const { return m_username; }
    void setUsername( const QString &username ) { m_username = username; }

    QString password() const { return m_password; }
    void setPassword( const QString &password ) { m_password = password; }

    QString sessionKey() const { return m_sessionKey; }
    void setSessionKey( const QString& sessionKey ) { m_sessionKey = sessionKey; }
    void clearSessionKey();

    bool scrobble() const { return m_scrobble; }
    void setScrobble( bool scrobble ) { m_scrobble = scrobble; }

    bool fetchSimilar() const { return m_fetchSimilar; }
    void setFetchSimilar( bool fetchSimilar ) { m_fetchSimilar = fetchSimilar; }

    bool scrobbleComposer() const { return m_scrobbleComposer; }
    void setScrobbleComposer( bool scrobbleComposer ) { m_scrobbleComposer = scrobbleComposer; }

private slots:
    void textDialogYes();
    void textDialogNo();

private:
    friend class LastFmService; // to be able to call constructor
    friend class LastFmServiceSettings; // to be able to call constructor

    /**
     * Construct LastFmServiceConfig. Reserved only to LastFmService. (and temporarily to
     * LastFmServiceSettings too)
     */
    LastFmServiceConfig();

    /**
     * Tries to open KWallet and assigns it to m_wallet if successful. Takes ignore
     * wallet configuration key into account.
     *
     * @return true when m_wallet is valid, false otherwise
     */
    bool tryToOpenWallet();
    void askAboutMissingKWallet();

    QString m_username;
    QString m_password;
    QString m_sessionKey;
    bool m_scrobble;
    bool m_fetchSimilar;
    bool m_scrobbleComposer;

    KDialog* m_askDiag;
    KWallet::Wallet* m_wallet;
};

#endif // LASTFMSERVICECONFIG_H
