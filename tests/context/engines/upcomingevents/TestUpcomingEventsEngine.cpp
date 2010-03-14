/***************************************************************************************
* Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
* Copyright (c) 2010 Oleksandr Khayrullin <saniokh@gmail.com>                          *
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

#include "TestUpcomingEventsEngine.h"

#include <KStandardDirs>
#include <QtTest/QTest>
#include <qtest_kde.h>
#include <QDebug>

#include "src/context/ContextObserver.h"
#include "src/context/engines/upcomingevents/UpcomingEventsEngine.h"

QTEST_KDEMAIN_CORE( TestUpcomingEventsEngine )


TestUpcomingEventsEngine::TestUpcomingEventsEngine(QObject* parent)
    : TestDataEngine(parent)
{

}


void
TestUpcomingEventsEngine::initTestCase()
{
     qDebug() << "coucou" ;
     //Write here initilizations
     QList<QVariant> args;
     qDebug() << "coucou" ;
     m_engine = new UpcomingEventsEngine(this, args);
}


void
TestUpcomingEventsEngine::testDataEngineMethod()
{
    //Tests on the engine
    QVERIFY(1 == 1);
}

void
TestUpcomingEventsEngine::cleanupTestCase()
{
    //Write here cleaning
    delete m_engine;
}

#include "TestUpcomingEventsEngine.moc"
