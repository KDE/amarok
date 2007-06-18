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

/**
    This base class defines the the methods necessary to allow the copying and moving of tracks between
    different collections in a generic way.

    This class should be used as follows in client code:

    -select a source and a destination CollectionLocation
    -call prepareCopy or prepareMove on the source CollectionLocation
    -forget about the rest of the workflow

    Implementations which are writeable must reimplement the following methods
    -prettyLocation()
    -isWriteable()
    -remove( Meta::Track )
    -copyUrlsToCollection( KUrl::List )

    Implementations which are only readable can reimplment getKIOCopyableUrls( Meta::TrackList )
    if it is necessary, but can use the default implementations if possible
*/
class CollectionLocation : public QObject
{
    Q_OBJECT
    public:
        CollectionLocation();
        virtual  ~CollectionLocation();

        /**
            a displayable string representation of the collection location. use the return value
            of this method to display the collection location to the user.
            @return a string representation of the collection location
        */
        virtual QString prettyLocation() const;

        /**
            Returns whether the collection location is writeable or not. For example, a local collection or an ipod
            collection would return true, a daap collection or a service collection false. The value returned by this
            method indicates if it is possible to copy tracks to the collection, and if it is generally possible to
            remove/delete files from the collection.
            @return @c true if the collection location is writeable
            @return @c false if the collection location is not writeable
        */
        virtual bool isWriteable() const;

        /**
            convienence method for copying a single track, @see prepareCopy( Meta::TrackList, CollectionLocation* )
        */
        void prepareCopy( Meta::TrackPtr track, CollectionLocation *destination );
        void prepareCopy( const Meta::TrackList &tracks, CollectionLocation *destination );

        /**
            convienence method for moving a single track, @see prepareMove( Meta::TrackList, CollectionLocation* )
        */
        void prepareMove( Meta::TrackPtr track, CollectionLocation *destination );
        void prepareMove( const Meta::TrackList &tracks, CollectionLocation *destination );

        virtual bool remove( Meta::TrackPtr track );

    signals:
        void startCopy( const KUrl::List &sources, bool removeSources );
        void finishCopy( bool removeSources );

    protected:
        /**
            this method is called on te source location, and should return a list of urls which the destination
            location can copy using KIO.
        */
        virtual KUrl::List getKIOCopyableUrls( const Meta::TrackList &tracks );
        /**
            this method is called on the destination. reimplement it if your implementation
            is writeable. you must call slotCopyOperationFinished() when you are done copying
            the files.

            Note: if you need additional inforamtion from the user, e.g. which directory structure to use, call
            the necessary code from this method. 
        */
        virtual void copyUrlsToCollection( const KUrl::List &sources );

    protected slots:
        void slotCopyOperationFinished();

    private slots:
        void slotStartCopy( const KUrl::List &sources, bool removeSources );
        void slotFinishCopy( bool removeSources );

    private:
        void setupConnections();
        void removeSourceTracks( const Meta::TrackList &tracks );

        //only used in the source ColllectionLocation
        CollectionLocation * m_destination;
        Meta::TrackList m_sourceTracks;

        //only used in the destination CollectionLocation
        bool m_removeSources;
};

#endif 
