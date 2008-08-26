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

#ifndef LASTFMSERVICE_H
#define LASTFMSERVICE_H


#include "../ServiceBase.h"

#include <KLineEdit>

class ScrobblerAdapter;
class RadioAdapter;
class LastFmService;
class LastFmServiceCollection;

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
};

class LastFmService : public ServiceBase
{
    Q_OBJECT

public:
    LastFmService( LastFmServiceFactory* parent, const QString &name, const QString &username, const QString &password, bool scrobble, bool fetchSimilar );
    virtual ~LastFmService();

    virtual void polish();

    RadioAdapter *radio() { return m_radio; }
    ScrobblerAdapter *scrobbler() { return m_scrobbler; }

    virtual Collection * collection();

private slots:
    void love();
    void skip();
    void ban();

    void playCustomStation();

    void setRadioButtons( bool hasRadio );

private:
    ScrobblerAdapter *m_scrobbler;
    RadioAdapter *m_radio;
    LastFmServiceCollection *m_collection;

    void playLastFmStation( const KUrl &url );

    bool m_polished;
    KHBox *m_buttonBox;
    QPushButton *m_loveButton;
    QPushButton *m_banButton;
    QPushButton *m_skipButton;

    QComboBox *m_globalComboBox;

    KLineEdit * m_customStationEdit;
    QPushButton * m_customStationButton;

    const QString m_userName;
    QString m_station;

    static LastFmService *ms_service;

    friend LastFmService *The::lastFmService();
};

#endif // LASTFMSERVICE_H
