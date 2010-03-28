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
 
#ifndef MP3TUNESSERVICECOLLECTIONLOCATION_H
#define MP3TUNESSERVICECOLLECTIONLOCATION_H

#include "ServiceCollectionLocation.h"
#include "Mp3tunesServiceCollection.h"

#include <QMap>
#include <QSet>
#include <QString>


class Mp3tunesServiceCollectionLocation : public ServiceCollectionLocation
{
    Q_OBJECT
    public:
        Mp3tunesServiceCollectionLocation( Collections::Mp3tunesServiceCollection const *parentCollection );
        virtual ~Mp3tunesServiceCollectionLocation();

        
        virtual QString prettyLocation() const;
        virtual bool isWritable() const;
        virtual bool remove( const Meta::TrackPtr &track );
        virtual void copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources );
        
    private:
        Collections::Mp3tunesServiceCollection *m_collection;
};

#endif
