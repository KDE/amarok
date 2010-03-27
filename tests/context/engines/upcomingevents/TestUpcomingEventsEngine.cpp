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

#include "core/support/Components.h"
#include "ContextObserver.h"
#include "EngineController.h"
#include "UpcomingEventsEngine.h"

#include <QtTest/QTest>
#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestUpcomingEventsEngine )


TestUpcomingEventsEngine::TestUpcomingEventsEngine(QObject* parent)
    : TestDataEngine(parent)
{

}


void
TestUpcomingEventsEngine::initTestCase()
{
    //apparently the engine controller is needed somewhere, or we will get a crash...
    EngineController *controller = new EngineController();
    Amarok::Components::setEngineController( controller );
    bool invoked = QMetaObject::invokeMethod( controller, "initializePhonon", Qt::DirectConnection );
    Q_ASSERT( invoked );

    //Write here initilizations
    QList<QVariant> args;
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
