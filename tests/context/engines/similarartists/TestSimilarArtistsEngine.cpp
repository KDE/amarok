/****************************************************************************************
* Copyright (c) 2009 Manuel Campomanes <campomanes.manuel@gmail.com>                             *
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
#include <QFile>
#include <QDomDocument>



TestSimilarArtistsEngine::TestSimilarArtistsEngine( const QStringList args, const QString &logPath )
    : TestBase("SimilarArtistsEngine"), TestDataEngine("amarok_data_engine_similarArtists")
{
    QStringList combinedArgs = args;
    addLogging( combinedArgs, logPath );
    QTest::qExec( this, combinedArgs );
}

void TestSimilarArtistsEngine::initTestCase()
{
    //Initialization of similarArtists tests
}


void TestSimilarArtistsEngine::testDataEngineMethod()
{
    //Verify if the engine has been found
    QVERIFY(m_engine != 0);
    
    //Tests on the engine
}