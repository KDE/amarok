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

#include "Debug.h"

#include <KStandardDirs>
#include <QFile>
#include <QDomDocument>

#include <qtest_kde.h>

#include <gmock/gmock.h>

QTEST_KDEMAIN_CORE( TestUpcomingEventsEngine )

// TestUpcomingEventsEngine::TestUpcomingEventsEngine( const QStringList args, const QString &logPath )
TestUpcomingEventsEngine::TestUpcomingEventsEngine() //: TestBase("UpcomingEventsEngine"), TestDataEngine("amarok_data_engine_upcomingEvents")
{
    /*QStringList combinedArgs = args;
    addLogging( combinedArgs, logPath );
    QTest::qExec( this, combinedArgs );*/
}

void
TestUpcomingEventsEngine::init()
{
    //Write here initilizations
}


void
TestUpcomingEventsEngine::testDataEngineMethod()
{
    //Verify if the engine has been found
//     QVERIFY(m_engine != 0);
    
    //Tests on the engine
}

#include "TestUpcomingEventsEngine.moc"
