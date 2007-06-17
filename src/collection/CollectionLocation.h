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
#ifndef AMAROK_COLLECTIONLOCATION_H
#define AMAROK_COLLECTIONLOCATION_H

#include "meta.h"

#include <QList>
#include <QObject>

#include <KUrl>

class Collection;

class CollectionLocation : public QObject
{
    Q_OBJECT
    public:
        CollectionLocation() {};
        virtual  ~CollectionLocation() {};

        /**
            a displayable string representation of the collection location. use the return value
            of this method to display the collection location to the user.
            @return a string representation of the collection location
        */
        virtual QString prettyLocation() const = 0;

        virtual void prepareCopy( Meta::TrackPtr track, CollectionLocation *destination ) = 0;
        virtual void prepareCopy( Meta::TrackList tracks, CollectionLocation *destination ) = 0;
        virtual void prepareMove( Meta::TrackPtr track, CollectionLocation *destination ) = 0;
        virtual void prepareMove( Meta::TrackList tracks, CollectionLocation *destination ) = 0;

        virtual bool remove( Meta::TrackPtr track ) = 0;

        virtual bool supportsRemoval() const = 0;

    signals:
        void startCopy( const KUrl::List &sources, CollectionLocation *sourceCollection, bool removeSources );

    public slots:
        virtual void slotStartCopy( const KUrl::List &sources, CollectionLocation *soucrceCollection, bool removeSources ) = 0;
};

#endif 
