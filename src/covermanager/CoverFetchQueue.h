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
 * A queue that keeps track of albums to fetch covers for.
 */
class CoverFetchQueue : public QObject
{
    Q_OBJECT

public:
    CoverFetchQueue( QObject *parent = 0 );
    ~CoverFetchQueue();

    bool add( const Meta::AlbumPtr album,
              CoverFetch::Options opt = CoverFetch::Automatic,
              const QByteArray &xml = QByteArray() );

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
    bool add( const CoverFetchUnit::Ptr unit );
    void remove( const Meta::AlbumPtr album );

    CoverFetchUnitList m_queue;
    Q_DISABLE_COPY( CoverFetchQueue );
};

#endif /* AMAROK_COVERFETCHQUEUE_H */
