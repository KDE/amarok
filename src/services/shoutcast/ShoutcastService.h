/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#ifndef SHOUTCASTSERVICE_H
#define SHOUTCASTSERVICE_H

#include "ServiceBase.h"

#include "ShoutcastServiceCollection.h"
#include "ServiceMetaBase.h"


class ShoutcastServiceFactory: public ServiceFactory
{
    Q_OBJECT

    public:
        ShoutcastServiceFactory() {}
        virtual ~ShoutcastServiceFactory() {}

        virtual void init();
        virtual QString name();
        virtual KPluginInfo info();
        virtual KConfigGroup config();
};

/**
    A service for showing the shoutcast directory of online radio stations.
    Based on the shoutcast directory in the 1.4 series
    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class ShoutcastService : public ServiceBase
{
    Q_OBJECT

    public:
        ShoutcastService( ShoutcastServiceFactory* parent, const QString &name, const QString &prettyName );
        ~ShoutcastService();

        void polish();

        virtual Amarok::Collection *collection() { return m_collection; }

    private:
        ShoutcastServiceCollection *m_collection;
        KHBox *bottomPanelLayout;
        QPushButton *m_top500ListButton;
        QPushButton *m_allListButton;

    private slots:
        void top500ButtonClicked();
        void allButtonClicked();
};

#endif

