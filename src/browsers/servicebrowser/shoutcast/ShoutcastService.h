/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

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
A service for showing the shoutcast directory of online radio stations. Based on the shoutcast directory in the 1.4 series by 

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class ShoutcastService : public ServiceBase
{
    Q_OBJECT

public:
    ShoutcastService( ShoutcastServiceFactory* parent, const char *name );
    ~ShoutcastService();

    void polish();

    virtual Collection * collection() { return m_collection; }


private:
    ShoutcastServiceCollection * m_collection;
    QString m_tempFileName;
    KIO::StoredTransferJob * m_storedTransferJob;
};


#endif

