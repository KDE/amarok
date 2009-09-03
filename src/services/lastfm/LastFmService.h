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


#include "../ServiceBase.h"

#include <KLineEdit>
#include <QLabel>
class ScrobblerAdapter;
class LastFmService;
class LastFmServiceCollection;

class QNetworkReply;

class KHBox;

class QComboBox;

namespace The
{
    LastFmService *lastFmService();
}

class LastFmServiceFactory : public ServiceFactory
{
    Q_OBJECT

public:
    LastFmServiceFactory() {}
    virtual ~LastFmServiceFactory() {}

    virtual void init();
    virtual QString name();
    virtual KPluginInfo info();
    virtual KConfigGroup config();

    virtual bool possiblyContainsTrack( const KUrl &url ) const { return url.protocol() == "lastfm"; }

private slots:
    void slotCreateLastFmService();
    void slotRemoveLastFmService();

private:
    ServiceBase* createLastFmService();
};

class LastFmService : public ServiceBase
{
    Q_OBJECT

public:
    LastFmService( LastFmServiceFactory* parent, const QString &name, const QString &username, QString password, const QString &sessionKey, bool scrobble, bool fetchSimilar );
    virtual ~LastFmService();

    virtual void polish();

    ScrobblerAdapter *scrobbler() { return m_scrobbler; }

    virtual Amarok::Collection * collection();

    void love( Meta::TrackPtr track );

private slots:
    void love();
    void skip();
    void ban();

    void playCustomStation();
    void updateEditHint( int index );

    void onAuthenticated();
    void onGetUserInfo();
    void onAvatarDownloaded( QPixmap );

private:
    void init();

    bool m_inited;
    bool m_scrobble;
    ScrobblerAdapter *m_scrobbler;
    LastFmServiceCollection *m_collection;

    void playLastFmStation( const KUrl &url );
    void updateProfileInfo();

    bool m_polished;
    QWidget *m_profileBox;
    QLabel *m_avatarLabel;
    QLabel *m_profile;
    QLabel *m_userinfo;

    QComboBox *m_globalComboBox;

    KLineEdit * m_customStationEdit;
    QPushButton * m_customStationButton;
    QComboBox * m_customStationCombo;

    QString m_userName;
    QString m_sessionKey;
    QString m_station;
    QString m_age;
    QString m_gender;
    QString m_country;
    QString m_playcount;
    QPixmap m_avatar;
    bool m_subscriber;

    QMap< QString, QNetworkReply* > m_jobs;
    static LastFmService *ms_service;

    friend LastFmService *The::lastFmService();
};

#endif // LASTFMSERVICE_H
