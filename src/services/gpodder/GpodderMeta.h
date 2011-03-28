/****************************************************************************************
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

#ifndef GPODDERMETA_H
#define GPODDERMETA_H

#include "../ServiceMetaBase.h"

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QList>

class GpodderService;

class GpodderMetaFactory : public ServiceMetaFactory
{
public:
    GpodderMetaFactory( const QString& dbPrefix, GpodderService *service );
    virtual ~GpodderMetaFactory() {}

    virtual Meta::TrackPtr createTrack( const QStringList& rows );

private:
    GpodderService* m_service;
};

namespace Meta
{
class GpodderFeed;

typedef KSharedPtr<GPodderFeed> GpodderFeedPtr;



class GpodderFeed : public ServiceTrack
{
public:
    GpodderFeed( const QString& name );
    GpodderFeed( const QStringList& resultRow );

    virtual QList< QAction* > customActions();

};

}

#endif // GPODDERMETA_H
