/****************************************************************************************
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef AMAROK_SERVICECOLLECTIONLOCATION_H
#define AMAROK_SERVICECOLLECTIONLOCATION_H

#include "core/collections/CollectionLocation.h"
#include "amarok_export.h"
#include "ServiceCollection.h"

#include <QSet>
#include <QMap>
#include <QString>

namespace Collections {

class AMAROK_EXPORT ServiceCollectionLocation : public CollectionLocation
{
    Q_OBJECT
    public:
        ServiceCollectionLocation();
        ServiceCollectionLocation( const ServiceCollection* parentCollection );
        virtual ~ServiceCollectionLocation();

        virtual void getKIOCopyableUrls( const Meta::TrackList &tracks );

        //These are service dependant
        virtual QString prettyLocation() const;
        virtual bool isWritable() const;
        virtual bool isOrganizable() const;
        //virtual bool remove( const Meta::TrackPtr &track );
    private:
        ServiceCollection *m_collection; //parent collection
        bool m_removeSources;    //used by the destination to remember the value, needed in copyurlsToCollection
        bool m_overwriteFiles;
};

} //namespace Collections

#endif
