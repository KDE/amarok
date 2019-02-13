/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef OPMLDIRECTORYSERVICE_H
#define OPMLDIRECTORYSERVICE_H


#include "amarokurls/AmarokUrlRunnerBase.h"
#include "../ServiceBase.h"
#include "OpmlDirectoryDatabaseHandler.h"
#include "ServiceSqlCollection.h"

#include "core/support/Amarok.h"
#include <KIO/Job>


class OpmlDirectoryServiceFactory: public ServiceFactory
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_service_opmldirectory.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

    public:
        OpmlDirectoryServiceFactory();
        virtual ~OpmlDirectoryServiceFactory();

        void init() override;
        QString name() override;
        KConfigGroup config() override;
};

/**
A service for displaying, previewing and downloading music from OpmlDirectory.com

	@author 
*/
class OpmlDirectoryService : public ServiceBase, public AmarokUrlRunnerBase
{
    Q_OBJECT
    public:
        OpmlDirectoryService( OpmlDirectoryServiceFactory* parent, const QString &name,
                              const QString &prettyName );

        ~OpmlDirectoryService() override;

        void polish() override;

        Collections::Collection * collection() override { return nullptr; }

        /* UrlRunnerBase methods */
        QString command() const override;
        QString prettyCommand() const override;
        bool run( const AmarokUrl &url ) override;
        QIcon icon() const override { return QIcon::fromTheme( QStringLiteral("view-services-opml-amarok") ); }

    private Q_SLOTS:
        void subscribe();
        void slotSelectionChanged( const QItemSelection &, const QItemSelection & );

    private:

        QPushButton *m_addOpmlButton;
        QPushButton *m_subscribeButton;

        int m_currentCategoryId;

        int m_numberOfFeeds;
        int m_numberOfCategories;
};

#endif
