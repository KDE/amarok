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
 
#ifndef MAGNATUNECOLLECTIONLOCATION_H
#define MAGNATUNECOLLECTIONLOCATION_H

#include "MagnatuneSqlCollection.h"
#include "ServiceCollectionLocation.h"

namespace Collections {

/**
A ServiceCollectionLocation subclass responsible for showing a small Magnatune specific dialog when copying tracks from Magnatune

	@author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class MagnatuneCollectionLocation : public ServiceCollectionLocation
{
public:
    MagnatuneCollectionLocation( MagnatuneSqlCollection const *parentCollection );

    virtual ~MagnatuneCollectionLocation();

    virtual void showSourceDialog( const Meta::TrackList &tracks, bool removeSources );

};

} //namespace Collections

#endif
