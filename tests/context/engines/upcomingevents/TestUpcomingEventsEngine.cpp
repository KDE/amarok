/****************************************************************************************
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

#include "TestUpcomingEventsEngine.h"

#include <context/engines/upcomingevents/UpcomingEventsEngine.h>
#include <KStandardDirs>
#include <QtTest/QTest>
#include <QFile>
#include <QDomDocument>



TestUpcomingEventsEngine::TestUpcomingEventsEngine( QStringList testArgumentList, bool stdout )
{
    if( !stdout )
        testArgumentList.replace( 2, testArgumentList.at( 2 ) + "UpcomingEventsEngine.xml" );
    QTest::qExec( this, testArgumentList );
}


void TestUpcomingEventsEngine::testUpcomingEventsParseResults()
{    
    QFile file("response_ok.xml");
    QDomDocument doc;
    if (!file.open(QIODevice::ReadOnly))
        return;
    if (!doc.setContent(&file)) {
        file.close();
        return;
    }
    file.close();
    
    m_engine.upcomingEventsParseResult(doc);
    QCOMPARE(engine.getUpcomingEvents().size(), 2);
}