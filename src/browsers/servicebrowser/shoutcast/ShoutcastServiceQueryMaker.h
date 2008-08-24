/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef SHOUTCASTSERVICEQUERYMAKER_H
#define SHOUTCASTSERVICEQUERYMAKER_H

#include "DynamicServiceQueryMaker.h"

#include "Meta.h"

#include "ShoutcastServiceCollection.h"

#include <kio/jobclasses.h>

namespace ThreadWeaver
{
    class Job;
}


/**
A query maker for fetching external data

	@author
*/
class ShoutcastServiceQueryMaker : public DynamicServiceQueryMaker
{
Q_OBJECT
public:
    ShoutcastServiceQueryMaker( ShoutcastServiceCollection * collection );
    ~ShoutcastServiceQueryMaker();

    virtual QueryMaker* reset();
    virtual void run();
    virtual void abortQuery();

    virtual QueryMaker* setQueryType( QueryType type );

    using DynamicServiceQueryMaker::addMatch;
    virtual QueryMaker* addMatch ( const Meta::GenrePtr &genre );
    //virtual QueryMaker* addMatch ( const Meta::DataPtr &data );

    virtual QueryMaker* setReturnResultAsDataPtrs ( bool resultAsDataPtrs );

    virtual QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false );

    //Methods "borrowed" from MemoryQueryMaker
    void runQuery();
    void handleResult();
    void handleResult( const Meta::TrackList &tracks );

    void fetchStations();
    void fetchGenres();

signals:
    void dynamicQueryComplete();

protected:
    ShoutcastServiceCollection * m_collection;
    KIO::StoredTransferJob * m_storedTransferJob;

    QString m_genreMatch;

    struct Private;
    Private * const d;

    Meta::TrackList m_currentTrackQueryResults;

    QString m_filter;

public slots:
    void genreDownloadComplete(KJob *job );
    void stationDownloadComplete(KJob *job );
};

#endif
