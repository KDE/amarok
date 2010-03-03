/****************************************************************************************
* Copyright (c) 2009 Manuel Campomanes <campomanes.manuel@gmail.com>                   *
* Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
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

#include "TestSimilarArtistsEngine.h"

#include <KStandardDirs>
#include <QtTest/QTest>
#include <qtest_kde.h>

#include <src/context/engines/similarartists/SimilarArtistsEngine.h>

QTEST_KDEMAIN_CORE( TestSimilarArtistsEngine )

void
TestSimilarArtistsEngine::initTestCase()
{
    //Write here initilizations
    QList<QVariant> args;
    //m_engine = new UpcomingEventsEngine(0, args);
}


void
TestSimilarArtistsEngine::testDataEngineMethod()
{
    //Tests on the engine
    QVERIFY(1 == 1);
}

void
TestSimilarArtistsEngine::cleanupTestCase()
{
    //Write here cleaning
    //delete m_engine;
}

#include "TestSimilarArtistsEngine.moc"