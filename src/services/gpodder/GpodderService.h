/****************************************************************************************
 * Copyright (c) 2010 - 2011 Stefan Derkits <stefan@derkits.at>                         *
 * Copyright (c) 2010 - 2011 Christian Wagner <christian.wagner86@gmx.at>               *
 * Copyright (c) 2010 - 2011 Felix Winter <ixos01@gmail.com>                            *
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

#ifndef GPODDERSERVICE_H
#define GPODDERSERVICE_H

#include "core/support/Amarok.h"
#include "GpodderProvider.h"
#include "services/ServiceBase.h"

#include <QItemSelectionModel>
#include <QSortFilterProxyModel>

class GpodderService;

namespace The { GpodderService *gpodderService(); }

class GpodderServiceFactory : public ServiceFactory
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_service_gpodder.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

public:
    GpodderServiceFactory();
    ~GpodderServiceFactory() override;

    void init() override;
    QString name() override;
    KConfigGroup config() override;

private Q_SLOTS:
    void slotCreateGpodderService();
    void slotRemoveGpodderService();

private:
    ServiceBase *createGpodderService();
};

class GpodderService : public ServiceBase
{
    Q_OBJECT

public:
    GpodderService( GpodderServiceFactory *parent, const QString &name );
    virtual ~GpodderService();

private Q_SLOTS:
    void subscribe();
    void itemSelected( CollectionTreeItem *selectedItem );

private:
    void init();
    void polish() override;

    void enableGpodderProvider( const QString &username );

    Collections::Collection *collection() override { return 0; }

    bool m_inited;

    mygpo::ApiRequest *m_apiRequest;

    Podcasts::GpodderProvider *m_podcastProvider;

    QSortFilterProxyModel *m_proxyModel;

    QPushButton *m_subscribeButton;
    QItemSelectionModel *m_selectionModel;
};

#endif  // GPODDERSERVICE_H
