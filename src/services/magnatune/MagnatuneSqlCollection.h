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
 
#ifndef MAGNATUNESQLCOLLECTION_H
#define MAGNATUNESQLCOLLECTION_H

#include "../ServiceSqlCollection.h"

namespace Collections {

/**
A simple ServiceSqlCollection subclass for providing a magnatune membership specific implementaion of trackForUrl

	@author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class MagnatuneSqlCollection : public ServiceSqlCollection
{
public:

    MagnatuneSqlCollection( const QString &id, const QString &prettyName, ServiceMetaFactory * metaFactory, ServiceSqlRegistry * registry );
    
    virtual Meta::TrackPtr trackForUrl( const KUrl &url );

    virtual CollectionLocation* location() const;

};

} //namespace Collections

#endif
