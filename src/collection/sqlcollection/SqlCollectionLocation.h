/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef AMAROK_SQLCOLLECTIONLOCATION_H
#define AMAROK_SQLCOLLECTIONLOCATION_H

#include "CollectionLocation.h"

#include <QMap>
#include <QString>

class SqlCollection;

class SqlCollectionLocation : public CollectionLocation
{
    Q_OBJECT
    public:
        SqlCollectionLocation( SqlCollection const *collection );
        virtual ~SqlCollectionLocation();

        virtual QString prettyLocation() const;
        virtual bool isWriteable() const;
        virtual bool isOrganizable() const;
        virtual bool remove( Meta::TrackPtr track );
        virtual void copyUrlsToCollection( const KUrl::List &sources );

    private:

        void insertTracks( const QMap<QString, Meta::TrackPtr > &trackMap );
        void insertStatistics( const QMap<QString, Meta::TrackPtr > &trackMap );

        SqlCollection *m_collection;
};

#endif
