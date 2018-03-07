/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2010 Stefan Derkits <stefan@derkits.at>                                *
 * Copyright (c) 2010 Christian Wagner <christian.wagner86@gmx.at>                      *
 * Copyright (c) 2010 Felix Winter <ixos01@gmail.com>                                   *
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

#ifndef GPODDERSERVICESETTINGS_H
#define GPODDERSERVICESETTINGS_H

#include "GpodderServiceConfig.h"
#include <mygpo-qt5/ApiRequest.h>

#include <KConfigWidgets/KCModule>

#include <QNetworkReply>

namespace Ui { class GpodderConfigWidget; }

class GpodderServiceSettings : public KCModule
{
    Q_OBJECT

public:
    GpodderServiceSettings( QWidget *parent, const QVariantList &args );

    virtual ~GpodderServiceSettings();

    void save() Q_DECL_OVERRIDE;
    void load() Q_DECL_OVERRIDE;
    void defaults() Q_DECL_OVERRIDE;

private Q_SLOTS:
    void testLogin();

    void finished();
    void onError( QNetworkReply::NetworkError code );
    void onParseError( );

    void deviceCreationFinished();
    void deviceCreationError( QNetworkReply::NetworkError code );
    void settingsChanged();

private:
    Ui::GpodderConfigWidget *m_configDialog;
    GpodderServiceConfig m_config;

    mygpo::DeviceListPtr m_devices;
    mygpo::AddRemoveResultPtr m_result;
    bool m_enableProvider;
    QNetworkReply *m_createDevice;
};

#endif // GPODDERSERVICESETTINGS_H
