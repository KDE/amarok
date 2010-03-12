/****************************************************************************************
 * Copyright (c) 2009 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#ifndef AMAROK_COVERFETCHQUEUE_H
#define AMAROK_COVERFETCHQUEUE_H

#include "meta/Meta.h"
#include "CoverFetchUnit.h"

#include <KIO/Job>
#include <KUrl>

#include <QByteArray>
#include <QList>
#include <QObject>

class CoverFetchPayload;

typedef QList< CoverFetchUnit::Ptr > CoverFetchUnitList;

/**
 * A queue that creates and keeps track of cover fetching units.
 * This queue creates cover fetch units with suitable payloads as requested by
 * the cover fetcher. It does not manage the state of those units, as in, what
 * to do next after each of those units is completed. The cover fetcher is
 * responsible for coordinating that.
 */

class CoverFetchQueue : public QObject
{
    Q_OBJECT

public:
    CoverFetchQueue( QObject *parent = 0 );
    ~CoverFetchQueue();

    /**
     * Add an album-associated work unit to the queue.
     * @param album album to fetch cover for.
     * @param opt cover fetch option.
     * @param xml xml document from the cover provider. Can be empty on first
     * pass of the fetching process.
     */
    void add( const Meta::AlbumPtr album,
              const CoverFetch::Options opt = CoverFetch::Automatic,
              const QByteArray &xml = QByteArray() );

    /**
     * Add a work unit to the queue that does not need to associate with any album.
     * @param opt cover fetch option.
     * @param xml xml document from the cover provider. Can be empty on first
     * pass of the fetching process.
     */
    void add( const CoverFetch::Options opt = CoverFetch::WildInteractive,
              const QByteArray &xml = QByteArray() );

    /**
     * Add a string query to the queue.
     * @param query text to be used for image search.
     */
    void addQuery( const QString &query );

    bool contains( const Meta::AlbumPtr album ) const;
    int index( const Meta::AlbumPtr album ) const;
    int size() const;
    bool isEmpty() const;

    void clear();
    const CoverFetchUnit::Ptr take( const Meta::AlbumPtr album );

public slots:
    void remove( const CoverFetchUnit::Ptr unit );

signals:
    void fetchUnitAdded( const CoverFetchUnit::Ptr );

private:
    void add( const CoverFetchUnit::Ptr unit );
    void remove( const Meta::AlbumPtr album );

    CoverFetchUnitList m_queue;
    Q_DISABLE_COPY( CoverFetchQueue );
};

#endif /* AMAROK_COVERFETCHQUEUE_H */
