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

#include "core/meta/forward_declarations.h"
#include "CoverFetchUnit.h"

#include <KIO/Job>

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QUrl>


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
    explicit CoverFetchQueue( QObject *parent = nullptr );
    ~CoverFetchQueue() override;

    /**
     * Add an album-associated work unit to the queue.
     * @param album album to fetch cover for.
     * @param opt cover fetch option.
     * @param src cover image source.
     * @param xml xml document from the cover provider. Can be empty on first
     * pass of the fetching process.
     */
    void add( const Meta::AlbumPtr &album,
              const CoverFetch::Option opt = CoverFetch::Automatic,
              const CoverFetch::Source src = CoverFetch::LastFm,
              const QByteArray &xml = QByteArray() );

    /**
     * Add a work unit to the queue that does not need to associate with any album.
     * @param opt cover fetch option.
     * @param src cover image source.
     * @param xml xml document from the cover provider. Can be empty on first
     * pass of the fetching process.
     */
    void add( const CoverFetch::Option opt = CoverFetch::WildInteractive,
              const CoverFetch::Source src = CoverFetch::LastFm,
              const QByteArray &xml = QByteArray() );

    /**
     * Add a string query to the queue.
     * @param query text to be used for image search.
     * @param src the image provider to search.
     * @param page the page number to jump to.
     * @param album optional album this query is associated with
     */
    void addQuery( const QString &query,
                   const CoverFetch::Source src = CoverFetch::LastFm,
                   unsigned int page = 0,
                   const Meta::AlbumPtr &album = Meta::AlbumPtr(0) );

    void clear();

public Q_SLOTS:
    void remove( const CoverFetchUnit::Ptr &unit );
    void remove( const Meta::AlbumPtr &album );

Q_SIGNALS:
    void fetchUnitAdded( CoverFetchUnit::Ptr );

private:
    void add( const CoverFetchUnit::Ptr &unit );
    bool contains( const Meta::AlbumPtr &album ) const;
    int index( const Meta::AlbumPtr &album ) const;
    const CoverFetchUnit::Ptr take( const Meta::AlbumPtr &album );

    CoverFetchUnitList m_queue;
    Q_DISABLE_COPY( CoverFetchQueue )
};

#endif /* AMAROK_COVERFETCHQUEUE_H */
