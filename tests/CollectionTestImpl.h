/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#ifndef COLLECTIONTESTIMPL_H
#define COLLECTIONTESTIMPL_H

#include "core/collections/Collection.h"
#include "core/collections/CollectionLocation.h"
#include "core-impl/collections/support/MemoryCollection.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"

#include <KIcon>
#include <QSharedPointer>

namespace Collections {

/**
 * A simple Collections::Collection implementation based on MemoryCollection
 */
class CollectionTestImpl : public Collection
{
public:
    CollectionTestImpl( const QString &collectionId )
        : Collection()
        , id( collectionId )
        , mc( new MemoryCollection() )
    {
    }

    QueryMaker* queryMaker()
    {
        return new MemoryQueryMaker( mc.toWeakRef(), id );
    }

    KIcon icon() const
    {
        return KIcon();
    }

    QString collectionId() const
    {
        return id;
    }

    QString prettyName() const
    {
        return id;
    }

    CollectionLocation *location()
    {
        return new CollectionLocationTestImpl( mc, this );
    }

    bool possiblyContainsTrack( const QUrl &url ) const
    {
        return findTrackForUrl( url );
    }

    Meta::TrackPtr trackForUrl( const QUrl &url )
    {
        return findTrackForUrl( url );
    }

    QString id;
    QSharedPointer<MemoryCollection> mc;

private:
    Meta::TrackPtr findTrackForUrl( const QUrl &url ) const
    {
        QReadLocker( mc->mapLock() );

        foreach( const Meta::TrackPtr track, mc->trackMap().values() )
            if( track->playableUrl() == url )
                return track;

        return Meta::TrackPtr( 0 );
    }

    class CollectionLocationTestImpl : public CollectionLocation
    {
    public:
        CollectionLocationTestImpl( QSharedPointer<MemoryCollection> mc,
                                    Collection *parentCollection )
            : CollectionLocation( parentCollection )
            , m_mc( mc )
        {
        }

        QString prettyLocation() const
        {
            return "/" + collection()->prettyName();
        }

        bool isWritable() const
        {
            return true;
        }

        void copyUrlsToCollection( const QMap<Meta::TrackPtr, QUrl> &sources,
                                   const Transcoding::Configuration &configuration )
        {
            Q_UNUSED( configuration );

            QWriteLocker( m_mc->mapLock() );

            foreach( Meta::TrackPtr track, sources.keys() )
            {
                m_mc->addTrack( track );
                transferSuccessful( track );
            }

            slotCopyOperationFinished();
        }

        bool insert( const Meta::TrackPtr &track, const QString &url )
        {
            Q_UNUSED( url );

            QWriteLocker( m_mc->mapLock() );

            if( m_mc->trackMap().contains( track->uidUrl() ) )
                return false;

            m_mc->addTrack( track );
            return true;
        }

        QStringList actualLocation() const
        {
            return QStringList() << prettyLocation();
        }

    private:
        QSharedPointer<MemoryCollection> m_mc;
    };
};

} //namespace Collections

#endif
