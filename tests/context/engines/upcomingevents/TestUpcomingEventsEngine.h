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

#ifndef TESTUPCOMINGEVENTSENGINE_H
#define TESTUPCOMINGEVENTSENGINE_H

// #include <tests/context/engines/TestDataEngine.h>
// #include <TestBase.h>
#include <QtTest/QTest>
#include <QtCore/QStringList>
#include "engines/upcomingevents/UpcomingEventsEngine.h"

class TestUpcomingEventsEngine : public QObject// : public TestBase, public TestDataEngine
{
    Q_OBJECT
    
    public:
//         TestUpcomingEventsEngine( const QStringList args, const QString &logPath );
        TestUpcomingEventsEngine();      
	
    private slots:
        void init();
        void testDataEngineMethod();
};

#endif // TESTUPCOMINGEVENTSENGINE_H