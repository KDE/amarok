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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#ifndef SHOUTCASTSERVICEQUERYMAKER_H
#define SHOUTCASTSERVICEQUERYMAKER_H

#include "DynamicServiceQueryMaker.h"

#include "meta.h"

#include "Mp3tunesServiceCollection.h"

#include <kio/jobclasses.h>

namespace ThreadWeaver
{
    class Job;
}


/**
A query maker for fetching external data

	@author
*/
class Mp3tunesServiceQueryMaker : public DynamicServiceQueryMaker
{
Q_OBJECT
public:
    Mp3tunesServiceQueryMaker( Mp3tunesServiceCollection * collection, const QString &sessionId );
    ~Mp3tunesServiceQueryMaker();

    virtual QueryMaker* reset();
    virtual void run();
    virtual void abortQuery();

    virtual QueryMaker* startArtistQuery();

    virtual QueryMaker* returnResultAsDataPtrs ( bool resultAsDataPtrs );

    //Methods "borrowed" from MemoryQueryMaker
    void runQuery();
    void handleResult();
    void handleResult( const Meta::TrackList &tracks );

    void fetchArtists();


protected:
    Mp3tunesServiceCollection * m_collection;
    KIO::StoredTransferJob * m_storedTransferJob;

    class Private;
    Private * const d;

    QString m_sessionId;

public slots:

    void artistDownloadComplete(KJob *job );


};

#endif
