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

#include "services/lastfm/amarok_service_lastfm_config_export.h"

#include <QObject>
#include <QSharedPointer>
#include <QString>

namespace KWallet {
    class Wallet;
}
class QMessageBox;
class LastFmServiceConfig;
typedef QSharedPointer<LastFmServiceConfig> LastFmServiceConfigPtr;

/**
 * Configuration of the Last.fm plugin. Because some operations are async, you should
 * connect to the updated() signal and listen to changes, especially ones to username,
 * password or sessionKey.
 */
class AMAROK_SERVICE_LASTFM_CONFIG_EXPORT LastFmServiceConfig : public QObject
{
    Q_OBJECT
public:
    static const QString configSectionName() { return QStringLiteral("Service_LastFm"); }

    /**
     * Singleton pattern accessor. Not thread safe - must be called from the main
     * thread.
     */
    static LastFmServiceConfigPtr instance();

    ~LastFmServiceConfig() override;

    /**
     * Saves the configuration back to the storage and notifies other users about
     * the change. This method must be called after calling any of the set* methods.
     */
    void save();

    QString username() const { return m_username; }
    void setUsername( const QString &username ) { m_username = username; }

    QString password() const { return m_password; }
    void setPassword( const QString &password ) { m_password = password; }

    QString sessionKey() const { return m_sessionKey; }
    void setSessionKey( const QString &sessionKey ) { m_sessionKey = sessionKey; }

    bool scrobble() const { return m_scrobble; }
    static bool defaultScrobble() { return true; }
    void setScrobble( bool scrobble ) { m_scrobble = scrobble; }

    bool fetchSimilar() const { return m_fetchSimilar; }
    static bool defaultFetchSimilar() { return true; }
    void setFetchSimilar( bool fetchSimilar ) { m_fetchSimilar = fetchSimilar; }

    bool scrobbleComposer() const { return m_scrobbleComposer; }
    static bool defaultScrobbleComposer() { return false; }
    void setScrobbleComposer( bool scrobbleComposer ) { m_scrobbleComposer = scrobbleComposer; }

    bool useFancyRatingTags() const { return m_useFancyRatingTags; }
    static bool defaultUseFancyRatingTags() { return true; }
    void setUseFancyRatingTags( bool useFancyRatingTags ) { m_useFancyRatingTags = useFancyRatingTags; }

    bool announceCorrections() const { return m_announceCorrections; }
    static bool defaultAnnounceCorrections() { return true; }
    void setAnnounceCorrections( bool announceCorrections ) { m_announceCorrections = announceCorrections; }

    bool filterByLabel() const { return m_filterByLabel; }
    static bool defaultFilterByLabel() { return false; }
    void setFilterByLabel( bool filterByLabel ) { m_filterByLabel = filterByLabel; }

    QString filteredLabel() const { return m_filteredLabel; }
    static QString defaultFilteredLabel() { return QString(); }
    void setFilteredLabel( const QString &filteredLabel ) { m_filteredLabel = filteredLabel; }

Q_SIGNALS:
    /**
     * Emitted when settings are changed. (after save() is called)
     */
    void updated();

private Q_SLOTS:
    void slotWalletOpenedToRead( bool success );
    void slotWalletOpenedToWrite( bool success );

    void slotStoreCredentialsInAscii();

private:
    Q_DISABLE_COPY( LastFmServiceConfig )
    LastFmServiceConfig();

    void openWalletToRead();
    void openWalletToWrite();
    void openWalletAsync();
    void prepareOpenedWallet();
    void askAboutMissingKWallet();

    // don't remove or reorder entries, would break saved config
    enum KWalletUsage {
        NoPasswordEnteredYet,
        PasswodInKWallet,
        PasswordInAscii
    };

    QString m_username;
    QString m_password;
    QString m_sessionKey;
    bool m_scrobble;
    bool m_fetchSimilar;
    bool m_scrobbleComposer;
    bool m_useFancyRatingTags;
    bool m_announceCorrections;
    bool m_filterByLabel;
    QString m_filteredLabel;
    KWalletUsage m_kWalletUsage;

    QMessageBox* m_askDiag;
    KWallet::Wallet* m_wallet;

    static QWeakPointer<LastFmServiceConfig> s_instance;
};

#endif // LASTFMSERVICECONFIG_H
