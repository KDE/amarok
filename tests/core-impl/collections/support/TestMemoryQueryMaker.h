/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef TESTMEMORYQUERYMAKER_H
#define TESTMEMORYQUERYMAKER_H

#include<QTest>

#include "core/meta/forward_declarations.h"
#include "core/collections/QueryMaker.h"

#include "core-impl/collections/support/MemoryCollection.h"
#include "core-impl/collections/support/MemoryFilter.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"


class TestMemoryQueryMaker : public QObject
{
    Q_OBJECT
public:
    TestMemoryQueryMaker();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testDeleteQueryMakerWhileQueryIsRunning();
    void testDeleteCollectionWhileQueryIsRunning();

    void testStringMemoryFilterSpeedFullMatch();
    void testStringMemoryFilterSpeedMatchBegin();
    void testStringMemoryFilterSpeedMatchEnd();
    void testStringMemoryFilterSpeedMatchAnywhere();

    void testFilterTitle();
    void testFilterRating();
    void testFilterAnd();
    void testFilterFormat();

private:
    Meta::TrackList executeQueryMaker( Collections::QueryMaker *qm );

    QSharedPointer<Collections::MemoryCollection> m_mc;
};

#endif // TESTMEMORYQUERYMAKER_H
