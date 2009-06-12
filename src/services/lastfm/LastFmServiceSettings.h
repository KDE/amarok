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

#ifndef LASTFMSERVICESETTINGS_H
#define LASTFMSERVICESETTINGS_H

#include "LastFmServiceConfig.h"

#include <kcmodule.h>

class QNetworkReply;

namespace Ui { class LastFmConfigWidget; }

class LastFmServiceSettings : public KCModule
{
    Q_OBJECT

public:
    explicit LastFmServiceSettings( QWidget *parent = 0, const QVariantList &args = QVariantList() );

    virtual ~LastFmServiceSettings();

    virtual void save();
    virtual void load();
    virtual void defaults();

private slots:
    void testLogin();
    void onAuthenticated();
private:
    Ui::LastFmConfigWidget *m_configDialog;
    LastFmServiceConfig     m_config;

    QNetworkReply* m_authQuery;

private slots:
    void settingsChanged();
};

#endif // LASTFMSERVICESETTINGS_H
