/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2013 Vedant Agarwala <vedant.kota@gmail.com>                           *
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

#ifndef LASTFMSERVICESETTINGS_H
#define LASTFMSERVICESETTINGS_H

#include "LastFmServiceConfig.h"
#include "core/meta/forward_declarations.h" // for using the Meta::LabelList

#include <kcmodule.h>

#include <QNetworkReply>

#include <Auth.h>

namespace Ui { class LastFmConfigWidget; }

class LastFmServiceSettings : public KCModule
{
    Q_OBJECT

public:
    explicit LastFmServiceSettings( QWidget *parent = nullptr, const QVariantList &args = QVariantList() );

    ~LastFmServiceSettings() override;

    void save() override;
    void load() override;
    void defaults() override;

private Q_SLOTS:
    void disconnectAccount();
    void initiateTokenAuth();
    void onAuthTokenReady();
    void getSessionToken( const QString &sessionToken );
    void onAuthenticated();
    void onError( QNetworkReply::NetworkError code );
    void onConfigUpdated();

private:
    /**
     * gets the index of the @param label in the QComboBox
     * If the label doesn't exist in the list, its added and then the index is returned
     */
    int filteredLabelComboIndex( const QString &label );

    Ui::LastFmConfigWidget *m_configDialog;
    LastFmServiceConfigPtr m_config;

    QNetworkReply* m_authQuery;

private Q_SLOTS:
    void settingsChanged();
    void addNewLabels( const Meta::LabelList &labels );
};

#endif // LASTFMSERVICESETTINGS_H
