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

#ifndef LASTFMSERVICE_H
#define LASTFMSERVICE_H

#include "services/ServiceBase.h"
#include "services/lastfm/LastFmServiceConfig.h"
#include "statsyncing/Provider.h"


namespace Collections {
    class LastFmServiceCollection;
}
namespace Dynamic {
    class AbstractBiasFactory;
}
class ScrobblerAdapter;
class KLineEdit;
class QComboBox;
class QLabel;
class QNetworkReply;

class LastFmServiceFactory : public ServiceFactory
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_service_lastfm.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

public:
    LastFmServiceFactory();

    virtual void init();
    virtual QString name();
    virtual KConfigGroup config();

    virtual bool possiblyContainsTrack( const QUrl &url ) const;
};

class LastFmService : public ServiceBase
{
    Q_OBJECT

public:
    LastFmService( LastFmServiceFactory* parent, const QString &name );
    virtual ~LastFmService();

    virtual void polish();

    virtual Collections::Collection * collection();

    void love( Meta::TrackPtr track );

private Q_SLOTS:
    void love();

    void playCustomStation();
    void updateEditHint( int index );

    void slotReconfigure();
    void onAuthenticated();
    void onGetUserInfo();
    void onAvatarDownloaded( const QString& username, QPixmap avatar );

private:
    void continueReconfiguring();
    void playLastFmStation( const QUrl &url );
    void updateProfileInfo();

    QSharedPointer<ScrobblerAdapter> m_scrobbler;
    StatSyncing::ProviderPtr m_synchronizationAdapter;
    Collections::LastFmServiceCollection *m_collection;
    QList<Dynamic::AbstractBiasFactory *> m_biasFactories;

    bool m_polished;
    QWidget *m_profileBox;
    QLabel *m_avatarLabel;
    QLabel *m_profile;
    QLabel *m_userinfo;
    QComboBox *m_globalComboBox;
    KLineEdit *m_customStationEdit;
    QPushButton *m_customStationButton;
    QComboBox *m_customStationCombo;

    QString m_station;
    QString m_age;
    QString m_gender;
    QString m_country;
    QString m_playcount;
    QPixmap m_avatar;
    bool m_subscriber;

    QNetworkReply *m_authenticateReply;
    LastFmServiceConfigPtr m_config;
};

#endif // LASTFMSERVICE_H
