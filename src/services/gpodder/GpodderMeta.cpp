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

#include "GpodderMeta.h"

#include "GpodderService.h"

using namespace Meta;

GpodderMetaFactory::GpodderMetaFactory( const QString& dbPrefix, GpodderService* service ): ServiceMetaFactory( dbPrefix ), m_service( service )
{
}

Meta::TrackPtr GpodderMetaFactory::createTrack( const QStringList& rows )
{
    return TrackPtr( new GpodderFeed( rows ) );
}

Meta::GpodderFeed::GpodderFeed( const QString& name ): ServiceTrack( name )
{
}

Meta::GpodderFeed::GpodderFeed( const QStringList& resultRow ): ServiceTrack( resultRow )
{
}

QList< QAction* > Meta::GpodderFeed::customActions()
{
    QList< QAction * > actions;
    return actions;
}
